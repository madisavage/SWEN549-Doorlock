/*
 * Passive BLE UUID triggers: each known 128-bit service UUID maps to a motor
 * state. A device must be "near" (EMA-filtered RSSI with hysteresis) for
 * DWELL_MS before its state fires; when it goes far or silent the board
 * returns to state 0.
 */
#include <string.h>
#include "main.h"
#include "app_common.h"
#include "dbg_trace.h"
#include "ble.h"
#include "hw_if.h"
#include "stm32_seq.h"
#include "uuid_trigger.h"

/* Tunables */
#define RSSI_ENTER_DBM   (-70)
#define RSSI_EXIT_DBM    (-80)
#define DWELL_MS         2000
#define TIMEOUT_MS       5000
#define TICK_MS          250
#define EMA_ALPHA        0.25f

typedef struct {
  const char *name;
  uint8_t uuid[16]; /* written in canonical order, as typed into nRF Connect */
  uint8_t state;
} Trigger_t;

static const Trigger_t triggers[] = {
  {"POS1", {0xF0,0xE1,0xD2,0xC3, 0xB4,0xA5, 0x46,0x97, 0x88,0x99, 0xAA,0xBB,0xCC,0xDD,0x00,0x01}, 1},
  {"POS2", {0xF0,0xE1,0xD2,0xC3, 0xB4,0xA5, 0x46,0x97, 0x88,0x99, 0xAA,0xBB,0xCC,0xDD,0x00,0x02}, 2},
};
#define N_TRIGGERS (sizeof(triggers) / sizeof(triggers[0]))

typedef struct {
  float rssi_f;
  uint8_t seen;
  uint8_t near;
  uint8_t pending;
  uint32_t pending_since;
  uint32_t last_seen;
} Device_t;

static Device_t devices[N_TRIGGERS];
static volatile uint8_t state;
static uint8_t tick_timer_id;

void UuidTrigger_SetState(uint8_t s)
{
  static const Led_TypeDef state_led[] = {LED_RED, LED_GREEN, LED_BLUE};

  BSP_LED_Off(state_led[state]);
  state = s;
  BSP_LED_On(state_led[state]);
  /* Same outputs as sandbox main.c: D0 = PA3, D1 = PA2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, state == 1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, state == 2);
  APP_DBG_MSG("STATE -> %d\r\n", state);
}

uint8_t UuidTrigger_GetState(void)
{
  return state;
}

static void cmd_near(uint8_t i)
{
  APP_DBG_MSG("NEAR %s\r\n", triggers[i].name);
  UuidTrigger_SetState(triggers[i].state);
}

static void cmd_far(uint8_t i)
{
  APP_DBG_MSG("FAR  %s\r\n", triggers[i].name);
  /* Only release the motor if this device is the one that set the state */
  if (state == triggers[i].state)
  {
    UuidTrigger_SetState(0);
  }
}

/* AD data stores UUIDs little-endian, the table is canonical (big-endian) */
static int uuid_match(const uint8_t *le, const uint8_t *canon)
{
  for (int b = 0; b < 16; b++)
  {
    if (le[b] != canon[15 - b])
    {
      return 0;
    }
  }
  return 1;
}

static int find_trigger(const uint8_t *ad, uint8_t len)
{
  uint8_t k = 0;

  while (k + 1 < len)
  {
    uint8_t ad_len = ad[k];
    uint8_t ad_type = ad[k + 1];

    if (ad_len == 0 || k + 1 + ad_len > len)
    {
      break; /* malformed */
    }
    if (ad_type == AD_TYPE_128_BIT_SERV_UUID || ad_type == AD_TYPE_128_BIT_SERV_UUID_CMPLT_LIST)
    {
      for (uint8_t u = 2; u + 16 <= ad_len + 1; u += 16)
      {
        for (uint8_t i = 0; i < N_TRIGGERS; i++)
        {
          if (uuid_match(&ad[k + u], triggers[i].uuid))
          {
            return i;
          }
        }
      }
    }
    k += ad_len + 1;
  }
  return -1;
}

void UuidTrigger_OnAdv(const uint8_t *ad, uint8_t len, int8_t rssi)
{
  int i = find_trigger(ad, len);
  if (i < 0 || rssi == 127) /* 127 = RSSI not available */
  {
    return;
  }

  Device_t *d = &devices[i];
  uint32_t now = HAL_GetTick();

  d->rssi_f = d->seen ? EMA_ALPHA * rssi + (1.0f - EMA_ALPHA) * d->rssi_f : rssi;
  d->seen = 1;
  d->last_seen = now;

  uint8_t want = d->near ? (d->rssi_f > RSSI_EXIT_DBM) : (d->rssi_f > RSSI_ENTER_DBM);
  if (want == d->near)
  {
    d->pending = 0;
  }
  else if (!d->pending)
  {
    d->pending = 1;
    d->pending_since = now;
  }
  else if (now - d->pending_since >= DWELL_MS)
  {
    d->pending = 0;
    d->near = want;
    want ? cmd_near(i) : cmd_far(i);
  }

  APP_DBG_MSG("%s raw=%d filt=%d near=%d%s\r\n", triggers[i].name, rssi,
              (int)d->rssi_f, d->near, d->pending ? " (pending)" : "");
}

static void tick(void)
{
  uint32_t now = HAL_GetTick();

  for (uint8_t i = 0; i < N_TRIGGERS; i++)
  {
    Device_t *d = &devices[i];
    if (d->seen && now - d->last_seen > TIMEOUT_MS)
    {
      APP_DBG_MSG("%s timeout\r\n", triggers[i].name);
      uint8_t was_near = d->near;
      memset(d, 0, sizeof(*d));
      if (was_near)
      {
        cmd_far(i);
      }
    }
  }
}

static void tick_isr(void)
{
  UTIL_SEQ_SetTask(1 << CFG_TASK_UUID_TICK_ID, CFG_SCH_PRIO_0);
}

void UuidTrigger_Init(void)
{
  GPIO_InitTypeDef out = {
      .Pin = GPIO_PIN_3 | GPIO_PIN_2,
      .Mode = GPIO_MODE_OUTPUT_PP,
      .Pull = GPIO_NOPULL,
      .Speed = GPIO_SPEED_FREQ_LOW,
  };
  HAL_GPIO_Init(GPIOA, &out);
  BSP_LED_Off(LED_GREEN);
  BSP_LED_Off(LED_BLUE);
  UuidTrigger_SetState(0);

  UTIL_SEQ_RegTask(1 << CFG_TASK_UUID_TICK_ID, UTIL_SEQ_RFU, tick);
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &tick_timer_id, hw_ts_Repeated, tick_isr);
  HW_TS_Start(tick_timer_id, (uint32_t)(TICK_MS * 1000 / CFG_TS_TICK_VAL));
}

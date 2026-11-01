/* Host self-check: cc -I stubs -I ../STM32_WPAN/App test_uuid_trigger.c ../STM32_WPAN/App/uuid_trigger.c -o uuid_test && ./uuid_test */
#include <assert.h>
#include <string.h>
#include "stubs.h"
#include "uuid_trigger.h"

uint32_t fake_tick;
void (*fake_task)(void);

/* Flags AD + complete 128-bit list holding POS2's UUID (little-endian on air) */
static uint8_t adv_pos2[] = {
  0x02, 0x01, 0x06,
  0x11, 0x07, 0x02,0x00,0xDD,0xCC,0xBB,0xAA,0x99,0x88,0x97,0x46,0xA5,0xB4,0xC3,0xD2,0xE1,0xF0,
};

static void feed(int8_t rssi, uint32_t ms_total)
{
  for (uint32_t t = 0; t < ms_total; t += 100)
  {
    fake_tick += 100;
    UuidTrigger_OnAdv(adv_pos2, sizeof adv_pos2, rssi);
  }
}

int main(void)
{
  UuidTrigger_Init();
  assert(UuidTrigger_GetState() == 0);

  /* Unknown UUID and malformed AD are ignored */
  uint8_t junk[] = {0x11, 0x07, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  UuidTrigger_OnAdv(junk, sizeof junk, -30);
  uint8_t bad[] = {0x20, 0x07, 0x00};
  UuidTrigger_OnAdv(bad, sizeof bad, -30);
  assert(UuidTrigger_GetState() == 0);

  /* Near, but not yet past dwell */
  feed(-50, 1500);
  assert(UuidTrigger_GetState() == 0);
  feed(-50, 1000);
  assert(UuidTrigger_GetState() == 2);

  /* Inside hysteresis band (-60..-72): stays near */
  feed(-66, 5000);
  assert(UuidTrigger_GetState() == 2);

  /* Silence past timeout returns home */
  fake_tick += 6000;
  fake_task();
  assert(UuidTrigger_GetState() == 0);

  puts("uuid_trigger: all checks passed");
  return 0;
}

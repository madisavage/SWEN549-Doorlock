/* Host stubs so uuid_trigger.c compiles and runs on a PC */
#pragma once
#include <stdint.h>
#include <stdio.h>
typedef enum { LED_BLUE, LED_GREEN, LED_RED } Led_TypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
#define GPIOA 0
#define GPIO_PIN_2 4
#define GPIO_PIN_3 8
#define GPIO_MODE_OUTPUT_PP 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_LOW 0
#define AD_TYPE_128_BIT_SERV_UUID 0x06
#define AD_TYPE_128_BIT_SERV_UUID_CMPLT_LIST 0x07
#define APP_DBG_MSG printf
#define CFG_TASK_UUID_TICK_ID 0
#define CFG_SCH_PRIO_0 0
#define CFG_TIM_PROC_ID_ISR 0
#define CFG_TS_TICK_VAL 1
#define UTIL_SEQ_RFU 0
#define hw_ts_Repeated 0
extern uint32_t fake_tick;
extern void (*fake_task)(void);
static inline uint32_t HAL_GetTick(void) { return fake_tick; }
static inline void BSP_LED_On(Led_TypeDef l) { (void)l; }
static inline void BSP_LED_Off(Led_TypeDef l) { (void)l; }
static inline void HAL_GPIO_Init(int p, GPIO_InitTypeDef *i) { (void)p; (void)i; }
static inline void HAL_GPIO_WritePin(int p, int pin, int v) { (void)p; (void)pin; (void)v; }
static inline void UTIL_SEQ_SetTask(uint32_t t, uint32_t p) { (void)t; (void)p; }
static inline void UTIL_SEQ_RegTask(uint32_t t, uint32_t f, void (*fn)(void)) { (void)t; (void)f; fake_task = fn; }
static inline void HW_TS_Create(int a, uint8_t *id, int m, void (*fn)(void)) { (void)a; (void)id; (void)m; (void)fn; }
static inline void HW_TS_Start(uint8_t id, uint32_t t) { (void)id; (void)t; }

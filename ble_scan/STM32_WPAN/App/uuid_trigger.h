#ifndef UUID_TRIGGER_H
#define UUID_TRIGGER_H

#include <stdint.h>

void UuidTrigger_Init(void);
void UuidTrigger_OnAdv(const uint8_t *ad, uint8_t len, int8_t rssi);
/* Same as SW1 in the sandbox app: 0 = home, 1..N = motor positions */
void UuidTrigger_SetState(uint8_t state);
uint8_t UuidTrigger_GetState(void);

#endif /* UUID_TRIGGER_H */

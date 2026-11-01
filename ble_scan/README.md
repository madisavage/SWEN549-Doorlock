# ble_scan — passive BLE UUID triggers for NUCLEO-WB55RG

The board listens for BLE advertisements without ever connecting. When a phone advertising a known 128-bit service UUID comes close enough, the board switches to the state mapped to that UUID: LEDs plus the D0/D1 outputs that drive the motor. These are the same states the `sandbox` app cycles through with SW1.

This is a separate project next to `sandbox/`. The original sandbox app is unchanged.

## How it works

```
phone (advertises UUID) ──radio──> CPU2 (M0+, ST BLE stack)
                                        │  HCI LE Advertising Report (IPCC mailbox)
                                        ▼
CPU1 (M4, this app):  app_ble.c  SVCCTL_App_Notification()
                                        │  AD data + RSSI
                                        ▼
                      uuid_trigger.c  UuidTrigger_OnAdv()
                         match UUID → EMA filter → hysteresis → dwell
                                        │
                                        ▼
                      UuidTrigger_SetState(n): LED + PA3/PA2
```

### Two cores
The STM32WB55 has two cores. The radio and BLE stack run on CPU2, which needs `stm32wb5x_BLE_Stack_full_fw.bin` flashed through STM32CubeProgrammer. This app runs on CPU1 and talks to the stack over the IPCC mailbox, using the transport layer from ST's `STM32_WPAN` middleware. Without the stack firmware, the app hangs at startup waiting for CPU2.

### Scanning (`STM32_WPAN/App/app_ble.c`)
- GAP is initialised with the **Central + Observer** roles.
- `Scan_Request()` calls `aci_gap_start_observation_proc()` with these settings:
  - passive scan (no scan requests sent)
  - interval = window = `0x50` (50 ms), so the radio listens close to 100% of the time
  - **duplicate filtering off**, so every packet comes through with a fresh RSSI
  - unfiltered policy, so every advertiser is reported
- If the stack ever reports `ACI_GAP_PROC_COMPLETE`, the scan is restarted.
- `SVCCTL_App_Notification()` takes each `HCI_LE_ADVERTISING_REPORT` and passes its AD data and RSSI to the trigger module. The raw report layout is `Num_Reports | Event_Type | Addr_Type | Addr[6] | Length | Data[Length] | RSSI`. The SDK's `Advertising_Report_t` contains a `Data` *pointer*, so it can't be laid over the raw bytes; the code reads by offset instead.

### Matching and state machine (`STM32_WPAN/App/uuid_trigger.c`)
- **Identify by payload, not MAC.** Phones randomise their address, so each device is identified by a 128-bit service UUID (AD type `0x06`/`0x07`). The UUIDs in `triggers[]` are written in normal (big-endian) order; on air they are little-endian, and `uuid_match()` handles the reversal.
- **Smoothing:** `filt = 0.25*raw + 0.75*filt`, because raw RSSI jumps by about ±10 dB.
- **Hysteresis:** a device becomes near above `RSSI_ENTER_DBM` (-70) and is only far again below `RSSI_EXIT_DBM` (-80).
- **Dwell:** a near/far change must hold for `DWELL_MS` (2 s) before it fires.
- **Timeout:** with no packets for `TIMEOUT_MS` (5 s), a 250 ms timer task (`HW_TS` → sequencer) marks the device gone.
- **Actions:** when a device becomes near, `cmd_near()` sets that device's state. When it becomes far or gone, `cmd_far()` returns to state 0, but only if that device was the one holding the state.

| State | LED   | PA3 (D0) | PA2 (D1) | Trigger |
|-------|-------|----------|----------|---------|
| 0     | red   | low      | low      | home / nobody near |
| 1     | green | high     | low      | `F0E1D2C3-B4A5-4697-8899-AABBCCDD0001` |
| 2     | blue  | low      | high     | `F0E1D2C3-B4A5-4697-8899-AABBCCDD0002` |

SW1 still cycles 0 → 1 → 2. The button ISR only queues a sequencer task, so all state changes and logging run outside interrupt context.

To add a position, add a row to `triggers[]` and extend `UuidTrigger_SetState()`.

## Build, flash, monitor

```sh
export PATH=~/.local/share/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin:$PATH
cmake --preset Debug && cmake --build build/Debug
~/.local/share/stm32cube/bundles/programmer/2.23.0/bin/STM32_Programmer_CLI -c port=SWD -w build/Debug/ble_scan.elf -v -rst
screen /dev/ttyACM0 115200        # ST-LINK virtual COM port (not /dev/tty0)
```

HAL, WPAN middleware, and utilities are compiled straight from the installed CubeWB package. Point the `CUBE_WB` CMake cache variable at it if it isn't in `~/STM32Cube/Repository/STM32Cube_FW_WB_V1.24.0`.

Example log output:

```
** START OBSERVATION (PASSIVE SCAN) **
POS2 raw=-62 filt=-68 near=0 (pending)
NEAR POS2
STATE -> 2
```

### Phone side
In nRF Connect (Android), open the Advertiser tab and add a "Complete list of 128-bit Service UUIDs" record containing one of the UUIDs above. iOS hides service UUIDs while an app is in the background, so on an iPhone the advertising app must stay in the foreground.

### Tuning
Put the phone where it should trigger and read `filt=` from the log. Set `RSSI_ENTER_DBM` a few dB below that value, and keep `RSSI_EXIT_DBM` about 10 dB lower so the state doesn't flicker.

### Host test
`test/test_uuid_trigger.c` runs the parser and state machine on a PC with stubbed HAL headers. It covers ignoring unknown and malformed packets, dwell, hysteresis, and timeout:

```sh
cd test && cc -I stubs -I ../STM32_WPAN/App test_uuid_trigger.c ../STM32_WPAN/App/uuid_trigger.c -o uuid_test && ./uuid_test
```

## How we got here

1. **Starting point.** `sandbox/` was a bare CubeMX HAL project, with no BLE middleware, where SW1 cycles three states on the LEDs and PA3/PA2. The goal was to have BLE advertisements drive those states instead.
2. **Separate project.** Adding the BLE stack to the CubeMX project would have meant new clocks (HSE/LSE), RTC, IPCC, shared-RAM linker sections, and the sequencer. Instead, ST's `BLE_p2pClient` example was copied into `ble_scan/`, given a CMake build matching sandbox's toolchain, and supplied with sandbox's `syscalls.c`/`sysmem.c` for newlib.
3. **Stripped the client.** All connection, GATT discovery, and p2p code was removed (`OOB_DEMO` set to 0). General discovery was replaced with continuous passive observation, with duplicate filtering off.
4. **Added `uuid_trigger.c`.** It does UUID matching, the EMA/hysteresis/dwell/timeout logic, and the sandbox state outputs.
5. **Bug: `aci_gap_start_observation_proc failed: 0xc`.** `0x0C` is *command disallowed*. The observation procedure only runs in the Observer role, and the example had initialised GAP as Central only. Fixed by adding `GAP_OBSERVER_ROLE` in `Ble_Hci_Gap_Gatt_Init()`.
6. **Reports arrived, but the LED didn't change.** The filtered RSSI peaked around -67 dBm, which never crossed the original -60 dBm threshold from the project brief. The thresholds were lowered to -70/-80 to match real readings.

## Known limits
- Only the first report in each advertising event is parsed. ST's controller sends one per event (see the `shortcut:` comment in `app_ble.c`).
- PA3/PA2 are plain on/off levels copied from sandbox. A motor driver that needs PWM, step pulses, or a position loop still needs that layer.
- When several trigger devices are near at once, the last one to become near wins.

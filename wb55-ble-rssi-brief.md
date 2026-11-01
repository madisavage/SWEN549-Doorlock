# Project brief: Multi-phone BLE RSSI proximity triggers on NUCLEO-WB55RG

## Goal
Measure signal strength (RSSI) of several phones at once and trigger commands when each crosses near/far thresholds. The board can't hold many simultaneous connections, so **don't connect at all**: scan BLE advertisements passively and read RSSI from each advertising report.

## Hardware / toolchain
- Board: NUCLEO-WB55RG (STM32WB55, Cortex-M4 app core + M0+ radio core, CPU2).
- SDK: STM32CubeWB, STM32CubeIDE/CubeMX.
- Prerequisite (manual, done by user): FUS + `stm32wb5x_BLE_Stack_full_fw.bin` flashed via STM32CubeProgrammer at the address in the release notes. Without it, the app hangs waiting for CPU2.

## Starting point
- Base on the CubeWB example `Projects/P-NUCLEO-WB55.Nucleo/Applications/BLE/BLE_p2pClient`.
- Strip the connect logic and keep the scanning. GAP role: Observer/Central.

## Scanning
- Prefer `aci_gap_start_observation_proc(interval, window, scan_type=passive, own_addr=public, filter_duplicates=0, filter_policy=accept_all)`. It scans continuously.
- Fallback: `aci_gap_start_general_discovery_proc(...)`, which times out after about 10 s and raises `ACI_GAP_PROC_COMPLETE`. Restart it on that event.
- **Filter_Duplicates must be 0**, or RSSI won't update.
- Use window == interval (e.g. 0x0050) for near-100% duty.

## Getting RSSI
- In `app_ble.c` → `SVCCTL_App_Notification()`, handle `HCI_LE_META_EVT_CODE` / `HCI_LE_ADVERTISING_REPORT_SUBEVT_CODE`.
- Iterate the `Num_Reports` reports. RSSI is the signed byte immediately after the AD data: `(int8_t)Data_RSSI[Length_Data]`.
- Advance each report by `sizeof(Advertising_Report_t) + Length_Data + 1`.
- Verify struct and field names against `ble_hci_le.h` / `ble_types.h` in the installed CubeWB version, because they vary across releases.

## Identifying phones
- Phones randomize MAC addresses, so identify them by **payload**, not by address.
- Parse AD structures (len, type, data). Match on manufacturer-specific data (type `0xFF`) containing a magic number + a phone ID byte. Optionally also match a custom 128-bit service UUID (type `0x06`/`0x07`) with service data (`0x21`).
- Keep a device table: `{id, rssi_filtered, near, dwell_start, last_seen}`.

## Signal processing (RSSI is noisy, about ±10 dBm)
- EMA smoothing: `f = 0.25*raw + 0.75*f`.
- Hysteresis: enter "near" above -60 dBm; exit below -72 dBm. Make these tunable constants.
- Dwell: the state change must persist 2000 ms before firing.
- Timeout: no packets for about 5 s → treat as "far/gone".
- Fire `cmd_near(id)` / `cmd_far(id)` on transitions. Initially toggle the Nucleo LEDs via BSP; later drive GPIO or other outputs.

## Debug
- Log `id, rssi_raw, rssi_filtered, state` over the ST-LINK virtual COM port (LPUART1) for threshold tuning.

## Phone side (separate)
- Android: an app using `BluetoothLeAdvertiser` with manufacturer data, run as a foreground service. For quick tests, use nRF Connect → Advertiser tab with custom manufacturer data.
- iOS: background advertising hides service UUIDs and drops manufacturer data, so it needs the app in the foreground or a role flip (board advertises, phone connects briefly).

## Suggested first task for Claude Code
1. Adapt `BLE_p2pClient` to scan continuously with duplicates unfiltered.
2. Parse advertising reports and print every matched device's RSSI over VCOM.
3. Add the filter, hysteresis, dwell, and timeout state machine with LED triggers.

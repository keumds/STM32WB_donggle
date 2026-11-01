# ANVIX P1 Dongle v0.5 (UUID128 verification patch)

## Important
This is a source-level patch of v0.4, **not** a tested and production-ready firmware. Keil hardware compile and integration have not been performed here.

## Confirmed UUIDs from Symbio Sigview
- Service: 0000fe40-cc7a-482a-984a-7f2ed5b3e58f
- FE41 write: 0000fe41-8e22-4541-9d4c-21edae82ed19
- FE42 control notify: 0000fe42-8e22-4541-9d4c-21edae82ed19
- FE43 data notify: 0000fe43-8e22-4541-9d4c-21edae82ed19

Service has a distinct base UUID from characteristics. The old v0.4 compared only FE4x short portions and did not check UUID bases. This v0.5 patch compares full 16-byte UUIDs as serialized in BLE events. Service list and characteristic list parsing rely on STM32CubeWB event layout and require board validation.

## First test
1. Rebuild all in Keil; flash newly generated HEX, verify timestamp.
2. Look for `[ANVIX P1] GATT UUID128 v0.5` in the COM debug log.
3. Verify `[P1] FE43 stream handle=...` appears. If not, capture service/characteristic discovery output.
4. No `+b+` command is sent, as device starts upon BLE connection.

## Caution
USB debug and EEG binary may be multiplexed on the same USB VCP; they MUST be separated before regular acquisition. This patch is for GATT UUID diagnosis, not validated RAW USB stream.

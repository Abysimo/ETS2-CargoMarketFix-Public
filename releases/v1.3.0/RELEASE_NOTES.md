# CargoMarketFix v1.3.0

Adds exact-build support for **ETS2 1.60.1.7**, Windows x64, retaining **1.57.2.7, 1.58.1.4 and 1.59.1.3**.

**ETS2 1.60.1.7: SUPPORTED / STRUCTURALLY VALIDATED**

**LIVE STARTUP SMOKE PASSED**

**GAMEPLAY PERFORMANCE NOT PHYSICALLY VALIDATED**

## Changes

- CAP4 current-trailer Cargo Market generation-attempt limit: budget 4.
- 60-minute normal refresh spreading, with the validated 1.60-specific bridge.
- Validated quiesced two-byte CAP4 backend for the 1.60 target and auxiliary-repair safety handling.
- **Patches are opt-in and disabled by default.** Unlike previous release packages, the default INI does not activate either behavior patch. Use the separate active example only when explicitly opting in.
- Unknown executable builds fail closed. Exact hashes and signatures are required; matching is not broadened. Historical/research observers, including membership shadow, are not compiled into the public DLL.

## Validation and limitations

The full automated suite passed during private validation (**1183/1183**). The validated private 1.60 build passed a real ETS2 startup smoke: DLL identity and load were confirmed, the exact executable was recognized, initialization succeeded, CAP4 target bytes matched and budget 4 installed, and Refresh Spread was ACTIVE for 60 minutes. Observers remained OFF. The game remained responsive in the menu for **131 seconds after initialization**, with no observed plugin/crash errors or repeated initialization.

Result: **startup smoke passed with shutdown untested**. The single normal-close attempt did not complete; clean live shutdown/restoration was **not confirmed**. Offline/native restoration tests do not replace that missing live evidence.

**Gameplay performance on ETS2 1.60 was NOT physically validated. Actual recurring-freeze reduction is NOT measured. Owned-trailer gameplay, sleep/bulk gameplay and Cargo Market offer quality were NOT tested.** No such performance or gameplay claim is made. The public 1.3.0 release wrapper/build is validated offline; the live smoke used private version 0.12.0 with the promoted patch implementation, not this public binary.

## Installation and opt-in

Close ETS2 before installation. Extract `CargoMarketFix.dll` and default `CargoMarketFix.ini` into `Euro Truck Simulator 2/bin/win_x64/plugins/` (create `plugins` if needed). Both patches are initially OFF.

To opt in to CAP4 plus 60-minute Refresh Spread, copy the packaged `config_examples/CargoMarketFix.cap4-spread-active.ini` to that plugin directory **as `CargoMarketFix.ini`**, replacing the disabled default. All observers stay OFF. This is not an `.scs` mod and does not use Mod Manager. Check the new startup log for exact-build acceptance and both active patches.

Normal offer refresh may be delayed by up to approximately 59 in-game minutes, with fewer or less-fresh offers possible. The algorithm does not intentionally spread activation or bulk/sleep refresh; that statement describes design, not new 1.60 gameplay evidence. Initial trailer activation may still take time.

## Exact executable set

| ETS2 | Executable SHA256 |
| --- | --- |
| 1.57.2.7 | `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9` |
| 1.58.1.4 | `25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644` |
| 1.59.1.3 | `E6FE1A58DF9D0BFFF21DCCC12B4F581DC0D3E3B5885DBF62ED05028DE794D35C` |
| 1.60.1.7 | `B7DFFE6B27402C7DB6DFD52CF982CD5BF292584138B35E3EB8EFB311814AB3F8` |

1.57 retains its previous practically tested status; 1.58 and 1.59 retain structurally validated status. Other executables, including other 1.60 builds, are not implicitly supported.

## Package

`ETS2-CargoMarketFix-v1.3.0-ETS2-1.60.1.7.zip` contains exactly the DLL, disabled default INI, active example INI, README and MIT LICENSE. The separate `SHA256SUMS.txt` asset records DLL and ZIP checksums. Previous releases remain unchanged.

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 324608 | `E4DCF6A5335AE642B73DC24616C068A2CCA1ECBF60C0850AA4CE0E5C3810CFE4` |
| ETS2-CargoMarketFix-v1.3.0-ETS2-1.60.1.7.zip | 165911 | `D9BC0FFDFA854E6CB857A257CC8DFC435CF552796F7A2A5173319D81B1706CE3` |

Public packaging validation: clean Release x64 build; **259/259 named maintenance cases, 7/7 suites, one run without retries**. Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`. Default INI parity, active example, exact four-build gates, rollback/containment and native restoration checks passed. No deployment or additional game run was performed for this public binary.

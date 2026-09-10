# ETS2 Cargo Market Fix v1.3.1 — ETS2 1.60.1.7

This release/package is specifically for **ETS2 1.60.1.7**, Windows x64.
Executable SHA256: `B7DFFE6B27402C7DB6DFD52CF982CD5BF292584138B35E3EB8EFB311814AB3F8`.

**SUPPORTED / STRUCTURALLY VALIDATED**

PRIOR LIVE STARTUP SMOKE PASSED. 180-MINUTE GAMEPLAY PERFORMANCE NOT PHYSICALLY VALIDATED. The prior private 60-minute build installed CAP4 and the dedicated Refresh Spread bridge and remained responsive at the menu for 131 seconds. That is historical patch-site compatibility evidence, not a live test of this 180-minute public binary. Clean live shutdown/restoration was not confirmed. Owned-trailer performance, sleep/bulk gameplay and offer quality were not physically validated on this version.

## 180-minute maintenance update

- CAP4 remains fixed at four current-trailer Cargo Market generation attempts.
- Refresh Spread accepts exactly 60 / 120 / 180 minutes; recommended active setting: **180**.
- Normal company refresh is spread across **three in-game hours**, with up to approximately **179 in-game minutes** of offer-refresh delay.
- Bulk/sleep and trailer activation are not intentionally spread by this scheduler; CAP4 and those established paths are unchanged.
- Both patches remain **opt-in and disabled by default**. The packaged active example enables both, with 180 minutes and observers OFF.
- Unsupported executable builds fail closed. No broader matching, memoization, new observer or residual-stutter investigation is included.

## Validation

Focused affected checks: **214/214** named cases.
One Release x64 maintenance run: **313/313** named cases, **7/7 suites**, 255.57 seconds, no retries. Covers exact descriptors, 60/120/180 contiguous coverage (including empty/uneven and 12921-company inputs), original CAP4, native state/unwind, install/restore/rollback/containment, configuration and unknown-build refusal.

The exact target hash/sites/ABI are retained from the previously certified private port; that historical executable was not freshly reread in this rollout. Fresh target-specific authored image tests exercise its exact descriptor, signature isolation, 180 publication, install/restore and fail-closed handling. Native bridge fixtures execute the corresponding validated ABI. The currently installed 1.57 image is also checked read-only by the common maintenance suite; that read is not presented as a fresh 1.60.1.7 executable validation.

The certified spread implementation is promoted from private main without changing its algorithm or bridge ABI. Each release has its own version metadata, build, ZIP, notes, checksums and tag. The shared binary internally recognizes the certified four-build set, but use the release specifically labeled for your game. This task did not launch ETS2, perform gameplay QA or deploy files.

## Install and opt in

Close ETS2. Put the DLL and disabled default INI into `Euro Truck Simulator 2/bin/win_x64/plugins/`.
To explicitly activate CAP4 plus 180-minute Spread, use the packaged `config_examples/CargoMarketFix.cap4-spread-active.ini` as `CargoMarketFix.ini` in that directory. Check the startup log for exact-build acceptance and `minutes=180`. No observers are compiled. This is not an .scs mod. Fewer/less-fresh offers are possible; no absolute freeze-elimination claim.

## Integrity

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 325120 | `6AC832A1E82436F72F0CA35453CDC1A43D5304A84299CD6017E52DC204AA9FB0` |
| ETS2-CargoMarketFix-v1.3.1-ETS2-1.60.1.7.zip | 165615 | `35F555326DF5513D98294A3B71F2F757CE422503C6D398958CFDB9210E65538D` |

Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`.
ZIP contains DLL, disabled INI, active example INI, target-scoped README and MIT LICENSE. Separate SHA256SUMS.txt records DLL/ZIP hashes. Old releases/assets remain unchanged.

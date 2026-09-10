# ETS2 Cargo Market Fix v1.0.1 — ETS2 1.57.2.7

This release/package is specifically for **ETS2 1.57.2.7**, Windows x64.
Executable SHA256: `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`.

**SUPPORTED / PRACTICALLY TESTED**

On the original very-large-map owned-trailer test setup, the 180-minute refresh spread reduced the recurring Cargo Market freezes to a very small residual stutter. The user accepts this result. This is not complete elimination or a universal guarantee. The public packaging binary is validated offline; the gameplay feedback used the corresponding private implementation.

## 180-minute maintenance update

- CAP4 remains fixed at four current-trailer Cargo Market generation attempts.
- Refresh Spread accepts exactly 60 / 120 / 180 minutes; recommended active setting: **180**.
- Normal company refresh is spread across **three in-game hours**, with up to approximately **179 in-game minutes** of offer-refresh delay.
- Bulk/sleep and trailer activation are not intentionally spread by this scheduler; CAP4 and those established paths are unchanged.
- Both patches remain **opt-in and disabled by default**. The packaged active example enables both, with 180 minutes and observers OFF.
- Unsupported executable builds fail closed. No broader matching, memoization, new observer or residual-stutter investigation is included.

## Validation

Focused affected checks: **148/148** named cases.
One Release x64 maintenance run: **313/313** named cases, **7/7 suites**, 267.68 seconds, no retries. Covers exact descriptors, 60/120/180 contiguous coverage (including empty/uneven and 12921-company inputs), original CAP4, native state/unwind, install/restore/rollback/containment, configuration and unknown-build refusal.

The installed exact 1.57.2.7 executable was SHA256-checked and mapped read-only for signature validation.

The certified spread implementation is promoted from private main without changing its algorithm or bridge ABI. Each release has its own version metadata, build, ZIP, notes, checksums and tag. The shared binary internally recognizes the certified four-build set, but use the release specifically labeled for your game. This task did not launch ETS2, perform gameplay QA or deploy files.

## Install and opt in

Close ETS2. Put the DLL and disabled default INI into `Euro Truck Simulator 2/bin/win_x64/plugins/`.
To explicitly activate CAP4 plus 180-minute Spread, use the packaged `config_examples/CargoMarketFix.cap4-spread-active.ini` as `CargoMarketFix.ini` in that directory. Check the startup log for exact-build acceptance and `minutes=180`. No observers are compiled. This is not an .scs mod. Fewer/less-fresh offers are possible; no absolute freeze-elimination claim.

## Integrity

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 325120 | `03471A0F80A9921799EE8713BEFCFF3DD957499FCCF43C680192A945C1CE1C25` |
| ETS2-CargoMarketFix-v1.0.1-ETS2-1.57.2.7.zip | 165527 | `CC5883E28D85C51DCBDA3FB71E9C8BD03E21E36AA1C06E83F49C40A2F8F6FCE9` |

Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`.
ZIP contains DLL, disabled INI, active example INI, target-scoped README and MIT LICENSE. Separate SHA256SUMS.txt records DLL/ZIP hashes. Old releases/assets remain unchanged.

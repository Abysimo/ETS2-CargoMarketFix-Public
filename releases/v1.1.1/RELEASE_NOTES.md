# ETS2 Cargo Market Fix v1.1.1 — ETS2 1.58.1.4

This release/package is specifically for **ETS2 1.58.1.4**, Windows x64.
Executable SHA256: `25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644`.

**SUPPORTED / STRUCTURALLY VALIDATED**

180-minute gameplay performance has not been physically validated on this version. Exact descriptors and previously validated patch sites are retained, with native coverage and release maintenance checks. No freeze-reduction or gameplay-quality claim is made for this version.

## 180-minute maintenance update

- CAP4 remains fixed at four current-trailer Cargo Market generation attempts.
- Refresh Spread accepts exactly 60 / 120 / 180 minutes; recommended active setting: **180**.
- Normal company refresh is spread across **three in-game hours**, with up to approximately **179 in-game minutes** of offer-refresh delay.
- Bulk/sleep and trailer activation are not intentionally spread by this scheduler; CAP4 and those established paths are unchanged.
- Both patches remain **opt-in and disabled by default**. The packaged active example enables both, with 180 minutes and observers OFF.
- Unsupported executable builds fail closed. No broader matching, memoization, new observer or residual-stutter investigation is included.

## Validation

Focused affected checks: **148/148** named cases.
One Release x64 maintenance run: **313/313** named cases, **7/7 suites**, 265.11 seconds, no retries. Covers exact descriptors, 60/120/180 contiguous coverage (including empty/uneven and 12921-company inputs), original CAP4, native state/unwind, install/restore/rollback/containment, configuration and unknown-build refusal.

The exact target hash/sites/ABI are retained from the previously certified private port; that historical executable was not freshly reread in this rollout. Fresh target-specific authored image tests exercise its exact descriptor, signature isolation, 180 publication, install/restore and fail-closed handling. Native bridge fixtures execute the corresponding validated ABI. The currently installed 1.57 image is also checked read-only by the common maintenance suite; that read is not presented as a fresh 1.58.1.4 executable validation.

The certified spread implementation is promoted from private main without changing its algorithm or bridge ABI. Each release has its own version metadata, build, ZIP, notes, checksums and tag. The shared binary internally recognizes the certified four-build set, but use the release specifically labeled for your game. This task did not launch ETS2, perform gameplay QA or deploy files.

## Install and opt in

Close ETS2. Put the DLL and disabled default INI into `Euro Truck Simulator 2/bin/win_x64/plugins/`.
To explicitly activate CAP4 plus 180-minute Spread, use the packaged `config_examples/CargoMarketFix.cap4-spread-active.ini` as `CargoMarketFix.ini` in that directory. Check the startup log for exact-build acceptance and `minutes=180`. No observers are compiled. This is not an .scs mod. Fewer/less-fresh offers are possible; no absolute freeze-elimination claim.

## Integrity

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 325120 | `650E75C30CCAEE8B0697BC801C0BC4DF42A23C31E2C341F4AF1395D9C2617E71` |
| ETS2-CargoMarketFix-v1.1.1-ETS2-1.58.1.4.zip | 165487 | `202ECB6101A0B8C551975C7834F9176745595DDAB42307C525BDB7421760C00B` |

Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`.
ZIP contains DLL, disabled INI, active example INI, target-scoped README and MIT LICENSE. Separate SHA256SUMS.txt records DLL/ZIP hashes. Old releases/assets remain unchanged.

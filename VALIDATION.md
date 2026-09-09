# Release snapshot validation

Version: 1.0.0. Validated executable: ETS2 1.57.2.7, SHA256
`06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`.

This is a fresh source snapshot, not a history export. The production refresh-spread implementation, assembly bridge, patch-site headers and lifecycle/platform machinery are retained unchanged. CAP4's implementation differs only in its startup log wording. The release-specific changes are a minimal SDK entry point, a small fail-closed configuration reader, version metadata and removal of a machine-specific log fallback. No observation implementation is compiled.

CAP4 remains the fixed two-byte change at RVA `0x006CE618`: `73 0A` to `90 90`, with four generation attempts. The normal-minute spread remains at `0x003EC647`, using 60 balanced contiguous buckets and `game minute % 60`. There is no persistent scheduler state, backlog, query observer or alteration of the bulk/activation paths beyond CAP4.

## Offline checks

Release x64, MSVC 19.51, Windows SDK 10.0.28000, static CRT. The required SDK headers were supplied locally, not included in the repository. No game process was launched and no deployed files were changed.

One complete maintenance run: **95/95 named cases**, four suites, 1.48 seconds:

| Suite | Cases |
| --- | ---: |
| CAP4 native/configuration/rollback/containment | 37 |
| Spread native/partition/state/unwind/quiescence | 37 |
| Release defaults/exact executable gates | 14 |
| Actual release DLL in an unsupported authored host | 7 |

The native suites' `default_disabled` cases refer to `tests/disabled.ini`, not the enabled public INI. Their original assertions remain intact; input paths are supplied by the test runner instead of assuming an installation location. The release cases separately require both public fixes enabled and observers off. The host test verifies exports are callable, invalid SDK/API inputs are rejected, an unsupported executable receives no patch, no observer registration occurs and the DLL is not pinned on that refusal.

Native fixtures verify exact CAP4 bytes, four attempts, balanced complete hourly coverage, original continuations, register/FP preservation, normal restoration, exception propagation, containment and unsafe-thread-range rejection. Game-image inputs are mapped read-only with execution disabled. This is not a new real-game performance certification of the packaging-only build.

## Local DLL identity

- Size: 316928 bytes.
- SHA256: `2718E19B6FFA0FAAEDA550D61748992955C0F2A98F71DF7592A099BCE3B3BA8E`.
- Exports only: `scs_telemetry_init`, `scs_telemetry_shutdown`.
- Dependencies: Windows `bcrypt.dll`, `KERNEL32.dll`.
- Built INI matches the source public INI byte-for-byte.

The DLL and local ZIP are deliberately not versioned or uploaded. License selection remains pending; no LICENSE is supplied. Independent review is required before making this repository public or publishing release artifacts.

The rebuilt bridge's code and unwind sections match the working implementation byte-for-byte. Its code-section SHA256 is `802A24864C844588F7FBC6868898B8248C9598F3DC7D11AF6B6BCD82850A912A`.

Local package: `ETS2-CargoMarketFix-v1.0.0-ETS2-1.57.zip`, 158390 bytes, SHA256 `99CD8F8CA84F1CB34A95304E03F161F0873A89009B23F96FA5F33B40DBE49D42`. It contains only the DLL, supplied INI and README. Source files and package printable strings passed credential/private-identifier checks. The source tree contains no SDK headers, game binaries, assets, saves, research archive or raw dumps. The sole build script invokes local compiler/build tools and file copying only, with no network or credential operations.

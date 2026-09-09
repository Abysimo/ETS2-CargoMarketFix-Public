# Release snapshot validation

Version: 1.0.0. Validated executable: ETS2 1.57.2.7, SHA256
`06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`.

This is a fresh source snapshot, not a history export. The production refresh-spread implementation, assembly bridge, patch-site headers and lifecycle/platform machinery are retained unchanged. CAP4's implementation differs only in its startup log wording. The release-specific changes are a minimal SDK entry point, a small fail-closed configuration reader, version metadata and removal of a machine-specific log fallback. No observation implementation is compiled.

CAP4 remains the fixed two-byte change at RVA `0x006CE618`: `73 0A` to `90 90`, with four generation attempts. The normal-minute spread remains at `0x003EC647`, using 60 balanced contiguous buckets and `game minute % 60`. There is no persistent scheduler state, backlog, query observer or alteration of the bulk/activation paths beyond CAP4.

## Initial snapshot offline checks (historical)

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

## Initial snapshot DLL identity (historical)

- Size: 316928 bytes.
- SHA256: `2718E19B6FFA0FAAEDA550D61748992955C0F2A98F71DF7592A099BCE3B3BA8E`.
- Exports only: `scs_telemetry_init`, `scs_telemetry_shutdown`.
- Dependencies: Windows `bcrypt.dll`, `KERNEL32.dll`.
- Built INI matches the source public INI byte-for-byte.

The DLL and local ZIP are deliberately not versioned or uploaded. The initial historical package contained no license. The current package includes the user-approved MIT LICENSE. Independent review and explicit publication approval are still required.

The rebuilt bridge's code and unwind sections match the working implementation byte-for-byte. Its code-section SHA256 is `802A24864C844588F7FBC6868898B8248C9598F3DC7D11AF6B6BCD82850A912A`.

Historical local package, NOT for publication: `ETS2-CargoMarketFix-v1.0.0-ETS2-1.57.zip`, 158390 bytes, SHA256 `99CD8F8CA84F1CB34A95304E03F161F0873A89009B23F96FA5F33B40DBE49D42`. It contains only the DLL, supplied INI and README. Source files and package printable strings passed credential/private-identifier checks. The source tree contains no SDK headers, game binaries, assets, saves, research archive or raw dumps. The sole build script invokes local compiler/build tools and file copying only, with no network or credential operations.

## Exact-build release package preparation (pre-license historical build)

A clean Release x64 rebuild of the unchanged source and build configuration completed for the exact-build asset naming workflow. The maintenance suite ran once: **95/95**, four suites, 1.73 seconds. No game launch, deployment or new practical performance test occurred.

The rebuilt DLL remains 316928 bytes. Its raw SHA256 is `EF5A96BD382B0EBEA03485A68D57C7FAE07A428EEF614334F1A913241A89073D`, different from the preceding DLL solely because of linker metadata. All differing bytes lie in the PE/COFF TimeDateStamp field at file offset 272 and IMAGE_DEBUG_DIRECTORY.TimeDateStamp at offset 263028. Both fields changed from 1788977674 to 1788979136. The two fields were located by parsing the headers, not by masking arbitrary differing bytes.

After zeroing only those two timestamp fields in memory, the complete files are byte-identical, with normalized SHA256 `81A2CFF236C0F6E1A967EE43501830A249372FDBB5FA4B3EA1221E4D72CDC247`. No artifact bytes were edited. Code, other data, unwind tables, resources, relocations and exports are unchanged. Raw output is therefore not bit-reproducible across link times with this configuration; the new raw hash is explicitly recorded, not silently substituted.

Exports remain exactly `scs_telemetry_init` and `scs_telemetry_shutdown`. Supplied INI remains byte-identical to source. The new ZIP includes the current user-facing README and uses the exact supported-build filename. Current sizes/checksums are maintained in [v1.0.0 notes](releases/v1.0.0/RELEASE_NOTES.md) and [SHA256SUMS](releases/v1.0.0/SHA256SUMS.txt), not the historical family-only package above.

All artifacts remain local and unapproved for publication. The MIT license gate is now satisfied; final ChatGPT review and explicit publication approval remain blocking requirements. Any change to packaged files requires repackaging, fresh checksums and a new artifact scan before publication.

## MIT-licensed final artifact preparation

The user approved the standard MIT License with `Copyright (c) 2026 Abysimo`. The license was checked against the standard GitHub MIT template with only the year/holder substitution. Repository-role and publication-approval rules remain intact; no license gate remains pending.

One clean Release x64 rebuild and one maintenance run completed: **95/95**, four suites, 1.82 seconds. Source, configuration, tests and build metadata were unchanged. The DLL is 316928 bytes with SHA256 `0CC2F6290ABCB670218927B7BCC49D74E153CF688865CAEB0FC05C1FDA1B6ABA`.

Compared with the preceding prepared DLL (`EF5A96BD382B0EBEA03485A68D57C7FAE07A428EEF614334F1A913241A89073D`), only the PE/COFF and debug-directory timestamps changed, from 1788979136 to 1788979824, at parsed file offsets 272 and 263028. Normalizing those two fields in memory produces byte-identical complete files with SHA256 `81A2CFF236C0F6E1A967EE43501830A249372FDBB5FA4B3EA1221E4D72CDC247`. No DLL bytes were edited; there are no code/data/unwind/export changes beyond those two metadata fields. Exports remain exactly `scs_telemetry_init` and `scs_telemetry_shutdown`.

The fresh `ETS2-CargoMarketFix-v1.0.0-ETS2-1.57.2.7.zip` is 160141 bytes, SHA256 `8937E1950C9B446DB356FD9E6D91AE5B60EB18F7288D7C8266EB8BF04F2C5357`. It contains exactly `CargoMarketFix.dll`, `CargoMarketFix.ini`, `README.md` and `LICENSE`; each entry matches its reviewed source/built file. Historical packages above are not the current release candidate. The current notes and SHA256SUMS identify this licensed package.

No game launch, deployment, tag, Release, asset upload or visibility change occurred. The artifacts await final ChatGPT review and explicit user publication approval.

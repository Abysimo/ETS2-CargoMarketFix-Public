# Release snapshot validation

## v1.2.1 — ETS2 1.59.1.3 180-minute maintenance

Focused 148/148; one Release maintenance run 313/313, seven suites, 259.65 seconds, no retries. Default patches OFF; active example 180. CAP4/sites/lifecycle unchanged. Exports, package entries, byte identities, default parity and public-safe DLL content verified. The exact target hash/sites/ABI are retained from the previously certified private port; that historical executable was not freshly reread in this rollout. Fresh target-specific authored image tests exercise its exact descriptor, signature isolation, 180 publication, install/restore and fail-closed handling. Native bridge fixtures execute the corresponding validated ABI. The currently installed 1.57 image is also checked read-only by the common maintenance suite; that read is not presented as a fresh 1.59.1.3 executable validation. See [release notes](releases/v1.2.1/RELEASE_NOTES.md) for exact hashes and qualification.

## v1.1.1 — ETS2 1.58.1.4 180-minute maintenance

Focused 148/148; one Release maintenance run 313/313, seven suites, 265.11 seconds, no retries. Default patches OFF; active example 180. CAP4/sites/lifecycle unchanged. Exports, package entries, byte identities, default parity and public-safe DLL content verified. The exact target hash/sites/ABI are retained from the previously certified private port; that historical executable was not freshly reread in this rollout. Fresh target-specific authored image tests exercise its exact descriptor, signature isolation, 180 publication, install/restore and fail-closed handling. Native bridge fixtures execute the corresponding validated ABI. The currently installed 1.57 image is also checked read-only by the common maintenance suite; that read is not presented as a fresh 1.58.1.4 executable validation. See [release notes](releases/v1.1.1/RELEASE_NOTES.md) for exact hashes and qualification.

## v1.0.1 — ETS2 1.57.2.7 180-minute maintenance

Focused 148/148; one Release maintenance run 313/313, seven suites, 267.68 seconds, no retries. Default patches OFF; active example 180. CAP4/sites/lifecycle unchanged. Exports, package entries, byte identities, default parity and public-safe DLL content verified. The installed exact 1.57.2.7 executable was SHA256-checked and mapped read-only for signature validation. See [release notes](releases/v1.0.1/RELEASE_NOTES.md) for exact hashes and qualification.

## v1.3.0 compatibility release

Promoted the already validated private 1.60 patch backends without functional edits: exact descriptors, CAP4 quiesced-copy backend, auxiliary-repair lifecycle safety, Refresh Spread owner and dedicated 1.60 MASM bridge match the validated private source (apart from line endings). The public minimal wrapper/parser remains separate, and no historical observer or membership code is imported. Public version metadata is 1.3.0.

One clean MSVC/MASM x64 Release build and one maintenance run passed: **259/259 named cases, 7/7 suites, 231.87 seconds, no retries**. Suite counts: CAP4 37, legacy spread 37, release contract 17, plugin host 7, four-build descriptors 61, quiesced two-byte backend 37, 1.60 spread backend 63. The backend suites include 1,000 CAP4 install/restore cycles and 100 spread boundary cycles. Unknown-build refusal with patches requested, default-disabled no-install/no-PIN, opt-in example, four exact descriptors, native restoration, rollback, containment, ABI and exception behavior are covered. The private 1183-case campaign was not rerun.

Release DLL: **324608 bytes**, SHA256 `E4DCF6A5335AE642B73DC24616C068A2CCA1ECBF60C0850AA4CE0E5C3810CFE4`. Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`. Default source/built INI SHA256 `B8BC2D8AF747AE31F17D7F3F22235C5A155F45A69BF158EB1983DAD209FAF0FF`; CAP4 and Spread are both disabled. The active example enables both, 60 minutes, with no observers. Public source and DLL text were screened for private paths, private version branding, credentials and unexpected endpoints; none were found in promoted source/binary.

The validated private 0.12.0 DLL (522752 bytes; SHA256 `E6BA99AC82E365EDFB7A090CC59A51EE3B288DC82DD307D0992290CDA2BE2FCD`) was used for the live smoke, not the smaller public-only 1.3.0 binary. The private build's full automated suite passed 1183/1183. Live ETS2 1.60.1.7 recognized the expected executable hash, installed CAP4 budget 4 and 60-minute Spread, and remained responsive in the menu 131 seconds after initialization with observers OFF and no observed plugin/crash errors. No repeated initialization occurred.

Qualification: **SUPPORTED / STRUCTURALLY VALIDATED; LIVE STARTUP SMOKE PASSED; GAMEPLAY PERFORMANCE NOT PHYSICALLY VALIDATED.** Clean live shutdown/restoration was not confirmed. Owned-trailer gameplay, actual freeze reduction, sleep/bulk gameplay and offer quality were not tested on 1.60. Offline restoration passes do not fill that live gap. This promotion performed no game launch, deployment or gameplay interaction. No forced termination was used; the previously identified process was already absent when checked.

See [v1.3.0 release notes](releases/v1.3.0/RELEASE_NOTES.md) and its checksums for the final package. Historical sections below retain their original version-specific defaults and validation scope; they do not override v1.3.0's disabled defaults.

## v1.1.0 compatibility release

ETS2 1.57.2.7: **SUPPORTED / PRACTICALLY TESTED**, retained from v1.0.0.
ETS2 1.58.1.4: **SUPPORTED / STRUCTURALLY VALIDATED**.
The original very-large-map owned-trailer reproduction setup was unavailable on 1.58; performance was not practically re-certified in-game. No physical test is requested or required solely to compensate for that unavailable setup.

This is a sanitized promotion of the completed compatibility port. Only per-build metadata, exact-hash selection, corresponding signature/range selection, startup identification and minimal maintenance tests changed. No private history, research documents or observation infrastructure were imported. The shared four-attempt CAP4 behavior, 60-bucket bridge, lifecycle/platform backend, supplied enabled INI and MIT license remain unchanged.

| Exact build | Executable SHA256 | CAP4 RVA | Spread gate |
| --- | --- | --- | --- |
| 1.57.2.7 | `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9` | `0x006CE618` | `0x003EC647` |
| 1.58.1.4 | `25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644` | `0x006F4CE8` | `0x00407627` |

The 1.58 executable is 49,878,928 bytes. CAP4 retains `73 0A -> 90 90`. The spread gate retains its nine-byte span and original continuation layout. Unknown hashes and cross-build descriptor use are rejected. The 1.58 live-image tests read the installed executable without executing it; retained 1.57 descriptors and install/restore behavior are exercised with authored fixtures.

Release x64 was built from this sanitized repository with MSVC 19.51 and static CRT. One maintenance run passed **122/122 named cases**, five suites, **2.63 seconds**, with no retries: CAP4 37, spread 37, release contract 14, actual-DLL unsupported host 7, and multi-build descriptors/install/restore 27. The release INI enables both fixes; observers are absent from the compiled source list. The host fixture confirms no observer registration and no module PIN on an unsupported executable.

DLL size: **320512 bytes**. SHA256: `86C47F0389E3321976681B1F8947014EAE36B6DE6D54DE6D02C740F1C1110103`.
Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`; dependencies are Windows `bcrypt.dll` and `KERNEL32.dll`. The built INI equals the source INI. Package integrity is recorded in [v1.1.0 checksums](releases/v1.1.0/SHA256SUMS.txt). No ETS2 launch, physical test request or deployment occurred in this promotion task.

The subsequent sections are **historical v1.0.0 preparation records**; their then-pending publication statements do not describe current release status.

The v1.1.0 ZIP is **162725 bytes**, SHA256 `C68D937A95790F31FFA88FC9679933C735011AF7C323C2B3D352308C342268BC`. Its four entries match the reviewed DLL, current README, enabled INI and unchanged MIT LICENSE byte-for-byte. Source/asset checks found no forbidden personal paths, credential patterns, private research payloads or unexpected DLL network endpoints. Binaries, local transcripts, staging files, SDK files and game data are excluded from Git. The v1.0.0 release directory is unchanged.

## Historical v1.0.0 preparation

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

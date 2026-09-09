# Versioning and release promotion

AGENTS.md is authoritative. This is the release snapshot repository, not the development source of truth. All new compatibility work starts in the permanently private `Abysimo/ETS2-CargoMarketFix` repository. Never assume reverse synchronization.

## Publication gates

License gate: **SATISFIED — MIT License explicitly approved by the user**, included in LICENSE. Each release requires completed review and explicit publication approval. Do not change the license without explicit user approval or insert placeholder license text.

This document describes the workflow; it does not itself authorize publication. The repository is public following approval. The user separately authorized v1.1.0 publication after its release gates pass. Future releases still require their own approval.

## Independent versions

Plugin versions and ETS2 versions are independent. Use `vMAJOR.MINOR.PATCH` plugin tags. Each release supports only its explicitly validated executable set; a family name alone is not a compatibility guarantee.

| Plugin version | Mapping / policy |
| --- | --- |
| v1.0.0 | ETS2 1.57.2.7: supported / practically tested |
| v1.1.0 | Retains 1.57.2.7; adds 1.58.1.4: supported / structurally validated |
| v1.2.0 | Expected first validated ETS2 1.59 support; not implemented or promised |
| v1.3.0 | Expected first validated ETS2 1.60 support; not implemented or promised |

Use a MINOR increment for the first supported executable in a new game-version family. Use PATCH increments for compatibility updates within an already supported family or compatible bug fixes. For example only: v1.1.0 supports the first validated 1.58 executable; v1.1.1 adds or replaces support for another exact 1.58 executable. Do not invent future patch-level ETS2 numbers or assume older executable support is retained. State the complete supported set each time. A MAJOR increment is reserved for deliberately incompatible plugin/configuration changes approved through private development.

## One release per compatibility version

Create a separate GitHub Release for every plugin release. Do not maintain one forever-replaced binary release or silently overwrite released artifacts. Corrections to binaries require a new version, validation and checksums. Keep older release identities understandable.

Every release must specify plugin version, exact supported ETS2 builds and executable hashes, practical-test status, changes/tradeoffs, installation reminder, fail-closed behavior, package filename, DLL SHA256 and ZIP SHA256.

Distinguish **SUPPORTED / PRACTICALLY TESTED** (large-map reproduction and recurring-freeze behavior checked in the game) from **SUPPORTED / STRUCTURALLY VALIDATED** (identity, native sites/tests and safety checks passed, but the practical scenario was unavailable). Never describe the latter as fully tested. Note if a packaging-only DLL was checked offline rather than rerun in-game. Do not request an unavailable physical test or block structural compatibility releases solely because a newer version lacks the original large-map setup. This policy applies to future ports unless the user says practical testing is possible; it does not pre-authorize support for an unvalidated build.

## Asset layout

Current release tag: `v1.1.0`. The published `v1.0.0` tag and assets must remain unchanged.

Current release title: **ETS2 Cargo Market Fix v1.1.0 — ETS2 1.58.1.4 support**.

Current package: `ETS2-CargoMarketFix-v1.1.0-ETS2-1.58.1.4.zip`. Its name highlights the new build, but the single DLL supports both exact 1.57.2.7 and 1.58.1.4 executables.

Do not publish broad names such as `latest.zip` or the old family-only package name. For a future multi-executable release within one family, use `ETS2-CargoMarketFix-vX.Y.Z-ETS2-<family>-validated-set.zip` and list every exact supported build prominently in the notes and checksum metadata; never imply arbitrary family-wide support.

ZIP contents are exactly `CargoMarketFix.dll`, `CargoMarketFix.ini`, `README.md` and the approved MIT `LICENSE`. No source, tests, build metadata, logs, debug files, game/SDK data, saves, profiles, mods, research or private paths.

Maintain per-version notes and `SHA256SUMS.txt` under `releases/<tag>/`. The checksum file contains bare filenames for the DLL and ZIP, with no local paths. Plan to upload it as a separate Release asset alongside the ZIP and repeat those two hashes in the release notes. Do not include it inside the minimal ZIP. A bare DLL upload is not required.

## Future port and promotion checklist

1. Update ETS2 locally only within an approved compatibility task.
2. Perform compatibility/reverse-engineering work exclusively in `Abysimo/ETS2-CargoMarketFix`.
3. Validate the exact executable identity and patch sites there.
4. Build/test the port privately and record structural and practical validation separately.
5. Obtain explicit user approval for promotion.
6. Export only sanitized release-ready changes into `Abysimo/ETS2-CargoMarketFix-Public` without private history or research artifacts.
7. Update the supported-version table, README compatibility, plugin version, release notes, package filename and DLL/ZIP checksums.
8. Clean-build Release x64 from the public snapshot and run its maintenance suite once. Verify exports, exact-build refusal and supplied INI. Stop on unexplained DLL identity changes; do not silently substitute a binary.
9. Package from an empty local staging directory. Verify exact ZIP entries and byte identity against the reviewed DLL, INI and README. Recompute SHA256 after any content or filename/packaging change; never inherit a prior checksum on trust. Scan tracked/staged files and package/DLL printable content for secrets, private data and unexpected endpoints. Keep binaries and local evidence untracked.
10. Commit reviewed source/docs/checksums and push. Verify inclusion of the approved license and complete final review. Confirm explicit publication and visibility approval before creating the matching tag/Release or uploading anything.
11. Once authorized, tag the reviewed release commit, create its matching GitHub Release, use the prepared title/notes, and attach the exact ZIP plus `SHA256SUMS.txt`. Verify live supported-build information, asset bytes/hashes and intended visibility. Never reuse an existing release/tag without explicit direction.

## Release records

Historical published notes/checksums remain in [releases/v1.0.0](releases/v1.0.0/RELEASE_NOTES.md). The new compatibility notes/checksums are in [releases/v1.1.0](releases/v1.1.0/RELEASE_NOTES.md). Keep each release immutable in practice: do not overwrite old assets.

Include the approved LICENSE in every package. Any later change to packaged files requires regenerating the ZIP and its checksums/notes and rescanning the final artifacts before approval. These prepared hashes are not valid for a later modified package. Gameplay source is unchanged by this workflow preparation.

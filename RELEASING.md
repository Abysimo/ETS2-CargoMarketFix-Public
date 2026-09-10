# Version-specific release workflow

This public repository receives sanitized, user-approved snapshots from the private development repository. AGENTS.md remains authoritative. MIT licensing is approved; publication still requires explicit user authorization.

## Per-game release lines

| Exact ETS2 | Plugin release line |
| --- | --- |
| 1.57.2.7 | v1.0.x |
| 1.58.1.4 | v1.1.x |
| 1.59.1.3 | v1.2.x |
| 1.60.1.7 | v1.3.x |

For the approved 180-minute rollout, process those lines in the order shown. Inspect live tags/releases, choose the next unused PATCH number within the corresponding line, validate, publish and verify each before proceeding. Never overwrite old releases, tags or assets. Shared binary support for certified hashes does not replace separately labeled per-game packages.

## Gates and package

Promote only already validated functionality. Retain exact-build fail-closed selection, CAP4 four attempts, and the correct legacy or 1.60 spread ABI. Spread accepts exactly 60/120/180; the recommended active setting is 180. Both behavior-changing gates remain false in the default INI. The separate active example enables CAP4 plus 180-minute Spread; no observers are compiled.

For each release, set coherent plugin/package version and exact game target metadata, build Release x64 from the public snapshot, run focused affected checks and ONE maintenance suite. Do not repeat a private research campaign. Document retained exact-executable evidence separately from fresh file reads and authored native tests; do not imply that an unavailable historical executable was reread.

Use title `ETS2 Cargo Market Fix vX.Y.Z — ETS2 A.B.C.D` and asset `ETS2-CargoMarketFix-vX.Y.Z-ETS2-A.B.C.D.zip`.

Build the ZIP from an empty local staging directory, containing exactly:

- CargoMarketFix.dll
- CargoMarketFix.ini (both patches disabled)
- config_examples/CargoMarketFix.cap4-spread-active.ini (both ON, 180)
- README.md (scoped to that exact game/version)
- LICENSE

Maintain notes and SHA256SUMS.txt under releases/<tag>. Upload the ZIP and checksum file; record DLL and ZIP hashes with bare filenames. Do not include binaries, logs, SDK/game files, private paths, saves, research, backups or paused work in source commits. Verify exports, default INI parity, active example, ZIP entries/byte identities, sanitized source/artifacts and license.

Commit reviewed source/docs/checksums, push, tag that commit, create the authorized GitHub Release, then verify live metadata, tag commit and downloaded asset hashes. Use the release matching the user's game; never direct them to another game's package.

## Qualification

1.57 is the practically tested reference: 180 reduced recurring freezes to a very small residual stutter on the original very-large-map owned-trailer setup, not complete elimination. 1.58 and 1.59 are structurally validated, without a physical gameplay-performance claim. 1.60 has prior 60-minute live startup evidence, not 180-minute gameplay certification; clean live shutdown/restoration was not confirmed.

Always explain the three-game-hour cycle and up to approximately 179-minute normal offer-refresh delay. Bulk/sleep and trailer activation are not intentionally spread by this scheduler. Unknown hashes fail closed. Historical release notes retain their original qualification/settings and are not rewritten.

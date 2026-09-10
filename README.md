# ETS2 Cargo Market Fix v1.0.1 — ETS2 1.57.2.7

This release package is specifically for **ETS2 1.57.2.7, Windows x64**.
Download **ETS2-CargoMarketFix-v1.0.1-ETS2-1.57.2.7.zip** from the [matching release](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/v1.0.1).

**SUPPORTED / PRACTICALLY TESTED.**

On the original very-large-map owned-trailer test setup, the 180-minute refresh spread reduced the recurring Cargo Market freezes to a very small residual stutter. The user accepts this result. This is not complete elimination or a universal guarantee. The public packaging binary is validated offline; the gameplay feedback used the corresponding private implementation.

## What it changes

CAP4 remains fixed at **four current-trailer Cargo Market generation attempts**. Refresh Spread supports exactly **60 / 120 / 180 in-game minutes**. The recommended active setting is now **180**: normal company refresh work is distributed in contiguous balanced partitions across **three in-game hours**, without a retained backlog.

**Both behavior patches remain opt-in and disabled by default.** All observers, including membership shadow, are absent from the public DLL. Unknown executable builds fail closed; exact hashes and signatures are required.

## Choose the release for your game

| Exact ETS2 build | Release line / current package | Qualification |
| --- | --- | --- |
| 1.57.2.7 | v1.0.1 | Practically tested reference |
| 1.58.1.4 | v1.1.0 | Structurally validated |
| 1.59.1.3 | v1.2.0 | Structurally validated |
| 1.60.1.7 | v1.3.0 | Structurally validated; prior startup smoke |

Each game version has its own release line and package. Do not substitute a different game's package. Older releases retain their original settings and documentation and remain unchanged. Internally this shared source recognizes the four certified exact hashes below; that does not change this package's single-game release scope.

- 1.57.2.7: `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`
- 1.58.1.4: `25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644`
- 1.59.1.3: `E6FE1A58DF9D0BFFF21DCCC12B4F581DC0D3E3B5885DBF62ED05028DE794D35C`
- 1.60.1.7: `B7DFFE6B27402C7DB6DFD52CF982CD5BF292584138B35E3EB8EFB311814AB3F8`

## Installation and explicit opt-in

1. Close ETS2 and back up the existing plugin DLL, INI and log.
2. Extract this game's package.
3. Copy `CargoMarketFix.dll` and `CargoMarketFix.ini` to `Euro Truck Simulator 2/bin/win_x64/plugins/` (create that directory if necessary).
4. The supplied default INI enables **neither patch**, although its preferred period is 180.
5. To opt in, copy `config_examples/CargoMarketFix.cap4-spread-active.ini` from this ZIP to the plugins directory **as `CargoMarketFix.ini`**. It enables CAP4 plus 180-minute Spread with observers off.
6. Launch normally and inspect `CargoMarketFix.log` for exact-build acceptance, CAP4 budget 4 and Spread `minutes=180`.

This is not an `.scs` mod and does not use Mod Manager. Close the game before changing configuration. Only 60, 120 and 180 are accepted; malformed, missing or unsupported values fail closed. Do not disable safety gates.

## Tradeoffs and limits

Normal company offer refresh can be delayed by up to approximately **179 in-game minutes** at 180 (119 at 120; 59 at 60). Offers may be fewer or less fresh. Each company is covered once per complete cycle when the company list remains fixed and every normal minute is processed; time jumps or changing lists are not a persistence/snapshot guarantee.

Bulk/sleep refresh and trailer activation are **not intentionally spread by this scheduler**; their established CAP4 behavior is unchanged. Initial activation may still take time. No claim of absolute freeze elimination is made.

## Updating, removal and troubleshooting

Replace both files using your exact game's release. Keep backups; do not reuse research INIs. To disable or remove, close ETS2, switch both install/enabled gates off or remove the DLL/INI. The executable on disk is not edited. Removal does not undo offers already saved by the game.

An unknown build receives no patches. After a game update, check compatibility again. A version-change notice can delay plugin initialization/log creation. For problems, inspect the log and report your exact game build and map setup, redacting personal information; do not upload saves or private traces.

## Privacy, build and license

No network access or uploaded telemetry. Output is a local plugin log. No historical/research observers are compiled.

Build with CMake 3.25+, Windows x64 MSVC/MASM and a locally obtained SCS Telemetry SDK: configure `-S . -B build -A x64 -DSCS_SDK_INCLUDE_DIR="<local include directory>"`, then build Release. SDK/game files are not bundled. Offline tests additionally use `-DCMF_BUILD_TESTS=ON` and `-DCMF_GAME_EXE="<read-only supported executable>"`; run CTest in Release. Native tests execute authored fixtures, never ETS2. See [VALIDATION.md](VALIDATION.md) and the per-release notes for evidence limits.

MIT License; see [LICENSE](LICENSE).

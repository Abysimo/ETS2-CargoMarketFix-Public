# ETS2 Cargo Market Fix

## What it fixes

Recurring Cargo Market freezes, typically about once per in-game minute, when using an owned trailer—especially with very large map combinations containing many companies.

Version **1.2.0** retains the four-attempt limit (CAP4) and spreads normal company refresh work across one in-game hour. This eliminated recurring freezes in the practically tested ETS2 1.57.2.7 setup; results with other setups may vary. This release adds structurally validated support for ETS2 1.59.1.3 without changing the fix algorithms.

## Supported versions

| ETS2 version | Status | Plugin release |
| --- | --- | --- |
| 1.57.2.7 | Supported / practically tested | v1.0.0+ |
| 1.58.1.4 | Supported / structurally validated | v1.1.0+ |
| 1.59.1.3 | Supported / structurally validated | v1.2.0+ |
| 1.60 | Not supported / not validated | — |

**Structurally validated** means the exact executable was identified, patch sites were ported, native/maintenance tests passed, and safety/fail-closed checks passed. The original very-large-map owned-trailer reproduction setup is only available for ETS2 1.57. Neither 1.58.1.4 nor 1.59.1.3 was practically performance-tested in-game with that setup. Their status is **SUPPORTED / STRUCTURALLY VALIDATED**, not practical in-game re-certification of freeze performance.

Windows x64 only. The plugin is **exact-build gated**: the executable hash and patch signatures must match. If ETS2 updates to an unsupported executable, it fails closed and does not apply the patch. Other 1.57, 1.58 or 1.59 executables are not automatically supported. Check this table and available releases after every game update; future-version rows do not promise support.

Exact supported executable SHA256 values:

- 1.57.2.7: `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`
- 1.58.1.4: `25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644`
- 1.59.1.3: `E6FE1A58DF9D0BFFF21DCCC12B4F581DC0D3E3B5885DBF62ED05028DE794D35C`

## Downloads

Download [v1.2.0](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/v1.2.0): `ETS2-CargoMarketFix-v1.2.0-ETS2-1.59.1.3.zip`. Despite the asset name highlighting the newly added build, its **single DLL supports all three exact builds listed above**. Previous v1.0.0 and v1.1.0 assets remain available unchanged with their original supported-build sets. Never use a release for an executable outside its supported set.

Lack of a compatible large-map setup does not prevent a structurally validated compatibility release. The same distinction applies to future ports: support is added only after exact-build validation, and practical testing is claimed only when it actually occurred. ETS2 1.60 remains unsupported.

## Installation

1. Close Euro Truck Simulator 2.
2. Download the release ZIP matching your supported game version, when available.
3. Extract `CargoMarketFix.dll` and `CargoMarketFix.ini`.
4. Open `Euro Truck Simulator 2/bin/win_x64/plugins/` in your game installation. Create the `plugins` directory if it does not exist.
5. Copy both extracted files into that directory. The supplied INI enables CAP4 and the 60-minute refresh spread.
6. Start ETS2 normally.

This is **NOT an `.scs` mod**. Do not put it in the ETS2 `mod` folder or add it through Mod Manager.

## Checking that the plugin loaded

After launching ETS2, `CargoMarketFix.log` should appear next to the DLL and INI. Check the latest startup entries for a matched supported build, CAP4 installed/active, and refresh spread active. An unsupported executable should instead produce an unsupported-build message and no patches.

## Updating

1. Close ETS2.
2. Replace **both** DLL and INI with those from the release matching your current game executable.
3. Do not reuse old experimental or research INI files.
4. Start ETS2 and check the latest startup log.

After ETS2 itself updates, verify compatibility before assuming the old plugin still works. Do not disable safety gates to force it to load.

## Uninstallation

1. Close ETS2.
2. Remove `CargoMarketFix.dll` and `CargoMarketFix.ini` from the plugins directory.
3. Optionally remove `CargoMarketFix.log`.

The plugin patches process memory only; it does not modify `eurotrucks2.exe` on disk. Uninstalling does not undo offers already saved by the game.

## Troubleshooting

- **Plugin does not load:** verify both files are in `bin/win_x64/plugins/`, that you are using Windows x64 and a supported ETS2 executable, and inspect `CargoMarketFix.log`.
- **Unsupported build:** your executable differs from the validated build. Keep the safety gate enabled and wait for a compatible release.
- **Freezes still occur:** confirm the owned-trailer Cargo Market scenario and that the plugin loaded successfully. When reporting an issue, describe the ETS2 version and map combination and attach `CargoMarketFix.log` after reviewing/redacting private information. Do not upload saves unless specifically requested later.
- **No log file:** the plugin likely did not load. Recheck the installation directory and whether the plugin can write a log there.

## Tradeoffs

Normal Cargo Market refresh may be delayed by up to approximately **59 in-game minutes**. Fewer offers or less frequent offer freshness are possible. Sleep/bulk refresh and trailer activation are not intentionally deferred by the hourly scheduler; they retain CAP4-only behavior. Initial trailer activation may still take time.

## Privacy

The plugin does not send telemetry, upload files or contact external servers. Its only file output is its local diagnostic log. Performance observers are not compiled into this release.

## Building and maintenance

Use local CMake 3.25+, Visual Studio with MSVC/MASM x64, a Windows SDK, and a separately obtained SCS Telemetry SDK. SDK headers and game files are not bundled. Build scripts perform no downloads or uploads.

Configure with `cmake -S . -B build -A x64 -DSCS_SDK_INCLUDE_DIR="<local SDK include directory>"`, then `cmake --build build --config Release`. The DLL and matching INI are in `build/Release/`.

For offline maintenance tests, additionally configure `-DCMF_BUILD_TESTS=ON -DCMF_GAME_EXE="<local supported executable>"`, build, then run `ctest --test-dir build -C Release --output-on-failure`. Tests only read the game image; native patch and exception tests execute authored fixtures, never ETS2.

## License

Licensed under the MIT License. See [LICENSE](LICENSE).

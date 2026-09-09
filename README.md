# ETS2 Cargo Market Fix

Version **1.0.0** targets recurring Cargo Market freezes with owned trailers on large map combinations containing many companies. These freezes can occur roughly once per in-game minute. The working CAP4 + refresh-spread approach eliminated these recurring freezes in the tested setup; results on other map combinations may vary.

The plugin limits generation attempts to four and spreads normal company refresh work across one in-game hour, using 60 balanced, contiguous groups. It does not run performance observers or collect telemetry.

## Compatibility

**Euro Truck Simulator 2 1.57 — validated on 1.57.2.7 only**, Windows x64. The exact executable hash and patch signatures must match. Unsupported executable builds fail closed: no binary patch is installed. Other or future 1.57 executables are not automatically supported.

## Installation

With ETS2 closed, copy `CargoMarketFix.dll` and the supplied `CargoMarketFix.ini` into:

`Euro Truck Simulator 2/bin/win_x64/plugins/`

The supplied configuration enables CAP4 and the 60-minute spread. Do not reuse an old diagnostic configuration. A small local `CargoMarketFix.log` records startup and restoration status; it contains no company, cargo, profile or save identifiers. Nothing is uploaded.

To uninstall, close ETS2 and remove the DLL and INI. The optional log can also be removed. Patches exist only in process memory; the executable on disk is not edited.

## Tradeoffs

Normal Cargo Market freshness may be delayed by up to about **59 in-game minutes**. Sleep/bulk refresh and trailer activation are not intentionally delayed by the spread scheduler: those paths retain CAP4-only behavior. The initial trailer-activation rebuild can still take time. Generated offers remain normal game state; uninstalling does not undo offers already saved by the game.

## Building and maintenance

Use local CMake 3.25+, Visual Studio with MSVC/MASM x64, a Windows SDK, and a separately obtained SCS Telemetry SDK. SDK headers and game files are not bundled. Build scripts perform no downloads or uploads.

Configure with `cmake -S . -B build -A x64 -DSCS_SDK_INCLUDE_DIR="<local SDK include directory>"`, then `cmake --build build --config Release`. The DLL and matching INI are in `build/Release/`.

For offline maintenance tests, additionally configure `-DCMF_BUILD_TESTS=ON -DCMF_GAME_EXE="<local supported executable>"`, build, then run `ctest --test-dir build -C Release --output-on-failure`. Tests only read the game image; native patch and exception tests execute authored fixtures, never ETS2.

This repository is private preparation for independent review. No public release is published yet. **License selection is pending; no redistribution license has been granted by this snapshot.**

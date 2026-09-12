# ETS2 Cargo Market Fix v1.4.0

**Choose the release matching your EXACT ETS2 version. Each DLL accepts only that
release's executable hash. Do not mix DLLs between game versions.**

| Exact ETS2 version | Download release | Spread1440 validation |
| --- | --- | --- |
| 1.57.2.7 | [v1.4.0-ets2-1.57.2.7](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/v1.4.0-ets2-1.57.2.7) | Physically gameplay tested; see qualification below |
| 1.58.1.4 | [v1.4.0-ets2-1.58.1.4](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/v1.4.0-ets2-1.58.1.4) | Structurally validated |
| 1.59.1.3 | [v1.4.0-ets2-1.59.1.3](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/v1.4.0-ets2-1.59.1.3) | Structurally validated |
| 1.60.1.7 | [v1.4.0-ets2-1.60.1.7](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/v1.4.0-ets2-1.60.1.7) | Structurally validated; earlier-settings startup smoke only |

Each release has one ZIP, one matching DLL, and SHA256SUMS.txt. Exact executable
hashes and artifact checksums are in its notes. Other known versions and unknown
builds fail closed. Older releases remain available with their original settings.

## Install — both patches are ACTIVE in v1.4.0 packages

1. Close ETS2. Back up any existing CargoMarketFix DLL and INI.
2. Download the ZIP for your exact game version from the table.
3. Copy **both CargoMarketFix.dll and CargoMarketFix.ini** into:
   `Euro Truck Simulator 2/bin/win_x64/plugins/`
4. Launch normally. The supplied INI enables **CAP4=4 + Refresh Spread=1440**.
   The local CargoMarketFix.log reports exact-build acceptance and active settings.

This is a native Windows x64 plugin, **not an .scs mod**. It does not use Mod Manager.
Do not reuse a diagnostic/research INI. No observers or diagnostics are compiled
into these DLLs; no collector is needed.

To disable both patches, close the game and replace CargoMarketFix.ini with
`config_examples/CargoMarketFix.disabled.ini` (renamed CargoMarketFix.ini), or
remove the DLL while the game is closed. The executable on disk is never patched.

## What changed

CAP4 keeps the fixed four-attempt current-trailer Cargo Market generation limit.
The recommended active Refresh Spread changes from 180 to **1440 game minutes
(24 game hours)**. It distributes the normal company sweep into balanced contiguous
slices without a backlog or adaptive scheduler. Accepted settings are exactly
60, 120, 180 and 1440; unsupported/malformed settings fail closed, not clamped.

180-minute spreading could still cluster many expensive regenerations in one
game-minute update. 1440 lowers the maximum company entries per normal slice.
This is not proof that time of day causes the issue and not a GPU/FPS optimization.

## Validation and limits

**1.57.2.7 — PHYSICALLY GAMEPLAY TESTED:** the user tested the equivalent private
Spread1440 implementation with CAP4=4 on a heavy real map combo, an owned trailer,
and daytime driving where strong recurring freezes were previously visible.
No recurring freezes were observed during that test; gameplay felt smooth.
The newly packaged public DLL has offline validation. This is not a universal
guarantee or a measured FPS improvement.

**1.58.1.4 / 1.59.1.3 / 1.60.1.7 — STRUCTURALLY VALIDATED:** no physical
Spread1440 gameplay-performance claim. The historical 1.60 startup smoke used
earlier settings, not Spread1440. Clean live shutdown was not confirmed in that
smoke. No new owned-trailer, sleep/bulk or offer-quality gameplay test is claimed.

A company may wait up to roughly **24 in-game hours** for its normal
spread-controlled refresh. Fewer or less-fresh offers may temporarily occur.
Complete-cycle coverage assumes a fixed list and all normal minutes being processed;
time jumps or changing lists are not a snapshot guarantee.
Sleep/bulk and activation/direct generation remain separate and unchanged.
Initial activation can still take time. No save format, serialization or persistent
cache is added, but changing refresh times can change offers subsequently saved.
Removing the plugin does not retroactively undo those offers.

## Build, privacy and license

No network access or uploaded telemetry; only a local health log.
See [VALIDATION.md](VALIDATION.md) and [RELEASING.md](RELEASING.md).
Build requires Windows x64 MSVC/MASM, CMake 3.25+, and a locally obtained SCS SDK.
Set `CMF_ETS2_TARGET` to exactly one version from the table and
`SCS_SDK_INCLUDE_DIR` to your local SDK include directory, then build Release.
An unspecified/unsupported release target is a configuration error.
SDK/game files and diagnostic tools are not included.

MIT License; see [LICENSE](LICENSE).

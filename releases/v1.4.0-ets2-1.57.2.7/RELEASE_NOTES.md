# ETS2 Cargo Market Fix v1.4.0 — ETS2 1.57.2.7

**Only ETS2 1.57.2.7, Windows x64. Do not use this DLL with another game version.**

Required eurotrucks2.exe SHA256:
`06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`

Runtime version: `1.4.0-ets2-1.57.2.7`. Other known builds and unknown executables
fail closed. This is a native plugin, not an .scs mod.

## What's changed

The packaged active configuration changes Refresh Spread from 180 to **1440 game
minutes (24 game hours)**, with CAP4's fixed generation-attempt budget of four.
180-minute spreading could still cluster many expensive Cargo Market regenerations
in one game-minute update. 1440 reduces the maximum number of company entries in
each normal refresh slice; individual regeneration cost can still vary.
This is not evidence that time of day itself causes the issue, nor a universal FPS,
GPU, or general-stutter optimization.

## Install

1. Close ETS2 and back up any existing CargoMarketFix DLL and INI.
2. Download the ZIP whose name matches **ETS2 1.57.2.7**.
3. Copy **both** CargoMarketFix.dll and CargoMarketFix.ini into
   `Euro Truck Simulator 2/bin/win_x64/plugins/`.
4. Start ETS2 normally. CAP4=4 and Spread1440 are active in this package.

No observers or diagnostics are compiled into this DLL. There are no collectors.
To disable both patches, close ETS2 and replace the INI with the supplied disabled
example (renamed CargoMarketFix.ini), or remove the plugin while the game is closed.
Do not mix DLLs from different target releases. Install the INI, not just the DLL.

## Trade-offs

A company may wait up to roughly 24 in-game hours for its next normal spread-controlled
refresh. Fewer or less-fresh Cargo Market offers may temporarily occur. Sleep/bulk
processing and activation/direct generation are separate and unchanged.
No save format or serialization mechanism is changed and no cache is persisted.
Changed refresh scheduling can nevertheless change the offers later saved; disabling
the plugin does not retroactively regenerate identical historical offers.
No membership cache, accepted-vector changes, or adaptive scheduler is included.

## Validation

PHYSICALLY GAMEPLAY TESTED: the user tested the equivalent Spread1440 implementation with CAP4=4 on a heavy real map combo, an owned trailer, and daytime driving where strong recurring freezes had previously been visible. No recurring freezes were observed during that test; gameplay felt smooth. This is not a universal guarantee or a measured FPS result. The newly packaged public DLL has offline validation.

Focused automated checks cover exact target selection and rejection, CAP4/Spread
signatures, both native spread ABIs with 1440 buckets, bounded arithmetic, empty
buckets, install/restore, rollback/containment and quiescence. Other targets' original
executables were not newly rerun: their previously validated descriptors and authored
native maintenance fixtures were used. The installed 1.57 executable was checked read-only.
See the repository VALIDATION.md for exact counts and limits.

## Files and integrity

This release contains one target DLL and one active INI, README, MIT LICENSE, and a
disabled example. Verify the ZIP and DLL against this release's SHA256SUMS.txt.
Older releases remain available unchanged.

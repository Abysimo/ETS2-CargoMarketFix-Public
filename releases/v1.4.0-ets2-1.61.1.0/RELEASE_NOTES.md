# ETS2 Cargo Market Fix v1.4.0 — ETS2 1.61.1.0

**Only ETS2 1.61.1.0, Windows x64. Native telemetry plugin, not an .scs mod.**

Required eurotrucks2.exe SHA256:
`4DCB548CAAD924254A60AF7C3BD1DB69DCAF42F7ADF19B5BB5D77EBA2814AF21`

Runtime version: `1.4.0-ets2-1.61.1.0`. Functional version remains **1.4.0**.
This is an exact-build compatibility port, not a new fix algorithm. Other known
builds and unknown executable hashes fail closed. Previous releases are unchanged.

## Maintained behavior

CAP4 keeps the current-trailer Cargo Market generation-attempt budget at **4**.
Refresh Spread partitions the normal company refresh sweep across **1440 game
minutes (24 game hours)**. Bulk/sleep and direct activation remain unspread.
The recompiled 1.61 normal sweep requires a dedicated nine-byte bridge layout;
the existing empty-list condition is preserved before any game-minute read.

## Install

1. Close ETS2 and back up your existing plugin DLL and INI.
2. Copy **both** CargoMarketFix.dll and CargoMarketFix.ini from this exact-target
   ZIP into `Euro Truck Simulator 2/bin/win_x64/plugins/`.
3. The supplied INI enables CAP4=4 and Spread1440. No observers, diagnostics,
   collectors or network telemetry are compiled into the production DLL.

To disable both patches, close the game and use the supplied disabled INI example
(rename it CargoMarketFix.ini), or remove the plugin while the game is closed.
Do not mix DLLs from different target releases. The executable on disk is not edited.

## Trade-offs

A company may wait roughly 24 in-game hours for its next normal refresh. Offers
may temporarily be fewer or less fresh. Initial trailer activation may still hitch.
Complete-cycle coverage assumes a fixed company list and every normal minute is
processed; it is not a snapshot guarantee across time jumps/list changes.
No save format or persistent cache is added. Scheduling can change offers later
saved; disabling the plugin does not retroactively restore identical offers.
This is not a universal FPS, GPU or all-stutter fix.

## Validation and limitations

STRUCTURALLY VALIDATED / NO PHYSICAL GAMEPLAY TEST PERFORMED. Static exact-image analysis and authored native/offline regression checks passed. ETS2 was not launched: no startup smoke, clean live shutdown, measured freeze reduction, owned-trailer gameplay, Cargo Market offers or sleep/bulk gameplay was tested on this target. Historical gameplay evidence is not 1.61 evidence.

The installed 1.61 executable was inspected read-only. Automated checks cover
exact signatures, CAP4 budget dataflow, normal/bulk/activation route separation,
1440-bucket arithmetic, low/high counts, empty buckets, GPR/stack/FP preservation,
exception paths, quiescence, installation, exact restoration, rollback and
containment. Earlier descriptors and native fixtures remain supported; their
original executables were not newly available for inspection. See VALIDATION.md.

## Integrity

Five files: DLL, active INI, this README, MIT LICENSE and disabled INI example.
Verify the DLL and ZIP against SHA256SUMS.txt. No private research/logs are included.

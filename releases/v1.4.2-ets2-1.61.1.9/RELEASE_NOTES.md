# ETS2 Cargo Market Fix v1.4.2 — ETS2 1.61.1.9

**Only ETS2 1.61.1.9, Windows x64. Native telemetry DLL, not an .scs mod.**

Required eurotrucks2.exe SHA256: `B70C2357CC56EB5834EE47FEA3C60B2F2D4591383FF2AC479CFD3CD254301B8C`

Runtime version: `1.4.2-ets2-1.61.1.9`. This is an exact-build compatibility
port with unchanged fix semantics. Other known and unknown executable builds
fail closed. Previous releases, including 1.61.1.1, remain unchanged.

## Behavior and installation

The supplied active INI enables **CAP4=4** and **Refresh Spread=1440 game minutes
(24 game hours)**. Normal company refresh uses the existing balanced buckets.
Bulk/sleep processing and direct trailer activation remain unspread.

Close ETS2 and back up your current DLL and INI. Copy **both CargoMarketFix.dll
and CargoMarketFix.ini** from this exact-target ZIP into
`Euro Truck Simulator 2/bin/win_x64/plugins/`. Do not mix target DLLs or reuse
a diagnostic INI. To disable the patches, close the game and use the supplied
disabled INI example as CargoMarketFix.ini, or remove the plugin.

Only production patch/lifecycle code is included. No historical observers,
runtime diagnostics, membership instrumentation, accepted-vector changes,
collectors, debug hooks or network telemetry are compiled into this DLL.
The executable on disk is never edited.

## Trade-off

Normal refresh at a company may be delayed by roughly 24 game hours. Offers
may temporarily be fewer or less fresh. Initial activation can still take time.
Time jumps and changing company lists are not a fixed-snapshot guarantee.
No save format or persistent cache is added, but changed scheduling can affect
offers later saved. Disabling the plugin does not recreate identical past offers.

## Validation and limitations

STRUCTURALLY VALIDATED / NO PHYSICAL GAMEPLAY TEST PERFORMED ON THIS EXACT BUILD. Exact-image semantic analysis and native/offline validation passed. No live startup, measured freeze reduction, large-map or owned-trailer gameplay, sleep/bulk gameplay, offer-quality or clean live shutdown test is claimed for ETS2 1.61.1.9. Historical live runtime diagnostic evidence belongs only to the separate exact ETS2 1.61.1.1 target.

Fresh read-only analysis re-identified CAP4's candidate-count clamp, existing
four-budget arm and attempt-loop consumer. It also verified the new normal
sweep's exact signature, register ABI, empty/body continuations, and separate
bulk/refill/activation routes. The existing quiesced two-byte CAP4 backend and
v161 empty-list bridge remain appropriate; neither algorithm was redesigned.
Native/offline tests cover exact-target isolation, 1440 arithmetic, low/high/empty
counts, register/FP/stack preservation, fault/unwind paths, quiescence,
install/restore, rollback and containment.

The earlier smaller-map live diagnostic A/B runs on 1.61.1.1 are historical
context only. This new exact target has **not** been launched or gameplay-tested.
See [VALIDATION.md](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/VALIDATION.md)
for the focused evidence and qualification limits.

## Integrity

The ZIP contains CargoMarketFix.dll, active CargoMarketFix.ini, this README,
MIT LICENSE and the disabled INI example. Verify the DLL and ZIP using this
release's SHA256SUMS.txt. No private logs, captures or research files are included.

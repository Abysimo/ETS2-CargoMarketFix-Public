# ETS2 Cargo Market Fix v1.4.1 — ETS2 1.61.1.1

**Only ETS2 1.61.1.1, Windows x64. Native telemetry plugin, not an .scs mod.**

Required eurotrucks2.exe SHA256: `2014BFCC850A06108F4CA3C233A5BB6D31D7D83346C5D449F94C8E0675E5199A`

Runtime version: `1.4.1-ets2-1.61.1.1`. This is an exact-build compatibility
update, not a new algorithm. Previous ETS2 1.61.1.0 and older releases remain
available unchanged. Other exact-target DLLs and unknown executables fail closed.

## Behavior and installation

The active INI enables CAP4's current-trailer attempt budget of **4** and spreads
the normal company refresh over **1440 game minutes (24 game hours)**. Bulk/sleep
processing and direct trailer activation remain unspread.

Close ETS2. Back up the current CargoMarketFix.dll and CargoMarketFix.ini. Copy
**both** files from this ZIP into `Euro Truck Simulator 2/bin/win_x64/plugins/`.
To disable both patches, close ETS2 and use the supplied disabled example as
CargoMarketFix.ini, or remove the plugin while the game is closed. Do not mix
DLLs and INIs across target releases. The executable on disk is never edited.

The DLL contains no research observers, call-cost diagnostics, membership hooks,
collectors or network telemetry. Only the intended SCS telemetry exports exist.

## Trade-off

Normal company refresh may be delayed up to roughly 24 in-game hours. Offers
may temporarily be fewer or less fresh. Time jumps or changing company lists
are not a fixed-snapshot guarantee. No save format or persistent cache is added,
but changed scheduling can affect offers later saved. Removing the plugin does
not retroactively recreate earlier offers.

## Validation and limitations

**STRUCTURALLY VALIDATED / LIVE RUNTIME DIAGNOSTIC TEST PASSED.**
Exact-image static analysis and native/offline checks passed. The exact ETS2
1.61.1.1 executable was also physically run with a private diagnostic build
retaining the production CAP4 and Spread1440 logic. In a smaller-map A/B test,
Mode A (CAP4=4, Spread1440 ON) selected normal-sweep buckets over 2,043
companies: 31 sweeps, no fallback or range-consistency failure, maximum
captured sweep about 88.7 us. Mode B (both OFF) traversed the full range:
37 sweeps, maximum captured sweep about 92.569 ms. Game-minute progression
and clean diagnostic shutdown/restoration were observed. Outside-sweep
Destination totals were 14,576 (A) and 24,830 (B); these sessions differ in
length and do not establish a normalized speedup.

The public ZIP DLL was not the diagnostic binary and the ZIP assets remain
unchanged. Its bundled README reflects the qualification at publication time;
this updated online release note is the current qualification. The 1.61.1.1
large-map recurring-freeze scenario was not reproduced or physically validated.
No universal stutter elimination, owned-trailer large-map performance,
sleep/bulk gameplay, or exhaustive offer-quality claim is made.

Static analysis proved the exact new CAP4 branch and budget dataflow, the
nine-byte normal-sweep bridge site, and the separate bulk and activation routes.
Native/offline fixtures cover 1440-bucket arithmetic, empty/low/high counts,
register and FP preservation, install/restore, rollback, containment, exception
paths and quiescence. See the current repository
[VALIDATION.md](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/VALIDATION.md)
for the later physical diagnostic evidence and its limitations.

## Integrity

The ZIP contains CargoMarketFix.dll, active CargoMarketFix.ini, this README,
MIT LICENSE, and a disabled INI example. Verify DLL and ZIP with SHA256SUMS.txt.

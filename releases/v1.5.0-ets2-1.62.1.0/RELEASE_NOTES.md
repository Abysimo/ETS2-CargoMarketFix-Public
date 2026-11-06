# ETS2 Cargo Market Fix v1.5.0 — ETS2 1.62.1.0

**Only exact ETS2 1.62.1.0, Windows x64. Native telemetry DLL, not an .scs mod.**

Required eurotrucks2.exe SHA256: `A938CB388121F9B884A2E1B0F048C230A7BC19638EF6D641A70775F8C7FEA956`

Runtime version: `1.5.0-ets2-1.62.1.0`. The new game branch increments the
fix's minor version. This is a compatibility port with the maintained fix
algorithm unchanged. Other known and unknown executable builds fail closed.
Previous releases and their assets remain available unchanged.

## Maintained behavior and installation

The supplied active INI enables **CAP4=4** and **Refresh Spread=1440 game minutes
(24 game hours)**. The normal company sweep retains the current balanced,
contiguous bucket distribution. Bulk/sleep and direct trailer activation remain
unspread. There is no adaptive scheduler, membership cache or new persistence.

Close ETS2 and back up your existing CargoMarketFix DLL and INI. Copy **both
CargoMarketFix.dll and CargoMarketFix.ini** from this exact-target ZIP into
`Euro Truck Simulator 2/bin/win_x64/plugins/`. Do not mix DLLs between targets
or reuse a research INI. To disable the patches, close the game and use the
supplied disabled example as CargoMarketFix.ini, or remove the plugin.

The DLL contains only production functionality. Historical observers, runtime
diagnostics, membership instrumentation, accepted-vector reads, collectors,
debug hooks and network access are absent. Only the two SCS telemetry exports
are provided. The game executable on disk is never edited.

## Trade-offs

A company's normal refresh may be delayed by roughly 24 game hours; offers may
temporarily be fewer or less fresh. Complete-cycle coverage assumes a fixed list
and all normal game minutes being processed, not a snapshot across time jumps
or list changes. Activation can still take time. No save format or persistent
cache changes, but changed refresh scheduling can affect offers later saved;
removing the plugin does not recreate identical past offers. This is not a
universal stutter/FPS guarantee.

## Validation and limitations

STRUCTURALLY VALIDATED / NO PHYSICAL GAMEPLAY TEST PERFORMED ON THIS EXACT BUILD. Fresh exact-image semantic proof and focused native/offline validation passed. No live startup, measured freeze reduction, large-map or owned-trailer gameplay, sleep/bulk gameplay, offer-quality, clean live shutdown or Convoy testing is claimed for ETS2 1.62.1.0. Historical live diagnostic evidence on exact ETS2 1.61.1.1 does not constitute a live test of 1.62.

The 1.62 generator changed its local stack layout. Fresh analysis proved the
new candidate-count source, original four/count/ten clamp, new budget slot,
and actual attempt-loop consumer. A new exact CAP4 signature and authored
native clamp fixture cover those changes. The existing quiesced two-byte write
backend is retained, even though the new target address is aligned.

The normal sweep was freshly decoded beyond signature matching: its live
registers, original empty path, minute source, body/backedge/end, stack,
flags and unwind assumptions still match the maintained v161 bridge. The
balanced Spread1440 arithmetic is unchanged. Bulk and activation routes
were separately verified to bypass the normal gate. Focused native tests cover
counts around 1440, medium/high counts, remainders, minute wrap, bounded
fallback, register/FP preservation, exact restore, rollback and containment.

See [VALIDATION.md](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/VALIDATION.md)
for counts and evidence. The earlier live diagnostic A/B results on 1.61.1.1
are historical context only; this exact 1.62 target was not launched or tested
in gameplay for this release.

## Integrity

Five intended files: CargoMarketFix.dll, active CargoMarketFix.ini, this README,
MIT LICENSE, and the disabled INI example. Verify the DLL and ZIP against this
release's SHA256SUMS.txt. No private captures, SDK/game files, or research tools
are packaged.

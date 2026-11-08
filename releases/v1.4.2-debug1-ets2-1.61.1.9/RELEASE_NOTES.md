# Cargo Market Fix v1.4.2 Debug 1 — ETS2 1.61.1.9

Diagnostic prerelease for **`prepatch_quiescence_failed_never_exposed`**.
Use the normal v1.4.2 release if it works for you. Debug 1 adds startup evidence
and contains no workaround for the refusal.

This Windows x64 DLL requires exact ETS2 **1.61.1.9** and this executable SHA256:

`B70C2357CC56EB5834EE47FEA3C60B2F2D4591383FF2AC479CFD3CD254301B8C`

The runtime version is `1.4.2-debug1-ets2-1.61.1.9`. The supplied INI keeps
CAP4 at four attempts and the normal refresh spread at 1440 game minutes.
The patch sites, thread limits, permissions, and fail-closed decisions match
v1.4.2. Historical observers remain off.

Close ETS2, back up your current plugin DLL and INI, then install both files
from this ZIP. Reproduce the failure, close the game normally, and send the
complete `CargoMarketFix.log` directly to the developer. It may contain thread
IDs and instruction addresses. See `README_DEBUG.txt` in the ZIP.

The debug block records the exact Win32 failure or safety refusal, up to four
snapshot passes, thread counts, and any offending instruction pointer. The
logger writes the block after the thread suspensions are released. A failed
resume leaves that release unconfirmed, so the logger withholds the new block.
An
`OpenThread` error consistent with a thread exiting is flagged as a possibility
for investigation. The error alone cannot establish the cause.

Validated with exact-image checks, authored Win32 failures, native CAP4 and
Spread tests, and an offline DLL host. The reporting user's startup failure
has yet to be captured with this build.

The source is tagged from the original public v1.4.2 snapshot. The diagnostic
was developed from private production commit `1c655e2b736038d9ed5cf7cdfb9a9b8890adadf6`.
Verify the ZIP and DLL using `SHA256SUMS.txt`.

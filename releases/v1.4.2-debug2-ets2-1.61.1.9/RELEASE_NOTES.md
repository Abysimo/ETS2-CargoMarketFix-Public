# Cargo Market Fix v1.4.2 Debug 2 — ETS2 1.61.1.9

This diagnostic prerelease tests narrow recovery from the CAP4 startup
failure seen in Debug 1 when `OpenThread` returned
`ERROR_INVALID_PARAMETER` (87) for a thread listed in the snapshot. The
reported error is consistent with the thread exiting between the snapshot
and the open call. Use the regular v1.4.2 release if it works for you.

Requires exact **ETS2 1.61.1.9 on Windows x64** with executable SHA256
`B70C2357CC56EB5834EE47FEA3C60B2F2D4591383FF2AC479CFD3CD254301B8C`.
Runtime version `1.4.2-debug2-ets2-1.61.1.9`.

For this one error, CAP4 continues the current snapshot and retains all
successful suspensions. The failed ID stays unseen. A thread that later
appears with the same ID must be opened, suspended, and checked. CAP4 can
patch only after a later complete pass with zero new threads and zero
transient errors, within the existing four-pass limit. Exhausted attempts
and every other quiescence failure still fail closed.

The active INI enables CAP4 at four attempts and Refresh Spread at 1440 game
minutes. The patch transaction and Spread behavior remain those of v1.4.2.
The diagnostic records the recovery count by pass, up to 16 observed IDs,
the stabilization pass, and the patch result after thaw. A recovered
quiescence is marked `SUCCESS_AFTER_TRANSIENT_THREAD_EXIT`.

Close ETS2, back up the existing plugin DLL and INI, and install both files
from this ZIP. Test startup on the setup that showed the error in Debug 1.
Close ETS2 normally and send the complete `CargoMarketFix.log` directly to
the developer whether startup succeeded or failed. The log can contain
thread IDs and instruction addresses.

Offline validation covered the reported failure sequence, reuse of the
failed numeric ID, persistent churn, other fatal errors, exact patch and
restore bytes, and Spread1440. This recovery is awaiting the external startup
test before it is considered for production. The source tag descends from
the public Debug 1 tag. Verify the ZIP and DLL with `SHA256SUMS.txt`.

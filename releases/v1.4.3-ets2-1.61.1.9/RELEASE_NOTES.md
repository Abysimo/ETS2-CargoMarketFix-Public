# ETS2 Cargo Market Fix v1.4.3 — ETS2 1.61.1.9

**Only exact ETS2 1.61.1.9, Windows x64.** Runtime `1.4.3-ets2-1.61.1.9`.

Required eurotrucks2.exe SHA256: `B70C2357CC56EB5834EE47FEA3C60B2F2D4591383FF2AC479CFD3CD254301B8C`

## Startup installation fix

A thread can exit after Windows captures the thread snapshot and before the
plugin opens that thread. The previous version refused installation in this
case. The new guard tolerates only `OpenThread` returning
`ERROR_INVALID_PARAMETER` (87). It keeps other threads suspended and requires
a later complete snapshot with no new threads and no transient errors before
patching. A reused thread ID is checked normally. The four-pass limit and
all other fail-closed checks remain in effect.

## Install

Close ETS2 and back up your current DLL and INI. Copy **both CargoMarketFix.dll
and CargoMarketFix.ini** into `Euro Truck Simulator 2/bin/win_x64/plugins/`.
The active INI enables CAP4 at four attempts and spreads the normal company
refresh over 1440 game minutes using the same balanced buckets. Bulk/sleep
and direct activation are unspread. The executable hash gate, patch sites
and save behavior are unchanged.

A company's normal refresh may be delayed by roughly 24 game hours, so
offers may temporarily be fewer or less fresh. To disable the patches, close
ETS2 and use the included disabled INI example or remove the DLL.

## Validation

LIVE INSTALLATION FIX VALIDATED ON ETS2 1.61.1.9. An external Debug 2 run recovered from the reported OpenThread failure, installed CAP4, and activated Spread1440. The production DLL passed native and offline validation. The external result covers startup installation. Large-map gameplay performance and live shutdown were not validated by that run.

Focused regression checks cover stabilization, thread ID reuse, refusal and
cleanup, exact CAP4 install and restore, and the Spread1440 lifecycle.
See [VALIDATION.md](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/VALIDATION.md).
The production DLL contains no debug blocks or observer hooks.

## Integrity

The ZIP contains the DLL, active INI, this README, MIT LICENSE and the
disabled INI example. Verify both the DLL and ZIP with `SHA256SUMS.txt`.

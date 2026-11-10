# ETS2 Cargo Market Fix v1.5.2 — ETS2 1.62.2.1

**Exact ETS2 1.62.2.1, Windows x64.** Native telemetry DLL with runtime
`1.5.2-ets2-1.62.2.1`.

Required eurotrucks2.exe SHA256: `8F17D3505C2E6C25A93F38A1119FFBBCCFBA6A3BE2EDDA3771E0186276AD9434`

## Compatibility

The new exact descriptor uses the re-identified CAP4 branch and the verified
normal-sweep ABI. CAP4 selects the existing four-attempt arm. Refresh Spread
uses the same balanced buckets across 1440 game minutes. Bulk/sleep and direct
activation remain unspread. The shared guard retains the narrow recovery for
`OpenThread` error 87 and still requires a later clean snapshot within four
passes. Other quiescence failures and unsupported executable hashes fail closed.

## Install

Close ETS2 and back up your current DLL and INI. Copy both **CargoMarketFix.dll**
and **CargoMarketFix.ini** to `Euro Truck Simulator 2/bin/win_x64/plugins/`.
Both patches are active in the supplied INI. A company's normal refresh may
be delayed by roughly 24 game hours, so offers may be fewer or less fresh.
To disable both patches, close ETS2 and use the disabled INI example or remove
the plugin.

## Validation

STRUCTURALLY VALIDATED / NO PHYSICAL GAMEPLAY TEST PERFORMED ON THIS EXACT BUILD. The exact executable passed read-only semantic checks, and the plugin passed native and offline checks. The transient-thread recovery has external startup evidence on ETS2 1.61.1.9 via Debug 2. That evidence applies to the older target.

The release DLL contains production functionality only. See
[VALIDATION.md](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/VALIDATION.md)
for exact sites, bridge proof and regression coverage.

## Integrity

The ZIP contains the DLL, active INI, this README, MIT LICENSE and the disabled
INI example. Verify both the DLL and ZIP with `SHA256SUMS.txt`.

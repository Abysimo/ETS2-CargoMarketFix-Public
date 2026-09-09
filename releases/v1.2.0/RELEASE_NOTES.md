# ETS2 Cargo Market Fix v1.2.0

## Added support

**ETS2 1.59.1.3 — Windows x64**.

Executable size: **50,231,184 bytes**.
Executable SHA256: `E6FE1A58DF9D0BFFF21DCCC12B4F581DC0D3E3B5885DBF62ED05028DE794D35C`.

Download `ETS2-CargoMarketFix-v1.2.0-ETS2-1.59.1.3.zip`. The package name highlights
the newly added build; its single DLL supports all three exact builds listed below.

## Existing support retained

- **ETS2 1.57.2.7 — SUPPORTED / PRACTICALLY TESTED**, introduced in v1.0.0.
- **ETS2 1.58.1.4 — SUPPORTED / STRUCTURALLY VALIDATED**, introduced in v1.1.0.

The v1.0.0 and v1.1.0 releases, tags and assets remain unchanged.

## Validation status

**ETS2 1.59.1.3: SUPPORTED / STRUCTURALLY VALIDATED**.

The original very-large-map owned-trailer reproduction setup is only available for
ETS2 1.57, so 1.59 was not practically performance-tested in-game.

The exact 1.59 executable was structurally analyzed. The same Cargo Market generator
and normal refresh semantics were confirmed. The port passed the compatibility/native
maintenance suite, including exact-build selection, native installation/restoration,
cross-build separation and fail-closed checks. This is not a claim of practical
in-game performance testing on 1.59. No physical test is requested for this release.

## Compatibility detail

The 1.59 executable changed the CAP4 branch encoding. The plugin now validates,
preserves and restores each supported build's own original bytes safely. CAP4
still uses the engine's existing four-attempt path; no new generation algorithm
or performance observer is introduced.

## Installation

Close ETS2. Extract `CargoMarketFix.dll` and `CargoMarketFix.ini` into
`Euro Truck Simulator 2/bin/win_x64/plugins/` (create `plugins` if needed).
Use both supplied files. This is not an `.scs` mod and does not use Mod Manager.
The release INI enables CAP4 and the 60-minute refresh spread. Observers are absent.

## Tradeoff

The same approximately **59-game-minute maximum normal refresh delay** applies.
The normal sweep uses 60 balanced contiguous buckets selected by `game minute % 60`.
There is no backlog or retained company scheduling state. Bulk/sleep and trailer
activation are not deferred by spread; CAP4 still applies to their generator path.
Initial trailer activation may still take time.

## Exact-build safety

Unknown executable builds receive no patch. Only the exact 1.57.2.7, 1.58.1.4 and
1.59.1.3 hashes listed in the README are supported. Other 1.59 builds and ETS2 1.60
are not automatically supported.

## Integrity and license

The ZIP contains exactly `CargoMarketFix.dll`, `CargoMarketFix.ini`, `README.md` and
`LICENSE`. The separate `SHA256SUMS.txt` asset contains DLL and ZIP SHA256 checksums.
MIT License retained, Copyright (c) 2026 Abysimo.

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 321536 | `BCD67E8F44DFCF2104623086932100073B6844A13442E169B476EA3290262FDE` |
| ETS2-CargoMarketFix-v1.2.0-ETS2-1.59.1.3.zip | 158211 | `70FB7FBA215CB1687E6557645A5B85DD19EF9342A76C4D015B5F9BC84C2BAF1E` |

Public Release x64 maintenance: **146/146 named cases passed**, five suites,
one run, no retries. Each supported build restores its exact original CAP4 bytes;
cross-build ownership/restoration, unknown-build refusal and source/built INI parity
are covered. The DLL exports only the two telemetry entry points. Public source
and DLL printable-string sanitization passed.

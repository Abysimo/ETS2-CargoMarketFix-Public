# ETS2 Cargo Market Fix v1.1.0

## Added support

**ETS2 1.58.1.4 — Windows x64**.

Executable size: 49,878,928 bytes.
Executable SHA256: `25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644`.

This is the first plugin release supporting ETS2 1.58. The package is named
`ETS2-CargoMarketFix-v1.1.0-ETS2-1.58.1.4.zip` to highlight this addition.

## Existing support retained

The same DLL also supports **ETS2 1.57.2.7**, introduced in v1.0.0.
Status: **SUPPORTED / PRACTICALLY TESTED**.
Executable SHA256: `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`.
The v1.0.0 release and its assets are unchanged.

## Validation status

ETS2 1.58.1.4: **SUPPORTED / STRUCTURALLY VALIDATED**.

The port preserves the same CAP4 + 60-minute spread implementation and passes the compatibility/native maintenance suite. Exact executable identification, patch-site matching, descriptor separation, install/restore and fail-closed checks are covered.

The original very-large-map owned-trailer reproduction setup was unavailable on 1.58, so the performance fix was not practically re-certified in-game on this build. Structural validation is the appropriate support status for this compatibility release; it is not a claim of practical in-game testing. No physical test was requested for this release.

## Installation

Close ETS2. Extract `CargoMarketFix.dll` and `CargoMarketFix.ini` into
`Euro Truck Simulator 2/bin/win_x64/plugins/` (create `plugins` if needed).
Use both supplied files. This is not an `.scs` mod and does not use Mod Manager.
The INI enables CAP4 and 60-minute spread. Performance observers are absent.

## Tradeoff

The same approximately **59-game-minute** maximum normal-refresh freshness delay applies. CAP4 keeps the generation attempt budget at four. The normal sweep uses 60 balanced contiguous buckets selected by `game minute % 60`, with no backlog. Bulk/sleep and trailer activation remain unaffected by spread, retaining CAP4-only behavior. Initial activation can still take time.

## Compatibility

Exact-build gated. Startup selects only the matching executable's descriptor.
Unknown game builds receive no patch. Other 1.58 builds and ETS2 1.59/1.60 are not supported by this release.

## Integrity and license

The ZIP contains exactly `CargoMarketFix.dll`, `CargoMarketFix.ini`, `README.md` and `LICENSE`.
Final DLL and ZIP SHA256 values are supplied in the separate `SHA256SUMS.txt` asset.

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 320512 | `86C47F0389E3321976681B1F8947014EAE36B6DE6D54DE6D02C740F1C1110103` |
| ETS2-CargoMarketFix-v1.1.0-ETS2-1.58.1.4.zip | 162725 | `C68D937A95790F31FFA88FC9679933C735011AF7C323C2B3D352308C342268BC` |

Sanitized public Release x64 maintenance: **122/122 named cases passed**, five suites, one run, no retries. Required exports and source/built INI parity were verified. Public source and package scans passed; no private paths, research artifacts, game/SDK data or credentials are included.

MIT License retained, Copyright (c) 2026 Abysimo.

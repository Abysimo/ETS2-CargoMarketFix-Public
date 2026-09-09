# ETS2 Cargo Market Fix v1.0.0

Release title: **ETS2 Cargo Market Fix v1.0.0 — ETS2 1.57.2.7**

## Supported game version

Euro Truck Simulator 2 **1.57.2.7**, Windows x64.

Validated executable SHA256: `06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9`.

Only this exact executable is supported. Unsupported executable builds fail closed and receive no patch. Other or future 1.57 builds are not automatically compatible.

## What this fixes

Recurring Cargo Market freezes, often approximately once per in-game minute, especially with large map combinations and an owned trailer.

## Fix

- Limits expensive Cargo Market generation attempts to four.
- Spreads normal company Cargo Market refresh processing across one in-game hour.
- Keeps unsupported executable builds fail-closed, with performance observers disabled.

## Installation

Close ETS2. Extract `CargoMarketFix.dll` and `CargoMarketFix.ini` from `ETS2-CargoMarketFix-v1.0.0-ETS2-1.57.2.7.zip` into:

`Euro Truck Simulator 2/bin/win_x64/plugins/`

Create `plugins` if necessary. Use the supplied INI. This is not an `.scs` mod: do not use the mod directory or Mod Manager. Start ETS2 normally and check the latest `CargoMarketFix.log` entries for supported-build acceptance, CAP4 installation and active refresh spread.

## Tradeoff

Normal Cargo Market freshness may be delayed by approximately **59 in-game minutes**; fewer offers or less frequent freshness are possible. Sleep/bulk refresh and trailer activation are not intentionally deferred by the hourly scheduler. The initial trailer-activation rebuild may still take time.

## Verification status

**SUPPORTED / PRACTICALLY TESTED.** The working CAP4 + 60-minute spread fix was tested successfully in the real game on ETS2 1.57.2.7, with the large-map / owned-trailer scenario that originally reproduced recurring freezes. Those recurring freezes were eliminated in that test setup. This is not a universal performance guarantee for every map combination.

The sanitized release build was validated offline: **95/95 maintenance cases** passed in one complete run following a clean Release x64 build. This packaging run did not launch ETS2 or repeat the practical game test. The rebuilt DLL differs from the preceding sanitized DLL only in two linker timestamps; its code, data and unwind information are otherwise identical. See [validation details](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/VALIDATION.md).

## Integrity

| File | Bytes | SHA256 |
| --- | ---: | --- |
| CargoMarketFix.dll | 316928 | `0CC2F6290ABCB670218927B7BCC49D74E153CF688865CAEB0FC05C1FDA1B6ABA` |
| ETS2-CargoMarketFix-v1.0.0-ETS2-1.57.2.7.zip | 160141 | `8937E1950C9B446DB356FD9E6D91AE5B60EB18F7288D7C8266EB8BF04F2C5357` |

The ZIP contains exactly the DLL, INI, README and LICENSE. [SHA256SUMS.txt](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/releases/v1.0.0/SHA256SUMS.txt) contains the same checksums and is prepared as a separate Release asset; it is not inside the ZIP.

## License

Licensed under the MIT License. Copyright (c) 2026 Abysimo. The standard license text is included in the package and in [LICENSE](https://github.com/Abysimo/ETS2-CargoMarketFix-Public/blob/main/LICENSE).

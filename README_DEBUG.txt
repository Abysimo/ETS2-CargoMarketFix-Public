ETS2 Cargo Market Fix 1.4.2-debug2-ets2-1.61.1.9
Startup diagnostic for exact ETS2 1.61.1.9 on Windows x64.

Required eurotrucks2.exe SHA256:
B70C2357CC56EB5834EE47FEA3C60B2F2D4591383FF2AC479CFD3CD254301B8C

Use this build to test recovery from the CAP4 startup failure where
OpenThread returned ERROR_INVALID_PARAMETER (87). CAP4 requires a later
clean thread snapshot within the existing four-pass limit. Other errors
still fail closed. If normal v1.4.2 works, keep using it.

1. Close ETS2 and back up CargoMarketFix.dll and CargoMarketFix.ini.
2. Copy both files from this ZIP into bin/win_x64/plugins.
3. Start ETS2 on the setup that previously reproduced the failure.
4. Close the game normally.
5. Send the complete bin/win_x64/plugins/CargoMarketFix.log directly to
   the developer, whether startup succeeded or failed. It can contain
   thread IDs and instruction addresses.

The supplied INI enables CAP4 (budget 4) and Refresh Spread (1440 minutes).
Use both files from this ZIP together. Do not use this DLL with another
ETS2 executable build.

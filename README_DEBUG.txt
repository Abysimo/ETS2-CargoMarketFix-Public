ETS2 Cargo Market Fix 1.4.2-debug1-ets2-1.61.1.9
Startup diagnostic for exact ETS2 1.61.1.9 on Windows x64.

Required eurotrucks2.exe SHA256:
B70C2357CC56EB5834EE47FEA3C60B2F2D4591383FF2AC479CFD3CD254301B8C

Use this build only to diagnose prepatch_quiescence_failed_never_exposed.
If normal v1.4.2 works, keep using it. This build adds logging and preserves
the original fail-closed behavior. There is no workaround.

1. Close ETS2 and back up CargoMarketFix.dll and CargoMarketFix.ini.
2. Copy both files from this ZIP into bin/win_x64/plugins.
3. Start ETS2 and reproduce the startup failure.
4. Close the game normally.
5. Send the complete bin/win_x64/plugins/CargoMarketFix.log directly to
   the developer. It can contain thread IDs and instruction addresses.

The supplied INI enables CAP4 (budget 4) and Refresh Spread (1440 minutes).
Use both files from this ZIP together. Do not use this DLL with another
ETS2 executable build.

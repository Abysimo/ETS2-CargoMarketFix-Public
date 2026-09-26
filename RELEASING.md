# Exact-target release workflow

This is a sanitized release snapshot. Development and validation originate in the
private repository; AGENTS.md remains authoritative. Publication needs explicit
user approval. The approved v1.4.0 family supersedes the old per-game minor-number
convention without rewriting any older release.

## Build profiles

The new 1.61.1.1 target uses functional version 1.4.1 and tag
`v1.4.1-ets2-1.61.1.1`. The prior five 1.4.0 profiles and releases stay intact.
Supported exact profiles: 1.57.2.7, 1.58.1.4, 1.59.1.3, 1.60.1.7,
1.61.1.0, 1.61.1.1.

Configure a separate output directory per target, for example:

```text
cmake -S . -B build/v140/1.57.2.7 -A x64 "-DCMF_ETS2_TARGET=1.57.2.7" "-DSCS_SDK_INCLUDE_DIR=<local-sdk-include>"
cmake --build build/v140/1.57.2.7 --config Release
```

The target macro restricts both hash recognition and descriptor ownership to one
descriptor. Runtime metadata names that exact target. Maintenance tests deliberately
retain the six-descriptor table to validate cross-target metadata; the separately
compiled scope test uses the same profile definitions as its DLL.

Enable CMF_BUILD_TESTS and supply CMF_GAME_EXE as read-only input for maintenance.
Run the shared native/config/lifecycle suites once; run scope and actual-DLL
unsupported-host checks for every profile. Do not claim fresh reads of unavailable
historical executables. Keep gameplay qualification separate from native validation.

## Package and publish

For this compatibility port, run `python tools/package_release.py --target 1.61.1.1 --build-root build/v141`
after that exact-target build and validation. This writes only the new target's
package/notes/checksums and leaves historical releases untouched. Omitting `--target`
packages every configured target and should not be used merely to add a new port.
It verifies x64, exactly two exports, only the intended compiled hash, distinct DLL
hashes, active source/built INI parity, and a strict five-file ZIP allowlist.
Outputs go to ignored release_staging; sanitized notes/manifests/checksums go to
releases/<tag>. No publication or deployment is performed by that script.

Each ZIP contains CargoMarketFix.dll, ACTIVE CargoMarketFix.ini (CAP4=4,
Spread1440), target-specific README, MIT LICENSE and the disabled INI example.
There are no diagnostics, SDK files, test binaries, logs or private tools.
Only 60/120/180/1440 are accepted. Port-specific patch sites/ABIs are exact gated;
the established balanced partition arithmetic is unchanged.

Review the source diff and packages, commit source/docs/checksums, push and verify
the remote commit. Create only the newly approved target tag at that commit and
publish an ordinary (non-draft, non-prerelease) release. Upload its ZIP and SHA256SUMS.txt.
Verify remote tag commits, notes, assets and downloaded bytes. Preserve all old
releases. If partially published, report exactly what is live and stop on failure;
do not delete successful releases automatically.

## Qualification

1.61.1.1: **STRUCTURALLY VALIDATED / LIVE RUNTIME DIAGNOSTIC TEST PASSED**.
The exact executable was physically run in a smaller-map setup with a private
diagnostic build retaining the production CAP4 and Spread1440 logic. A/B sessions
confirmed selected-bucket versus full-range normal sweeps, no observed fallback
or range failure, and clean diagnostic shutdown/restoration. The public ZIP DLL
was not the instrumented binary. Large-map freeze reduction, owned-trailer
large-map performance, sleep/bulk and offer quality remain unvalidated on this
exact target. 1.61.1.0 remains a separate 1.4.0 release with its original
structural-only qualification.

1.57: user-reported physical gameplay test of equivalent private Spread1440
implementation, heavy map combo / owned trailer / daytime, no observed recurring
freezes. Public binary is offline validated; no universal guarantee.
Others: structural only for1440. Historical 1.60 startup smoke is earlier-setting
evidence, not1440 gameplay or clean-shutdown certification. Explain the roughly
24-hour normal refresh delay and potentially fewer/less-fresh offers.

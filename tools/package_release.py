"""Package already validated target-specific builds. Does not publish or deploy."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile

TARGETS = {
    "1.57.2.7": "06C465048626DE0463B5FC7D4FE69DE917556AFB8CE99159DFB912F6D2806BF9",
    "1.58.1.4": "25CD132FB72576242C298E5EC5B6D940F2E38EBB183928AC1F16D075256BC644",
    "1.59.1.3": "E6FE1A58DF9D0BFFF21DCCC12B4F581DC0D3E3B5885DBF62ED05028DE794D35C",
    "1.60.1.7": "B7DFFE6B27402C7DB6DFD52CF982CD5BF292584138B35E3EB8EFB311814AB3F8",
}
ROOT = Path(__file__).resolve().parents[1]

def sha(data):
    return hashlib.sha256(data).hexdigest().upper()

def exports(data):
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    assert data[pe:pe+4] == b"PE\0\0"
    machine, sections = struct.unpack_from("<HH", data, pe+4)
    assert machine == 0x8664, "not x64"
    opt_size = struct.unpack_from("<H", data, pe+20)[0]
    opt = pe+24
    assert struct.unpack_from("<H", data, opt)[0] == 0x20b, "not PE32+"
    table = opt+opt_size
    def off(rva):
        for i in range(sections):
            vs, va, rs, rp = struct.unpack_from("<IIII", data, table+i*40+8)
            if va <= rva < va+max(vs, rs):
                assert rva-va < rs
                return rp+rva-va
        raise ValueError("unmapped RVA")
    exp = off(struct.unpack_from("<I", data, opt+112)[0])
    function_count, name_count, _, names = struct.unpack_from("<IIII", data, exp+20)
    result = []
    for i in range(name_count):
        start = off(struct.unpack_from("<I", data, off(names)+i*4)[0])
        result.append(data[start:data.index(0, start)].decode("ascii"))
    assert function_count == 2 and sorted(result) == ["scs_telemetry_init", "scs_telemetry_shutdown"]
    return sorted(result)

def qualification(version):
    if version == "1.57.2.7":
        return ("PHYSICALLY GAMEPLAY TESTED: the user tested the equivalent Spread1440 implementation "
                "with CAP4=4 on a heavy real map combo, an owned trailer, and daytime driving where "
                "strong recurring freezes had previously been visible. No recurring freezes were "
                "observed during that test; gameplay felt smooth. This is not a universal guarantee "
                "or a measured FPS result. The newly packaged public DLL has offline validation.")
    extra = (" A historical 1.60 startup smoke passed with earlier spread settings; it was not "
             "a Spread1440 gameplay test, and clean live shutdown was not confirmed.") if version == "1.60.1.7" else ""
    return ("STRUCTURALLY VALIDATED. No physical Spread1440 gameplay test is claimed for this target. "
            "No measured freeze reduction, owned-trailer, sleep/bulk gameplay, or offer-quality "
            "validation is claimed." + extra)

def notes(version, exe_hash):
    return f"""# ETS2 Cargo Market Fix v1.4.0 — ETS2 {version}

**Only ETS2 {version}, Windows x64. Do not use this DLL with another game version.**

Required eurotrucks2.exe SHA256:
`{exe_hash}`

Runtime version: `1.4.0-ets2-{version}`. Other known builds and unknown executables
fail closed. This is a native plugin, not an .scs mod.

## What's changed

The packaged active configuration changes Refresh Spread from 180 to **1440 game
minutes (24 game hours)**, with CAP4's fixed generation-attempt budget of four.
180-minute spreading could still cluster many expensive Cargo Market regenerations
in one game-minute update. 1440 reduces the maximum number of company entries in
each normal refresh slice; individual regeneration cost can still vary.
This is not evidence that time of day itself causes the issue, nor a universal FPS,
GPU, or general-stutter optimization.

## Install

1. Close ETS2 and back up any existing CargoMarketFix DLL and INI.
2. Download the ZIP whose name matches **ETS2 {version}**.
3. Copy **both** CargoMarketFix.dll and CargoMarketFix.ini into
   `Euro Truck Simulator 2/bin/win_x64/plugins/`.
4. Start ETS2 normally. CAP4=4 and Spread1440 are active in this package.

No observers or diagnostics are compiled into this DLL. There are no collectors.
To disable both patches, close ETS2 and replace the INI with the supplied disabled
example (renamed CargoMarketFix.ini), or remove the plugin while the game is closed.
Do not mix DLLs from different target releases. Install the INI, not just the DLL.

## Trade-offs

A company may wait up to roughly 24 in-game hours for its next normal spread-controlled
refresh. Fewer or less-fresh Cargo Market offers may temporarily occur. Sleep/bulk
processing and activation/direct generation are separate and unchanged.
No save format or serialization mechanism is changed and no cache is persisted.
Changed refresh scheduling can nevertheless change the offers later saved; disabling
the plugin does not retroactively regenerate identical historical offers.
No membership cache, accepted-vector changes, or adaptive scheduler is included.

## Validation

{qualification(version)}

Focused automated checks cover exact target selection and rejection, CAP4/Spread
signatures, both native spread ABIs with 1440 buckets, bounded arithmetic, empty
buckets, install/restore, rollback/containment and quiescence. Other targets' original
executables were not newly rerun: their previously validated descriptors and authored
native maintenance fixtures were used. The installed 1.57 executable was checked read-only.
See the repository VALIDATION.md for exact counts and limits.

## Files and integrity

This release contains one target DLL and one active INI, README, MIT LICENSE, and a
disabled example. Verify the ZIP and DLL against this release's SHA256SUMS.txt.
Older releases remain available unchanged.
"""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-root", type=Path, default=ROOT/"build"/"v140")
    args = parser.parse_args()
    cfg = (ROOT/"CargoMarketFix.ini").read_bytes()
    ini = configparser.ConfigParser()
    ini.read_string(cfg.decode("utf-8-sig"))
    assert ini.getboolean("General", "enabled")
    for sec, prefix in [("CMF_CAP4", "cap4"), ("CMF_REFRESH_SPREAD", "refresh_spread")]:
        assert ini.getboolean(sec, prefix+"_install") and ini.getboolean(sec, prefix+"_enabled")
    assert ini.getint("CMF_REFRESH_SPREAD", "refresh_spread_minutes") == 1440
    assert not ini.getboolean("Hooking", "enabled") and ini.getboolean("Hooking", "fail_closed")
    assert ini.getboolean("Build", "target_this_build_only") and not ini.getboolean("Build", "allow_unknown_build")
    assert set(ini.sections()) == {"General", "CMF_CAP4", "CMF_REFRESH_SPREAD", "Build", "Hooking"}
    disabled = configparser.ConfigParser()
    disabled.read(ROOT/"config_examples"/"CargoMarketFix.disabled.ini", encoding="utf-8")
    for sec, prefix in [("CMF_CAP4", "cap4"), ("CMF_REFRESH_SPREAD", "refresh_spread")]:
        assert not disabled.getboolean(sec, prefix+"_install")
        assert not disabled.getboolean(sec, prefix+"_enabled")
    assert not disabled.getboolean("Hooking", "enabled")
    records = []
    for version, exe_hash in TARGETS.items():
        tag = f"v1.4.0-ets2-{version}"
        dll = (args.build_root/version/"Release"/"CargoMarketFix.dll").read_bytes()
        assert (args.build_root/version/"Release"/"CargoMarketFix.ini").read_bytes() == cfg
        found = [v for v, h in TARGETS.items() if h.encode() in dll]
        assert found == [version], (version, "unexpected compiled hash set", found)
        assert f"1.4.0-ets2-{version}".encode() in dll
        for forbidden in [b"cmf_call_cost", b"CMF_DIAGNOSTIC_CALLS", b"cmf_membership_entry_bridge",
                          b"spread_density", b"cmf_observation_runtime", b"cmf_internal_destination_bridge"]:
            assert forbidden not in dll, forbidden
        export_names = exports(dll)
        text = notes(version, exe_hash).encode("utf-8")
        stage = ROOT/"release_staging"/tag
        stage.mkdir(parents=True, exist_ok=True)
        filename = f"ETS2-CargoMarketFix-v1.4.0-ETS2-{version}.zip"
        files = {"CargoMarketFix.dll": dll, "CargoMarketFix.ini": cfg, "README.md": text,
                 "LICENSE": (ROOT/"LICENSE").read_bytes(),
                 "config_examples/CargoMarketFix.disabled.ini":
                     (ROOT/"config_examples"/"CargoMarketFix.disabled.ini").read_bytes()}
        with zipfile.ZipFile(stage/filename, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name, content in files.items():
                info = zipfile.ZipInfo(name, (2026, 9, 12, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, content)
        with zipfile.ZipFile(stage/filename) as archive:
            assert set(archive.namelist()) == set(files) and archive.testzip() is None
            for name, content in files.items():
                assert archive.read(name) == content
        zipped = (stage/filename).read_bytes()
        sums = f"{sha(dll)}  CargoMarketFix.dll\n{sha(zipped)}  {filename}\n"
        (stage/"SHA256SUMS.txt").write_text(sums, encoding="utf-8", newline="\n")
        docs = ROOT/"releases"/tag
        docs.mkdir(parents=True, exist_ok=True)
        (docs/"RELEASE_NOTES.md").write_bytes(text)
        (docs/"SHA256SUMS.txt").write_text(sums, encoding="utf-8", newline="\n")
        record = dict(target=version, tag=tag, executable_sha256=exe_hash,
                      url=f"https://github.com/Abysimo/ETS2-CargoMarketFix-Public/releases/tag/{tag}",
                      dll_version=f"1.4.0-ets2-{version}", dll_bytes=len(dll), dll_sha256=sha(dll),
                      zip_name=filename, zip_bytes=len(zipped), zip_sha256=sha(zipped),
                      exports=export_names, qualification=qualification(version))
        (docs/"manifest.json").write_text(json.dumps(record, indent=2)+"\n", encoding="utf-8", newline="\n")
        records.append(record)
    assert len({r["dll_sha256"] for r in records}) == 4, "DLLs must be distinct"
    print(json.dumps(records, indent=2))

if __name__ == "__main__":
    main()

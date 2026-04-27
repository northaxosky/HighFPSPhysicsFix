#!/usr/bin/env python3
"""
Test harness for High FPS Physics Fix.

Deploys the built DLL to the MO2 mod folder, launches Fallout 4 via MO2's
"OG - F4SE" custom executable, waits for StartOnSave to load a save, then
force-closes Fallout4.exe (NEVER ModOrganizer.exe — that would brick the
RootBuilder VFS) and reports whether the plugin loaded cleanly.

Usage:
    python scripts/test_deploy_run.py [--dll PATH] [--dry-run] [--no-launch]
                                      [--load-wait 60] [--spawn-timeout 90]
                                      [--timeout 60]
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

# --- Configuration (environment-specific paths) -----------------------------
MO2_EXE         = Path(r"C:\Games\Modding\Nucleus\ModOrganizer.exe")
MO2_EXECUTABLE  = "OG - F4SE"
MO2_PROFILE     = "Default"
DEPLOY_DIR      = Path(r"C:\Games\Modding\Nucleus\mods\High FPS Physics Fix\F4SE\Plugins")
DEPLOYED_DLL    = DEPLOY_DIR / "HighFPSPhysicsFix.dll"
# Default source for --dll: the freshly-built Release binary. Falling back to
# DEPLOYED_DLL would be a footgun (it would deploy the currently-installed DLL
# onto itself, silently re-validating stale bits after a rebuild).
REPO_ROOT       = Path(__file__).resolve().parent.parent
BUILT_DLL       = REPO_ROOT / "build" / "Release" / "HighFPSPhysicsFix.dll"
F4SE_LOG_DIR    = Path.home() / "Documents" / "My Games" / "Fallout4" / "F4SE"
PLUGIN_LOG      = F4SE_LOG_DIR / "HighFPSPhysicsFix.log"

# --- Output helpers ---------------------------------------------------------
def step(msg: str) -> None: print(f"==> {msg}", flush=True)
def ok  (msg: str) -> None: print(f"  OK  {msg}", flush=True)
def warn(msg: str) -> None: print(f"  !!  {msg}", flush=True)
def fail(msg: str) -> None: print(f"  XX  {msg}", flush=True)

# --- Process helpers --------------------------------------------------------
def find_processes(name: str) -> list[dict]:
    """Return [{Id, ProcessName}, ...] for processes whose .exe stem matches name (case-insensitive)."""
    try:
        out = subprocess.run(
            ["tasklist.exe", "/FO", "CSV", "/NH", "/FI", f"IMAGENAME eq {name}.exe"],
            capture_output=True, text=True, check=False,
        )
    except OSError as e:
        warn(f"tasklist failed: {e}")
        return []
    rows: list[dict] = []
    for line in out.stdout.splitlines():
        # CSV: "Image","PID","Session","Session#","MemUsage"
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) >= 2 and parts[0].lower().startswith(name.lower()):
            try:
                rows.append({"name": parts[0], "pid": int(parts[1])})
            except ValueError:
                continue
    return rows

def kill_pid(pid: int) -> None:
    try:
        subprocess.run(["taskkill.exe", "/F", "/PID", str(pid)],
                       capture_output=True, check=False)
    except OSError as e:
        warn(f"taskkill PID {pid} failed: {e}")

def assert_not_mo2(pid: int, name: str) -> None:
    if name.lower().startswith("modorganizer"):
        raise RuntimeError(f"REFUSED to operate on ModOrganizer.exe (PID {pid})")

# --- Main pipeline ----------------------------------------------------------
def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dll", type=Path, default=None,
                   help=f"Source DLL to deploy (default: {BUILT_DLL}).")
    p.add_argument("--profile", default=MO2_PROFILE)
    p.add_argument("--executable", default=MO2_EXECUTABLE)
    p.add_argument("--load-wait", type=int, default=None,
                   help="Seconds to wait after Fallout4.exe spawns. "
                        "Defaults to --timeout (60).")
    p.add_argument("--spawn-timeout", type=int, default=90,
                   help="Seconds to wait for Fallout4.exe to appear.")
    p.add_argument("--timeout", type=int, default=60,
                   help="Convenience override for --load-wait (default 60s).")
    p.add_argument("--dry-run", action="store_true",
                   help="Skip DLL deployment (uses currently-deployed DLL).")
    p.add_argument("--no-launch", action="store_true",
                   help="Deploy only; do not launch Fallout 4 or analyze logs.")
    args = p.parse_args()

    load_wait = args.load_wait if args.load_wait is not None else args.timeout

    # --- Pre-flight ---------------------------------------------------------
    step("Pre-flight checks")
    src_dll = args.dll if args.dll is not None else BUILT_DLL
    src_dll = Path(src_dll)

    required = [
        ("MO2 executable", MO2_EXE),
        ("Deploy directory", DEPLOY_DIR),
        ("F4SE log directory", F4SE_LOG_DIR),
    ]
    for label, path in required:
        if not path.exists():
            fail(f"Missing {label}: {path}")
            return 2
        ok(f"{label}: {path}")

    if not src_dll.exists():
        fail(f"DLL not found: {src_dll}")
        return 2
    ok(f"Source DLL: {src_dll}  ({src_dll.stat().st_size} bytes)")

    # --- Snapshot log/crash state ------------------------------------------
    step("Snapshotting existing log state")
    pre_size = PLUGIN_LOG.stat().st_size if PLUGIN_LOG.exists() else 0
    pre_mtime = PLUGIN_LOG.stat().st_mtime if PLUGIN_LOG.exists() else 0.0
    pre_crashes = {p.name for p in F4SE_LOG_DIR.glob("crash-*.log")}
    ok(f"Plugin log size before: {pre_size} bytes")
    ok(f"Existing crash logs: {len(pre_crashes)}")

    # --- Deploy DLL ---------------------------------------------------------
    if args.dry_run:
        warn("DryRun: skipping DLL deployment")
    else:
        step("Deploying DLL")
        # Atomic-ish: copy to .new then replace
        tmp = DEPLOYED_DLL.with_suffix(".dll.new")
        try:
            shutil.copy2(src_dll, tmp)
            os.replace(tmp, DEPLOYED_DLL)
        except OSError as e:
            fail(f"Deploy failed: {e}")
            return 2
        ok(f"Deployed -> {DEPLOYED_DLL}")

    if args.no_launch:
        step("--no-launch: skipping launch and log analysis")
        print()
        print("RESULT: PASS")
        return 0

    # --- Confirm no stale game ---------------------------------------------
    stale = find_processes("Fallout4")
    if stale:
        fail(f"Fallout4.exe already running (PID {stale[0]['pid']}). Aborting.")
        return 3

    # --- Launch via MO2 -----------------------------------------------------
    # subprocess.Popen with a list passes argv directly to CreateProcess —
    # no shell quoting hell, spaces in the URL survive intact.
    shortcut = f"moshortcut://:{args.executable}"
    step(f"Launching '{args.executable}' (profile: {args.profile})")
    print(f"    argv: {[str(MO2_EXE), '-p', args.profile, shortcut]}")
    try:
        subprocess.Popen(
            [str(MO2_EXE), "-p", args.profile, shortcut],
            cwd=str(MO2_EXE.parent),
        )
    except OSError as e:
        fail(f"Failed to spawn MO2: {e}")
        return 4

    # --- Wait for Fallout4.exe ---------------------------------------------
    step(f"Waiting up to {args.spawn_timeout}s for Fallout4.exe")
    fo4_pid = None
    deadline = time.monotonic() + args.spawn_timeout
    while time.monotonic() < deadline:
        procs = find_processes("Fallout4")
        if procs:
            fo4_pid = procs[0]["pid"]
            assert_not_mo2(fo4_pid, procs[0]["name"])
            break
        time.sleep(0.5)
    if fo4_pid is None:
        fail("Fallout4.exe never appeared. F4SE/MO2 launch likely failed.")
        return 4
    ok(f"Fallout4.exe spawned (PID {fo4_pid})")

    # --- Wait for save load, watch for early exit (crash) ------------------
    step(f"Waiting {load_wait}s for StartOnSave to load the save")
    early_exit = False
    deadline = time.monotonic() + load_wait
    while time.monotonic() < deadline:
        time.sleep(0.5)
        if not find_processes("Fallout4"):
            early_exit = True
            break

    if early_exit:
        fail("Fallout4.exe exited early. Likely crash on load.")
    else:
        ok(f"Fallout4.exe still alive after {load_wait}s")

    # --- Force-close Fallout4.exe (NEVER MO2) ------------------------------
    procs = find_processes("Fallout4")
    if procs:
        step("Force-closing Fallout4.exe (PID-targeted, MO2 untouched)")
        for proc in procs:
            assert_not_mo2(proc["pid"], proc["name"])
            kill_pid(proc["pid"])
        # Wait for shutdown
        for _ in range(20):
            if not find_processes("Fallout4"):
                break
            time.sleep(0.5)
        ok("Fallout4.exe terminated")

    mo2_after = find_processes("ModOrganizer")
    if mo2_after:
        ok(f"ModOrganizer.exe still running (PID {mo2_after[0]['pid']}) - VFS intact")
    else:
        warn("ModOrganizer.exe is no longer running.")

    # --- Analyze logs ------------------------------------------------------
    step("Analyzing logs")
    plugin_log_grew = False
    plugin_loaded = False
    game_version = None
    log_tail = ""
    if PLUGIN_LOG.exists():
        st = PLUGIN_LOG.stat()
        plugin_log_grew = (st.st_size != pre_size) or (st.st_mtime != pre_mtime)
        try:
            text = PLUGIN_LOG.read_text(encoding="utf-8", errors="replace")
        except OSError:
            text = ""
        lines = text.splitlines()
        log_tail = "\n".join(lines[-25:])
        for line in lines:
            if "HighFPSPhysicsFix v" in line:
                plugin_loaded = True
            if "Game version" in line:
                # "[ts][info] Game version : 1-10-163-0" — split on "Game version"
                # to skip leading timestamp colons.
                _, _, rhs = line.partition("Game version")
                game_version = rhs.lstrip(" :").strip()

    post_crashes = {p.name for p in F4SE_LOG_DIR.glob("crash-*.log")}
    new_crashes = sorted(post_crashes - pre_crashes)

    # --- Verdict -----------------------------------------------------------
    print()
    step("Verdict")
    passed = True
    if not plugin_log_grew:
        fail("Plugin log did not advance — plugin likely never loaded")
        passed = False
    elif not plugin_loaded:
        fail("Plugin log present but no version banner — load failure")
        passed = False
    else:
        ok(f"Plugin loaded (game version: {game_version})")

    if early_exit:
        fail("Fallout4.exe exited before wait window — probable crash")
        passed = False
    if new_crashes:
        fail("New crash logs detected:")
        for c in new_crashes:
            print(f"    {c}")
        passed = False

    print()
    print("----- HighFPSPhysicsFix.log (tail) -----")
    print(log_tail)
    print("----------------------------------------")

    print()
    print(f"RESULT: {'PASS' if passed else 'FAIL'}")
    return 0 if passed else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        sys.exit(130)
    except RuntimeError as e:
        print(f"\nRefused: {e}", file=sys.stderr)
        sys.exit(5)

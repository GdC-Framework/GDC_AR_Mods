"""Automated tests of "Update MM Libraries" (GDC Mission Maker Tools), see README.md.

Runs the Workbench from the command line on a test library in _work/ (never the mission maker's own library),
then checks the files (check_library.py), the Workbench log and what the Workbench registered
(GDC MM Tools Tests addon).

Usage: python run_tests.py [--full] [--authorize-scripts]
  --full               also updates the library with the whole GDC modset and reports the duration
  --authorize-scripts  passes -scriptAuthorizeAll: no "Script Authorization Required" dialog (FileIO calls).
                       Off by default: the user decides each time whether to lift this protection;
                       without it, someone must click the dialog at each Workbench start.
Exit code 0 when every step passes. Logs and reports are kept in _work/results/.
"""
import os
import re
import shutil
import subprocess
import sys
import time

import check_library

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
WORK = os.path.join(HERE, "_work")
LIBRARY = os.path.join(WORK, "GDC Mission Maker Libraries")
RESULTS = os.path.join(WORK, "results")

STEAM = r"C:\Program Files (x86)\Steam\steamapps\common"
WORKBENCH_DIR = os.path.join(STEAM, "Arma Reforger Tools", "Workbench")
WORKBENCH = os.path.join(WORKBENCH_DIR, "ArmaReforgerWorkbenchSteamDiag.exe")
PROFILE = "GDCMMTest"
PROFILE_LOGS = os.path.join(os.path.expanduser("~"), "Documents", "My Games", PROFILE, "logs")
WORKBENCH_ADDONS = os.path.join(os.path.expanduser("~"), "Documents", "My Games", "ArmaReforgerWorkbench", "addons")
MMTOOLS_GPROJ = os.path.join(REPO, "GDC_AR_Misc", "GDC AR Mission Maker Tools", "addon.gproj")

# The first folder holding an addon ID wins: _work comes before the Workbench addons folder,
# where the mission maker's own GDC Mission Maker Libraries usually is.
ADDON_DIRS = [
    os.path.join(STEAM, "Arma Reforger", "addons"),
    os.path.join(REPO, "GDC_AR_Misc"),
    HERE,
    WORK,
    WORKBENCH_ADDONS,
]

LIBRARY_ID = check_library.LIBRARY_ADDON_ID
TESTS_ID = "GDC MM Tools Tests"

# TILW and GDC AR Lib bring PSCore, Reforger Lobby and Coalition VON as dependencies
BASE = ["TilWMissionFramework", "GDC AR Lib"]
RHS = BASE + ["RHS_Core", "RHS_Content_01", "RHS_Content_02"]
FULL = [
    "ACE_Core_Dev", "ACE_Backblast_Dev", "ACE_Carrying_Dev", "ACE_Chopping_Dev", "ACE_Compass_Dev", "ACE_Finger_Dev",
    "ACE_Medical_Core_Dev", "ACE_Medical_Hitzones_Dev", "ACE_Medical_Circulation_Dev", "ACE_Trenches_Dev",
    "ACE_MagRepack_Dev", "ACE_Explosives_Dev", "ACE_Captives_Dev", "ACE_TacticalLadder_Dev", "ACE_Radio_Dev",
    "ACE_TacticalPeriscope_Dev", "ACE_Overheating_Dev", "ACE_CookOff_Dev", "ACE_Weather_Dev", "ACE_Facepaint_Dev",
    "ACE_Ballistics_Dev", "ACE_Medical_Breathing_Dev", "ACE_Scopes_Dev", "ACE_AllInOne_Dev", "CoalitionVON",
    "CoalitionSquadInterface", "FixedLoadCasualtyVehicles", "FNGGMBudgetless", "GameMasterFX", "Grays_Adjustable_Doors",
    "ImprovedBloodEffect", "MaehNoWeapDropUnCon", "NoIconForUncon", "NoSupplySystem", "RealismOverhaulAI",
    "RealismOverhaulBallistics", "WCS_Armbands", "Youcansit", "GDC AR Core", "ChungusLaunchers", "MarkerRoundsProject",
    "EchoesOf89UGLTextures", "RHS_Content_01", "RHS_Content_02", "RHS_Core", "FPTR", "WCS_Armaments", "WCS_SpaceCore",
    "JLTV", "M2A2", "BTR70Mountable", "RHIB", "RussianMarkerGrenades", "LaserRangefinder", "M4MAAWS", "STRYKER",
    "Wirecutters2", "WZLaunchers", "ZU23", "GDC AR Equipment", "TilWMissionFramework", "PSCoreDevelopment",
    "ReforgerLobbyCommunity", "GDC AR Lib",
]

SUMMARY_RE = re.compile(r"\[MM Libraries\] (\d+) new, (\d+) kept, (\d+) removed, (\d+) failed\.")
RESULT_RE = re.compile(r"\[MM Libraries\] command line result: (\w+) in (\d+) ms")
SCRIPT_ERROR_RE = re.compile(r"SCRIPT\s+\(E\).*")


class Step:
    def __init__(self, name):
        self.name = name
        self.failures = []
        self.notes = []

    def expect(self, condition, message):
        if not condition:
            self.failures.append(message)
        return condition


steps = []


def norm(path):
    return path.replace("\\", "/")


def run_workbench(step, addons, plugin, extra="", expected_error=None):
    """Runs the Workbench headless; returns (exit code, console log, seconds).
    expected_error: text of a script error the step expects, not counted as a failure."""
    dirs = ",".join(norm(d) for d in ADDON_DIRS)
    # -scriptAuthorizeAll (opt-in, --authorize-scripts): no "Script Authorization Required" dialog
    authorize = " -scriptAuthorizeAll" if "--authorize-scripts" in sys.argv else ""
    command = (
        f'"{WORKBENCH}" -gproj "{norm(MMTOOLS_GPROJ)}" -addonsDir "{dirs}" -addons "{",".join(addons)}" '
        f'-profile {PROFILE}{authorize} -wbModule=ResourceManager -plugin={plugin} -run {extra}'
    )
    start = time.time()
    process = subprocess.Popen(command, cwd=WORKBENCH_DIR)
    code = None
    blocked = None
    # a compile error opens a dialog and the Workbench waits forever: watch the log and stop it
    while time.time() - start < 900:
        code = process.poll()
        if code is not None:
            break
        if "Can't compile" in read_log(start):
            blocked = "compile error, Workbench stopped"
            break
        time.sleep(2)
    if code is None:
        process.kill()
        process.wait()
        blocked = blocked or "Workbench timeout"
    seconds = time.time() - start

    log = read_log(start, os.path.join(RESULTS, step.name + ".console.log"))
    step.expect(not blocked, blocked)
    step.expect(log, "no Workbench log found")
    # a missing addon or dependency: the requested addons are not all loaded
    for line in re.findall(r"ENGINE\s+\(E\): (?:Addon .* can't be added|Game addon .* not found|.*Cannot initialize.*)", log):
        step.failures.append("addon loading: " + line.strip())
    for line in SCRIPT_ERROR_RE.findall(log):
        if not (expected_error and expected_error in line):
            step.failures.append("script error: " + line.strip())
    step.notes.append(f"Workbench {seconds:.0f} s")
    return code, log, seconds


def read_log(start, copy_to=None):
    """Console log of the Workbench run started at start, empty when not written yet."""
    if not os.path.isdir(PROFILE_LOGS):
        return ""
    folders = [os.path.join(PROFILE_LOGS, f) for f in os.listdir(PROFILE_LOGS)]
    folders = [f for f in folders if os.path.getctime(f) >= start - 2]
    if not folders:
        return ""
    path = os.path.join(max(folders, key=os.path.getctime), "console.log")
    if not os.path.isfile(path):
        return ""
    if copy_to:
        shutil.copy(path, copy_to)
    with open(path, encoding="utf-8", errors="replace") as file:
        return file.read()


def loaded_library_path(log):
    match = re.search(r"Loaded addons:(.*?)GameProject load", log, re.S)
    if not match:
        return None
    for path in re.findall(r"gproj: '([^']+)'", match.group(1)):
        if path.endswith("/GDC Mission Maker Libraries/addon.gproj"):
            return path
    return None


def generate(step, addons, library_loaded, extra=""):
    """Runs Update MM Libraries; returns (new, kept, removed, failed) or None."""
    all_addons = addons + [LIBRARY_ID] if library_loaded else addons
    code, log, _ = run_workbench(step, all_addons, "GDC_MMCatalogGeneratorPlugin", extra)

    loaded = loaded_library_path(log)
    if library_loaded:
        step.expect(loaded == norm(LIBRARY) + "/addon.gproj", f"wrong library loaded: {loaded}")
    else:
        step.expect(loaded is None, f"library unexpectedly loaded: {loaded}")

    result = RESULT_RE.search(log)
    step.expect(result, "no command line result in the log (plugin not run, compile error?)")
    if result:
        step.notes.append(f"generation {int(result.group(2)) / 1000:.1f} s")
    summary = SUMMARY_RE.search(log)
    if not summary:
        step.failures.append(f"no summary in the log (exit code {code})")
        return code, None
    counts = tuple(int(x) for x in summary.groups())
    step.notes.append("%d new, %d kept, %d removed, %d failed" % counts)
    for line in re.findall(r"\[MM Libraries\] failed: .*", log):
        step.failures.append(line)
    return code, counts


def check_files(step):
    library = check_library.check(LIBRARY)
    for message in library.errors[:20]:
        step.failures.append("files: " + message)
    if len(library.errors) > 20:
        step.failures.append(f"files: {len(library.errors) - 20} more problems")
    step.notes.append(f"{len(library.prefabs)} prefabs, {len(library.index)} index entries")
    return library


def check_registered(step, addons, library):
    """Runs the test plugin; returns {path: (parent, intercepted, addon ID, addon title)}."""
    report_path = os.path.join(RESULTS, step.name + ".report.tsv")
    if os.path.exists(report_path):
        os.remove(report_path)
    code, log, _ = run_workbench(step, addons + [LIBRARY_ID, TESTS_ID], "GDC_MMLibrariesCheckPlugin",
                                 f'-mmTestReport="{norm(report_path)}"')
    step.expect(code == 0, f"test plugin exit code {code}")
    if not step.expect(os.path.isfile(report_path), "no report written"):
        return {}

    report = {}
    with open(report_path, encoding="utf-8") as file:
        for line in file:
            fields = line.rstrip("\n").split("\t")
            # ResourceName printed as {GUID}path
            path = re.sub(r"^\{[0-9A-F]{16}\}", "", fields[0])
            report[path] = tuple(fields[1:])

    on_disk = set(library.prefabs)
    missing = sorted(on_disk - set(report))
    step.expect(not missing, f"{len(missing)} library prefabs not registered by the Workbench (forbidden sign on drop), "
                             f"e.g. {missing[:3]}")
    extra = sorted(set(report) - on_disk)
    step.expect(not extra, f"registered but not on disk: {extra[:3]}")

    index = {entry[0]: entry for entry in library.index}
    for path, (parent, intercepted, addon_id, addon_title) in report.items():
        if intercepted != "1":
            step.failures.append(f"not intercepted on drop: {path}")
        if path in index and (addon_id, addon_title) != index[path][1:]:
            step.failures.append(f"index lookup differs for {path}: {addon_id}")
        if path not in index and addon_id:
            step.failures.append(f"index lookup found an entry not in the file: {path}")
        if path in library.prefabs and parent:
            _, guid, parent_path = library.prefabs[path]
            if parent != "{%s}%s" % (guid, parent_path):
                step.failures.append(f"parent on drop {parent} differs from the file: {path}")
    return report


def snapshot(library):
    return dict(library.guids)


# ---------------------------------------------------------------------------------------------------------------------
def scenario(full):
    # empty _work rather than delete it: a shell or explorer opened in results/ would lock the folder
    if os.path.isdir(LIBRARY):
        shutil.rmtree(LIBRARY)
    os.makedirs(RESULTS, exist_ok=True)
    for name in os.listdir(RESULTS):
        os.remove(os.path.join(RESULTS, name))

    # 1. first run, library addon not loaded: created with its meta files
    step = Step("1_create_base")
    steps.append(step)
    code, counts = generate(step, BASE, False, f'-mmLibraryFolder="{norm(LIBRARY)}"')
    step.expect(code == 0, f"exit code {code}")
    if counts:
        step.expect(counts[0] > 0 and counts[1:] == (0, 0, 0), "expected only new prefabs")
    base = check_files(step)
    base_snapshot = snapshot(base)
    for library_name in ("MM Forces", "MM Markers", "MM Systems"):
        step.expect(any(p.split("/")[1] == library_name for p in base.prefabs), f"{library_name} empty")
    if not base.prefabs:
        return

    # 2. the Workbench opens it: every prefab registered, parent and index readable as on drop
    step = Step("2_registered_base")
    steps.append(step)
    report = check_registered(step, BASE, base)
    step.expect(all(r[0] for r in report.values()), "a parent prefab cannot be read with its addon loaded")

    # 3. update with the same addons: nothing changes
    step = Step("3_update_same")
    steps.append(step)
    code, counts = generate(step, BASE, True)
    step.expect(code == 0, f"exit code {code}")
    if counts:
        step.expect(counts == (0, len(base.prefabs), 0, 0), "expected every prefab kept")
    library = check_files(step)
    step.expect(snapshot(library) == base_snapshot, "GUIDs or paths changed")

    # 4. RHS added: new prefabs registered at once, existing ones untouched
    step = Step("4_update_rhs")
    steps.append(step)
    code, counts = generate(step, RHS, True)
    step.expect(code == 0, f"exit code {code}")
    if counts:
        step.expect(counts[0] > 0 and counts[2:] == (0, 0), "expected new prefabs and no removal")
    rhs = check_files(step)
    rhs_snapshot = snapshot(rhs)
    step.expect(all(rhs_snapshot.get(p) == g for p, g in base_snapshot.items()), "existing GUIDs or paths changed")
    rhs_indexed = [entry[0] for entry in rhs.index if entry[1].startswith("RHS_")]
    step.expect(rhs_indexed, "no RHS prefab in the index")
    step.notes.append(f"{len(rhs_indexed)} RHS prefabs indexed")

    step = Step("5_registered_rhs")
    steps.append(step)
    report = check_registered(step, RHS, rhs)
    step.expect(all(r[0] for r in report.values()), "a parent prefab cannot be read with its addon loaded")

    # 6. opened without RHS before updating: RHS prefabs unreadable, the index names RHS (dialog on drop)
    step = Step("6_registered_without_rhs")
    steps.append(step)
    report = check_registered(step, BASE, rhs)
    # RHS factions also list vanilla prefabs (tripods...): those still drop. The others must be named by the index,
    # otherwise the dialog cannot tell which addon to load.
    rhs_only = set(rhs.prefabs) - set(base.prefabs)
    readable = [p for p in rhs_only if report.get(p, ("",))[0]]
    unnamed = sorted(p for p in rhs_only if p in report and not report[p][0] and not report[p][2].startswith("RHS_"))
    step.expect(not unnamed, f"{len(unnamed)} RHS prefabs neither droppable nor named RHS by the index, e.g. {unnamed[:3]}")
    step.expect(len(readable) < len(rhs_only), "every RHS prefab still readable: RHS not unloaded?")
    step.notes.append(f"{len(rhs_only)} RHS-faction prefabs, {len(readable)} with a vanilla parent")
    step.expect(all(report[p][0] for p in base.prefabs if p in report), "a base prefab lost its parent")

    # 7. update without RHS: mirror of the loaded addons again, back to the first library
    step = Step("7_update_without_rhs")
    steps.append(step)
    code, counts = generate(step, BASE, True)
    step.expect(code == 0, f"exit code {code}")
    if counts:
        step.expect(counts == (0, len(base.prefabs), len(rhs_only), 0), "expected the RHS prefabs removed only")
    library = check_files(step)
    step.expect(snapshot(library) == base_snapshot, "library differs from the first one")

    # 8. creation refused in a folder already holding an addon
    step = Step("8_create_refused")
    steps.append(step)
    code, log, _ = run_workbench(step, BASE, "GDC_MMCatalogGeneratorPlugin", f'-mmLibraryFolder="{norm(LIBRARY)}"',
                                 expected_error="an addon already exists")
    step.expect(code == 1, f"exit code {code}, expected 1")
    step.expect("an addon already exists" in log, "no error message in the log")
    step.expect(snapshot(check_library.check(LIBRARY)) == base_snapshot, "library modified")

    if full:
        step = Step("9_update_full_modset")
        steps.append(step)
        code, counts = generate(step, FULL, True)
        step.expect(code == 0, f"exit code {code}")
        check_files(step)


def main():
    # safety: no other library addon may come before _work in the addon folders
    for directory in ADDON_DIRS[:3]:
        for name in os.listdir(directory) if os.path.isdir(directory) else []:
            gproj = os.path.join(directory, name, "addon.gproj")
            if os.path.isfile(gproj) and f'ID "{LIBRARY_ID}"' in open(gproj, encoding="utf-8").read():
                print(f"Refused: {gproj} would hide the test library.")
                return 2

    started = time.time()
    scenario("--full" in sys.argv)

    lines = []
    for step in steps:
        status = "FAIL" if step.failures else "ok"
        lines.append(f"[{status}] {step.name}: {'; '.join(step.notes)}")
        lines.extend("       - " + failure for failure in step.failures)
    failed = sum(1 for step in steps if step.failures)
    lines.append(f"{len(steps) - failed}/{len(steps)} steps passed in {time.time() - started:.0f} s")

    text = "\n".join(lines)
    print(text)
    with open(os.path.join(RESULTS, "summary.txt"), "w", encoding="utf-8") as file:
        file.write(text + "\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

"""Automatic tests of Check Mission (GDC Mission Maker Tools) and of the loadout analysis it shares with the Loadout
Editor. Runs the Workbench without its interface; can run while another Workbench is open.

    python run_tests.py                 every step, about 1 minute
    python run_tests.py --gdc-missions  also the skeleton world of GDC Missions (real mission files)
    python run_tests.py --keep-logs     keep every Workbench log in _work/results/

Exit code 0 when every step passes. See README.md.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time
from collections import Counter
from datetime import datetime
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
STEAM = Path(r"C:\Program Files (x86)\Steam\steamapps\common")
WB_DIR = STEAM / "Arma Reforger Tools" / "Workbench"
WB = WB_DIR / "ArmaReforgerWorkbenchSteamDiag.exe"
MM_TOOLS = REPO / "GDC_AR_Misc" / "GDC AR Mission Maker Tools"
TEST_ADDON = HERE / "GDC Check Mission Tests"
TEST_ADDON_GUID = "6C0A11E0C4EC5001"
WORK = HERE / "_work"
PROFILE = "GDCCheckTests"
LOGS = Path.home() / "Documents" / "My Games" / PROFILE / "logs"
ADDON_DIRS = [STEAM / "Arma Reforger" / "addons", REPO / "GDC_AR_Misc", REPO / "GDC_AR_Missions", HERE,
              Path.home() / "Documents" / "My Games" / "ArmaReforgerWorkbench" / "addons"]
TIMEOUT = 300

# T_Faulty: rule -> (count, severity, text expected in the "where" of one of the findings, or None)
FAULTY = {
    "1.1": (1, "INFO", None), "1.2": (2, "ERROR", "PS_GameMode_Lobby2"), "1.3": (1, "WARNING", None),
    "1.4": (3, "ERROR", "SCR_AIWorld"), "1.5": (1, "INFO", None), "1.6": (1, "INFO", "b_Situation"),
    "1.7": (1, "INFO", "a_Contexte"),
    "2.1": (1, "ERROR", "D_Missing"), "2.2": (1, "ERROR", "D_Proxy"), "2.3": (1, "ERROR", "SCR_AIGroup"),
    # D_WrongGuid holds an unknown GUID on an existing path: the Workbench repairs the GUID when it loads the world, so it
    # must give no finding (2.1 x1 is D_Missing only; the former rule "unknown GUID" was removed for that reason)
    "3.2": (1, "ERROR", "G_NoFaction"), "3.3": (1, "ERROR", "G_Freq"), "3.4": (1, "ERROR", "G_NoFaction"),
    "3.5": (1, "WARNING", "G_NoFaction"), "3.6": (1, "WARNING", "G_NoFaction"), "3.7": (1, "ERROR", "G_NoFaction"),
    "3.8": (1, "WARNING", "G_NoFaction"), "3.9": (1, "WARNING", "G_NoFaction"),
    "4.1": (1, "WARNING", "D_NoFilter"), "4.2": (1, "ERROR", "V_Crew"), "4.3": (2, "WARNING", "SCR_AIWorld"),
    "4.4": (3, "ERROR", "G_BadFaction"), "4.5": (1, "ERROR", "T_Teleport"), "4.6": (1, "ERROR", "T_Teleport"),
    "4.7": (1, "ERROR", "T_Support"), "4.8": (1, "ERROR", "T_Support"), "4.9": (1, "ERROR", "T_Support"),
    "4.10": (1, "ERROR", "T_Artillery"),
    "5.1": (1, "WARNING", "TILW_SetFlagInteraction"), "5.2": (1, "WARNING", "Trg_Presence"), "5.3": (1, "ERROR", "Trg_Faction"),
    "5.4": (1, "WARNING", "Trg_Faction"), "5.5": (1, "ERROR", "Trg_Presence"),
    # mission events of TILW_MF_A (event and meta flag without condition...), TILW_MF_B (second framework entity), P_Spawner
    "5.6": (2, "ERROR", "TILW_MF_A"), "5.7": (1, "ERROR", "TILW_MF_A"), "5.8": (2, "WARNING", "P_Spawner"),
    "5.9": (1, "ERROR", "TILW_MF_A"), "5.10": (1, "WARNING", "TILW_MF_A"), "5.11": (2, "ERROR", "P_Spawner"),
    "5.12": (1, "ERROR", "TILW_MF_"), "5.13": (1, "WARNING", "TILW_MF_A"), "5.14": (1, "WARNING", "TILW_MF_A"),
}
# engine behaviours the TILW mission event rules rely on (GDC_MMCheckEngineProbePlugin): probe -> markers expected
PROBES = {
    "terms": ["EMPTY_AND true", "EMPTY_OR false"],  # 5.7: an empty Conjunction is true, an empty Disjunction false
    # 5.6: a call on null logs an error and the script goes on, the test of TILW_MissionEvent then skips the event
    "loop": ["STEP_SKIPPED 0", "STEP_RAN 1", "AFTER_LOOP"],
    "nullarray": ["NULL_AND true"],  # 5.7: an operand array never created gives the same result as an empty one
}
# Reforger Lobby without playable group, no TILW entity: the TILW rules must not run
NO_PLAYABLE = {"1.3": (1, "WARNING", None), "3.1": (1, "ERROR", None)}
# skeleton of GDC Missions, RHS not loaded (its group then has no prefab: 2.3)
SKELETON = {"1.7": (5, "INFO", "a_Contexte"), "2.3": (1, "ERROR", None), "3.1": (1, "ERROR", None), "4.1": (1, "WARNING", None),
            "4.2": (1, "ERROR", "Vehicle_1"), "5.1": (1, "WARNING", None)}
# French second person singular, forbidden in texts for mission makers (vouvoiement)
TUTOIEMENT = re.compile(r"\b(tu|toi|ton|ta|tes|te)\b|\b\w+-(toi|le|la|les)\b(?<!-la)", re.IGNORECASE)


class Results:
    def __init__(self):
        self.failures = []
        self.lines = []

    def check(self, name, ok, detail=""):
        status = "PASS" if ok else "FAIL"
        line = f"{status} {name}" + (f" - {detail}" if detail else "")
        self.lines.append(line)
        if not ok:
            self.failures.append(line)
        print(line, flush=True)


# ---- Workbench ----------------------------------------------------------------------------------

def run_workbench(results, label, gproj, addons, module, plugin, params="", keep_logs=False):
    """Runs the Workbench headless. \\return its console.log text, empty on failure"""
    cmd = (f'"{WB}" -gproj "{gproj}" -addonsDir "{",".join(str(d) for d in ADDON_DIRS)}" -addons "{addons}" '
           f'-profile {PROFILE} -wbModule={module} -plugin={plugin} -run {params}')
    before = set(glob.glob(str(LOGS / "*")))
    start = time.time()
    process = subprocess.Popen(cmd, cwd=WB_DIR)
    log_file = None

    def read_log():
        nonlocal log_file
        if not log_file:
            # the log folder of this run: one that did not exist before it started
            dirs = [d for d in glob.glob(str(LOGS / "*")) if d not in before]
            if dirs:
                log_file = Path(max(dirs, key=os.path.getmtime)) / "console.log"
        if log_file and log_file.exists():
            return log_file.read_text(encoding="utf-8", errors="replace")
        return ""

    status = "timeout"
    while time.time() - start < TIMEOUT:
        if process.poll() is not None:
            status = f"exit {process.returncode}"
            break
        if "Can't compile" in read_log():
            status = "compile error"
            break
        time.sleep(1)

    if process.poll() is None:
        process.kill()
        process.wait()

    text = read_log()
    duration = time.time() - start
    errors = [l.strip() for l in text.splitlines() if "Can't compile" in l or re.search(r"SCRIPT\s+\(E\): @\"scripts/", l)]
    results.check(f"{label}: Workbench ran", status == "exit 0" and not errors, f"{status}, {duration:.0f} s" + (f", {errors[:3]}" if errors else ""))
    if keep_logs and log_file:
        target = WORK / "results" / f"{label.replace(' ', '_').replace('/', '_')}.log"
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(log_file, target)
    return text if status == "exit 0" else ""


def findings(text):
    """\\return [(rule, severity, where)] of the [GDC_CHECK_DATA] lines, and whether the run said DONE"""
    found = []
    done = False
    for line in text.splitlines():
        if "[GDC_CHECK_DATA]" not in line:
            continue
        data = line.split("[GDC_CHECK_DATA]", 1)[1].strip()
        if data == "DONE":
            done = True
        elif "|" in data and not data.startswith("OPEN_FAILED"):
            rule, severity, where = (data.split("|", 2) + ["", ""])[:3]
            found.append((rule, severity, where))
    return found, done


def check_world(results, label, text, expected):
    found, done = findings(text)
    results.check(f"{label}: checked to the end", done)
    if not done:
        return
    counts = Counter(rule for rule, _, _ in found)
    for rule in sorted(set(counts) | set(expected), key=lambda r: [int(x) for x in r.split(".")]):
        count, severity, where = expected.get(rule, (0, None, None))
        results.check(f"{label}: rule {rule} x{count}", counts[rule] == count, f"found {counts[rule]}")
        if count and counts[rule]:
            severities = {s for r, s, _ in found if r == rule}
            results.check(f"{label}: rule {rule} severity {severity}", severities == {severity}, str(severities))
            if where:
                places = [w for r, _, w in found if r == rule]
                results.check(f"{label}: rule {rule} where", any(where in w for w in places), str(places))


# ---- static checks ------------------------------------------------------------------------------

def static_checks(results):
    kb = subprocess.run([sys.executable, "-I", str(REPO / "Tools" / "MissionCheckKB" / "check_kb.py")], capture_output=True, text=True)
    results.check("knowledge base: check_kb.py", kb.returncode == 0, kb.stdout.strip().splitlines()[-1] if kb.stdout.strip() else kb.stderr)
    doc = subprocess.run([sys.executable, "-I", str(REPO / "Tools" / "MissionCheckKB" / "rules_doc.py"), "--check"], capture_output=True, text=True)
    results.check("user guide: list of the checks up to date (rules_doc.py --check)", doc.returncode == 0, doc.stdout.strip())

    folders = [MM_TOOLS / "scripts" / "WorkbenchGame" / "GDC" / "MissionCheck", MM_TOOLS / "scripts" / "WorkbenchGame" / "GDC" / "LoadoutEditor",
               MM_TOOLS / "GDC_AR_MissionCheck_KB", REPO / "Tools" / "MissionCheckKB", HERE]
    crlf = []
    for folder in folders:
        for path in folder.rglob("*"):
            if path.is_file() and "_work" not in path.parts and path.suffix in (".c", ".md", ".py", ".layer", ".et", ".ent", ".meta", ".gproj"):
                if b"\r" in path.read_bytes():
                    crlf.append(str(path.relative_to(REPO)))
    results.check("files: LF line endings", not crlf, ", ".join(crlf[:5]))

    catalogue = (MM_TOOLS / "scripts" / "WorkbenchGame" / "GDC" / "MissionCheck" / "GDC_MMMissionCheckCatalog.c").read_text(encoding="utf-8")
    pairs = re.findall(r'rule\.Set\w+\(\s*"((?:[^"\\]|\\.)*)",\s*"((?:[^"\\]|\\.)*)"\)', catalogue)
    results.check("catalogue: texts in both languages found", len(pairs) > 40, f"{len(pairs)} pairs")
    french = [fr for _, fr in pairs] + [p.read_text(encoding="utf-8") for p in (MM_TOOLS / "GDC_AR_MissionCheck_KB").glob("*.fr.md")]
    tutoiement = sorted({m.group(0) for text in french for m in re.finditer(r"\b(tu|toi|ton|ta|tes)\b", text)})
    results.check("French texts: vouvoiement (no tu, ton, ta, tes, toi)", not tutoiement, ", ".join(tutoiement))
    same = [en for en, fr in pairs if en and en == fr and re.search(r"[a-z]{4,} [a-z]{4,}", en)]
    results.check("catalogue: French text differs from English", not same, "; ".join(same[:3]))


# ---- main ---------------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gdc-missions", action="store_true", help="also check the skeleton world of GDC Missions")
    parser.add_argument("--keep-logs", action="store_true", help="copy the Workbench logs to _work/results/")
    args = parser.parse_args()

    results = Results()
    started = time.time()
    subprocess.run([sys.executable, "-I", str(HERE / "make_fixtures.py")], check=True, capture_output=True)

    print("== static checks", flush=True)
    static_checks(results)

    gproj = TEST_ADDON / "addon.gproj"
    print("== unit tests", flush=True)
    text = run_workbench(results, "unit tests", gproj, TEST_ADDON_GUID, "ResourceManager", "GDC_MMCheckUnitTestsPlugin", keep_logs=args.keep_logs)
    tests = [l.split("[GDC_CHECK_TEST]", 1)[1].strip() for l in text.splitlines() if "[GDC_CHECK_TEST]" in l]
    failed = [t for t in tests if t.startswith("FAIL")]
    ended = any(t.startswith("END") for t in tests)
    results.check(f"unit tests: {len(tests) - 1 if ended else len(tests)} run, all pass", ended and not failed, "; ".join(failed[:10]))

    print("== engine probes", flush=True)
    for probe, markers in PROBES.items():
        text = run_workbench(results, f"engine probe {probe}", gproj, TEST_ADDON_GUID, "ResourceManager", "GDC_MMCheckEngineProbePlugin",
                             f"-probe={probe}", keep_logs=args.keep_logs)
        written = [l.split("[GDC_CHECK_PROBE]", 1)[1].strip() for l in text.splitlines() if "[GDC_CHECK_PROBE]" in l]
        for marker in markers:
            results.check(f"engine probe {probe}: {marker}", marker in written, ", ".join(written))

    # rule 2.4 is unit tested with made-up addon lists: never load an addon that may be missing, the Workbench then
    # opens a modal "Missing Addon" dialog on the user's screen
    worlds = [("T_Clean", "en", {}, ""), ("T_Faulty", "en", FAULTY, ""), ("T_Faulty", "fr", FAULTY, ""),
              ("T_NoPlayable", "en", NO_PLAYABLE, "")]
    for world, language, expected, extra in worlds:
        label = f"{world} ({language}{', + Anarchy Map Markers' if extra else ''})"
        print(f"== {label}", flush=True)
        addons = TEST_ADDON_GUID + ("," + extra if extra else "")
        text = run_workbench(results, label, gproj, addons, "WorldEditor", "GDC_MMMissionCheckPlugin",
                             f'-checkWorld="worlds/GDC_CheckTests/{world}/{world}.ent" -checkLanguage={language}', args.keep_logs)
        check_world(results, label, text, expected)
        if text and language == "fr":
            results.check(f"{label}: report in French", "VÉRIFICATION DE MISSION" in text and "RÉSULTAT" in text)
        if text and language == "en":
            results.check(f"{label}: report in English", "CHECK MISSION - " in text and "RESULT - " in text)
        if text and world == "T_NoPlayable":
            results.check(f"{label}: TILW not used, its rules skipped", re.search(r"Origins\s*: vanilla, RL \(GDC, TILW not used\)", text) is not None)

    if args.gdc_missions:
        print("== skeleton (GDC Missions)", flush=True)
        addons = "66273C703D4DFCEF,6AC540B09694CF5F,690756F509F253EB"
        text = run_workbench(results, "skeleton", REPO / "GDC_AR_Missions" / "GDC_Missions" / "addon.gproj", addons, "WorldEditor",
                             "GDC_MMMissionCheckPlugin", '-checkWorld="worlds/tanin69/_Tests/skeleton/skeleton.ent"', args.keep_logs)
        check_world(results, "skeleton", text, SKELETON)

    summary = f"{len(results.lines) - len(results.failures)} passed, {len(results.failures)} failed, {time.time() - started:.0f} s"
    print("== " + summary, flush=True)
    out = WORK / "results" / f"{datetime.now():%Y-%m-%d_%H-%M-%S}.txt"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(results.lines + ["", summary]) + "\n", encoding="utf-8")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.exit(main())

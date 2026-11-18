"""Supervision of the addons GDC depends on, around the game -> Workbench sync (sync_workbench_mods.py).

Commands:
  scope   lists the supervised addons: the GDC root projects and all their dependencies, recursively
  before  run BEFORE the sync: keeps the text files (scripts, configs, prefabs, layouts) and the file list of
          every supervised addon the sync is going to update, so that they can be compared afterwards
  after   run AFTER the sync: for every supervised addon whose version differs from the last supervision,
          compares the kept version with the new one, cross-references the changes with the GDC code and
          runs the checks (obsolete .pak, dependency versions, MissionTests addon list). Writes report.json.
  conflicts  run AFTER the MissionTests compile test (its log gives the load order): methods several addons modify
          where one does not call super, and files several addons modify (same property, or replaced whole), with
          who wins. Writes conflicts.json and adds it to report.json.
  candidate  preventive supervision of an addon not in the modset yet (downloaded folder): its dependencies, what it
          changes in the game, the conflicts it would create. Local only; writes candidate_<GUID>.json.
  order   checks server mod lists against the load order rule (config.json "load_order"); --apply rewrites the
          repo's _ServerConfigs lists by the rule first
  keep    keeps a downloaded addon folder (an old version fetched from the Workshop) as the copy to compare with
  save    records the current versions as supervised (state.json), once the report is published

Needs the pak reader of enfusion-mcp-py: run it with C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe.
The game and the Workbench may stay open for scope/after; before only reads.
"""

import argparse
import datetime
import json
import os
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sync_workbench_mods as sync  # noqa: E402

try:
    from enfusion_mcp.formats.pak import read_index, read_file
except ImportError:
    sys.exit("enfusion_mcp not found: run this script with C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe")

REPO = Path(__file__).resolve().parents[2]
SUPERVISION_DIR = Path(__file__).resolve().parent / "supervision"
STATE_FILE = SUPERVISION_DIR / "state.json"
CONFIG_FILE = SUPERVISION_DIR / "config.json"
# Kept versions and run data: outside the repo, they are large and only useful on this PC
WORK_DIR = Path(sync.MY_GAMES) / "GDC_ModSupervision"
SNAPSHOT_DIR = WORK_DIR / "snapshots"
LAST_RUN = WORK_DIR / "last_run.json"
GAME_PATH = Path(os.environ.get("ENFUSION_GAME_PATH", r"C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger"))
MISSION_TESTS_SCRIPT = REPO / "Tools" / "MissionTests" / "run_mission_test.ps1"

TEXT_EXTENSIONS = (".c", ".conf", ".et", ".layout")
GDC_TEXT_EXTENSIONS = (".c", ".conf", ".et", ".layout", ".layer", ".ent")
CLASS_RE = re.compile(r"^\s*(?:sealed\s+)?(modded\s+)?class\s+(\w+)(?:\s*(?::|extends)\s*(\w+))?", re.MULTILINE)
RESOURCE_REF_RE = re.compile(r"\{([0-9A-F]{16})\}([^\"\s]+)")
GPROJ_ID_RE = re.compile(r'^\s*ID\s+"([^"]+)"', re.MULTILINE)
GPROJ_GUID_RE = re.compile(r'^\s*GUID\s+"([0-9A-F]{16})"', re.MULTILINE)
GPROJ_TITLE_RE = re.compile(r'^\s*TITLE\s+"([^"]*)"', re.MULTILINE)
GPROJ_DEPS_RE = re.compile(r"Dependencies\s*\{([^}]*)\}", re.MULTILINE)


# --- Projects and addons ---------------------------------------------------------------------------------------

def read_gproj(path):
    text = Path(path).read_text(encoding="utf-8-sig", errors="replace")
    deps = GPROJ_DEPS_RE.search(text)

    def first(regex):
        match = regex.search(text)
        return match.group(1) if match else ""

    return {"id": first(GPROJ_ID_RE), "guid": first(GPROJ_GUID_RE), "title": first(GPROJ_TITLE_RE),
            "dependencies": re.findall(r'"([0-9A-F]{16})"', deps.group(1)) if deps else []}


def local_projects():
    """GDC projects of the repo, by GUID (the Tools test projects are not GDC addons)."""
    projects = {}
    for gproj in REPO.rglob("addon.gproj"):
        if ".git" in gproj.parts or "Tools" in gproj.relative_to(REPO).parts:
            continue
        info = read_gproj(gproj)
        if info["guid"]:
            info["folder"] = gproj.parent
            projects[info["guid"]] = info
    return projects


def load_config():
    with open(CONFIG_FILE, encoding="utf-8") as file:
        return json.load(file)


def resolve_scope(config, projects, workbench, game):
    """Supervised addons by GUID: roots and their recursive dependencies.

    Each entry: guid, name, gproj_id, version, source (local/workbench/game/missing), roots (GDC roots needing it).
    """
    scope = {}
    pending = []
    for root_id in config["roots"]:
        root = next((p for p in projects.values() if p["id"] == root_id), None)
        if not root:
            print(f"  ! root project '{root_id}' not found in the repo")
            continue
        pending.append((root["guid"], root["id"]))

    while pending:
        guid, root_id = pending.pop()
        if guid in sync.VANILLA_IDS:
            continue
        if guid in scope:
            if root_id not in scope[guid]["roots"]:
                scope[guid]["roots"].append(root_id)
            continue
        entry = {"guid": guid, "roots": [root_id], "local": guid in projects}
        wb, gm = workbench.get(guid), game.get(guid)
        if wb:
            entry.update(name=wb.name, version=wb.version, source="workbench", folder=wb.folder,
                         gproj_id=gproj_id_of(wb.path), dependencies=wb.dependencies)
        elif gm:
            entry.update(name=gm.name, version=gm.version, source="game only", folder=gm.folder,
                         gproj_id=gproj_id_of(gm.path), dependencies=gm.dependencies)
        elif guid in projects:
            p = projects[guid]
            entry.update(name=p["title"] or p["id"], version="local", source="local", folder=str(p["folder"]),
                         gproj_id=p["id"], dependencies=p["dependencies"])
        else:
            entry.update(name=guid, version="", source="missing", folder="", gproj_id="", dependencies=[])
        if guid in projects:
            # A GDC project's own dependencies come from its source, not from its published copy
            entry["dependencies"] = projects[guid]["dependencies"]
        scope[guid] = entry
        pending.extend((dep, root_id) for dep in entry["dependencies"])
    return scope


def gproj_id_of(folder):
    gproj = sync.project_file(folder)
    return read_gproj(gproj)["id"] if gproj else ""


def addon_paks(folder):
    return sorted(Path(folder).glob("*.pak"))


# --- Snapshots -------------------------------------------------------------------------------------------------

def snapshot_path(guid, version):
    return SNAPSHOT_DIR / guid / version


def extract_text_files(folder, destination):
    """Writes the text files of the addon's paks under destination/files and the full file list to index.txt."""
    destination = Path(destination)
    files_dir = destination / "files"
    index = []
    for pak in addon_paks(folder):
        for entry in read_index(pak):
            index.append(entry.path)
            if entry.path.lower().endswith(TEXT_EXTENSIONS):
                target = files_dir / entry.path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(read_file(entry))
    (destination / "index.txt").write_text("\n".join(sorted(index)), encoding="utf-8")
    return len(index)


def take_snapshot(addon):
    path = snapshot_path(addon.id, addon.version)
    if (path / "index.txt").is_file():
        return path, False
    shutil.rmtree(path, ignore_errors=True)
    extract_text_files(addon.path, path)
    prune_snapshots(addon.id, keep=addon.version)
    return path, True


def prune_snapshots(guid, keep):
    """Keeps only the given version of an addon (the one about to be replaced)."""
    base = SNAPSHOT_DIR / guid
    for child in base.iterdir() if base.is_dir() else []:
        if child.name != keep:
            shutil.rmtree(child, ignore_errors=True)


# --- Comparison ------------------------------------------------------------------------------------------------

def read_tree(root):
    root = Path(root)
    return {p.relative_to(root).as_posix(): p.read_bytes() for p in root.rglob("*") if p.is_file()} \
        if root.is_dir() else {}


def classes_in(text):
    """(name, modded, parent) of every class declared in a script."""
    return [(m.group(2), bool(m.group(1)), m.group(3) or "") for m in CLASS_RE.finditer(text)]


def compare(old_dir, new_dir):
    old_files, new_files = read_tree(Path(old_dir) / "files"), read_tree(Path(new_dir) / "files")
    old_index = set((Path(old_dir) / "index.txt").read_text(encoding="utf-8").splitlines())
    new_index = set((Path(new_dir) / "index.txt").read_text(encoding="utf-8").splitlines())
    added = sorted(set(new_files) - set(old_files))
    removed = sorted(set(old_files) - set(new_files))
    modified = sorted(p for p in set(old_files) & set(new_files) if old_files[p] != new_files[p])

    changed_classes = {}
    for path in added + removed + modified:
        if not path.endswith(".c"):
            continue
        for source in (old_files.get(path), new_files.get(path)):
            if source:
                for name, modded, parent in classes_in(source.decode("utf-8", errors="replace")):
                    changed_classes.setdefault(name, {"modded": modded, "files": set()})["files"].add(path)

    def by_folder(paths):
        counts = {}
        for p in paths:
            key = "/".join(p.split("/")[:2])
            counts[key] = counts.get(key, 0) + 1
        return dict(sorted(counts.items(), key=lambda kv: -kv[1]))

    return {
        "text_files": {"added": len(added), "removed": len(removed), "modified": len(modified)},
        "by_folder": {"added": by_folder(added), "removed": by_folder(removed), "modified": by_folder(modified)},
        "scripts": {"added": [p for p in added if p.endswith(".c")], "removed": [p for p in removed if p.endswith(".c")],
                    "modified": [p for p in modified if p.endswith(".c")]},
        "other_files": {"added": len(new_index - old_index), "removed": sorted(old_index - new_index)},
        "changed_classes": {k: {"modded": v["modded"], "files": sorted(v["files"])} for k, v in changed_classes.items()},
        "_changed_paths": added + modified,
        "_new_index": new_index,
        "_old_index": old_index,
    }


def base_game_paths():
    paths = set()
    for pak in (GAME_PATH / "addons").rglob("*.pak"):
        try:
            paths.update(e.path.lower() for e in read_index(pak))
        except Exception:  # noqa: BLE001 - an unreadable pak only reduces the check
            pass
    return paths


# --- Cross-reference with the GDC code -------------------------------------------------------------------------

def gdc_code_index(projects):
    """Classes the GDC code modifies or inherits from, and the resource references of its files."""
    uses = {}      # class name -> list of (file, "modded"/"inherits")
    refs = {}      # resource path (lowercase) -> list of files
    for project in projects.values():
        for path in Path(project["folder"]).rglob("*"):
            if not path.is_file() or not path.name.lower().endswith(GDC_TEXT_EXTENSIONS):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            rel = path.relative_to(REPO).as_posix()
            if path.suffix == ".c":
                for name, modded, parent in classes_in(text):
                    if modded:
                        uses.setdefault(name, []).append((rel, "modded"))
                    elif parent:
                        uses.setdefault(parent, []).append((rel, "inherits"))
            for _, resource in RESOURCE_REF_RE.findall(text):
                refs.setdefault(resource.lower(), set()).add(rel)
    return uses, refs


def exposure(new_dir, uses, refs):
    """What GDC relies on in an addon, from its current files only: classes GDC modifies or inherits from, and
    resources GDC references. Tells whether an old version is worth fetching when no copy was kept."""
    new_dir = Path(new_dir)
    classes = {}
    for path in (new_dir / "files").rglob("*.c"):
        for name, _, _ in classes_in(path.read_text(encoding="utf-8", errors="replace")):
            classes.setdefault(name, path.relative_to(new_dir / "files").as_posix())
    index = {p.lower() for p in (new_dir / "index.txt").read_text(encoding="utf-8").splitlines()}
    used_classes = [{"class": name, "addon_file": classes[name], "gdc_files": sorted({f for f, _ in uses[name]})}
                    for name in sorted(uses) if name in classes]
    used_resources = sorted(resource for resource in refs if resource in index)
    return {"classes": used_classes, "resources": len(used_resources), "resource_examples": used_resources[:10]}


def cross_reference(diff, uses, refs):
    touched = []
    for name, info in diff["changed_classes"].items():
        for gdc_file, how in uses.get(name, []):
            touched.append({"class": name, "gdc_file": gdc_file, "how": how, "addon_files": info["files"]})
    new_lower = {p.lower() for p in diff["_new_index"]}
    old_lower = {p.lower() for p in diff["_old_index"]}
    broken = [{"resource": resource, "gdc_files": sorted(files)}
              for resource, files in refs.items() if resource in old_lower and resource not in new_lower]
    return touched, broken


# --- Checks ----------------------------------------------------------------------------------------------------

def mission_tests_addons():
    if not MISSION_TESTS_SCRIPT.is_file():
        return None
    match = re.search(r"^\$addons\s*=\s*'([^']*)'", MISSION_TESTS_SCRIPT.read_text(encoding="utf-8"), re.MULTILINE)
    return set(match.group(1).split(",")) if match else None


def run_checks(scope, workbench, config):
    issues = []
    for entry in scope.values():
        if entry["source"] == "missing":
            issues.append({"addon": entry["name"], "check": "missing",
                           "detail": f"dependency of {', '.join(entry['roots'])} found neither in the Workbench nor in the game"})
            continue
        if entry["source"] == "game only" and not entry["local"]:
            issues.append({"addon": entry["name"], "check": "missing from Workbench",
                           "detail": "present in the game only: run the sync"})
        wb = workbench.get(entry["guid"])
        if not wb:
            continue
        stale = wb.stale_paks()
        if stale:
            issues.append({"addon": wb.name, "check": "obsolete pak", "detail": ", ".join(stale)})
        if not wb.is_complete():
            issues.append({"addon": wb.name, "check": "incomplete", "detail": "missing manifest or corrupted flag"})
        # Minimum versions required by this addon's dependencies
        data = json.loads((Path(wb.path) / "ServerData.json").read_text(encoding="utf-8-sig"))
        for dep in (data.get("revision") or {}).get("dependencies") or []:
            installed = workbench.get(dep.get("assetId", ""))
            wanted = dep.get("version", "")
            if installed and wanted and (sync.parse_version(installed.version) or ()) < (sync.parse_version(wanted) or ()):
                issues.append({"addon": wb.name, "check": "dependency version",
                               "detail": f"needs {installed.name} {wanted}, Workbench has {installed.version}"})

    tests = mission_tests_addons()
    if tests is not None:
        for entry in scope.values():
            if entry.get("gproj_id") and entry["gproj_id"] not in tests \
                    and entry["gproj_id"] not in config.get("not_in_mission_tests", []):
                issues.append({"addon": entry["name"], "check": "MissionTests addon list",
                               "detail": f"'{entry['gproj_id']}' missing from $addons in run_mission_test.ps1"})
    return issues


# --- Commands --------------------------------------------------------------------------------------------------

def load_state():
    if STATE_FILE.is_file():
        with open(STATE_FILE, encoding="utf-8") as file:
            return json.load(file)
    return {"artifact_url": "", "supervised_at": "", "versions": {}}


def context():
    projects = local_projects()
    workbench = sync.read_addons(sync.WORKBENCH_ADDONS)
    game = sync.read_addons(sync.GAME_ADDONS)
    scope = resolve_scope(load_config(), projects, workbench, game)
    return projects, workbench, game, scope


def cmd_scope(_args):
    _, _, _, scope = context()
    for entry in sorted(scope.values(), key=lambda e: e["name"].lower()):
        print(f"  {entry['name']:<45} {entry['version']:<14} {entry['source']:<10} needed by {', '.join(sorted(entry['roots']))}")
    print(f"\n{len(scope)} supervised addon(s).")


def cmd_before(_args):
    _, workbench, game, scope = context()
    updates, additions, notes = sync.build_plan(game, workbench)
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    planned = []
    for source, wb in updates:
        if source.id not in scope:
            continue
        if scope[source.id]["local"]:
            print(f"  update  {wb.name} {wb.version} -> {source.version} (GDC addon, not kept)")
            planned.append({"guid": wb.id, "name": wb.name, "from": wb.version, "to": source.version})
            continue
        path, created = take_snapshot(wb)
        print(f"  kept    {wb.name} {wb.version} -> {path}" + ("" if created else " (already kept)"))
        planned.append({"guid": wb.id, "name": wb.name, "from": wb.version, "to": source.version})
    for source, _ in additions:
        print(f"  new     {source.name} {source.version} (added by the sync)")
        planned.append({"guid": source.id, "name": source.name, "from": "", "to": source.version})
    for note in notes:
        print(f"  note    {note}")
    LAST_RUN.write_text(json.dumps({"date": datetime.datetime.now().isoformat(timespec="seconds"),
                                    "planned": planned, "notes": notes}, indent=1), encoding="utf-8")
    out_of_scope = [s.name for s, _ in updates if s.id not in scope]
    print(f"\n{len(planned)} supervised addon(s) will change, {len(out_of_scope)} other(s): {', '.join(out_of_scope) or '-'}")


def cmd_after(args):
    projects, workbench, game, scope = context()
    state = load_state()
    previous = state.get("versions", {})
    uses, refs = gdc_code_index(projects)
    game_paths = None
    changed = []
    for entry in sorted(scope.values(), key=lambda e: e["name"].lower()):
        wb = workbench.get(entry["guid"])
        old_version = previous.get(entry["guid"], {}).get("version", "")
        if not wb or not old_version or old_version == wb.version:
            continue
        item = {"guid": wb.id, "name": wb.name, "gproj_id": entry["gproj_id"], "from": old_version,
                "to": wb.version, "gdc": entry["local"], "roots": sorted(entry["roots"])}
        # Changelog of the installed version as published, or a link to it (ACE: GitHub release)
        server_data = json.loads((Path(wb.path) / "ServerData.json").read_text(encoding="utf-8-sig"))
        item["installed_changelog"] = ((server_data.get("revision") or {}).get("changelog") or "").strip()
        old_dir = snapshot_path(wb.id, old_version)
        if entry["local"]:
            # Our own addons: their history is in git and CHANGELOG.md, and their code would match itself
            item["diff"] = None
            item["note"] = "GDC addon: see CHANGELOG.md and git log"
            changed.append(item)
            continue
        new_dir = WORK_DIR / "current" / wb.id
        shutil.rmtree(new_dir, ignore_errors=True)
        extract_text_files(wb.path, new_dir)
        item["gdc_exposure"] = exposure(new_dir, uses, refs)
        if (old_dir / "index.txt").is_file():
            diff = compare(old_dir, new_dir)
            touched, broken = cross_reference(diff, uses, refs)
            if game_paths is None:
                game_paths = base_game_paths()
            # Changed files that replace or modify a base game file (prefabs, configs, layouts)
            item["base_game_overrides_changed"] = sorted(p for p in diff["_changed_paths"] if p.lower() in game_paths)
            item.update(diff={k: v for k, v in diff.items() if not k.startswith("_")}, gdc_touched=touched,
                        gdc_broken_refs=broken, compared_files=str(new_dir), kept_files=str(old_dir))
        else:
            item["diff"] = None
            item["note"] = f"no kept copy of {old_version}: no file comparison possible"
        changed.append(item)

    new_entries = [e["name"] for e in scope.values() if e["guid"] not in previous and e["source"] != "missing"]
    report = {"date": datetime.datetime.now().isoformat(timespec="seconds"),
              "previous_supervision": state.get("supervised_at", ""),
              "scope": [{k: e[k] for k in ("guid", "name", "gproj_id", "version", "source", "roots", "local")}
                        for e in sorted(scope.values(), key=lambda e: e["name"].lower())],
              "changed": changed, "new_in_scope": sorted(new_entries) if previous else [],
              "checks": run_checks(scope, workbench, load_config()) + order_checks(projects, workbench, game)}
    if LAST_RUN.is_file():
        report["sync_run"] = json.loads(LAST_RUN.read_text(encoding="utf-8"))
    out = Path(args.output) if args.output else WORK_DIR / "report.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=1, default=sorted), encoding="utf-8")

    for item in changed:
        diff = item["diff"]
        summary = (f"{diff['text_files']['modified']} modified, {diff['text_files']['added']} added, "
                   f"{diff['text_files']['removed']} removed text files; {len(item['gdc_touched'])} GDC class link(s), "
                   f"{len(item['gdc_broken_refs'])} broken GDC reference(s)") if diff else item["note"]
        if "gdc_exposure" in item:
            exp = item["gdc_exposure"]
            summary += f"; GDC relies on {len(exp['classes'])} class(es), {exp['resources']} resource(s)"
        print(f"  changed {item['name']}: {item['from']} -> {item['to']}  ({summary})")
    for issue in report["checks"]:
        print(f"  check   {issue['addon']}: {issue['check']} - {issue['detail']}")
    if not previous:
        print("  note    first supervision: no previous versions recorded, nothing compared")
    print(f"\n{len(scope)} supervised, {len(changed)} changed, {len(report['checks'])} check issue(s). Report: {out}")


def conflict_key(item):
    if "method" in item:
        return f"method {item['class']}.{item['method']}"
    return f"file {item['path']} {'+'.join(item['addons'])}"


def scope_sources(projects, workbench, scope):
    import mod_conflicts

    sources = []
    for entry in scope.values():
        if entry["local"] and entry["guid"] in projects:
            sources.append(mod_conflicts.Source(entry["guid"], entry["name"], projects[entry["guid"]]["folder"], False))
        elif entry["guid"] in workbench:
            sources.append(mod_conflicts.Source(entry["guid"], entry["name"], workbench[entry["guid"]].path, True))
    return sources


SERVER_CONFIG = REPO / "GDC_AR_Missions" / "_ServerConfigs" / "addons.json"


def server_order(config_file, projects, workbench, game, extra=()):
    """Load order of a server config's mod list ({"mods": [{"modId": ...}]}), as the game resolves it, with extra
    GUIDs (a candidate) appended. Returns (GUIDs, {GUID: top-level modId bringing it}, label)."""
    import mod_conflicts

    # A full server config ({"game": {"mods": ...}}) or a bare mod list ({"mods": ...}, the repo's addons.json)
    data = json.loads(Path(config_file).read_text(encoding="utf-8-sig"))
    mods = data["mods"] if "mods" in data else data.get("game", {}).get("mods", [])

    def dependencies(guid):
        if guid in projects:
            return projects[guid]["dependencies"]
        addon = workbench.get(guid) or game.get(guid)
        return addon.dependencies if addon else []

    order, brought_by = mod_conflicts.resolve_order([m["modId"] for m in mods] + list(extra), dependencies,
                                                    ignored=sync.VANILLA_IDS)
    return order, brought_by, str(Path(config_file).relative_to(REPO)) if REPO in Path(config_file).parents else str(config_file)


# --- Load order rule (config.json "load_order") ---------------------------------------------------------------

class OrderRule:
    """The load order rule of the server mod lists and the checks that go with it."""

    def __init__(self, projects, workbench, game):
        self.rule = load_config()["load_order"]
        self.projects, self.workbench, self.game = projects, workbench, game
        self.sequence = [e["modId"] for e in self.rule["sequence"]]
        self.core = self.sequence[0]
        mission_root = REPO / "GDC_AR_Missions"
        self.missions = {g for g, p in projects.items() if mission_root in Path(p["folder"]).parents}

    def dependencies(self, guid):
        if guid in self.projects:
            return self.projects[guid]["dependencies"]
        addon = self.workbench.get(guid) or self.game.get(guid)
        return addon.dependencies if addon else []

    def closure(self, guids):
        import mod_conflicts

        return set(mod_conflicts.resolve_order(list(guids), self.dependencies, ignored=sync.VANILLA_IDS)[0])

    def name(self, guid):
        if guid in self.projects:
            return self.projects[guid]["title"] or self.projects[guid]["id"]
        addon = self.workbench.get(guid) or self.game.get(guid)
        return addon.name if addon else guid

    def reorder(self, entries, full_list):
        """Entries ({modId, name}) rearranged by the rule. A mission list keeps top-level mods only: entries that are
        dependencies of other entries are dropped. The global list (full_list) holds every mod, dependencies included,
        in the resolved load order."""
        import mod_conflicts

        ids = [e["modId"] for e in entries]
        names = {e["modId"]: e.get("name", "") for e in entries}
        gdc = set(self.sequence) | self.missions
        brought_by_gdc = self.closure([g for g in ids if g in gdc]) - gdc
        others = [g for g in ids if g not in gdc and g not in brought_by_gdc]
        if not full_list:
            # Drop entries another entry already brings (TilW comes with GDC AR Lib, for instance)
            others = [g for g in others if not any(g in self.closure([o]) - {o} for o in others if o != g)]
        top = others + [g for g in self.sequence if g in ids] + [g for g in ids if g in self.missions]
        if full_list:
            top = mod_conflicts.resolve_order(top, self.dependencies, ignored=sync.VANILLA_IDS)[0]
        return [{"modId": g, "name": names.get(g) or self.name(g)} for g in top]

    def check_list(self, entries):
        """Problems of a mod list against the rule, as sentences."""
        import mod_conflicts

        ids = [e["modId"] for e in entries]
        problems = []
        order, _ = mod_conflicts.resolve_order(ids, self.dependencies, ignored=sync.VANILLA_IDS)
        present = [g for g in self.sequence if g in order]
        if [g for g in order if g in present] != present:
            problems.append("GDC sequence out of order: " + " > ".join(self.name(g) for g in order if g in present))
        ace = self.rule["ace"]
        if ace in order:
            rank = {g: i for i, g in enumerate(order)}
            last_ace = max(rank[g] for g in self.closure([ace]))
            for g, label in self.rule["after_ace"].items():
                if g in rank and rank[g] < last_ace:
                    culprit = next((self.name(e) for e in ids if g in self.closure([e])), "?")
                    problems.append(f"{label} loads before ACE (brought by {culprit})")
        missions = [g for g in ids if g in self.missions]
        if missions and ids[-len(missions):] != missions:
            problems.append("the mission addon is not last")
        return problems

    def check_core_dependencies(self):
        deps = self.projects[self.core]["dependencies"] if self.core in self.projects else []
        problems = []
        for first, second in self.rule["core_dependency_order"]:
            if first in deps and second in deps and deps.index(first) > deps.index(second):
                problems.append(f"GDC Core addon.gproj lists {self.name(second)} before {self.name(first)}")
        return problems


def order_checks(projects, workbench, game):
    """Load order rule problems, as check issues: GDC Core's dependency order and every repo server list."""
    rule = OrderRule(projects, workbench, game)
    issues = [{"addon": "GDC Core", "check": "load order rule", "detail": p} for p in rule.check_core_dependencies()]
    for path in server_lists():
        entries = json.loads(path.read_text(encoding="utf-8-sig"))["mods"]
        issues += [{"addon": str(path.relative_to(REPO)), "check": "load order rule", "detail": p}
                   for p in rule.check_list(entries)]
    return issues


def server_lists():
    root = REPO / "GDC_AR_Missions" / "_ServerConfigs"
    # Mission lists mirror the world path, at any depth (<author>/<mission>/, <author>/_En cours/<mission>/...)
    return [root / "addons.json"] + sorted(p for p in root.rglob("addons.json") if p.parent != root)


def cmd_order(args):
    projects, workbench, game, _ = context()
    rule = OrderRule(projects, workbench, game)
    files = [Path(f) for f in args.files] if args.files else server_lists()
    issues = 0
    for problem in rule.check_core_dependencies():
        print(f"  ! {problem}")
        issues += 1
    for path in files:
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        entries = data["mods"] if "mods" in data else data.get("game", {}).get("mods", [])
        label = path.relative_to(REPO) if REPO in path.parents else path
        full = path == SERVER_CONFIG
        if args.apply:
            if "mods" not in data:
                print(f"  skip {label}: only the repo's mod lists are rewritten")
                continue
            new = rule.reorder(entries, full)
            if [e["modId"] for e in new] != [e["modId"] for e in entries]:
                added = [e["name"] for e in new if e["modId"] not in {x["modId"] for x in entries}]
                dropped = [e["name"] for e in entries if e["modId"] not in {x["modId"] for x in new}]
                data["mods"] = new
                path.write_text(json.dumps(data, indent=4, ensure_ascii=False) + "\n", encoding="utf-8")
                print(f"  rewritten {label}" + (f" (added: {', '.join(added)})" if added else "")
                      + (f" (dropped: {', '.join(dropped)})" if dropped else ""))
            entries = new
        for problem in rule.check_list(entries):
            print(f"  ! {label}: {problem}")
            issues += 1
    print(f"\n{len(files)} list(s) checked, {issues} problem(s).")
    return 1 if issues else 0


def mission_tests_order():
    import mod_conflicts

    return mod_conflicts.load_order(Path(sync.MY_GAMES) / "GDCAutoServer" / "logs")


def cmd_candidate(args):
    """Preventive supervision of an addon that is not in the modset yet: its dependencies, what it changes in the
    game, and the conflicts it would create with the modset (GDC included). Local use only, nothing is published."""
    import mod_conflicts

    projects, workbench, game, scope = context()
    folder = Path(args.folder)
    if not (folder / "ServerData.json").is_file():
        print(f"{folder}: no ServerData.json, not a downloaded addon folder")
        return 1
    candidate = sync.Addon(folder.name, str(folder), json.loads((folder / "ServerData.json").read_text(encoding="utf-8-sig")))
    if candidate.id in scope:
        print(f"{candidate.name} is already in the modset")
        return 1
    # Other downloaded addons next to the candidate (its dependencies, fetched with it) count as available
    nearby = sync.read_addons(str(folder.parent))

    deps, pending, seen = {"in_modset": [], "new": [], "missing": []}, list(candidate.dependencies), set()
    new_addons = {candidate.id: candidate}
    while pending:
        guid = pending.pop()
        if guid in seen or guid in sync.VANILLA_IDS:
            continue
        seen.add(guid)
        if guid in scope:
            deps["in_modset"].append(scope[guid]["name"])
            continue
        found = nearby.get(guid) or workbench.get(guid) or game.get(guid)
        if not found:
            deps["missing"].append(guid)
            continue
        deps["new"].append(f"{found.name} {found.version}")
        new_addons[guid] = found
        pending.extend(found.dependencies)

    # What the candidate and its new dependencies change in the game
    game_paths = base_game_paths()
    footprint = []
    for addon in new_addons.values():
        source = mod_conflicts.Source(addon.id, addon.name, addon.path, True)
        modded = sorted({cls for cls, _, _, _ in mod_conflicts.method_overrides(source)})
        scripts = [p for p in source.entries() if p.endswith(".c")]
        replaced = sorted(path for lower, (path, _) in source.entries().items()
                          if lower in game_paths and not lower.endswith(".meta"))
        footprint.append({"name": addon.name, "version": addon.version, "scripts": len(scripts),
                          "modded_classes": modded, "base_game_files": replaced})

    sources = scope_sources(projects, workbench, scope)
    sources += [mod_conflicts.Source(a.id, a.name, a.path, True) for a in new_addons.values()]
    # As if the candidate were added at the end of the server's mod list
    order, _, log_name = server_order(SERVER_CONFIG, projects, workbench, game, extra=[candidate.id])
    result = mod_conflicts.find_conflicts(sources, order, load_config().get("families", {}))
    newcomers = {a.name for a in new_addons.values()}
    conflicts = {"methods": [m for m in result["methods"] if newcomers & {a["name"] for a in m["addons"]}],
                 "files": [f for f in result["files"] if newcomers & set(f["addons"])]}
    order_known = candidate.id in order

    report = {"candidate": {"guid": candidate.id, "name": candidate.name, "version": candidate.version,
                            "gproj_id": gproj_id_of(candidate.path), "folder": str(folder)},
              "dependencies": deps, "footprint": footprint, "conflicts": conflicts,
              "load_order_known": order_known, "load_order_log": log_name}
    out = WORK_DIR / f"candidate_{candidate.id}.json"
    out.write_text(json.dumps(report, indent=1, ensure_ascii=False), encoding="utf-8")

    print(f"Candidate: {candidate.name} {candidate.version} ({candidate.id}, project ID '{report['candidate']['gproj_id']}')")
    print(f"  dependencies in the modset: {', '.join(sorted(deps['in_modset'])) or '-'}")
    print(f"  new dependencies: {', '.join(sorted(deps['new'])) or '-'}")
    if deps["missing"]:
        print(f"  MISSING dependencies (download them next to the candidate): {', '.join(deps['missing'])}")
    for item in footprint:
        print(f"  footprint {item['name']}: {item['scripts']} scripts, {len(item['modded_classes'])} modded classes, "
              f"{len(item['base_game_files'])} base game files replaced or modified")
    for item in conflicts["methods"]:
        names = ", ".join(a["name"] + ("" if a["calls_super"] else " [no super]") for a in item["addons"])
        print(f"  method  {item['class']}.{item['method']}: {names}"
              + (f" -> never run: {', '.join(item['never_run'])}" if item["never_run"] else ""))
    for item in conflicts["files"]:
        detail = (f"{item['property_count']} propert(y/ies)" if item["kind"] == "same property"
                  else f"{len(item['files'])} files" if "files" in item else "whole file")
        print(f"  file    {item['path']} ({item['kind']}, {detail}): {', '.join(item['addons'])}"
              + (f" -> {item['wins']} wins" if item["wins"] else ""))
    for item in conflicts["methods"]:
        advice = item["advice"]
        if advice["kind"] == "load first":
            print(f"  to keep all in {item['method']}: load {advice['addon']} before {', '.join(advice['before'])}")
        else:
            print(f"  {item['method']}: no order keeps all ({', '.join(advice['addons'])} skip super)")
    ids = ",".join(a.id for a in new_addons.values())
    print(f"\n  Load order: candidate appended to {log_name}. Compile test with it (Workbench closed):\n"
          f"  powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 -Compile "
          f"-ExtraAddonsDir \"{folder.parent}\" -ExtraAddons \"{ids}\"")
    print(f"\n{len(conflicts['methods'])} method conflict(s), {len(conflicts['files'])} file conflict(s). Details: {out}")
    return 0


def cmd_conflicts(args):
    """Conflicts between supervised addons (see mod_conflicts.py). Run after the MissionTests compile test,
    whose server log gives the load order."""
    import mod_conflicts

    projects, workbench, game, scope = context()
    sources = scope_sources(projects, workbench, scope)
    config_file = Path(args.server_config) if args.server_config else SERVER_CONFIG
    order, brought_by, log_name = server_order(config_file, projects, workbench, game)
    # Every addon the server loads counts, not only GDC's dependencies (server-side extras such as spectator or
    # map tools are loaded in every session too)
    known = {s.guid for s in sources}
    for guid in order:
        addon = workbench.get(guid) or game.get(guid)
        if guid not in known and addon:
            sources.append(mod_conflicts.Source(guid, addon.name, addon.path, True))
            known.add(guid)
    result_extra = sorted(s.name for s in sources if s.guid not in scope)
    rule = OrderRule(projects, workbench, game)
    result = mod_conflicts.find_conflicts(sources, order, load_config().get("families", {}),
                                          depends_on=lambda a, b: b in rule.closure([a]) - {a})
    result.update(load_order_log=log_name, load_order_count=len(order), outside_scope=result_extra)
    names = {s.guid: s.name for s in sources}
    name_to_guid = {v: k for k, v in names.items()}
    for item in result["methods"]:
        # Which entry of the mod list brings each addon: that is the line to move in the list
        item["brought_by"] = {a["name"]: names.get(brought_by.get(name_to_guid.get(a["name"], ""), ""), "")
                              for a in item["addons"]}
    # Conflicts not seen at the last supervision
    known = set(load_state().get("known_conflicts", []))
    for item in result["methods"] + result["files"]:
        item["new"] = conflict_key(item) not in known

    out = WORK_DIR / "conflicts.json"
    out.write_text(json.dumps(result, indent=1, ensure_ascii=False), encoding="utf-8")
    report_file = WORK_DIR / "report.json"
    if report_file.is_file():
        report = json.loads(report_file.read_text(encoding="utf-8"))
        report["conflicts"] = result
        report_file.write_text(json.dumps(report, indent=1, default=sorted), encoding="utf-8")

    print(f"Load order: {len(order)} addons resolved from {log_name}")
    if result_extra:
        print(f"Also loaded by this server list (outside the supervised scope): {', '.join(result_extra)}")
    for item in result["methods"]:
        names = ", ".join(a["name"] + ("" if a["calls_super"] else " [no super]") for a in item["addons"])
        verdict = (f"never run: {', '.join(item['never_run'])}" if item["never_run"] else "no override lost") \
            if item["order_known"] else "load order unknown"
        print(f"  method  {'NEW ' if item['new'] else ''}{item['class']}.{item['method']}: {names} -> {verdict}")
        advice = item["advice"]
        if advice["kind"] == "load first":
            state = {True: "already the case", False: "NOT the case", None: "order unknown"}[advice["satisfied"]]
            print(f"          to keep all: load {advice['addon']} before {', '.join(advice['before'])} ({state})")
        elif advice["kind"] == "replaces its dependency":
            print(f"          {advice['addon']} depends on {', '.join(advice['dependency'])} and replaces it on purpose: "
                  f"no order changes it, only {advice['addon']} itself")
        else:
            print(f"          no order keeps all ({', '.join(advice['addons'])} skip super): choose one, or a compatibility patch")
    for item in result["files"]:
        detail = (f"{item['property_count']} propert(y/ies)" if item["kind"] == "same property"
                  else f"{len(item['files'])} files" if "files" in item else "whole file")
        print(f"  file    {'NEW ' if item['new'] else ''}{item['path']} ({item['kind']}, {detail}): {', '.join(item['addons'])}"
              + (f" -> {item['wins']} wins" if item["wins"] else ""))
    print(f"\n{len(result['methods'])} method conflict(s), {len(result['files'])} file conflict(s). Details: {out}")


def cmd_keep(args):
    """Keeps an addon folder downloaded elsewhere (an old version fetched from the Workshop) as its snapshot."""
    folder = Path(args.folder)
    data = json.loads((folder / "ServerData.json").read_text(encoding="utf-8-sig"))
    addon = sync.Addon(folder.name, str(folder), data)
    if not addon.is_complete():
        print(f"{addon.name} {addon.version}: incomplete download (missing manifest), not kept")
        return 1
    path = snapshot_path(addon.id, addon.version)
    shutil.rmtree(path, ignore_errors=True)
    extract_text_files(addon.path, path)
    print(f"  kept    {addon.name} {addon.version} -> {path}")
    return 0


def cmd_save(args):
    _, _, _, scope = context()
    state = load_state()
    state["supervised_at"] = datetime.date.today().isoformat()
    state["versions"] = {e["guid"]: {"name": e["name"], "version": e["version"]}
                         for e in scope.values() if e["source"] != "missing"}
    if args.artifact_url:
        state["artifact_url"] = args.artifact_url
    conflicts_file = WORK_DIR / "conflicts.json"
    if conflicts_file.is_file():
        conflicts = json.loads(conflicts_file.read_text(encoding="utf-8"))
        state["known_conflicts"] = sorted(conflict_key(i) for i in conflicts["methods"] + conflicts["files"])
    STATE_FILE.write_text(json.dumps(state, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"State saved: {len(state['versions'])} versions, {STATE_FILE}")


def main():
    # Addon names and paths are not all ASCII; the Windows console would otherwise stop the script
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("scope").set_defaults(func=cmd_scope)
    sub.add_parser("before").set_defaults(func=cmd_before)
    after = sub.add_parser("after")
    after.add_argument("--output", help="report.json path (default: in the work folder)")
    after.set_defaults(func=cmd_after)
    conflicts = sub.add_parser("conflicts")
    conflicts.add_argument("--server-config", help="server mod list giving the load order (default: the global "
                           "GDC_AR_Missions/_ServerConfigs/addons.json; a mission's addons.json for that mission)")
    conflicts.set_defaults(func=cmd_conflicts)
    order = sub.add_parser("order")
    order.add_argument("files", nargs="*", help="server mod lists or server configs (default: the repo's _ServerConfigs lists)")
    order.add_argument("--apply", action="store_true", help="rewrite the repo's lists by the rule before checking them")
    order.set_defaults(func=cmd_order)
    candidate = sub.add_parser("candidate")
    candidate.add_argument("folder", help="downloaded folder of the candidate addon (its dependencies may sit next to it)")
    candidate.set_defaults(func=cmd_candidate)
    keep = sub.add_parser("keep")
    keep.add_argument("folder", help="downloaded addon folder (with ServerData.json and its .pak)")
    keep.set_defaults(func=cmd_keep)
    save = sub.add_parser("save")
    save.add_argument("--artifact-url", help="URL of the published report page")
    save.set_defaults(func=cmd_save)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

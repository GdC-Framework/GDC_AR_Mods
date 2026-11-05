"""Brings the Workbench Workshop addons up to date from the game's ones.

The game launcher updates its addons (Documents/My Games/ArmaReforger/addons), the Workbench keeps its own copies
(Documents/My Games/ArmaReforgerWorkbench/addons) that are only updated by hand. Rules:
  - the game is the reference: an addon present on both sides whose game version is higher replaces the whole
    Workbench folder (no obsolete .pak left behind);
  - a dependency missing from the Workbench (new dependency of an updated addon, for instance) is copied too;
  - nothing else is copied or deleted: addons only on the game side and local projects are left alone.

Usage: python sync_workbench_mods.py [--dry-run] [--yes]
  --dry-run  shows the plan and stops
  --yes      applies without asking
The game and the Workbench must be closed.
"""

import json
import os
import re
import shutil
import subprocess
import sys

MY_GAMES = os.path.join(os.path.expanduser("~"), "Documents", "My Games")
GAME_ADDONS = os.path.join(MY_GAMES, "ArmaReforger", "addons")
WORKBENCH_ADDONS = os.path.join(MY_GAMES, "ArmaReforgerWorkbench", "addons")
# Work folders inside the Workbench addons folder (same drive: moves are instant)
TEMP_DIR = os.path.join(WORKBENCH_ADDONS, ".sync_new")
OLD_DIR = os.path.join(WORKBENCH_ADDONS, ".sync_old")

# Processes that read or write the addons folders
BLOCKING_PROCESSES = ["ArmaReforgerSteam.exe", "ArmaReforgerSteamDiag.exe", "ArmaReforgerWorkbenchSteam.exe",
                      "ArmaReforgerWorkbenchSteamDiag.exe"]

VANILLA_IDS = {"58D0FB3206B6F859", "5614BBCCBB55ED1C"}
FOLDER_RE = re.compile(r"^.+_([0-9A-F]{16})$")


class Addon:
    def __init__(self, folder, path, data):
        self.folder = folder
        self.path = path
        self.id = data.get("id", "")
        self.name = data.get("name", folder)
        revision = data.get("revision") or {}
        self.version = revision.get("version", "")
        self.corrupted = bool(revision.get("corrupted"))
        self.dependencies = [d.get("assetId", "") for d in revision.get("dependencies") or []]

    def is_complete(self):
        """True when the folder holds the files of its declared version (not a download in progress)."""
        if self.corrupted or not self.version:
            return False
        paks = [f for f in os.listdir(self.path) if f.endswith(".pak")]
        return bool(paks) and all(
            os.path.isfile(os.path.join(self.path, f"{pak}_{self.version}_manifest.json")) for pak in paks)

    def stale_paks(self):
        """.pak files without a manifest for the current version: left over by an older version."""
        return [f for f in os.listdir(self.path) if f.endswith(".pak")
                and not os.path.isfile(os.path.join(self.path, f"{f}_{self.version}_manifest.json"))]

    def size(self):
        return sum(os.path.getsize(os.path.join(root, f)) for root, _, files in os.walk(self.path) for f in files)


def read_addons(directory):
    """Workshop addons of a folder, by ID. Local projects (no ServerData.json) are ignored."""
    addons = {}
    for folder in sorted(os.listdir(directory)):
        path = os.path.join(directory, folder)
        server_data = os.path.join(path, "ServerData.json")
        if not FOLDER_RE.match(folder) or not os.path.isfile(server_data):
            continue
        try:
            with open(server_data, encoding="utf-8-sig") as file:
                addon = Addon(folder, path, json.load(file))
        except (OSError, ValueError) as error:
            print(f"  ! {folder}: unreadable ServerData.json ({error})")
            continue
        if addon.id:
            addons[addon.id] = addon
    return addons


def parse_version(version):
    """Version as a tuple of numbers, None when it is not purely numeric."""
    try:
        return tuple(int(part) for part in version.split("."))
    except ValueError:
        return None


def running_blockers():
    output = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True).stdout.lower()
    return [name for name in BLOCKING_PROCESSES if f'"{name.lower()}"' in output]


def gigabytes(size):
    return f"{size / 1024 ** 3:.2f} GB"


def build_plan(game, workbench):
    """Returns (updates, additions, notes): lists of (game addon, workbench addon or None) and messages."""
    updates, notes = [], []
    for addon_id, wb in workbench.items():
        source = game.get(addon_id)
        if not source or source.version == wb.version:
            continue
        game_version, wb_version = parse_version(source.version), parse_version(wb.version)
        if game_version is None or wb_version is None:
            notes.append(f"{wb.name}: versions not comparable (game {source.version}, Workbench {wb.version}), skipped")
        elif game_version < wb_version:
            notes.append(f"{wb.name}: Workbench more recent than the game ({wb.version} > {source.version}), kept")
        elif not source.is_complete():
            notes.append(f"{wb.name}: game copy {source.version} incomplete (download in progress?), skipped")
        else:
            updates.append((source, wb))

    # Dependencies missing from the Workbench, followed through the addons copied over
    additions, missing = [], set()
    present = set(workbench)
    # An updated addon brings the dependencies of its new version
    after_sync = dict(workbench, **{source.id: source for source, _ in updates})
    pending = [dependency for addon in after_sync.values() for dependency in addon.dependencies]
    while pending:
        dependency = pending.pop()
        if dependency in present or dependency in VANILLA_IDS:
            continue
        present.add(dependency)
        source = game.get(dependency)
        if not source:
            missing.add(dependency)
        elif not source.is_complete():
            notes.append(f"{source.name}: missing dependency, game copy incomplete, skipped")
        else:
            additions.append((source, None))
            pending.extend(source.dependencies)
    for dependency in sorted(missing):
        notes.append(f"dependency {dependency} missing from the Workbench and from the game: subscribe to it")

    replaced = {source.id for source, _ in updates}
    for addon_id, wb in workbench.items():
        if addon_id not in replaced and wb.version and wb.stale_paks():
            notes.append(f"{wb.name}: obsolete .pak in the Workbench ({', '.join(wb.stale_paks())})")
    return updates, additions, notes


def copy_addon(source, target):
    """Copies the game folder to a temporary folder, then swaps it with the Workbench one."""
    new_path = os.path.join(TEMP_DIR, source.folder)
    shutil.rmtree(new_path, ignore_errors=True)
    try:
        shutil.copytree(source.path, new_path)
    except OSError:
        shutil.rmtree(new_path, ignore_errors=True)
        raise
    destination = os.path.join(WORKBENCH_ADDONS, source.folder)
    old_path = None
    if target:
        os.makedirs(OLD_DIR, exist_ok=True)
        old_path = os.path.join(OLD_DIR, target.folder)
        shutil.rmtree(old_path, ignore_errors=True)
        os.replace(target.path, old_path)
    os.replace(new_path, destination)
    if old_path:
        shutil.rmtree(old_path, ignore_errors=True)


def main():
    dry_run = "--dry-run" in sys.argv
    print(f"Game:      {GAME_ADDONS}\nWorkbench: {WORKBENCH_ADDONS}\n")
    for directory in (GAME_ADDONS, WORKBENCH_ADDONS):
        if not os.path.isdir(directory):
            print(f"Folder not found: {directory}")
            return 1

    game, workbench = read_addons(GAME_ADDONS), read_addons(WORKBENCH_ADDONS)
    updates, additions, notes = build_plan(game, workbench)

    for source, wb in updates:
        print(f"  update  {source.name}: {wb.version} -> {source.version}")
    for source, _ in additions:
        print(f"  add     {source.name} {source.version} (missing dependency)")
    for note in notes:
        print(f"  note    {note}")
    work = updates + additions
    if not work:
        print("\nWorkbench addons already up to date." if not notes else "\nNothing to copy.")
        return 0
    print(f"\n{len(work)} addon(s) to copy, {gigabytes(sum(s.size() for s, _ in work))}.")
    if dry_run:
        return 0

    blockers = running_blockers()
    if blockers:
        print(f"Close the game and the Workbench first ({', '.join(blockers)}).")
        return 1
    if "--yes" not in sys.argv and input("Apply? [y/N] ").strip().lower() not in ("y", "yes", "o", "oui"):
        print("Cancelled.")
        return 0

    failures = 0
    for source, wb in work:
        print(f"  copying {source.name}...", end=" ", flush=True)
        try:
            copy_addon(source, wb)
            print("ok")
        except OSError as error:
            failures += 1
            print(f"FAILED ({error})")
    for directory in (TEMP_DIR, OLD_DIR):
        if os.path.isdir(directory) and not os.listdir(directory):
            os.rmdir(directory)
    print(f"\nDone: {len(work) - failures} copied, {failures} failed.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

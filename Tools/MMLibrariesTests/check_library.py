"""File-level checks of a generated "GDC AR MM Libraries" addon (no Workbench needed).

Usage: python check_library.py "<folder of the GDC AR MM Libraries addon>"
Exit code 0 when no problem is found. Also imported by run_tests.py.
"""
import os
import re
import sys

LIBRARY_ADDON_ID = "GDC AR MM Libraries"
MMTOOLS_GUID = "6AC540B09694CF5F"
INDEX = "Configs/MM Libraries/LibraryAddons.conf"

# Allowed first levels below PrefabLibrary/
CATEGORIES = {
    "MM Forces": {"Groups", "Static Weapons", "Vehicles - Air", "Vehicles - Land"},
    "MM Markers": {"Icons", "Areas"},
    "MM Systems": {"Waypoints", "Triggers", "Systems"},
}

GUID = r"[0-9A-F]{16}"
PREFAB_RE = re.compile(r'^(\w+) : "\{(' + GUID + r')\}([^"]+\.et)" \{\r?\n\}\r?\n?$')
META_NAME_RE = re.compile(r'^\s*Name "\{(' + GUID + r')\}([^"]+)"', re.M)
INDEX_ENTRY_RE = re.compile(
    r'GDC_MMLibraryIndexEntry \{\s*m_sPrefab "([^"]*)"\s*m_sAddonID "([^"]*)"\s*m_sAddonTitle "([^"]*)"\s*\}')


class Library:
    """Content of a library addon, read from disk."""

    def __init__(self, root):
        self.root = root
        self.prefabs = {}   # relative path -> (class name, parent GUID, parent path)
        self.guids = {}     # relative path -> own GUID, from the meta file
        self.index = []     # (library prefab, addon ID, addon title)
        self.errors = []

    def error(self, message):
        self.errors.append(message)

    def by_library(self):
        """Counts per PrefabLibrary/<library>/<category>/<folder>."""
        counts = {}
        for path in self.prefabs:
            key = "/".join(path.split("/")[1:4])
            counts[key] = counts.get(key, 0) + 1
        return dict(sorted(counts.items()))


def read_text(path):
    with open(path, encoding="utf-8") as file:
        return file.read()


def check_project(library):
    path = os.path.join(library.root, "addon.gproj")
    if not os.path.isfile(path):
        library.error("addon.gproj missing")
        return
    text = read_text(path)
    if f'ID "{LIBRARY_ADDON_ID}"' not in text:
        library.error("addon.gproj: wrong ID")
    if f'TITLE "{LIBRARY_ADDON_ID}"' not in text:
        library.error("addon.gproj: wrong TITLE")
    if not re.search(r'GUID "' + GUID + '"', text):
        library.error("addon.gproj: GUID is not 16 hexadecimal digits")
    if f'"{MMTOOLS_GUID}"' not in text:
        library.error("addon.gproj: no dependency on GDC AR Mission Maker Tools")


def check_meta(library, relative, resource_class):
    meta_path = os.path.join(library.root, relative + ".meta")
    if not os.path.isfile(meta_path):
        library.error(f"meta missing: {relative}")
        return None
    text = read_text(meta_path)
    match = META_NAME_RE.search(text)
    if not match:
        library.error(f"meta without Name: {relative}")
        return None
    if match.group(2) != relative:
        library.error(f"meta Name path {match.group(2)} differs from {relative}")
    if resource_class not in text:
        library.error(f"meta not {resource_class}: {relative}")
    return match.group(1)


def check_prefabs(library):
    base = os.path.join(library.root, "PrefabLibrary")
    if not os.path.isdir(base):
        library.error("PrefabLibrary missing")
        return

    for directory, folders, files in os.walk(base):
        relative_dir = os.path.relpath(directory, library.root).replace("\\", "/")
        if not folders and not files:
            library.error(f"empty folder: {relative_dir}")

        for name in files:
            relative = f"{relative_dir}/{name}"
            if name.endswith(".meta"):
                if not os.path.isfile(os.path.join(directory, name[:-5])):
                    library.error(f"meta without file: {relative}")
                continue
            if not name.endswith(".et"):
                library.error(f"unexpected file: {relative}")
                continue

            parts = relative.split("/")
            # PrefabLibrary/<library>/<category>/<faction or addon>/[sub folders/]<file>
            if len(parts) < 5 or parts[1] not in CATEGORIES or parts[2] not in CATEGORIES[parts[1]]:
                library.error(f"outside the library tree: {relative}")
            elif parts[1] in ("MM Forces", "MM Markers") and len(parts) != 5:
                library.error(f"unexpected sub folder: {relative}")

            match = PREFAB_RE.match(read_text(os.path.join(directory, name)))
            if not match:
                library.error(f"not an empty child prefab: {relative}")
                continue

            class_name, parent_guid, parent_path = match.groups()
            library.prefabs[relative] = (class_name, parent_guid, parent_path)

            # file name = parent file name, suffixed _2, _3... on a clash in the same folder
            stem = os.path.splitext(os.path.basename(parent_path))[0]
            own = os.path.splitext(name)[0]
            if own != stem and not re.fullmatch(re.escape(stem) + r"_\d+", own):
                library.error(f"file name differs from its parent {parent_path}: {relative}")

            guid = check_meta(library, relative, "EntityTemplateResourceClass")
            if guid:
                library.guids[relative] = guid

    seen = {}
    for relative, guid in library.guids.items():
        if guid in seen:
            library.error(f"GUID {guid} shared by {seen[guid]} and {relative}")
        seen[guid] = relative

    pairs = {}
    for relative, (_, parent_guid, _) in library.prefabs.items():
        key = (os.path.dirname(relative), parent_guid)
        if key in pairs:
            library.error(f"same parent twice in a folder: {pairs[key]} and {relative}")
        pairs[key] = relative


def check_index(library):
    path = os.path.join(library.root, INDEX)
    if not os.path.isfile(path):
        library.error("index missing")
        return
    check_meta(library, INDEX, "CONFResourceClass")

    library.index = INDEX_ENTRY_RE.findall(read_text(path))
    prefabs = [entry[0] for entry in library.index]
    if prefabs != sorted(prefabs):
        library.error("index not sorted")
    if len(set(prefabs)) != len(prefabs):
        library.error("index holds a prefab twice")
    for prefab, addon_id, addon_title in library.index:
        if prefab not in library.prefabs:
            library.error(f"index entry without file: {prefab}")
        if not addon_id or not addon_title:
            library.error(f"index entry without addon: {prefab}")
        if addon_id == "ArmaReforger":
            library.error(f"vanilla prefab in the index: {prefab}")
        # MM Markers and MM Systems are sorted by addon title: the index must agree
        parts = prefab.split("/")
        if parts[1] in ("MM Markers", "MM Systems") and len(parts) > 3 and parts[3] != addon_title:
            library.error(f"index addon {addon_title} differs from the folder: {prefab}")

    indexed = set(prefabs)
    for relative in library.prefabs:
        parts = relative.split("/")
        if parts[1] in ("MM Markers", "MM Systems") and parts[3] != "Arma Reforger" and relative not in indexed:
            library.error(f"addon prefab missing from the index: {relative}")


def check(root):
    library = Library(root)
    check_project(library)
    check_prefabs(library)
    check_index(library)
    return library


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    library = check(sys.argv[1])
    for key, count in library.by_library().items():
        print(f"{count:5}  {key}")
    print(f"{len(library.prefabs)} prefabs, {len(library.index)} index entries, {len(library.errors)} problems")
    for message in library.errors:
        print("PROBLEM: " + message)
    return 1 if library.errors else 0


if __name__ == "__main__":
    sys.exit(main())

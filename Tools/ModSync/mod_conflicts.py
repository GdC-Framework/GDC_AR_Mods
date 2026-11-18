"""Conflicts between the supervised addons, GDC's own projects included.

Two kinds, both resolved with the real load order read from the last MissionTests server log:
  - methods: the same method of the same class modified by several addons, at least one of them without calling
    super. The chain runs from the last loaded addon backwards and stops at the first override that does not call
    super: the overrides loaded before it never run ("dead").
  - files: the same resource path shipped by several addons. Text formats Enfusion merges (prefabs, configs,
    layouts) conflict only where two addons set the same property of the same object to different values; other
    formats (materials, particles, animation graphs, scripts...) are replaced whole by the last loaded addon.

The super detection is textual: a conditional call to super counts as calling it.
"""

import re
from collections import defaultdict
from pathlib import Path

from enfusion_mcp.formats.pak import read_index, read_file

# Formats written as property deltas over the parent resource, merged by the engine
MERGED_EXTENSIONS = (".et", ".conf", ".layout", ".ct")
# Not game resources, project bookkeeping every addon has, or test worlds shipped by mistake
SKIPPED_EXTENSIONS = (".meta", ".md", ".gproj", ".rdb", ".txt", ".json", ".gitignore", ".ent", ".layer")
MODDED_CLASS_RE = re.compile(r"modded\s+class\s+(\w+)[^{;]*\{")
METHOD_RE = re.compile(r"override\s+(?:(?:protected|private|static|proto|native|sealed)\s+)*[\w<>,\s]+?\s+(\w+)\s*\([^)]*\)\s*\{", re.S)
LOADED_LINE_RE = re.compile(r"gproj: '([^']*)' guid: '([0-9A-F]{16})'")


# --- Load order ------------------------------------------------------------------------------------------------

def load_order(log_root):
    """GUIDs in load order from the newest server log under log_root, and that log's folder name."""
    logs = sorted((p for p in Path(log_root).glob("*/console.log")), key=lambda p: p.stat().st_mtime, reverse=True)
    for log in logs:
        order, inside = [], False
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            if "Loaded addons:" in line:
                inside, order = True, []
                continue
            if inside:
                match = LOADED_LINE_RE.search(line)
                if not match:
                    break
                order.append(match.group(2))
        if order:
            return order, log.parent.name
    return [], ""


def resolve_order(top_level, dependencies, ignored=()):
    """Load order the game builds from a mod list: the list order, each mod preceded by its dependencies not loaded
    yet (checked with the server logs). Returns (GUIDs in load order, {GUID: top-level entry that brings it})."""
    order, brought_by, seen = [], {}, set()

    def add(guid, root):
        if guid in seen or guid in ignored:
            return
        seen.add(guid)
        for dependency in dependencies(guid):
            add(dependency, root)
        order.append(guid)
        brought_by[guid] = root

    for guid in top_level:
        add(guid, guid)
    return order, brought_by


# --- Reading the addons ----------------------------------------------------------------------------------------

class Source:
    """Files of one addon: from its paks (downloaded addon) or its folder (GDC project)."""

    def __init__(self, guid, name, folder, packed):
        self.guid, self.name, self.folder, self.packed = guid, name, Path(folder), packed
        self._entries = None

    def entries(self):
        """{lowercase path: (path, reader)}"""
        if self._entries is None:
            self._entries = {}
            if self.packed:
                for pak in sorted(self.folder.glob("*.pak")):
                    for entry in read_index(pak):
                        self._entries[entry.path.lower()] = (entry.path, lambda e=entry: read_file(e))
            else:
                for path in self.folder.rglob("*"):
                    if path.is_file() and ".git" not in path.parts:
                        rel = path.relative_to(self.folder).as_posix()
                        self._entries[rel.lower()] = (rel, lambda p=path: p.read_bytes())
        return self._entries

    def text(self, lower_path):
        return self.entries()[lower_path][1]().decode("utf-8", errors="replace")


def block_end(text, start):
    """Index just after the brace block opened before start."""
    depth, i = 1, start
    while i < len(text) and depth:
        char = text[i]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        elif char == '"':
            i = text.find('"', i + 1)
            if i == -1:
                return len(text)
        elif text.startswith("//", i):
            i = text.find("\n", i)
            if i == -1:
                return len(text)
        i += 1
    return i


def method_overrides(source):
    """[(class, method, calls_super, file)] for every override inside a modded class of the addon."""
    found = []
    for lower, (path, _) in source.entries().items():
        if not lower.endswith(".c"):
            continue
        text = source.text(lower)
        for cls in MODDED_CLASS_RE.finditer(text):
            body_end = block_end(text, cls.end())
            body = text[cls.end():body_end]
            for method in METHOD_RE.finditer(body):
                method_end = block_end(body, method.end())
                calls_super = f"super.{method.group(1)}" in body[method.end():method_end]
                found.append((cls.group(1), method.group(1), calls_super, path))
    return found


def block_key(header):
    """Key of a block header, and whether objects under it can be matched between addons.

    'SCR_X "{GUID}" : "{...}parent.et"' -> 'SCR_X "{GUID}"' (matched by its id). 'm_aActions' or 'components'
    (a member) is matched by name. 'SCR_X' alone, an object without id in a list, cannot be matched: two addons
    adding such objects to a list add different objects, so their properties are not compared."""
    header = header.split(" : ")[0].strip()
    if not header:
        return "{", True
    if '"' in header or " " in header:
        return header, True  # "Class "id"", "Action Name" (named entry) or "m_Member Class" (member object)
    return header, not header[0].isupper()


def property_values(text):
    """{(object path..., property): value} of a merged-format file, for the objects that can be matched."""
    values, stack = {}, []  # stack of (key, matchable)
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        if line == "}":
            if stack:
                stack.pop()
            continue
        if line.endswith("{"):
            stack.append(block_key(line[:-1]))
            continue
        matchable = all(ok for _, ok in stack[1:])  # the root object is always the file's own
        if "{" in line and line.endswith("}"):  # one-line block: 'header { values }'
            header, _, inner = line[:-1].partition("{")
            key, ok = block_key(header)
            if matchable and ok:
                values[tuple(k for k, _ in stack) + (key,)] = inner.strip()
            continue
        name, _, value = line.partition(" ")
        if matchable:
            values[tuple(k for k, _ in stack) + (name,)] = value.strip()
    return values


# --- Analysis --------------------------------------------------------------------------------------------------

def find_conflicts(sources, order, families=None, depends_on=None):
    """sources: list of Source. order: GUIDs in load order (may be empty: then nobody is said to win).
    families: {family: [addon name prefixes]}; addons of one family are made to work together by their authors,
    so a conflict between members of a single family only is not reported."""
    rank = {guid: i for i, guid in enumerate(order)}
    by_guid = {s.guid: s for s in sources}
    families = families or {}

    def family(guid):
        name = by_guid[guid].name
        return next((f for f, prefixes in families.items() if any(name.startswith(p) for p in prefixes)), name)

    def one_family(guids):
        return len({family(g) for g in guids}) < 2

    def ranked(guids):
        known = [g for g in guids if g in rank]
        return sorted(known, key=rank.get) + [g for g in guids if g not in rank]

    # Methods
    overrides = defaultdict(lambda: defaultdict(dict))  # (class, method) -> guid -> calls_super, file
    for source in sources:
        for cls, method, calls_super, path in method_overrides(source):
            current = overrides[(cls, method)].get(source.guid)
            # Several overrides of the same method in one addon: it breaks the chain if any of them does
            if current is None or (current[0] and not calls_super):
                overrides[(cls, method)][source.guid] = (calls_super, path)
    methods = []
    for (cls, method), per_addon in sorted(overrides.items()):
        if len(per_addon) < 2 or all(calls for calls, _ in per_addon.values()) or one_family(per_addon):
            continue
        chain = list(reversed(ranked(list(per_addon))))  # runs first -> last
        dead, stopped_by = [], None
        for guid in chain:
            if stopped_by:
                dead.append(guid)
            elif not per_addon[guid][0]:
                stopped_by = guid
        order_known = all(g in rank for g in per_addon)
        # Order that keeps every override: the only one that skips super must be loaded first (it then ends the
        # chain); with two or more skipping super, no order keeps them all
        skippers = [g for g in per_addon if not per_addon[g][0]]
        others_guids = [g for g in per_addon if skippers and g != skippers[0]]
        if len(skippers) == 1 and depends_on and any(depends_on(skippers[0], g) for g in others_guids):
            # A patch over its own dependency (a compatibility bridge): it always loads after it, by design
            advice = {"kind": "replaces its dependency", "addon": by_guid[skippers[0]].name,
                      "dependency": [by_guid[g].name for g in others_guids if depends_on(skippers[0], g)]}
        elif len(skippers) == 1:
            others = [by_guid[g].name for g in per_addon if g != skippers[0]]
            first = skippers[0] in rank and all(rank.get(skippers[0], 0) < rank.get(g, 0) for g in per_addon if g != skippers[0])
            advice = {"kind": "load first", "addon": by_guid[skippers[0]].name, "before": others,
                      "satisfied": first if order_known else None}
        else:
            advice = {"kind": "no order keeps all", "addons": [by_guid[g].name for g in skippers]}
        methods.append({
            "advice": advice,
            "class": cls, "method": method,
            "addons": [{"name": by_guid[g].name, "calls_super": per_addon[g][0], "file": per_addon[g][1]} for g in chain],
            "stopped_by": by_guid[stopped_by].name if stopped_by and order_known else "",
            "never_run": [by_guid[g].name for g in dead] if order_known else [],
            "order_known": order_known,
        })

    # Files
    owners = defaultdict(list)
    for source in sources:
        for lower in source.entries():
            # Files at the addon root (thumbnail, project files) are not game resources
            if "/" in lower and not lower.endswith(SKIPPED_EXTENSIONS):
                owners[lower].append(source.guid)
    files = []
    for lower, guids in sorted(owners.items()):
        if len(guids) < 2 or one_family(guids):
            continue
        contents = {g: by_guid[g].entries()[lower][1]() for g in guids}
        if len(set(contents.values())) == 1:
            continue  # identical copies
        guids = ranked(guids)
        winner = by_guid[guids[-1]].name if all(g in rank for g in guids) else ""
        path = by_guid[guids[0]].entries()[lower][0]
        if lower.endswith(MERGED_EXTENSIONS):
            per_addon = {g: property_values(contents[g].decode("utf-8", errors="replace")) for g in guids}
            clashes = []
            for key in sorted({k for values in per_addon.values() for k in values}):
                if len(key) == 2 and key[1] == "ID":
                    continue  # the root object's ID is the file's own identity, not a setting
                setters = [(g, per_addon[g][key]) for g in guids if key in per_addon[g]]
                if len({value for _, value in setters}) > 1 and not one_family([g for g, _ in setters]):
                    clashes.append({"property": " / ".join(key),
                                    "values": [{"name": by_guid[g].name, "value": v[:120]} for g, v in setters]})
            if clashes:
                files.append({"path": path, "kind": "same property", "addons": [by_guid[g].name for g in guids],
                              "wins": winner, "properties": clashes[:20], "property_count": len(clashes)})
        else:
            files.append({"path": path, "kind": "replaced whole", "addons": [by_guid[g].name for g in guids],
                          "wins": winner})
    return {"methods": methods, "files": group_replaced(files)}


def group_replaced(files, minimum=3):
    """Files replaced whole by the same addons in the same folder become one entry listing them."""
    groups = defaultdict(list)
    for item in files:
        if item["kind"] == "replaced whole":
            groups[(item["path"].rsplit("/", 1)[0], tuple(item["addons"]), item["wins"])].append(item)
    result = []
    for item in files:
        if item["kind"] != "replaced whole":
            result.append(item)
            continue
        key = (item["path"].rsplit("/", 1)[0], tuple(item["addons"]), item["wins"])
        group = groups.get(key)
        if not group:
            continue  # already emitted with its group
        if len(group) < minimum:
            result.append(item)
            group.remove(item)
            continue
        result.append({"path": key[0] + "/", "kind": "replaced whole", "addons": list(key[1]), "wins": key[2],
                       "files": [g["path"].rsplit("/", 1)[1] for g in group]})
        groups[key] = []
    return result

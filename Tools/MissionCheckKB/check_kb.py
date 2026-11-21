"""Checks the Check Mission knowledge base against the rule catalogue.

Knowledge base: GDC AR Mission Maker Tools/GDC_AR_MissionCheck_KB/, one sheet per rule, <id>.md (English) and
<id>.fr.md (French), listed in README.md and README.fr.md.
Catalogue: scripts/WorkbenchGame/GDC/MissionCheck/GDC_MMMissionCheckCatalog.c, rule.m_sKB = "<id>".
The report of the plugin gives the GitHub address of each finding's sheet: the sheets must be pushed.

    python check_kb.py    exit 1 on an error
"""
import re
import sys
from pathlib import Path

ADDON = Path(__file__).resolve().parents[2] / "GDC_AR_Misc" / "GDC AR Mission Maker Tools"
KB_DIR = ADDON / "GDC_AR_MissionCheck_KB"
CATALOGUE = ADDON / "scripts" / "WorkbenchGame" / "GDC" / "MissionCheck" / "GDC_MMMissionCheckCatalog.c"
ID_PATTERN = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")
PLANNED = re.compile(r"^(Rule planned|Règle prévue)\b", re.MULTILINE)


def main() -> int:
    errors = []
    sheets = {}  # id -> {language: path}
    for path in sorted(KB_DIR.glob("*.md")):
        if path.name.lower().startswith("readme"):
            continue

        french = path.name.endswith(".fr.md")
        sheet_id = path.name[: -len(".fr.md")] if french else path.name[: -len(".md")]
        if not ID_PATTERN.match(sheet_id):
            errors.append(f"{path.name}: the id must be lowercase words joined by '-'")
            continue

        sheets.setdefault(sheet_id, {})["fr" if french else "en"] = path

    for sheet_id, paths in sorted(sheets.items()):
        for language, name in (("en", f"{sheet_id}.md"), ("fr", f"{sheet_id}.fr.md")):
            if language not in paths:
                errors.append(f"{sheet_id}: {name} missing")

    named = set(re.findall(r'm_sKB\s*=\s*"([^"]+)"', CATALOGUE.read_text(encoding="utf-8")))
    for sheet_id in sorted(named - set(sheets)):
        errors.append(f"{sheet_id}: named by a rule of the catalogue, no sheet")
    for sheet_id in sorted(set(sheets) - named):
        texts = [p.read_text(encoding="utf-8") for p in sheets[sheet_id].values()]
        if not all(PLANNED.search(t) for t in texts):
            errors.append(f"{sheet_id}: named by no rule; for a planned rule, write 'Rule planned' / 'Règle prévue' as its second line")

    for index, suffix in (("README.md", ".md"), ("README.fr.md", ".fr.md")):
        index_path = KB_DIR / index
        if not index_path.exists():
            errors.append(f"{index} missing")
            continue
        text = index_path.read_text(encoding="utf-8")
        for sheet_id in sorted(sheets):
            if f"({sheet_id}{suffix})" not in text:
                errors.append(f"{index}: does not list {sheet_id}{suffix}")

    for error in errors:
        print("error:", error)
    if not errors:
        print(f"OK: {len(sheets)} sheets, in English and French, listed in both indexes, every rule's sheet exists")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

"""Prepare GDC loading screens for the Workbench import.

Reads every image of _Media/LoadingScreens/_Originaux (never modified), center-crops it
to 16:9, resizes it to 1280x720 and saves it as an RGB PNG with the same name in
_Media/LoadingScreens/Exports. Only new or modified originals are processed, unless
--force is given.

The exports are then imported into GDC AR Core with the Workbench plugin
"GDC Import Loading Screens" (Resource Manager > Plugins).
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is missing: run  python -m pip install --user pillow")

MEDIA_DIR = Path(__file__).resolve().parent.parent
SOURCE_DIR = MEDIA_DIR / "LoadingScreens" / "_Originaux"
EXPORT_DIR = MEDIA_DIR / "LoadingScreens" / "Exports"
SOURCE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tif", ".tiff"}
TARGET_SIZE = (1280, 720)


def crop_to_16_9(image):
    """Center-crop the image to a 16:9 ratio."""
    width, height = image.size
    if width * 9 > height * 16:
        new_width = height * 16 // 9
        left = (width - new_width) // 2
        return image.crop((left, 0, left + new_width, height))
    if width * 9 < height * 16:
        new_height = width * 9 // 16
        top = (height - new_height) // 2
        return image.crop((0, top, width, top + new_height))
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--force", action="store_true", help="re-export every original, even unchanged ones")
    args = parser.parse_args()

    EXPORT_DIR.mkdir(exist_ok=True)
    exported = skipped = errors = 0
    source_stems = set()

    for source in sorted(SOURCE_DIR.iterdir()):
        if source.suffix.lower() not in SOURCE_EXTENSIONS:
            continue
        source_stems.add(source.stem)

        # Workbench ignores non-ASCII paths, and the name becomes the .edds resource name
        if not source.stem.isascii() or " " in source.stem:
            print(f"ERREUR  {source.name} : nom avec accent ou espace, renommer l'original")
            errors += 1
            continue

        target = EXPORT_DIR / (source.stem + ".png")
        if not args.force and target.exists() and target.stat().st_mtime >= source.stat().st_mtime:
            skipped += 1
            continue

        with Image.open(source) as image:
            original_size = image.size
            result = crop_to_16_9(image.convert("RGB")).resize(TARGET_SIZE, Image.LANCZOS)
            result.save(target, optimize=True)

        print(f"export  {source.name} {original_size[0]}x{original_size[1]} -> {target.name} {TARGET_SIZE[0]}x{TARGET_SIZE[1]}")
        exported += 1

    for orphan in sorted(EXPORT_DIR.glob("*.png")):
        if orphan.stem not in source_stems:
            print(f"ATTENTION  {orphan.name} : export sans original dans _Originaux")

    print(f"\n{exported} exporte(s), {skipped} deja a jour, {errors} erreur(s).")
    if exported:
        print("Etape suivante : Workbench > Resource Manager > Plugins > GDC Import Loading Screens")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

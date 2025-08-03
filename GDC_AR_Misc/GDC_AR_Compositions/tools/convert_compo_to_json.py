"""
convert_compo_to_json.py

This script parses Enfusion composition (.et) files and extracts all child entities from the main composition block.
It outputs a JSON fragment containing only the entity list, formatted as:
"campItems": [ ... ] to be used with darc missions mod (https://github.com/mokdevel/DarcMods/tree/main/DarcMissions/docs)

Usage:
    python convert_compo_to_json.py -f <path/to/file.et>
    python convert_compo_to_json.py -d <path/to/directory>

- With -f, parses a single file and writes <filename>.json in the same directory.
- With -d, parses all .et files in the directory and writes corresponding .json files.
- If the output file already exists, the script asks for confirmation before overwriting.

Author: (your name or organization)
"""

import re
import json
import argparse
import os
import sys

def parse_vector(coords):
    # Converts a string "x y z" to a list of floats
    return [float(v) for v in coords.strip().split()]

def parse_rotation(props):
    # Looks for angleX, angleY, angleZ in properties, returns [x, y, z]
    rx = float(props.get('angleX', 0))
    ry = float(props.get('angleY', 0))
    rz = float(props.get('angleZ', 0))
    return [rx, ry, rz]

def extract_entities_from_block(block):
    entities = []
    entity_pattern = re.compile(
        r'(?P<type>GenericEntity|StaticModelEntity)\s*:\s*"(?P<prefab>[^"]+)"\s*\{', re.DOTALL
    )
    prop_pattern = re.compile(r'(\w+)\s+([^\{\}\n]+)')

    def parse_block(block):
        pos = 0
        while pos < len(block):
            match = entity_pattern.search(block, pos)
            if not match:
                break
            prefab = match.group('prefab')
            start = match.end()
            # Find the end of the block by balancing braces
            depth = 1
            i = start
            while i < len(block) and depth > 0:
                if block[i] == '{':
                    depth += 1
                elif block[i] == '}':
                    depth -= 1
                i += 1
            content = block[start:i-1]
            props = dict(prop_pattern.findall(content))
            if 'coords' in props:
                entity = {
                    "m_Resource": prefab,
                    "m_Position": parse_vector(props['coords']),
                    "m_Rotation": parse_rotation(props)
                }
                entities.append(entity)
            # Recursive for sub-entities
            parse_block(content)
            pos = i
    parse_block(block)
    return entities

def extract_main_block(text):
    # Finds the first main block (the composition)
    main_entity_pattern = re.compile(
        r'(GenericEntity|StaticModelEntity)\s*:\s*"[^"]+"\s*\{', re.DOTALL
    )
    match = main_entity_pattern.search(text)
    if not match:
        return None
    start = match.end()
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
        i += 1
    # Extract the content of the main block
    return text[start:i-1]

def process_file(filepath, output_dir=None):
    with open(filepath, encoding="utf-8") as f:
        text = f.read()
    main_block = extract_main_block(text)
    if main_block is None:
        print(f"Main block not found in {filepath}")
        return
    entities = extract_entities_from_block(main_block)
    base = os.path.splitext(os.path.basename(filepath))[0]
    output_name = base + ".json"
    if output_dir:
        output_path = os.path.join(output_dir, output_name)
    else:
        output_path = output_name

    # Check if the output file already exists
    if os.path.exists(output_path):
        resp = input(f"The file {output_path} already exists. Overwrite? (y/N): ")
        if resp.lower() not in ["y", "yes", "o", "oui"]:
            print("Write cancelled.")
            return

    with open(output_path, "w", encoding="utf-8") as out:
        out.write('"campItems": ')
        json.dump(entities, out, indent=2, ensure_ascii=False)
    print(f"File written: {output_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert Enfusion composition files to JSON.")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-f", "--file", help="Single file to parse")
    group.add_argument("-d", "--dir", help="Directory containing files to parse")
    args = parser.parse_args()

    if args.file:
        process_file(args.file)
    elif args.dir:
        for root, _, files in os.walk(args.dir):
            for file in files:
                if file.endswith(".et"):
                    process_file(os.path.join(root, file), output_dir=args.dir)
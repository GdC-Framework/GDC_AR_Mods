"""Writes the test addon "GDC Check Mission Tests" (never published): two mission worlds on Arland and test character
prefabs. T_Clean must give no finding; T_Faulty plants one case per rule (expected results in run_tests.py).

    python make_fixtures.py     rewrites the addon folder next to this script

Prefab and world GUIDs are fixed here, so that the layers can reference the test prefabs.
"""
from pathlib import Path

HERE = Path(__file__).resolve().parent
ADDON = HERE / "GDC Check Mission Tests"
ADDON_GUID = "6C0A11E0C4EC5001"
WORLDS = "worlds/GDC_CheckTests"
CHARACTERS = "Prefabs/GDC_CheckTests/Characters"

# base game prefabs
RIFLEMAN = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et"
UNARMED = "{2F912ED6E399FF47}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Unarmed.et"
FIRE_TEAM = "{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et"
MAGAZINE = "{D8F2CA92583B23D3}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_M855_M856_Last_5Tracer.et"
CANTEEN = "{630FB4AD4A735264}Prefabs/Items/Equipment/Canteens/Canteen_US_01.et"
RADIO_R107M = "{54C68E438DD34265}Prefabs/Items/Equipment/Radios/Radio_R107M.et"
AI_WORLD_ARLAND = "{01DC74137CFDDB6A}Prefabs/AI/SCR_AIWorld_Arland.et"
AI_WORLD = "{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et"
PERCEPTION = "{028DAEAD63E056BE}Prefabs/World/Game/PerceptionManager.et"
GAME_MODE_RL = "{9C2001FE7A2F2281}Prefabs/MP/Modes/PS_GameMode_Lobby.et"
TILW_MF = "{8F846D0FD5D6EA51}Prefabs/MP/TILW_MissionFramework.et"
BRIEFING = "{3136BE42592F3B1B}PrefabsEditable/MissionDescription/EditableMissionDescription.et"
S105 = "{54C3CC22DEBD57BE}Prefabs/Vehicles/Wheeled/S105/S105_beige.et"
BEETLE_TRAP = "{E9146AD559A299BB}Prefabs/Structures/Forest/BarkBeetleTrap_E_01.et"
TILW_INTERACTION = "{584EB34600E9698D}Prefabs/Utils/Interactions/TILW_SetFlagInteraction.et"
ARLAND = "{A9806AF617972E97}worlds/Arland/Arland.ent"
SKELETON_HINT = "<i>Toile de fond : lieu, époque, camps en présence.</i>"

# test character prefabs: name -> (GUID, base prefab, inventory entries)
MAG_ENTRIES = ("{54A49B42D138207E}", "{5538C040738A83D1}")  # the Rifleman's magazine pouches


def entry(entry_id, prefabs, target=None, inherited=True):
    """One ItemsInitConfigurationItem; inherited=True overrides the base prefab's entry of that id."""
    lines = [f'    ItemsInitConfigurationItem "{entry_id}" {{']
    if target is not None:
        lines.append(f'     TargetStorage "{target}"')
    lines.append("     PrefabsToSpawn {")
    for start in range(0, len(prefabs), 8):
        lines.append("      " + " ".join(f'"{p}"' for p in prefabs[start:start + 8]))
    lines.append("     }")
    lines.append("    }")
    return lines


CHARACTER_PREFABS = {
    # no magazine left: rule 3.7
    "T_NoSpare": ("{6C0A11E0C4EC5101}", RIFLEMAN, [*entry(MAG_ENTRIES[0], []), *entry(MAG_ENTRIES[1], [])]),
    # an item no addon provides: rule 3.8
    "T_Missing": ("{6C0A11E0C4EC5102}", RIFLEMAN, entry("{6C0A11E0C4EC5201}", ["{6C0A11E0C4EC59F1}Prefabs/GDC_CheckTests/MissingItem.et"])),
    # 600 magazines (0.12 kg each) in automatic placement: rule 3.9 (about 23 kg of gear + 72 kg, above 80 kg)
    "T_Heavy": ("{6C0A11E0C4EC5103}", RIFLEMAN, entry("{6C0A11E0C4EC5202}", [MAGAZINE] * 600)),
    # items aimed at a vest the character does not wear: rule 3.10
    "T_NotOnGear": ("{6C0A11E0C4EC5104}", RIFLEMAN, entry("{6C0A11E0C4EC5203}", [CANTEEN], target="Vest/Vest_Missing_GDCTest.et")),
    # a base game radio CVON does not replace, carried in the inventory: rule 3.5
    "T_VanillaRadio": ("{6C0A11E0C4EC5105}", RIFLEMAN, entry("{6C0A11E0C4EC5204}", [RADIO_R107M])),
}


def prefab_ref(name):
    guid = CHARACTER_PREFABS[name][0]
    return f"{guid}{CHARACTERS}/{name}.et"


def meta(resource, resource_class):
    configs = ["PC", "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5", "HEADLESS"]
    lines = ["MetaFileClass {", f' Name "{resource}"', " Configurations {", f"  {resource_class} PC {{", "  }"]
    for config in configs[1:]:
        lines += [f"  {resource_class} {config} : PC {{", "  }"]
    lines += [" }", "}"]
    return "\n".join(lines) + "\n"


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(text.encode("utf-8"))  # LF, as the rest of the repository


def character_prefab(name):
    guid, base, entries = CHARACTER_PREFABS[name]
    lines = [f'SCR_ChimeraCharacter : "{base}" {{', " components {", '  SCR_InventoryStorageManagerComponent "{520EA1D2DB118DE5}" {',
             "   InitialInventoryItems {", *entries, "   }", "  }", " }", "}"]
    return "\n".join(lines) + "\n"


def briefing(sections):
    lines = [f'$grp PS_MissionDescription : "{BRIEFING}" {{']
    for i, (name, title, text) in enumerate(sections):
        escaped = text.replace('"', '\\"')
        lines += [f" {name} {{", f"  coords 2040 39.438 {2048 + 2 * i}", f'  m_sTitle "{title}"', f'  m_sTextData "{escaped}"',
                  "  m_bEmptyFactionVisibility 1", "  m_bShowForAnyFaction 1", f"  m_iOrder {i}", " }"]
    lines.append("}")
    return "\n".join(lines) + "\n"


AI_WORLD_WITH_NAVMESHES = f'''SCR_AIWorld : "{AI_WORLD_ARLAND}" {{
 components {{
  NavmeshWorldComponent "{{5584F30E67F617AD}}" {{
   NavmeshSettings NavmeshWorld "{{50FC63BEBE3973C5}}" {{
    NavmeshFilesConfig BaseNavmeshFilesConfig "{{60B8A2995A7E8BC2}}" {{
     NavmeshFile "{{E0E9D55644237D27}}worlds/MP/Navmeshes/CTI_Campaign_Arland.nmn"
    }}
   }}
  }}
  NavmeshWorldComponent "{{5584F30EEFEE1223}}" {{
   NavmeshSettings ChimeraNavmeshWorld "{{50FC63BEBE3973C5}}" {{
    NavmeshFilesConfig BaseNavmeshFilesConfig "{{60B8A299509CF377}}" {{
     NavmeshFile "{{3B27B3547843CC74}}worlds/MP/Navmeshes/CTI_Campaign_Arland_vehicles.nmn"
    }}
   }}
  }}
 }}
 coords 2048 39.438 2048
}}
'''


def group(name, slots, x, faction=None, frequencies=None):
    lines = [f'SCR_AIGroup {name} : "{FIRE_TEAM}" {{', f" coords {x} 39.438 2070"]
    if faction is not None:
        lines.append(f' m_faction "{faction}"')
    lines.append(" m_aUnitPrefabSlots {")
    lines += [f'  "{slot}"' for slot in slots]
    lines.append(" }")
    lines.append(" m_bSetPlayable 1")
    if frequencies is not None:
        lines.append(" m_aGDC_SlotShortRangeFrequencies {")
        lines += [f'  "{f}"' for f in frequencies]
        lines.append(" }")
    lines.append("}")
    return "\n".join(lines) + "\n"


def world(name, layers):
    ent = f"{WORLDS}/{name}/{name}.ent"
    guid = {"T_Clean": "{6C0A11E0C4EC5301}", "T_Faulty": "{6C0A11E0C4EC5302}", "T_NoPlayable": "{6C0A11E0C4EC5303}"}[name]
    write(ADDON / ent, f'SubScene {{\n Parent "{ARLAND}"\n}}\n')
    write(ADDON / (ent + ".meta"), meta(guid + ent, "ENTResourceClass"))
    for layer, text in layers.items():
        write(ADDON / f"{WORLDS}/{name}/{name}_Layers/{layer}.layer", text)


def main():
    write(ADDON / "addon.gproj", f'''GameProject {{
 ID "GDC Check Mission Tests"
 GUID "{ADDON_GUID}"
 TITLE "GDC Check Mission Tests"
 Dependencies {{
  "58D0FB3206B6F859" "6AC540B09694CF5F" "690756F509F253EB"
 }}
}}
''')

    for name in CHARACTER_PREFABS:
        path = f"{CHARACTERS}/{name}.et"
        write(ADDON / path, character_prefab(name))
        write(ADDON / (path + ".meta"), meta(CHARACTER_PREFABS[name][0] + path, "EntityTemplateResourceClass"))

    # ---- T_Clean: what Create Mission Skeleton makes, briefing written, one clean playable group: no finding
    world("T_Clean", {
        "default": AI_WORLD_WITH_NAVMESHES
        + f'PerceptionManager PerceptionManager : "{PERCEPTION}" {{\n coords 2048 39.438 2052\n}}\n'
        + f'PS_GameModeCoop PS_GameMode_Lobby1 : "{GAME_MODE_RL}" {{\n coords 2052 39.438 2048\n}}\n',
        "systems": f'TILW_MissionFrameworkEntity TILW_MF : "{TILW_MF}" {{\n coords 2052 41.188 2052\n}}\n',
        "briefing": briefing([("a_Contexte", "Contexte", "Test du contexte."), ("b_Situation", "Situation", "Test de la situation."),
                              ("c_Mission", "Mission", "Test de la mission."), ("d_Renseignements", "Renseignements", "Test des renseignements."),
                              ("e_Insertion_Extraction", "Insertion et extraction", "Test de l'insertion.")]),
        "players": group("G_Clean", [RIFLEMAN, RIFLEMAN], 2070),
        "ia/opfor": "",
        "build/decor": "",
    })

    # ---- T_NoPlayable: Reforger Lobby without any playable group (3.1); no TILW entity at all, so the TILW rules must not
    # run (origin not used) and only its framework entity is reported missing (1.3)
    world("T_NoPlayable", {
        "default": AI_WORLD_WITH_NAVMESHES
        + f'PerceptionManager PerceptionManager : "{PERCEPTION}" {{\n coords 2048 39.438 2052\n}}\n'
        + f'PS_GameModeCoop PS_GameMode_Lobby1 : "{GAME_MODE_RL}" {{\n coords 2052 39.438 2048\n}}\n',
        "systems": "",
        "briefing": briefing([("a_Contexte", "Contexte", "Test du contexte."), ("b_Situation", "Situation", "Test de la situation."),
                              ("c_Mission", "Mission", "Test de la mission."), ("d_Renseignements", "Renseignements", "Test des renseignements."),
                              ("e_Insertion_Extraction", "Insertion et extraction", "Test de l'insertion.")]),
        "players": "",
        "ia/opfor": "",
        "build/decor": "",
    })

    # ---- T_Faulty: one planted case per rule
    world("T_Faulty", {
        # 1.4 x2 (base AI world: Soldiers and BTRlike without file), 1.2 x2 (two game modes), 1.3 (no PerceptionManager)
        "default": f'SCR_AIWorld : "{AI_WORLD}" {{\n coords 2048 39.438 2048\n}}\n'
        + f'PS_GameModeCoop PS_GameMode_Lobby1 : "{GAME_MODE_RL}" {{\n coords 2052 39.438 2048\n}}\n'
        + f'PS_GameModeCoop PS_GameMode_Lobby2 : "{GAME_MODE_RL}" {{\n coords 2056 39.438 2048\n}}\n',
        # no "systems" layer: 1.1, and no TILW framework entity: 1.3
        # 1.7 (skeleton hint kept), 1.6 (empty), 1.5 (the three other sections)
        "briefing": briefing([("a_Contexte", "Contexte", SKELETON_HINT), ("b_Situation", "Situation", "")]),
        "players": group("G_NoFaction", [UNARMED, prefab_ref("T_NoSpare"), prefab_ref("T_Missing"), prefab_ref("T_Heavy"),
                                         prefab_ref("T_NotOnGear"), prefab_ref("T_VanillaRadio")], 2070, faction="")
        + group("G_Freq", [RIFLEMAN, RIFLEMAN], 2080, frequencies=["31000"]),
        "ia/opfor": "",
        "build/decor": "\n".join([
            "PK_EntityDeleter D_NoFilter {\n coords 2100 39.438 2100\n}",
            'PK_EntityDeleter D_Missing {\n coords 2102 39.438 2100\n m_prefabFilterList {\n  "{6C0A11E0C4EC59F2}Prefabs/GDC_CheckTests/MissingPrefab.et"\n }\n}',
            'PK_EntityDeleter D_Proxy {\n coords 2104 39.438 2100\n m_prefabFilterList {\n  "{6C0A11E0C4EC59F3}PrefabLibrary/MM Forces/GDC_CheckTests/Proxy.et"\n }\n}',
            f'PK_EntityDeleter D_WrongGuid {{\n coords 2106 39.438 2100\n m_prefabFilterList {{\n  "{{6C0A11E0C4EC59F4}}{AI_WORLD.split("}")[1]}"\n }}\n}}',
            "SCR_AIGroup {\n coords 2108 39.438 2100\n}",
            f'Vehicle V_Crew : "{S105}" {{\n components {{\n  TILW_VehicleCrewComponent "{{6C0A11E0C4EC5401}}" {{\n  }}\n }}\n coords 2110 39 2110\n}}',
            f'SCR_DestructibleEntity : "{BEETLE_TRAP}" {{\n coords 2120 39 2120\n {{\n  GenericEntity : "{TILW_INTERACTION}" {{\n   coords 0 0 0\n  }}\n }}\n}}',
            'GenericEntity T_Teleport {\n components {\n  GDC_TeleportModuleComponent "{6C0A11E0C4EC5402}" {\n   m_eMode ALL_PLAYERS\n   m_eMinRankRequired CAPTAIN\n  }\n }\n coords 2130 39.438 2130\n}',
        ]) + "\n",
    })
    print(f"written: {ADDON}")


if __name__ == "__main__":
    main()

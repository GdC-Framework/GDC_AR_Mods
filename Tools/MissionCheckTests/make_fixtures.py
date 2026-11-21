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


# an entity setting a flag from a component: the mission makes it appear (Spawn Prefab), so its flag counts as set (5.8)
FLAG_SETTER = "{6C0A11E0C4EC5106}Prefabs/GDC_CheckTests/T_FlagSetter.et"
FLAG_SETTER_PREFAB = '''GenericEntity {
 components {
  TILW_Flag_EntityDamage "{6C0A11E0C4EC5601}" {
   m_flagName "Dmg_GDCTest"
  }
 }
}
'''


# a group prefab without faction, as the RHS ones: spawned by a TILW spawner in T_Faulty (6.1)
NO_FACTION_GROUP = "{6C0A11E0C4EC5107}Prefabs/GDC_CheckTests/T_NoFactionGroup.et"
WP_MOVE = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et"
WP_CYCLE = "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et"
WP_GET_IN = "{712F4795CF8B91C7}Prefabs/AI/Waypoints/AIWaypoint_GetIn.et"
GROUND_Y = "39.5"  # Arland around 2048 / 2048 (39.44 to 39.63 m): the AI traps stand there (6.5: on the ground but one)
# terrain height of Arland where the playable groups stand (z 2070): rule 6.5 checks that they stand on the ground
GROUND_2070 = {2070: "48.5", 2080: "49.094", 2090: "48.906"}


def faulty_ai():
    """T_Faulty's AI layer: one trap per rule of block 6"""
    riflemen = "".join(f'  "{RIFLEMAN}"\n' for _ in range(31))
    return "\n".join([
        # 6.1 (no faction), 6.2 (WP_Missing, and G_BadFaction is a group, not a waypoint), 6.3 (NoVeh, and Trg_Presence is
        # no vehicle); its 31 riflemen and the spawner's 4 give 35 AI at start, above the limit of 30: 6.6, 6.7
        f'SCR_AIGroup G_AINoFaction : "{FIRE_TEAM}" {{\n coords 2048 {GROUND_Y} 2048\n m_faction ""\n m_aUnitPrefabSlots {{\n{riflemen} }}\n'
        ' m_aStaticWaypoints {\n  "WP_Used_GDCTest"\n  "WP_Missing_GDCTest"\n  "G_BadFaction"\n  "C_Cycle_GDCTest"\n'
        '  "WP_Air_GDCTest"\n  "WP_Entity_GDCTest"\n }\n m_aStaticVehicles {\n  "NoVeh_GDCTest"\n  "Trg_Presence"\n }\n}',
        f'SCR_AIWaypoint WP_Used_GDCTest : "{WP_MOVE}" {{\n coords 2048.2 {GROUND_Y} 2048\n}}',
        f'SCR_AIWaypoint WP_Orphan_GDCTest : "{WP_MOVE}" {{\n coords 2048.4 {GROUND_Y} 2048\n}}',  # 6.4
        f'SCR_AIWaypoint WP_Air_GDCTest : "{WP_MOVE}" {{\n coords 2048.6 80 2048\n}}',  # 6.5
        f'AIWaypointCycle C_Cycle_GDCTest : "{WP_CYCLE}" {{\n coords 2048.8 {GROUND_Y} 2048\n CycleWaypoints {{\n  "WP_Used_GDCTest"\n'
        '  "WP_Ghost_GDCTest"\n }\n}',  # 6.2
        f'SCR_EntityWaypoint WP_Entity_GDCTest : "{WP_GET_IN}" {{\n coords 2049 {GROUND_Y} 2048\n m_EntityWaypointParameters '
        'SCR_AIEntityWaypointParameters "{6C0A11E0C4EC5801}" {\n  m_sEntityName "NoTarget_GDCTest"\n }\n}',  # 6.2
        f'TILW_PrefabSpawnerEntity P_GroupSpawner {{\n coords 2049.2 {GROUND_Y} 2048\n m_prefab "{NO_FACTION_GROUP}"\n}}',  # 6.1
    ]) + "\n"


class Ids:
    """Object GUIDs of the layers, fixed so that the files do not change from one run to the next"""
    def __init__(self, first):
        self.next = first

    def __call__(self):
        self.next += 1
        return "{%016X}" % self.next


def obj(ids, cls, body=(), key=None):
    """One object of a layer: [key] Class "{GUID}" { body }"""
    head = f'{key + " " if key else ""}{cls} "{ids()}" {{'
    return [head, *[" " + line for line in body], "}"]


def objects(key, items):
    lines = [key + " {"]
    for item in items:
        lines += [" " + line for line in item]
    return lines + ["}"]


def strings(key, values):
    return [key + " {", *[f' "{v}"' for v in values], "}"]


def literal(ids, flag):
    return obj(ids, "TILW_LiteralTerm", [f'm_flagName "{flag}"'])


def all_of(ids, flags):
    return obj(ids, "TILW_ConjunctionTerm", objects("m_operands", [literal(ids, f) for f in flags]), key="m_condition")


def event(ids, name, condition=None, instructions=(), pregame=False):
    body = [f'm_name "{name}"']
    if instructions:
        body += objects("m_instructions", instructions)
    if condition:
        body += condition
    if pregame:
        body.append("m_pregameEvent 1")
    return obj(ids, "TILW_MissionEvent", body)


def faulty_framework():
    """T_Faulty's TILW Mission Framework entity: one trap per mission event rule, and the cases that must not fire"""
    ids = Ids(0x6C0A11E0C4EC5700)
    instructions = [
        obj(ids, "TILW_RunEventsInstruction", strings("m_eventNames", ["E_RunOnly", "Ghost_GDCTest"])),  # 5.9 (Ghost)
        obj(ids, "TILW_ReactivateEventsInstruction", strings("m_eventNames", ["E_Dup"])),  # 5.10
        obj(ids, "TILW_AssignWaypointsInstruction", ['m_groupName "NoGroup_GDCTest"', *strings("m_waypointNames", ["Trg_Presence"])]),  # 5.11
        obj(ids, "TILW_AssignWaypointsInstruction", ['m_groupName "Crew_GDCTest"']),  # named by the crew below: no finding
        obj(ids, "TILW_SpawnPrefabInstruction", [f'm_prefab "{FLAG_SETTER}"', 'm_locationName "Trg_Faction"', 'm_setEntityName "Spawned_GDCTest"']),
        obj(ids, "TILW_SpawnVehicleInstruction", ['m_locationName "Trg_Faction"', *obj(ids, "TILW_CrewConfig", objects(
            "m_crewGroups", [obj(ids, "TILW_CrewGroup", ['m_entityName "Crew_GDCTest"'])]), key="m_crewConfig")]),
        obj(ids, "TILW_DeleteEntitiesInstruction", strings("m_entityNames", ["Spawned_GDCTest"])),  # named at spawn: no finding
        obj(ids, "TILW_EndGameInstruction"),  # with m_suppressGameEnd: 5.14
    ]
    events = [
        event(ids, "E_NoCondition"),  # 5.6
        event(ids, "E_EmptyAnd", obj(ids, "TILW_ConjunctionTerm", key="m_condition")),  # 5.7
        # 5.8 for Never_GDCTest only: F_GDCTest (trigger), Dmg_GDCTest (spawned prefab), M_Meta_GDCTest (meta flag) are set
        event(ids, "E_Flags", all_of(ids, ["Never_GDCTest", "F_GDCTest", "Dmg_GDCTest", "M_Meta_GDCTest"]), instructions),
        event(ids, "E_RunOnly", all_of(ids, ["NoFlag_GDCTest"])),  # run by name: its condition may be unreachable
        event(ids, "E_Dup", all_of(ids, ["F_GDCTest"])),
        event(ids, "E_Dup", all_of(ids, ["F_GDCTest"])),
        event(ids, "E_Pregame", all_of(ids, ["F_GDCTest", "R_GDCTest"]), pregame=True),  # 5.13 for F_GDCTest only
    ]
    meta_flags = [
        obj(ids, "TILW_MetaFlag", ['m_flagName "M_NoDef_GDCTest"']),  # 5.6
        obj(ids, "TILW_MetaFlag", ['m_flagName "M_Meta_GDCTest"', *obj(ids, "TILW_LiteralTerm", ['m_flagName "F_GDCTest"'], key="m_definition")]),
    ]
    body = ["coords 2180 39.438 2180", *objects("m_missionEvents", events), *objects("m_metaFlags", meta_flags),
            *objects("m_randomFlags", [obj(ids, "TILW_RandomFlag", ['m_flagName "R_GDCTest"'])]), "m_suppressGameEnd 1"]
    first = "\n".join([f'TILW_MissionFrameworkEntity TILW_MF_A : "{TILW_MF}" {{', *[" " + line for line in body], "}"])
    # a second framework entity: 5.12
    second = f'TILW_MissionFrameworkEntity TILW_MF_B : "{TILW_MF}" {{\n coords 2184 39.438 2180\n}}'
    # a spawner waiting for a flag nothing sets (5.8) and aimed at a waypoint nobody holds (5.11)
    spawner = 'TILW_PrefabSpawnerEntity P_Spawner {\n coords 2190 39.438 2190\n m_conditionFlag "Never2_GDCTest"\n m_waypointNames {\n  "NoWp_GDCTest"\n }\n}'
    return [first, second, spawner]


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
 "Active AI limit" 128
}}
'''

# game mode with the values of the mission skeleton (rule 4.2)
GAME_MODE_SKELETON = (f'PS_GameModeCoop PS_GameMode_Lobby1 : "{GAME_MODE_RL}" {{\n coords 2052 39.438 2048\n m_bTeamSwitch 1\n'
                      " m_bPublicCommandBriefing 1\n m_bRemoveSquadMarkers 1\n m_iFreezeTime 0\n m_bDisableGarbageSystem 1\n"
                      " m_bDisableArmaVision 0\n}\n")


def group(name, slots, x, faction=None, frequencies=None):
    lines = [f'SCR_AIGroup {name} : "{FIRE_TEAM}" {{', f" coords {x} {GROUND_2070[x]} 2070"]
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

    for reference, text in ((FLAG_SETTER, FLAG_SETTER_PREFAB), (NO_FACTION_GROUP, f'SCR_AIGroup : "{FIRE_TEAM}" {{\n m_faction ""\n}}\n')):
        guid, path = reference[:18], reference[18:]
        write(ADDON / path, text)
        write(ADDON / (path + ".meta"), meta(guid + path, "EntityTemplateResourceClass"))

    # ---- T_Clean: what Create Mission Skeleton makes, briefing written, one clean playable group: no finding
    world("T_Clean", {
        "default": AI_WORLD_WITH_NAVMESHES
        + f'PerceptionManager PerceptionManager : "{PERCEPTION}" {{\n coords 2048 39.438 2052\n}}\n'
        + GAME_MODE_SKELETON,
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
        + GAME_MODE_SKELETON,
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
        # 1.4 x3 (base AI world: no navmesh file), 4.2 (AI limit 30 instead of 128, and the game mode values),
        # 1.2 x2 (two game modes), 1.3 (no PerceptionManager)
        "default": f'SCR_AIWorld : "{AI_WORLD}" {{\n coords 2048 39.438 2048\n "Active AI limit" 30\n}}\n'
        + f'PS_GameModeCoop PS_GameMode_Lobby1 : "{GAME_MODE_RL}" {{\n coords 2052 39.438 2048\n}}\n'
        + f'PS_GameModeCoop PS_GameMode_Lobby2 : "{GAME_MODE_RL}" {{\n coords 2056 39.438 2048\n}}\n',
        # no "systems" layer: 1.1 (the TILW framework entities are in build/decor, with the mission event traps)
        # 1.7 (skeleton hint kept), 1.6 (empty), 1.5 (the three other sections)
        "briefing": briefing([("a_Contexte", "Contexte", SKELETON_HINT), ("b_Situation", "Situation", "")]),
        "players": group("G_NoFaction", [UNARMED, prefab_ref("T_NoSpare"), prefab_ref("T_Missing"), prefab_ref("T_Heavy"),
                                         prefab_ref("T_NotOnGear"), prefab_ref("T_VanillaRadio")], 2070, faction="")
        + group("G_Freq", [RIFLEMAN, RIFLEMAN], 2080, frequencies=["31000"])
        + group("G_BadFaction", [RIFLEMAN], 2090, faction="XX_GDCTest"),
        "ia/opfor": faulty_ai(),
        "build/decor": "\n".join([
            "PK_EntityDeleter D_NoFilter {\n coords 2100 39.438 2100\n}",
            'PK_EntityDeleter D_Missing {\n coords 2102 39.438 2100\n m_prefabFilterList {\n  "{6C0A11E0C4EC59F2}Prefabs/GDC_CheckTests/MissingPrefab.et"\n }\n}',
            'PK_EntityDeleter D_Proxy {\n coords 2104 39.438 2100\n m_prefabFilterList {\n  "{6C0A11E0C4EC59F3}PrefabLibrary/MM Forces/GDC_CheckTests/Proxy.et"\n }\n}',
            f'PK_EntityDeleter D_WrongGuid {{\n coords 2106 39.438 2100\n m_prefabFilterList {{\n  "{{6C0A11E0C4EC59F4}}{AI_WORLD.split("}")[1]}"\n }}\n}}',
            "SCR_AIGroup {\n coords 2108 56.375 2100\n}",
            f'Vehicle V_Crew : "{S105}" {{\n components {{\n  TILW_VehicleCrewComponent "{{6C0A11E0C4EC5401}}" {{\n  }}\n }}\n coords 2110 39 2110\n}}',
            f'SCR_DestructibleEntity : "{BEETLE_TRAP}" {{\n coords 2120 39 2120\n {{\n  GenericEntity : "{TILW_INTERACTION}" {{\n   coords 0 0 0\n  }}\n }}\n}}',
            'GenericEntity T_Teleport {\n components {\n  GDC_TeleportModuleComponent "{6C0A11E0C4EC5402}" {\n   m_eMode ALL_PLAYERS\n   m_eMinRankRequired CAPTAIN\n  }\n }\n coords 2130 39.438 2130\n}',
            'TILW_PresenceTriggerEntity Trg_Presence {\n coords 2140 39.438 2140\n m_entityNames {\n  "Nobody_GDCTest"\n }\n}',
            'TILW_FactionTriggerEntity Trg_Faction {\n coords 2150 39.438 2150\n m_sendStatusMessages 1\n m_statusMessages {\n  "a" "b"\n }\n'
            ' m_displayTo {\n  "ZZ_GDCTest"\n }\n m_flagName "F_GDCTest"\n}',
            'GenericEntity T_Support {\n components {\n  GDC_FireSupportModuleComponent "{6C0A11E0C4EC5403}" {\n   m_eMinRankRequired GENERAL\n'
            '   m_eFireAreaMode WHITELIST\n   m_aAreaNames {\n    "NoArea_GDCTest"\n   }\n   m_iMinDispersion 200\n   m_iMaxDispersion 100\n'
            '   m_sMarkerVisibleToFaction "YY_GDCTest"\n  }\n }\n coords 2160 39.438 2160\n}',
            'GenericEntity T_Artillery {\n components {\n  GDC_ArtilleryFireSupportComponent "{6C0A11E0C4EC5404}" {\n   m_sAIGroupName "NoMortar_GDCTest"\n  }\n }\n coords 2170 39.438 2170\n}',
            *faulty_framework(),
        ]) + "\n",
    })
    print(f"written: {ADDON}")


if __name__ == "__main__":
    main()

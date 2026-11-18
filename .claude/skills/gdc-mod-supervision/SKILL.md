---
name: gdc-mod-supervision
description: Supervise the addons GDC depends on around the game -> Workbench sync - keep the old versions, sync, summarise what changed in every updated addon (GitHub, Workshop changelog, file comparison), check that GDC code still fits (modded classes, prefab references), run the checks and the compile test, and publish a French report page for the GDC admins. Use it whenever the user asks to sync the addons/mods between the game and the Workbench ("synchro des mods", "mets à jour les mods du WB"), asks what changed in the mods after a game or Workshop update, asks for a "supervision des mods", or wants to know whether a mod update broke something for GDC - even if they only ask for the sync. Also use it
before adding an addon to the GDC modset ("ce mod est-il compatible avec notre modset", "on pourrait ajouter X") to
check its dependencies, footprint and conflicts.
---

# GDC addon supervision

The game launcher updates the Workshop addons; the Workbench keeps its own copies, synced by
`Tools/ModSync/sync_workbench_mods.py`. Each sync can bring changes that break GDC (CSI 1.9 rewrote the
name tag class GDC Core modifies, for instance) and authors often publish no changelog. This skill turns the sync
into a supervision: what changed, does GDC still work, what to watch. The result is a page the GDC admins read.

Supervised addons: the root GDC projects listed in `Tools/ModSync/supervision/config.json` and all their
dependencies, recursively (57 addons in November 2026). The versions seen at the last supervision are in
`Tools/ModSync/supervision/state.json`, so "what changed" always means "since the last supervision", even when a
sync was done without this skill.

The mechanical work is done by `Tools/ModSync/mod_supervision.py`. Run it with the enfusion-mcp-py interpreter,
which provides the .pak reader:

```
C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe Tools/ModSync/mod_supervision.py <scope|before|after|save>
```

## Steps

### 1. Keep the old versions (`before`)

Run `before`. It shows the sync plan and keeps the text files (scripts, configs, prefabs, layouts) and the file
list of every supervised addon about to be replaced, in `Documents/My Games/GDC_ModSupervision/snapshots`. This is
what makes a comparison possible afterwards: once synced, the old folder is gone. It only reads, so the Workbench
may be open.

### 2. Sync

Run `python Tools/ModSync/sync_workbench_mods.py --yes`, then `--dry-run` to confirm that nothing is left.
The script refuses while the game or the Workbench runs: ask the user to close them, never close them yourself.
If nothing was to be synced, carry on anyway: addons may have changed since the last supervision.

### 3. Analyse (`after`)

Run `after`. It writes `Documents/My Games/GDC_ModSupervision/report.json`:
- `changed`: supervised addons whose version differs from state.json, with for each, when an old copy was kept:
  `diff` (files and script classes changed), `gdc_touched` (GDC files that modify or inherit a changed class),
  `gdc_broken_refs` (resources GDC references that the addon removed), `base_game_overrides_changed`;
- `checks`: missing addon, obsolete .pak, incomplete download, dependency version below what an addon needs,
  addon missing from the MissionTests addon list;
- `gdc_exposure` (every changed addon except GDC's own, even without an old copy): the classes GDC modifies or
  inherits from that this addon also declares or modifies, and the resources GDC references in it. This says
  how much GDC relies on the addon, hence how closely to look at it;
- `new_in_scope`: addons that became dependencies since the last supervision.

If `changed` is empty and `checks` is empty, say so in two lines and skip to step 7 (still save the state).

### 4. Summarise each change

For each changed addon, find what changed, following `references/sources.md` (Git repository, then Workshop
changelog, then the kept copy). Update that file when you learn a new source. Write a short summary in French:
what changes for players and mission makers, not a commit list. Always name the source, and say plainly when there
is none ("aucun changelog, comparaison des fichiers seulement").

For the kept copy, compare `kept_files` and `compared_files` (`diff -r`), read the changed scripts that matter and
describe the behaviour change. A large addon can have hundreds of changed files: group them, and go deep only where
GDC is exposed (step 5).

#### Old version from the Workshop, only when needed

The Workshop keeps the last 50 versions of each addon, so a missing old copy can be downloaded. Downloading costs
bandwidth and reading a diff costs a lot of context, so do it only when all of these hold:
- no kept copy of the old version (`note` says so);
- no usable source of changes (no repository, changelog empty or too vague to judge the GDC impact);
- GDC is exposed: `gdc_exposure` lists classes, or the addon is one GDC relies on heavily (see the memories).

Compare only the two ends (the old supervised version and the current one): one diff covers every version in
between, whatever their number. Fetch intermediate versions only to find which version introduced a breakage,
by halving the range (about log2 of the number of versions, not all of them).

Ask the user before the first download of a supervision (which addon, which version, why). Procedure (tested on
2026-11-18 with Chungus Launchers 1.0.34: 60 MB in about 7 s; an addon with Workshop dependencies may also pull
them, not tested yet: check the disk on the first such case and complete this section):
1. In `Documents/My Games/GDC_ModSupervision/download/<GUID>_<version>/`, write `server.json`:
   ```json
   {"bindPort": 2311, "publicPort": 2311,
    "game": {"name": "GDC mod fetch", "scenarioId": "{ECC61978EDCC2B5A}Missions/23_Campaign.conf",
             "maxPlayers": 1, "visible": false,
             "mods": [{"modId": "<GUID>", "name": "<name>", "version": "<version>"}]}}
   ```
2. Start in the background (PowerShell `Start-Process -PassThru`, keep the PID):
   `C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Server\ArmaReforgerServer.exe -config <server.json>
   -addonDownloadDir <that folder> -profile GDCModFetch -maxFPS 30`.
3. Wait until `<folder>/addons/<addon folder>/ServerData.json` exists and its `temp` subfolder is gone. Not the
   manifests: they are written at the start of the download, before the data. Then stop the server by its PID
   (it would otherwise start the scenario and keep running).
4. `mod_supervision.py keep "<addon folder>"`, then `after` again: the comparison now runs.
5. Delete the download folder and `Documents/My Games/GDCModFetch` (the server's profile and logs).

GDC's own addons (GDC AR Core, GDC Equipment, GDC AR Lib) are summarised from `CHANGELOG.md` and the git log, never
compared file by file.

Minor addons (a texture pack, a patch version without changelog and few changed files) get one line.

### 5. Impact on GDC

This is the part the admins care about most. For every `gdc_touched` entry, read the GDC file and the addon's new
file and judge whether GDC still works, the way it was done for CSI 1.9 (GDC Core skips UNCONSCIOUS in
`SCR_NameTagData.ActivateEntityState`; CSI's new override still calls super, the role icon still depends on the
state GDC blocks, so it still works). Give a verdict per case: compatible, to test in game, or broken.

Also weigh changes against how GDC uses the addon. The project memories record it (CSI: only the 3D name tags and
colours, no radar, no map icons; TILW, lobby, WCS...): a big change in a part GDC does not use is a footnote.

`gdc_broken_refs` are serious: the Workbench silently strips a missing prefab reference when the world is saved.
List the GDC files and tell the user not to save them in the Workbench before fixing.

### 6. Checks and compile test

Report each `checks` issue with what to do. Then, with the Workbench closed, run the compile test:

```
powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 -Compile
```

About 3 s, it compiles the scripts of the whole modset and stops before the world. On failure, read the errors
(file and line) and say which addon they come from. Run `-Smoke` (about 30 s on Arland) only if the user asks or a
change touches world loading or the game mode. If the compile test cannot run (Workbench open, script missing),
say so in the report instead of skipping silently.

### 6b. Conflicts between addons (`conflicts`)

Run `conflicts`. The load order is an admin choice: it is resolved from the server's mod list
(`GDC_AR_Missions/_ServerConfigs/addons.json`; `--server-config <mission addons.json>` for one mission), the way the
game does it (list order, each mod preceded by its dependencies not loaded yet). Never take it from the MissionTests
log: that list has its own order. The global list is kept in the order of the GDC rule (`load_order` in config.json:
terrain, mission mods, GDC Core, GDC AR Lib, GDC Equipment, AFI bridge, mission addon); `order` checks any list
against it (`--apply` rewrites the repo's lists), and `after` reports rule breaks among its checks. It writes `conflicts.json` and adds it to report.json. Every supervised addon counts, GDC's projects included; conflicts
inside one family of addons made by the same authors (`families` in config.json) are left out. Items not seen at the
last supervision have `"new": true`.
- **Methods**: the same method modified by several addons where at least one does not call `super`. `never_run`
  lists the addons whose version is dead because a later one replaces the method without calling it. That is the
  case to analyse: read both versions (and vanilla's) and say what the dead version did that is now lost, as done for
  ACE Medical vs Reforger Lobby (`UpdateConsciousness`, harmless with ACE defaults, see the memories). A conditional
  `super` call counts as a call: the detection is textual.
- **Files**: `same property` lists the properties two addons set differently in the same prefab or config (the last
  loaded wins); `replaced whole` lists files (materials, particles, animation graphs, scripts...) where the last
  loaded addon's copy replaces the others. Ballistics, damage, medical and animation files matter; cosmetic ones
  (particle materials, tracers) get one grouped line.
- **Order advice** (`advice` of each method conflict): when only one addon skips `super`, loading it first keeps every
  override (`satisfied` says whether the list already does it; `brought_by` names the list entry to move). When two
  or more skip it, no order keeps them all: say what each version does, which one the current order runs, and
  propose either the order that keeps the most important behaviour or a compatibility patch. Propose list changes,
  never edit the server lists without the user's go-ahead.
- A conflict GDC itself creates on purpose (GDC Equipment retuning a Chungus rocket) is expected: say so.

Analyse new conflicts in depth; known ones get one line unless an update changed one of the addons involved.

### 7. Follow-ups

Look in `BACKLOG.md` and in the project memories for the names of the changed addons (and their short names: CSI,
TILW, RL, WCS, ACE, RHS...). An update of an addon tied to an open issue is the moment to re-check it: list these
items in the report ("à revérifier").

### 8. Report page

Write the page in French for the GDC admins: plain words, short, verdict first. They are not necessarily
developers: explain a technical risk by its effect in game. Structure:

1. **Verdict**: one line (rien à signaler / à vérifier / cassé), the date, the previous supervision date.
2. **Addons mis à jour**: one block per addon, GDC impact first, then the summary and its source.
   Minor updates grouped in a table.
3. **Vérifications**: compile test result, check issues, broken references.
4. **Conflits entre mods**: new ones first with their effect in game, then the known ones in a short table.
5. **À revérifier**: follow-ups from step 7.
6. **Périmètre**: the full list of supervised addons with versions, collapsed.
7. **Historique**: one line per previous supervision (date, verdict), carried over from the previous page.

Save the page as `Tools/ModSync/supervision/report.html` and publish it with the Artifact tool:
- first time (no `artifact_url` in state.json): follow the Artifact tool's procedure for a new page;
- next times: read the page at `artifact_url` first (it gives the history to carry over), then publish
  `report.html` to that same URL, so the admins keep one link.

### 9. Save

Run `save --artifact-url <url>`: it records the current versions, the conflicts seen and the page URL in state.json. Then tell the user
what to commit (state.json, report.html, sources.md if updated); commit only when asked.

## Preventive supervision of a candidate addon

When the user considers adding an addon to the modset ("ce mod est-il compatible", "on pourrait intégrer X"),
check it before it goes in. This is the user's own decision aid: answer in the terminal, in French, publish nothing,
do not touch state.json or the admins' page (offer a page only if asked).

1. **Get it.** If the user is subscribed, it is in the game or Workbench addons folder. Otherwise download its
   latest version with the dedicated server procedure above (omit `version`), into
   `Documents/My Games/GDC_ModSupervision/download/<GUID>_candidate/`; its dependencies come with it, next to it.
2. **Analyse**: `mod_supervision.py candidate "<addon folder>"`. It lists the dependencies (already in the modset,
   new, missing), the footprint of the candidate and of each new dependency (scripts, modded classes, base game files
   it replaces or modifies) and the conflicts it would create with the modset, GDC included.
3. **Compile with it**: the command prints the compile test line with `-ExtraAddonsDir` and `-ExtraAddons`. Run it
   (Workbench closed), then `candidate` again: the load order is then known and each conflict says who wins.
4. **Judge** for the user:
   - conflicts first: an override the candidate kills, or one of ours it would make dead, is a blocker to analyse
     like in step 6b;
   - its footprint against what GDC wants: a mod that changes base game vehicles, the map, name tags, medical or
     the HUD affects every mission. Check it against the GDC rules in the memories (no player icons on the map,
     unconscious state hidden, CSI tags only...);
   - its weight: new dependencies, their size, whether they are maintained (recent Workshop versions, repo);
   - a verdict: can go in, can go in with these checks in game, or not as is.
5. **Clean up**: delete the download folder and `Documents/My Games/GDCModFetch`.

## Rules

- Diagnose, never fix. Breakages and fixes are discussed with the user afterwards.
- Never close or launch the game or the Workbench.
- Keep the report factual: no guessing what an undocumented update "probably" does without saying it is a guess.

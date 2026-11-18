# Where each addon's changes can be read

Update this file whenever a supervision finds a better source (a repo, an empty changelog, a Discord-only author).

## Generic sources, in order of preference

0. **`installed_changelog`** in report.json: the changelog the author published with the installed version, read
   from its `ServerData.json`. Free and local, but it covers the newest version only. Sometimes it is a link to
   the real notes (ACE: a GitHub release URL), and often it is empty.
1. **Public Git repository** (GitHub mostly). `gh` is installed (`C:\Program Files\GitHub CLI\gh.exe`, call it by
   full path if the shell does not find it). Look for release commits or tags matching the two versions, then list
   the commits between them: `gh api "repos/<owner>/<repo>/compare/<old>...<new>"` gives the commits and the changed
   files. Unknown repo: `gh search repos "<addon name>"`, and check the Workshop description for a link.
2. **Workshop changelog**: `https://reforger.armaplatform.com/workshop/<GUID>-<anything>/changelog` (the GUID alone
   is enough for the site). Read it through the web-fetch agent and ask for the entries between the two versions,
   verbatim, with their dates. Many authors leave it empty: say so instead of guessing.
3. **Comparison of the kept copy** (`kept_files` / `compared_files` in report.json): the only source that never
   lies. Use it to confirm or complete 1 and 2, and alone when they are empty.

## Known addons

| Addon | GUID | Best source | Notes |
|---|---|---|---|
| COALITION Squad Interface | 5B0D1E4380971EBD | GitHub `CoalitionArma/Coalition-Squad-Interface`, branch `release` | No tags; commits named "Release x.y.z". Workshop changelog terse ("Next Major CSI Release!"). |
| WCS_Armaments, WCS_SpaceCore, WCS_JLTV, WCS_M2A2, WCS_Stryker, WCS_Armbands | 629B2BA37EFFD577 (Armaments), 5E389BB9F58B79A6 (SpaceCore), 5C721177A220B42F (JLTV), 63120AE07E6C0966 (M2A2), 5B02128D896F7DE8 (Stryker) | Kept copy only | No public repo; Workshop changelogs all empty (checked 2026-11-18); author announces on Discord (discord.gg/armawcs). Armaments 9.0.1 fetched from the Workshop: 424 MB, 23 s, no dependency pulled. |
| Zagoria 89 Launchers | 61D8DE60644F9103 | Workshop changelog | Short but real entries. 1.0.55 added TALOS (696AFA6207142000) as a dependency, declared only in its addon.gproj. |
| Chungus Launchers | 6190F1B505C08562 | Workshop changelog + kept copy | Changelog terse ("Reduced mod size", "Internal cleanup"). |
| ACE (24 "… Dev" addons, e.g. ACE Core Dev 65AD7D0D9941A380) | see `ServerData.json` | GitHub `acemod/ACE-Anvil`, releases `v1.5.0-devNN` (addon version 1.5.NN) | All modules share one version and one release. Project file is `dev.gproj`, not `addon.gproj`. A module can be one patch ahead of the last GitHub release (Core and Medical Breathing 1.5.41 while the last release is dev40): then compare commits on `master` after the release tag. ACE modifies many vanilla classes (medical, damage, character): GDC exposure is high, look closely at every ACE update. |
| GDC AR Core, GDC Equipment, GDC AR Lib | 663710CD8BEB01D4, 663E97A021C36ABF, 690756F509F253EB | `CHANGELOG.md` at the repo root, `git log` of the addon folder | Our own addons: never compared file by file. |

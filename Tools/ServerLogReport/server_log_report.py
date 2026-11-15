"""Build a standalone HTML report from an Arma Reforger console.log.

The report shows the server FPS over time (with the game state changes),
the players and AI counts, the player deaths (victim, shooter, distance,
last damage) and the addons the server actually loaded.

Usage:
    python server_log_report.py <console.log or logs folder> [--out report.html] [--open]
"""

from __future__ import annotations

import argparse
import datetime as dt
import html
import json
import re
import sys
import webbrowser
from pathlib import Path

WORKSHOP_URL = "https://reforger.armaplatform.com/workshop/"
BASE_GAME_GUIDS = {"58D0FB3206B6F859", "5614BBCCBB55ED1C", "6566A2D70498DA6B"}

RE_TIME = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3})")
RE_STARTED = re.compile(r"started at (\d{4}-\d{2}-\d{2}) (\d{2}:\d{2}:\d{2})")
RE_VERSION = re.compile(r"Creating game instance\([^)]*\), version ([\d.]+)")
RE_CONFIG = re.compile(r"-config\s+(\S+)")
RE_MISSION = re.compile(r"for mission '(\{[0-9A-F]{16}\})([^']+)'")
RE_SCENARIO = re.compile(r"SCRIPT\s+: (\{[0-9A-F]{16}\})\S+ \((.+)\)\s*$")
RE_GPROJ = re.compile(r"gproj: '([^']+)' guid: '([0-9A-F]{16})'")
RE_PACKAGE = re.compile(r"Adding package '([^']+?)/?' .* under name (\S+)")
RE_FPS = re.compile(r"DEFAULT\s+: FPS: ([\d.]+).*?Player: (\d+), AI: (\d+), AIChar: (\d+)")
RE_FREEZE = re.compile(r"Freezes\(cnt: \d+, max: ([\d.]+)")
RE_STATE = re.compile(r"OnGameStateChanged = (\w+)")
RE_AUTH = re.compile(r"Authenticated player: (?:.*?\bname=)?(.+?)\s*$")

# Player deaths, written by the base game (SCR_BaseGameMode.LogKillEvent) on the server.
_IDENTITY = r"(?P<{p}name>.+?) \(playerID = (?P<{p}id>\d+) \| UUID = (?P<{p}uuid>[^)]*)\)"
RE_KILL = re.compile(
    r"(?P<level>INFO|WARNING): KILL (?P<kind>[A-Z_]+): "
    + _IDENTITY.format(p="v")
    + r"(?: from (?P<vfac>\S+) faction)?"
    r"(?: at <(?P<vpos>[^>]+)>)?"
    r"(?P<disguised> while being disguised as enemy)?"
    r"(?: (?P<suicide>killed himself!)"
    r"| was killed by AI(?: from (?P<aifac>\S+) faction)?"
    r"| was killed by " + _IDENTITY.format(p="k") + r"(?: from (?P<kfac>\S+) faction)?)"
    r"(?: who was at that time at <(?P<kpos>[^>]+)> \[(?P<dist>[\d.]+)m away from the corpse\]\.)?"
    r"(?P<nosurface> No surface found within 1m under that position!)?"
    r"(?: With last inflicted damage type (?P<dmg>\w+)(?: to the '(?P<zone>[^']*)' hit zone)?)?\s*$"
)

STATE_LABELS = {
    "PREGAME": "Avant-partie",
    "SLOTSELECTION": "Choix des slots",
    "BRIEFING": "Briefing",
    "GAME": "Début de partie",
    "DEBRIEFING": "Débriefing",
    "POSTGAME": "Fin de partie",
}


def parse_vector(text: str | None) -> list[float] | None:
    """'<x, y, z>' content to [x, y, z] (y is the altitude)."""
    if not text:
        return None
    try:
        return [float(v) for v in text.split(",")]
    except ValueError:
        return None


def parse_kill(line: str, t: str) -> dict | None:
    m = RE_KILL.search(line)
    if not m:
        return None
    g = m.groupdict()
    if g["suicide"]:
        killer = {"type": "self", "name": g["vname"], "faction": g["vfac"]}
    elif g["kname"]:
        killer = {"type": "player", "name": g["kname"], "id": int(g["kid"]), "faction": g["kfac"]}
    else:
        killer = {"type": "ai", "name": None, "faction": g["aifac"]}
    killer["pos"] = parse_vector(g["kpos"])
    return {
        "t": t,
        "kind": g["kind"],
        "victim": {"name": g["vname"], "id": int(g["vid"]), "faction": g["vfac"], "pos": parse_vector(g["vpos"])},
        "killer": killer,
        "dist": float(g["dist"]) if g["dist"] else None,
        "dmg": g["dmg"],
        "zone": g["zone"],
        "disguised": bool(g["disguised"]),
        "noSurface": bool(g["nosurface"]),
    }


def find_log(path: Path) -> Path:
    """Accept a console.log file or a logs_YYYY-... folder holding one."""
    if path.is_dir():
        candidate = path / "console.log"
        if candidate.is_file():
            return candidate
        sys.exit(f"Aucun console.log dans {path}")
    if not path.is_file():
        sys.exit(f"Fichier introuvable : {path}")
    return path


def parse_log(path: Path) -> dict:
    start_date = None
    version = config = mission_guid = mission_path = None
    scenarios: dict[str, str] = {}
    blocks: list[list[tuple[str, str]]] = []
    in_loaded = False
    package_names: dict[str, str] = {}
    samples = []
    states = []
    players: list[str] = []
    kills: list[dict] = []
    unparsed_kills = 0

    day_offset = 0
    last_seconds = -1

    def stamp(line: str):
        """Return an ISO timestamp for the line, handling midnight rollover."""
        nonlocal day_offset, last_seconds
        m = RE_TIME.match(line)
        if not m or start_date is None:
            return None
        h, mi, s, ms = (int(x) for x in m.groups())
        seconds = h * 3600 + mi * 60 + s
        if last_seconds - seconds > 12 * 3600:
            day_offset += 1
        last_seconds = seconds
        moment = dt.datetime.combine(start_date + dt.timedelta(days=day_offset), dt.time(h, mi, s, ms * 1000))
        return moment.isoformat(timespec="seconds")

    with path.open(encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if start_date is None:
                m = RE_STARTED.search(line)
                if m:
                    start_date = dt.date.fromisoformat(m.group(1))
                continue

            if "Loaded addons:" in line:
                blocks.append([])
                in_loaded = True
                continue
            if in_loaded:
                m = RE_GPROJ.search(line)
                if m:
                    blocks[-1].append((m.group(1), m.group(2)))
                    continue
                in_loaded = False

            m = RE_PACKAGE.search(line)
            if m:
                package_names[m.group(1).rstrip("/")] = m.group(2)
                continue

            if "DEFAULT      : FPS:" in line:
                m = RE_FPS.search(line)
                t = stamp(line)
                if m and t:
                    z = RE_FREEZE.search(line)
                    samples.append([t, float(m.group(1)), int(m.group(2)), int(m.group(4)), float(z.group(1)) if z else 0.0])
                continue

            if ": KILL " in line:
                t = stamp(line)
                kill = parse_kill(line, t) if t else None
                if kill:
                    kills.append(kill)
                elif t:
                    unparsed_kills += 1
                continue

            m = RE_STATE.search(line)
            if m:
                t = stamp(line)
                if t:
                    states.append({"t": t, "state": m.group(1), "label": STATE_LABELS.get(m.group(1), m.group(1))})
                continue

            m = RE_AUTH.search(line)
            if m:
                stamp(line)
                if m.group(1) not in players:
                    players.append(m.group(1))
                continue

            if version is None:
                m = RE_VERSION.search(line)
                if m:
                    version = m.group(1)
            if config is None and "CLI Params" in line:
                m = RE_CONFIG.search(line)
                if m:
                    config = Path(m.group(1)).name
            if mission_guid is None:
                m = RE_MISSION.search(line)
                if m:
                    mission_guid, mission_path = m.group(1), m.group(2)
            m = RE_SCENARIO.search(line)
            if m:
                scenarios[m.group(1)] = m.group(2)
            stamp(line)

    if start_date is None:
        sys.exit("Ce fichier ne ressemble pas à un console.log d'Enfusion (pas de ligne « started at »).")

    # The last "Loaded addons" block is the one the game ran with.
    mods = []
    for i, (gproj, guid) in enumerate(blocks[-1] if blocks else [], start=1):
        folder = str(Path(gproj).parent).replace("\\", "/")
        base = guid in BASE_GAME_GUIDS
        folder_name = Path(folder).name
        name = Path(gproj).stem if base else re.sub(r"_[0-9A-F]{16}$", "", folder_name)
        project_id = next((v for k, v in package_names.items() if k.rstrip("/").endswith(folder_name)), "")
        mods.append({"n": i, "name": name, "id": project_id, "guid": guid, "base": base})

    mission_name = None
    if mission_guid:
        mission_name = scenarios.get(mission_guid) or Path(mission_path).stem

    return {
        "start": start_date.isoformat(),
        "version": version,
        "config": config,
        "mission": mission_name,
        "missionPath": f"{mission_guid}{mission_path}" if mission_guid else None,
        "samples": samples,
        "states": states,
        "players": players,
        "kills": kills,
        "unparsedKills": unparsed_kills,
        "mods": mods,
    }


def build_html(data: dict) -> str:
    day = dt.date.fromisoformat(data["start"]).strftime("%d/%m/%Y")
    title = f"{data['mission'] or 'Session'} du {day}"
    payload = json.dumps(data, ensure_ascii=False).replace("</", "<\\/")
    return TEMPLATE.replace("__TITLE__", html.escape(title)).replace("__DATA__", payload)


TEMPLATE = r"""<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>__TITLE__</title>
<meta name="claude:viewport-fit-sides" content="cover">
<style>
  :root {
    --bg: #f6f7f9; --panel: #ffffff; --fg: #1d2430; --muted: #5d6676; --line: #dfe3ea;
    --grid: #e8ebf0; --shade: #eef1f5; --ref: #8a93a3;
    --c0: #2f6fdb; --c1: #d9622b; --bad: #c4352b;
    color-scheme: light;
  }
  @media (prefers-color-scheme: dark) {
    :root:not([data-theme="light"]) {
      --bg: #12151b; --panel: #1a1f27; --fg: #e6e9ef; --muted: #9aa3b2; --line: #2c333e;
      --grid: #252b35; --shade: #20262f; --ref: #7d8696;
      --c0: #6b9cf0; --c1: #ef8a57; --bad: #ef6b60;
      color-scheme: dark;
    }
  }
  :root[data-theme="dark"] {
    --bg: #12151b; --panel: #1a1f27; --fg: #e6e9ef; --muted: #9aa3b2; --line: #2c333e;
    --grid: #252b35; --shade: #20262f; --ref: #7d8696;
    --c0: #6b9cf0; --c1: #ef8a57; --bad: #ef6b60;
    color-scheme: dark;
  }
  * { box-sizing: border-box; }
  body { margin: 0; background: var(--bg); color: var(--fg); font: 14px/1.45 system-ui, "Segoe UI", Roboto, sans-serif; }
  main { max-width: 1400px; margin: 0 auto; padding-block: 24px 48px;
         padding-left: max(16px, 3vw, env(safe-area-inset-left, 0px)); padding-right: max(16px, 3vw, env(safe-area-inset-right, 0px)); display: grid; gap: 20px; }
  h1 { font-size: 1.6rem; font-weight: 600; margin: 0; text-wrap: balance; }
  h2 { font-size: 1.05rem; font-weight: 600; margin: 0; }
  .meta { color: var(--muted); margin: 4px 0 0; }
  .take { margin: 6px 0 0; max-width: 75ch; }
  .tiles { display: grid; grid-template-columns: repeat(auto-fit, minmax(min(100%, 11rem), 1fr)); gap: 12px; }
  .tile, .card { background: var(--panel); border: 1px solid var(--line); border-radius: 8px; padding: 16px; min-width: 0; }
  .tile .lbl, .note, .sub { color: var(--muted); font-size: .8rem; }
  .tile .val { font-size: 1.9rem; font-variant-numeric: tabular-nums; margin-top: 2px; }
  .note { margin: 4px 0 12px; }
  .chart { position: relative; }
  .chart svg { display: block; width: 100%; height: auto; overflow: visible; }
  .chart text { fill: var(--muted); font-size: 11px; font-family: inherit; }
  .legend { display: flex; flex-wrap: wrap; gap: 16px; font-size: .8rem; color: var(--muted); margin-bottom: 8px; }
  .legend i { display: inline-block; width: 14px; height: 2px; vertical-align: middle; margin-right: 6px; }
  .tip { position: absolute; top: 8px; pointer-events: none; background: var(--panel); border: 1px solid var(--line); border-radius: 6px;
         box-shadow: 0 4px 14px rgba(0,0,0,.18); padding: 6px 10px; font-size: .8rem; font-variant-numeric: tabular-nums; white-space: nowrap; }
  .tip .t { color: var(--muted); }
  .tip .sw { display: inline-block; width: 8px; height: 8px; border-radius: 50%; margin-right: 5px; }
  .mods-head { display: flex; flex-wrap: wrap; gap: 12px; align-items: baseline; justify-content: space-between; }
  .mods-head input { font: inherit; padding: 5px 9px; border: 1px solid var(--line); border-radius: 6px; background: var(--bg); color: var(--fg); min-width: 0; width: 16rem; max-width: 100%; }
  .scroll { overflow: auto; max-height: 70vh; margin-top: 10px; }
  table { border-collapse: collapse; width: 100%; font-variant-numeric: tabular-nums; }
  th, td { text-align: left; padding: 5px 10px; border-bottom: 1px solid var(--line); }
  th { position: sticky; top: 0; background: var(--panel); color: var(--muted); font-weight: 500; font-size: .8rem; }
  td.n { color: var(--muted); width: 3rem; }
  td.g { font-family: ui-monospace, Consolas, monospace; font-size: .8rem; }
  a { color: var(--c0); }
  .tag { font-size: .72rem; color: var(--muted); border: 1px solid var(--line); border-radius: 4px; padding: 0 5px; margin-left: 6px; }
  .players { color: var(--muted); font-size: .8rem; margin-top: 8px; }
  .kind { font-size: .75rem; border-radius: 4px; padding: 1px 6px; border: 1px solid var(--line); white-space: nowrap; }
  .kind.tk { color: var(--bad); border-color: var(--bad); font-weight: 600; }
  td.warn { color: var(--muted); font-size: .8rem; min-width: 16rem; }
  td.warn .w { color: var(--bad); }
  td.c { font-family: ui-monospace, Consolas, monospace; font-size: .78rem; white-space: nowrap; }
  td.r, th.r { text-align: right; }
  tr.tk td { background: color-mix(in srgb, var(--bad) 8%, transparent); }
  .kills h3 { font-size: .9rem; font-weight: 600; margin: 18px 0 0; }
  .death-mark { cursor: help; }
</style>
</head>
<body>
<main>
  <header>
    <h1 id="title"></h1>
    <p class="meta" id="meta"></p>
    <p class="take" id="take"></p>
  </header>
  <section class="tiles" id="tiles"></section>
  <section class="card">
    <h2>FPS du serveur</h2>
    <p class="note">Un relevé toutes les 5 secondes. La zone grisée précède le début de la partie ; les traits pointillés marquent les changements de phase.</p>
    <div class="chart" id="fps"></div>
  </section>
  <section class="card">
    <h2>Joueurs et IA présents</h2>
    <p class="note">Même échelle de temps que le graphique du dessus.</p>
    <div class="legend"><span><i style="background:var(--c0)"></i>Personnages IA</span><span><i style="background:var(--c1)"></i>Joueurs</span><span id="death-legend" hidden><i style="background:var(--bad);width:3px;height:10px"></i>Mort d'un joueur</span></div>
    <div class="chart" id="load"></div>
    <p class="players" id="players"></p>
  </section>
  <section class="card kills">
    <h2 id="kills-title">Morts de joueurs</h2>
    <p class="note">Écrites par le jeu de base sur le serveur, une ligne par joueur tué (les morts d'IA ne sont pas journalisées). Le tireur IA n'est connu que par sa faction et sa position. Le « dernier dégât » est le dernier dégât infligé par ce tireur, pas forcément le coup mortel. Positions : X, Z (altitude).</p>
    <div id="kills-body"></div>
  </section>
  <section class="card">
    <div class="mods-head">
      <h2 id="mods-title">Mods chargés</h2>
      <input id="mods-filter" type="search" placeholder="Filtrer les mods" aria-label="Filtrer les mods">
    </div>
    <p class="note">Dans l'ordre de chargement du serveur. Le log ne donne pas les versions.</p>
    <div class="scroll">
      <table>
        <thead><tr><th>#</th><th>Mod</th><th>ID du projet</th><th>GUID</th></tr></thead>
        <tbody id="mods"></tbody>
      </table>
    </div>
  </section>
</main>
<script>
const D = __DATA__;
const NS = 'http://www.w3.org/2000/svg';
const $ = (id) => document.getElementById(id);
const fmt1 = (v) => v.toLocaleString('fr-FR', { minimumFractionDigits: 1, maximumFractionDigits: 1 });
const hhmm = (d) => d.toTimeString().slice(0, 5);
const hhmmss = (d) => d.toTimeString().slice(0, 8);
const rows = D.samples.map(([t, fps, players, ai, freeze]) => ({ t: new Date(t), fps, players, ai, freeze }));
const gameState = D.states.filter((s) => s.state === 'GAME').map((s) => new Date(s.t))[0] || null;
const kills = D.kills.map((k) => ({ ...k, t: new Date(k.t) }));

const KIND_LABELS = { ENEMY: 'Ennemi', TK: 'Tir ami', GM: 'Game Master', SUICIDE: 'Suicide' };
const DMG_LABELS = {
  KINETIC: 'balle', FRAGMENTATION: 'éclats', EXPLOSIVE: 'explosion', INCENDIARY: 'feu', FIRE: 'feu',
  COLLISION: 'collision', MELEE: 'corps à corps', BLEEDING: 'saignement', FALL: 'chute', DROWNING: 'noyade',
  HEALING: 'soin', REGENERATION: 'régénération', ACE_MEDICAL_HYPOXIA: 'hypoxie (ACE)',
};
const ZONE_LABELS = {
  Head: 'tête', Neck: 'cou', Chest: 'poitrine', Abdomen: 'abdomen', Hips: 'bassin',
  LArm: 'bras G.', RArm: 'bras D.', LForearm: 'avant-bras G.', RForearm: 'avant-bras D.', LHand: 'main G.', RHand: 'main D.',
  LThigh: 'cuisse G.', RThigh: 'cuisse D.', LCalf: 'mollet G.', RCalf: 'mollet D.', LFoot: 'pied G.', RFoot: 'pied D.',
  ACE_Medical_Brain: 'cerveau (ACE)',
};
const kindLabel = (kind) => KIND_LABELS[kind] || kind;
function killerLabel(k) {
  const fac = k.killer.faction ? ` (${k.killer.faction})` : '';
  if (k.killer.type === 'self') return 'lui-même';
  if (k.killer.type === 'ai') return `IA${fac}`;
  return k.killer.name + fac;
}

$('title').textContent = `Rapport de partie : ${D.mission || 'session'}`;
const startDate = new Date(D.start + 'T00:00:00').toLocaleDateString('fr-FR', { weekday: 'long', day: 'numeric', month: 'long', year: 'numeric' });
$('meta').textContent = [startDate, rows.length ? `${hhmm(rows[0].t)} → ${hhmm(rows[rows.length - 1].t)}` : null,
  D.version ? `jeu ${D.version}` : null, D.config ? `config ${D.config}` : null].filter(Boolean).join(' · ');

const inGame = gameState ? rows.filter((r) => r.t >= gameState) : rows;
const scope = gameState ? 'en partie' : 'sur la session';
function el(tag, attrs, parent) { const e = document.createElementNS(NS, tag); for (const k in attrs) e.setAttribute(k, attrs[k]); if (parent) parent.append(e); return e; }

if (inGame.length) {
  const mean = inGame.reduce((s, r) => s + r.fps, 0) / inGame.length;
  const low = inGame.reduce((a, r) => (r.fps < a.fps ? r : a));
  const aiMax = Math.max(...rows.map((r) => r.ai));
  const plMax = Math.max(...rows.map((r) => r.players));
  const worstFreeze = rows.reduce((a, r) => (r.freeze > a.freeze ? r : a));
  $('take').textContent = `Le serveur tient ${fmt1(mean)} FPS en moyenne ${scope}, avec un minimum de ${fmt1(low.fps)} FPS à ${hhmm(low.t)}.`;
  const tiles = [
    [`FPS moyen ${scope}`, fmt1(mean), gameState ? `à partir de ${hhmm(gameState)}` : ''],
    [`FPS le plus bas ${scope}`, fmt1(low.fps), `à ${hhmm(low.t)}`],
    ['Personnages IA au maximum', String(aiMax), 'sur la session'],
    ['Joueurs au maximum', String(plMax), `${D.players.length} joueurs différents`],
    ['Gel le plus long', worstFreeze.freeze ? `${Math.round(worstFreeze.freeze)} ms` : 'aucun', worstFreeze.freeze ? `à ${hhmm(worstFreeze.t)}` : ''],
  ];
  for (const [l, v, s] of tiles) {
    const d = document.createElement('div'); d.className = 'tile';
    for (const [c, txt] of [['lbl', l], ['val', v], ['sub', s]]) { const p = document.createElement('div'); p.className = c; p.textContent = txt; d.append(p); }
    $('tiles').append(d);
  }
}
$('players').textContent = D.players.length ? `Joueurs vus : ${D.players.join(', ')}.` : '';

// ---- Player deaths ----
function td(text, cls) { const c = document.createElement('td'); if (cls) c.className = cls; if (text instanceof Node) c.append(text); else c.textContent = text ?? ''; return c; }
function th(text, cls) { const c = document.createElement('th'); if (cls) c.className = cls; c.textContent = text; return c; }
function table(headers, rightCols) {
  const t = document.createElement('table'); const head = document.createElement('tr');
  headers.forEach((h, i) => head.append(th(h, rightCols.includes(i) ? 'r' : '')));
  const thead = document.createElement('thead'); thead.append(head); const tbody = document.createElement('tbody');
  t.append(thead, tbody); const wrap = document.createElement('div'); wrap.className = 'scroll'; wrap.append(t);
  return { wrap, tbody };
}
const pos = (p) => p ? `${Math.round(p[0])}, ${Math.round(p[2])} (${Math.round(p[1])} m)` : '';
const fr1 = (v) => v == null ? '' : `${fmt1(v)} m`;
function remarks(k) {
  const out = [];
  if (k.dmg === 'HEALING' || k.dmg === 'REGENERATION')
    out.push(['w', 'Cause faussée : un soin est pris pour le dernier dégât (bug du jeu de base). Le tireur reste juste.']);
  if (k.dmg === 'ACE_MEDICAL_HYPOXIA') out.push(['', 'Mort différée (ACE) : le tireur est celui qui a blessé, la blessure n\'est pas connue.']);
  if (k.dmg === 'BLEEDING') out.push(['', 'Mort par saignement : le tireur est le dernier à avoir blessé.']);
  if (k.noSurface) out.push(['w', 'Aucun sol sous le tireur au moment du tir : à vérifier.']);
  if (k.disguised) out.push(['', 'Victime déguisée en ennemi.']);
  if (k.killer.type !== 'self' && k.kind !== 'GM' && !k.killer.pos) out.push(['', 'Tueur sans position (véhicule, mine, explosion ?).']);
  return out;
}

if (kills.length) {
  const tk = kills.filter((k) => k.kind === 'TK');
  const byAi = kills.filter((k) => k.killer.type === 'ai').length;
  const d = document.createElement('div'); d.className = 'tile';
  for (const [c, txt] of [['lbl', 'Morts de joueurs'], ['val', String(kills.length)], ['sub', tk.length ? `dont ${tk.length} tir${tk.length > 1 ? 's' : ''} ami${tk.length > 1 ? 's' : ''}` : 'aucun tir ami']]) {
    const p = document.createElement('div'); p.className = c; p.textContent = txt; d.append(p);
  }
  $('tiles').append(d);
  $('death-legend').hidden = false;
  $('kills-title').textContent = `Morts de joueurs (${kills.length})`;
  const tkText = tk.length ? `${tk.length} par tir ami (${tk.map((k) => `${k.killer.name} sur ${k.victim.name}`).join(', ')})` : 'aucune par tir ami';
  $('take').textContent += ` ${kills.length} mort${kills.length > 1 ? 's' : ''} de joueur${kills.length > 1 ? 's' : ''}, dont ${byAi} par l'IA et ${tkText}.`;

  // Per player recap
  const stats = new Map();
  const S = (name) => { if (!stats.has(name)) stats.set(name, { deaths: 0, byAi: 0, tkSuffered: 0, tkDone: 0, pvp: 0, suicide: 0 }); return stats.get(name); };
  for (const k of kills) {
    const v = S(k.victim.name); v.deaths++;
    if (k.killer.type === 'ai') v.byAi++;
    if (k.kind === 'SUICIDE') v.suicide++;
    if (k.kind === 'TK') { v.tkSuffered++; if (k.killer.type === 'player') S(k.killer.name).tkDone++; }
    if (k.kind === 'ENEMY' && k.killer.type === 'player') S(k.killer.name).pvp++;
  }
  const h3a = document.createElement('h3'); h3a.textContent = 'Par joueur';
  const recap = table(['Joueur', 'Morts', 'Tué par l\'IA', 'Tir ami subi', 'Suicide', 'Tir ami commis', 'Joueurs ennemis tués'], [1, 2, 3, 4, 5, 6]);
  [...stats.entries()].sort((a, b) => b[1].deaths - a[1].deaths || a[0].localeCompare(b[0])).forEach(([name, s]) => {
    const tr = document.createElement('tr');
    tr.append(td(name), ...[s.deaths, s.byAi, s.tkSuffered, s.suicide, s.tkDone, s.pvp].map((v) => td(v ? String(v) : '·', 'r')));
    recap.tbody.append(tr);
  });

  const h3b = document.createElement('h3'); h3b.textContent = 'Chronologie';
  const chrono = table(['Heure', 'Victime', 'Type', 'Tireur', 'Distance', 'Dernier dégât', 'Zone', 'Position victime', 'Position tireur', 'Remarques'], [4]);
  for (const k of kills) {
    const tr = document.createElement('tr'); if (k.kind === 'TK') tr.className = 'tk';
    const badge = document.createElement('span'); badge.className = 'kind' + (k.kind === 'TK' ? ' tk' : ''); badge.textContent = kindLabel(k.kind);
    const dmg = document.createElement('span'); dmg.textContent = k.dmg ? (DMG_LABELS[k.dmg] || k.dmg) : ''; if (k.dmg) dmg.title = k.dmg;
    const zone = document.createElement('span'); zone.textContent = k.zone ? (ZONE_LABELS[k.zone] || k.zone) : ''; if (k.zone) zone.title = k.zone;
    const rem = document.createElement('div');
    for (const [cls, txt] of remarks(k)) { const p = document.createElement('div'); if (cls) p.className = cls; p.textContent = txt; rem.append(p); }
    tr.append(td(hhmmss(k.t)), td(`${k.victim.name}${k.victim.faction ? ` (${k.victim.faction})` : ''}`), td(badge), td(killerLabel(k)),
      td(fr1(k.dist), 'r'), td(dmg), td(zone), td(pos(k.victim.pos), 'c'), td(pos(k.killer.pos), 'c'), td(rem, 'warn'));
    chrono.tbody.append(tr);
  }
  $('kills-body').append(h3a, recap.wrap, h3b, chrono.wrap);
} else {
  $('kills-body').textContent = 'Aucune mort de joueur dans ce log.';
}
if (D.unparsedKills) {
  const p = document.createElement('p'); p.className = 'note';
  p.textContent = `${D.unparsedKills} ligne(s) « KILL » n'ont pas pu être lues : leur format a peut-être changé avec une mise à jour du jeu.`;
  $('kills-body').append(p);
}

function chart(box, height, series, yMax, tipLines) {
  box.replaceChildren();
  if (!rows.length) { box.textContent = 'Aucun relevé de FPS dans ce log.'; return; }
  const W = Math.max(320, box.clientWidth), H = height, top = 22, bottom = 24;
  const niceMax = Math.ceil(yMax / 10) * 10 || 10;
  const ticks = []; const step = niceMax <= 60 ? 10 : Math.ceil(niceMax / 6 / 10) * 10;
  for (let v = 0; v <= niceMax; v += step) ticks.push(v);
  const svg = el('svg', { viewBox: `0 0 ${W} ${H}`, role: 'img' }, box);
  const probe = el('text', {}, svg); probe.textContent = String(ticks[ticks.length - 1]);
  const left = Math.ceil(probe.getComputedTextLength()) + 10; probe.remove();
  const t0 = rows[0].t.getTime(), t1 = rows[rows.length - 1].t.getTime();
  const x = (d) => left + (d.getTime() - t0) / (t1 - t0 || 1) * (W - 8 - left);
  const y = (v) => H - bottom - v / ticks[ticks.length - 1] * (H - bottom - top);
  if (gameState) el('rect', { x: left, y: top, width: Math.max(0, x(gameState) - left), height: H - bottom - top, fill: 'var(--shade)' }, svg);
  for (const v of ticks) {
    el('line', { x1: left, x2: W - 8, y1: y(v), y2: y(v), stroke: 'var(--grid)' }, svg);
    const tx = el('text', { x: left - 6, y: y(v), dy: '0.32em', 'text-anchor': 'end' }, svg); tx.textContent = v;
  }
  // time ticks every 15 or 30 minutes depending on width
  const span = (t1 - t0) / 60000, every = span / ((W - left) / 80) > 20 ? 30 : 15;
  const first = new Date(t0); first.setSeconds(0, 0); first.setMinutes(Math.ceil(first.getMinutes() / every) * every);
  for (let d = first; d.getTime() <= t1; d = new Date(d.getTime() + every * 60000)) {
    el('line', { x1: x(d), x2: x(d), y1: H - bottom, y2: H - bottom + 4, stroke: 'var(--ref)' }, svg);
    const tx = el('text', { x: x(d), y: H - 6, 'text-anchor': 'middle' }, svg); tx.textContent = hhmm(d);
  }
  el('line', { x1: left, x2: W - 8, y1: H - bottom, y2: H - bottom, stroke: 'var(--ref)' }, svg);
  let lastLabelEnd = -Infinity, row = 0;
  for (const s of D.states) {
    const d = new Date(s.t); if (d.getTime() < t0 || d.getTime() > t1) continue;
    el('line', { x1: x(d), x2: x(d), y1: top, y2: H - bottom, stroke: 'var(--ref)', 'stroke-dasharray': '4 3' }, svg);
    if (box.dataset.labels) {
      row = x(d) < lastLabelEnd ? (row + 1) % 2 : 0;
      const tx = el('text', { x: x(d) + 4, y: 10 + row * 12 }, svg); tx.textContent = s.label;
      lastLabelEnd = x(d) + 4 + tx.getComputedTextLength() + 6;
    }
  }
  for (const s of series) {
    const dpath = rows.map((r, i) => `${i ? 'L' : 'M'}${x(r.t).toFixed(1)},${y(r[s.key]).toFixed(1)}`).join('');
    el('path', { d: dpath, fill: 'none', stroke: s.color, 'stroke-width': 2, 'stroke-linejoin': 'round', 'stroke-linecap': 'round' }, svg);
  }
  const rule = el('line', { y1: top, y2: H - bottom, stroke: 'var(--ref)', opacity: 0 }, svg);
  const dots = series.map((s) => el('circle', { r: 4, fill: s.color, stroke: 'var(--panel)', 'stroke-width': 2, opacity: 0 }, svg));
  const tip = document.createElement('div'); tip.className = 'tip'; tip.hidden = true; box.append(tip);
  const hit = el('rect', { x: left, y: 0, width: W - 8 - left, height: H, fill: 'transparent' }, svg);
  hit.addEventListener('pointermove', (ev) => {
    const rect = svg.getBoundingClientRect();
    const mx = (ev.clientX - rect.left) / rect.width * W;
    const target = t0 + (mx - left) / (W - 8 - left) * (t1 - t0);
    let lo = 0, hi = rows.length - 1;
    while (hi - lo > 1) { const mid = (lo + hi) >> 1; if (rows[mid].t.getTime() < target) lo = mid; else hi = mid; }
    const r = Math.abs(rows[lo].t - target) < Math.abs(rows[hi].t - target) ? rows[lo] : rows[hi];
    rule.setAttribute('x1', x(r.t)); rule.setAttribute('x2', x(r.t)); rule.setAttribute('opacity', 1);
    series.forEach((s, i) => { dots[i].setAttribute('cx', x(r.t)); dots[i].setAttribute('cy', y(r[s.key])); dots[i].setAttribute('opacity', 1); });
    tip.replaceChildren();
    const t = document.createElement('div'); t.className = 't'; t.textContent = hhmmss(r.t); tip.append(t);
    for (const [color, label, value] of tipLines(r)) {
      const ln = document.createElement('div');
      if (color) { const sw = document.createElement('span'); sw.className = 'sw'; sw.style.background = color; ln.append(sw); }
      ln.append(`${label} : ${value}`); tip.append(ln);
    }
    tip.hidden = false;
    const px = x(r.t) / W * rect.width;
    tip.style.left = `${Math.min(Math.max(0, px + 12), box.clientWidth - tip.offsetWidth)}px`;
  });
  hit.addEventListener('pointerleave', () => { tip.hidden = true; rule.setAttribute('opacity', 0); dots.forEach((c) => c.setAttribute('opacity', 0)); });
  // Death marks sit above the hover area so their own tooltip shows.
  if (box.dataset.deaths) {
    for (const k of kills) {
      if (k.t.getTime() < t0 || k.t.getTime() > t1) continue;
      const g = el('g', { class: 'death-mark' }, svg);
      el('rect', { x: x(k.t) - 1.5, y: H - bottom - 12, width: 3, height: 12, fill: 'var(--bad)' }, g);
      el('rect', { x: x(k.t) - 5, y: H - bottom - 16, width: 10, height: 16, fill: 'transparent' }, g);
      el('title', {}, g).textContent = `${hhmmss(k.t)} — ${k.victim.name} : ${kindLabel(k.kind).toLowerCase()}, ${killerLabel(k)}`;
    }
  }
}

function draw() {
  $('fps').dataset.labels = '1';
  if (kills.length) $('load').dataset.deaths = '1';
  chart($('fps'), 300, [{ key: 'fps', color: 'var(--c0)' }], 60, (r) => [
    [null, 'FPS', fmt1(r.fps)], [null, 'Joueurs', r.players], [null, 'IA', r.ai],
    ...(r.freeze ? [[null, 'Gel max', `${Math.round(r.freeze)} ms`]] : []),
  ]);
  chart($('load'), 200, [{ key: 'ai', color: 'var(--c0)' }, { key: 'players', color: 'var(--c1)' }],
    Math.max(10, ...rows.map((r) => r.ai)), (r) => [['var(--c0)', 'Personnages IA', r.ai], ['var(--c1)', 'Joueurs', r.players]]);
}
draw();
let resizeTimer; window.addEventListener('resize', () => { clearTimeout(resizeTimer); resizeTimer = setTimeout(draw, 150); });

const tbody = $('mods');
const external = D.mods.filter((m) => !m.base).length;
$('mods-title').textContent = `Mods chargés (${external}, plus le jeu de base)`;
for (const m of D.mods) {
  const tr = document.createElement('tr');
  tr.dataset.search = `${m.name} ${m.id} ${m.guid}`.toLowerCase();
  const n = document.createElement('td'); n.className = 'n'; n.textContent = m.n;
  const name = document.createElement('td');
  if (m.base) { name.textContent = m.name; const tag = document.createElement('span'); tag.className = 'tag'; tag.textContent = 'jeu de base'; name.append(tag); }
  else { const a = document.createElement('a'); a.href = 'https://reforger.armaplatform.com/workshop/' + m.guid; a.target = '_blank'; a.rel = 'noopener'; a.textContent = m.name; name.append(a); }
  const id = document.createElement('td'); id.textContent = m.id;
  const g = document.createElement('td'); g.className = 'g'; g.textContent = m.guid;
  tr.append(n, name, id, g); tbody.append(tr);
}
$('mods-filter').addEventListener('input', (e) => {
  const q = e.target.value.trim().toLowerCase();
  for (const tr of tbody.children) tr.hidden = q && !tr.dataset.search.includes(q);
});
</script>
</body>
</html>
"""


def main() -> None:
    parser = argparse.ArgumentParser(description="Rapport HTML (FPS, joueurs, IA, morts, mods chargés) à partir d'un console.log de serveur Arma Reforger.")
    parser.add_argument("log", type=Path, help="console.log, ou dossier logs_AAAA-MM-JJ_... qui en contient un")
    parser.add_argument("--out", type=Path, help="fichier HTML à écrire (par défaut : à côté du log, même nom en .html)")
    parser.add_argument("--open", action="store_true", help="ouvrir le rapport dans le navigateur")
    args = parser.parse_args()

    log = find_log(args.log)
    data = parse_log(log)
    out = args.out or log.with_suffix(".html")
    out.write_text(build_html(data), encoding="utf-8")

    external = sum(1 for m in data["mods"] if not m["base"])
    print(f"Mission  : {data['mission'] or 'inconnue'}")
    print(f"Relevés  : {len(data['samples'])} points de FPS, {len(data['states'])} changements de phase")
    print(f"Mods     : {external} chargés (plus le jeu de base)")
    tk = sum(1 for k in data["kills"] if k["kind"] == "TK")
    print(f"Morts    : {len(data['kills'])} de joueurs, dont {tk} par tir ami"
          + (f" ({data['unparsedKills']} lignes KILL illisibles)" if data["unparsedKills"] else ""))
    print(f"Rapport  : {out}")
    if args.open:
        webbrowser.open(out.resolve().as_uri())


if __name__ == "__main__":
    main()

# Automated mission test: local dedicated server (Diag) + 2 clients, sources loaded from the repository.
# Usage: powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 [-Mission Davay] [-NoClients] [-Compile] [-Smoke]
#        [-ExtraAddonsDir <folders>] [-ExtraAddons <IDs or GUIDs>]
# Workbench must be closed. Results: PASS/FAIL/WARN lines, then the errors of the three logs, grouped.
param(
    [string]$Mission = 'Davay',
    [int]$TimeoutMinutes = 60,
    # Server only: checks the build and the start of the scenario without clients (stops at "Two players connected")
    [switch]$NoClients,
    # Server only: compiles the game scripts of the whole modset and stops before the world loads (a few seconds).
    # The WorkbenchGame module (Workbench plugins) is not compiled by the server.
    [switch]$Compile,
    # Server only, on an Arland world (fastest terrain to load): compiles, loads the world and starts a game, no scenario
    [switch]$Smoke,
    # Extra addons to load on top of the modset (a candidate addon and its dependencies, for instance):
    # folders containing them (comma separated) and their IDs or GUIDs (comma separated)
    [string]$ExtraAddonsDir = '',
    [string]$ExtraAddons = ''
)

$worlds = @{
    'Davay' = 'worlds/tanin69/_En cours/Davai_duy_na_Gruwice/Davay_duy_na_Gruwice.ent'
    'Uncon' = 'worlds/tanin69/_Tests/GDC_Teleport_Arland.ent'
}
$compileWorld = 'worlds/tanin69/_Tests/GDC_Teleport_Arland.ent'
if ($Compile -or $Smoke) {
    $world = $compileWorld
    $NoClients = $true
    $testArg = ''
}
else {
    if (-not $worlds.ContainsKey($Mission)) { "Unknown mission '$Mission'. Known: $($worlds.Keys -join ', ')"; exit 2 }
    $world = $worlds[$Mission]
    $testArg = "-gdcTest $Mission"
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path -replace '\\', '/'
$dirs = @(
    "$repo/GDC_AR_ModPack/GDC_AR_Core/",
    "$repo/GDC_AR_ModPack/GDC_AR_Equipment/",
    "$repo/GDC_AR_Misc/GDC_AR_Lib/",
    'C:/Dev/Enfusion/Zimnitrita_map/',
    "$repo/GDC_AR_Missions/GDC_Missions/",
    "$repo/Tools/MissionTests/GDC_MissionTests/",
    ((Join-Path $env:USERPROFILE 'Documents/My Games/ArmaReforgerWorkbench/addons') -replace '\\', '/')
) -join ','
$addons = 'ACE_Core_Dev,ACE_Backblast_Dev,ACE_Carrying_Dev,ACE_Chopping_Dev,ACE_Compass_Dev,ACE_Finger_Dev,ACE_Medical_Core_Dev,ACE_Medical_Hitzones_Dev,ACE_Medical_Circulation_Dev,ACE_Trenches_Dev,ACE_MagRepack_Dev,ACE_Explosives_Dev,ACE_Captives_Dev,ACE_TacticalLadder_Dev,ACE_Radio_Dev,ACE_TacticalPeriscope_Dev,ACE_Overheating_Dev,ACE_CookOff_Dev,ACE_Weather_Dev,ACE_Facepaint_Dev,ACE_Ballistics_Dev,ACE_Medical_Breathing_Dev,ACE_Scopes_Dev,ACE_AllInOne_Dev,CoalitionVON,CoalitionSquadInterface,FixedLoadCasualtyVehicles,Grays_Adjustable_Doors,ImprovedBloodEffect,RealismOverhaulBallistics,WCS_Armbands,Youcansit,GDC AR Core,ChungusLaunchers,MarkerRoundsProject,EchoesOf89UGLTextures,RHS_Content_01,RHS_Content_02,RHS_Core,FPTR,WCS_Armaments,WCS_SpaceCore,WCS_JLTV,WCS_M2A2,BTR70Mountable,RussianMarkerGrenades,LaserRangefinder,M4MAAWS,WCS_Stryker,Wirecutters2,WZLaunchers,ZU23,GDC AR Equipment,TilWMissionFramework,PSCoreDevelopment,ReforgerLobbyCommunity,AFI_CVON_RL_Compat,GDC AR Lib,MinusBuildingPack,Zimnitrita,GDC Missions,GDC Mission Tests'

if ($ExtraAddonsDir) { $dirs = "$dirs,$($ExtraAddonsDir -replace '\\', '/')" }
if ($ExtraAddons) { $addons = "$addons,$ExtraAddons" }

$serverDir = 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Server'
$gameDir = 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger'
$games = Join-Path $env:USERPROFILE 'Documents\My Games'
$common = "-addonsDir `"$dirs`" -addons `"$addons`" -rpl-timeout-disable -maxFPS 60"
$tag = '\[GDC_MissionTest\]'

if (Get-Process -Name 'ArmaReforgerWorkbench*' -ErrorAction SilentlyContinue) { 'Workbench is running: close it first.'; exit 2 }

function LatestLog($profile, $since) {
    $root = Join-Path $games "$profile\logs"
    if (-not (Test-Path $root)) { return $null }
    $d = Get-ChildItem $root -Directory | Where-Object { $_.CreationTime -ge $since } | Sort-Object CreationTime | Select-Object -Last 1
    if ($d) { return Join-Path $d.FullName 'console.log' }
    return $null
}
# Stops waiting as soon as one of the processes has exited (crash, init error of a client...)
function WaitFor($profile, $since, $pattern, $seconds, $processes, $poll = 5) {
    $end = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $end) {
        $log = LatestLog $profile $since
        if ($log -and (Test-Path $log)) {
            $hit = Select-String -Path $log -Pattern $pattern -List -ErrorAction SilentlyContinue
            if ($hit) { return $true }
        }
        foreach ($process in $processes) {
            if ($process -and $process.HasExited) { return $false }
        }
        Start-Sleep -Seconds $poll
    }
    return $false
}

$since = (Get-Date).AddSeconds(-2)
$procs = @()
$status = 'unknown'
try { do {
    $server = Start-Process -FilePath "$serverDir\ArmaReforgerServerDiag.exe" -WorkingDirectory $serverDir -PassThru -ArgumentList "-gproj `"$serverDir\addons\data\ArmaReforger.gproj`" -server `"$world`" -profile GDCAutoServer -nothrow -scrDefine ENABLE_DIAG -scrDefine GAME_MODE_DEBUG -rpl-validation-scr-disable $testArg $common"
    $procs += $server
    "server pid $($server.Id), world $world"

    if ($Compile) {
        # The game scripts compile right after start, before the world loads: no need to wait for the terrain
        $compiled = WaitFor 'GDCAutoServer' $since 'Compiling Game scripts took|Can.t compile' 120 $server 1
        $log = LatestLog 'GDCAutoServer' $since
        if (-not $compiled) { $status = 'COMPILATION NOT REACHED' }
        elseif ($log -and (Select-String -Path $log -Pattern 'Can.t compile' -Quiet)) { $status = 'COMPILATION FAILED' }
        else { $status = 'COMPILED' }
        break
    }

    if (-not (WaitFor 'GDCAutoServer' $since 'Starting RPL server|Entered online game state' 300 $server)) { $status = 'SERVER NOT READY'; break }
    'server ready'

    if ($Smoke) {
        # Scripts compiled and world loaded: let the game mode start, then stop
        Start-Sleep -Seconds 15
        if ($server.HasExited) { $status = "SERVER EXITED (code $($server.ExitCode))" } else { $status = 'COMPILED AND STARTED' }
        break
    }

    if (-not $NoClients) {
        foreach ($p in 'GDCAutoClient1', 'GDCAutoClient2') {
            $c = Start-Process -FilePath "$gameDir\ArmaReforgerSteamDiag.exe" -WorkingDirectory $gameDir -PassThru -ArgumentList "-gproj `"$gameDir\addons\data\ArmaReforger.gproj`" -window -screenWidth 800 -screenHeight 450 -forceupdate -nofocus -noBackend -noThrow -client -profile $p $testArg $common"
            $procs += $c
            "$p pid $($c.Id)"
            Start-Sleep -Seconds 25
        }
        $wait = $TimeoutMinutes * 60
    }
    else { $wait = 120 }

    if (WaitFor 'GDCAutoServer' $since "$tag DONE" $wait $procs) { $status = 'DONE' }
    else {
        $exited = $procs | Where-Object { $_.HasExited }
        if ($exited) { $status = 'PROCESS EXITED: ' + (($exited | ForEach-Object { "pid $($_.Id) code $($_.ExitCode)" }) -join ', ') }
        else { $status = 'TIMEOUT' }
    }
    Start-Sleep -Seconds 3
} while ($false) }
finally {
    foreach ($proc in $procs) {
        if ($proc -and -not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
    }
}

"status: $status"
$serverLog = LatestLog 'GDCAutoServer' $since
"GDCAutoServer log: $serverLog"
if ($serverLog) {
    Select-String -Path $serverLog -Pattern "$tag (PASS|FAIL|WARN|INFO|START|DONE)" | ForEach-Object { $_.Line -replace '^.*\[GDC_MissionTest\] ', '' }
}
if ($Compile -and $serverLog) {
    "--- script errors:"
    Select-String -Path $serverLog -Pattern 'SCRIPT\s+\(E\)' | ForEach-Object { '  ' + ($_.Line -replace '^\S+\s+', '') }
}
foreach ($p in 'GDCAutoClient1', 'GDCAutoClient2') {
    $log = LatestLog $p $since
    if (-not $log) { continue }
    $lines = Select-String -Path $log -Pattern "$tag (PASS|FAIL|WARN|INFO|START|DONE)" | ForEach-Object { $_.Line -replace '^.*\[GDC_MissionTest\] ', '' }
    if ($lines) { "--- $p test lines:"; $lines }
}
foreach ($p in 'GDCAutoServer', 'GDCAutoClient1', 'GDCAutoClient2') {
    $log = LatestLog $p $since
    if (-not $log) { continue }
    "--- $p errors (grouped, numbers ignored): $log"
    Select-String -Path $log -Pattern '\(E\):|Failed move|Virtual Machine Exception' |
        ForEach-Object { (($_.Line -replace '^\S+\s+', '') -replace '\{[0-9A-F]{16}\}', '{GUID}') -replace '\d+(\.\d+)?', '#' } |
        Group-Object | Sort-Object Count -Descending | Select-Object -First 25 |
        ForEach-Object { '{0,5} x {1}' -f $_.Count, $_.Name }
}

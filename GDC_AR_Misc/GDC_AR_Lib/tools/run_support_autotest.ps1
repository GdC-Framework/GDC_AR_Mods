$dirs = 'C:/Dev/Enfusion/GDC_AR_Mods/GDC_AR_ModPack/GDC_AR_Core/,C:/Dev/Enfusion/GDC_AR_Mods/GDC_AR_ModPack/GDC_AR_Equipment/,C:/Dev/Enfusion/GDC_AR_Mods/GDC_AR_Misc/GDC_AR_Lib/,C:/Dev/Enfusion/Zimnitrita_map/,C:/Dev/Enfusion/GDC_AR_Mods/GDC_AR_Missions/GDC_Missions/,C:/Users/Didier/Documents/My Games/ArmaReforgerWorkbench/addons'
$addons = 'ACE_Core_Dev,ACE_Backblast_Dev,ACE_Carrying_Dev,ACE_Chopping_Dev,ACE_Compass_Dev,ACE_Finger_Dev,ACE_Medical_Core_Dev,ACE_Medical_Hitzones_Dev,ACE_Medical_Circulation_Dev,ACE_Trenches_Dev,ACE_MagRepack_Dev,ACE_Explosives_Dev,ACE_Captives_Dev,ACE_TacticalLadder_Dev,ACE_Radio_Dev,ACE_TacticalPeriscope_Dev,ACE_Overheating_Dev,ACE_CookOff_Dev,ACE_Weather_Dev,ACE_Facepaint_Dev,ACE_Ballistics_Dev,ACE_Medical_Breathing_Dev,ACE_Scopes_Dev,ACE_AllInOne_Dev,CoalitionVON,CoalitionSquadInterface,FixedLoadCasualtyVehicles,FNGGMBudgetless,GameMasterFX,Grays_Adjustable_Doors,ImprovedBloodEffect,MaehNoWeapDropUnCon,NoIconForUncon,NoSupplySystem,RealismOverhaulAI,RealismOverhaulBallistics,WCS_Armbands,Youcansit,GDC AR Core,ChungusLaunchers,MarkerRoundsProject,EchoesOf89UGLTextures,RHS_Content_01,RHS_Content_02,RHS_Core,FPTR,WCS_Armaments,WCS_SpaceCore,JLTV,M2A2,BTR70Mountable,RHIB,RussianMarkerGrenades,LaserRangefinder,M4MAAWS,STRYKER,Wirecutters2,WZLaunchers,ZU23,GDC AR Equipment,TilWMissionFramework,PSCoreDevelopment,ReforgerLobbyCommunity,GDC AR Lib,MinusBuildingPack,Zimnitrita,GDC Missions'

$serverDir = 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Server'
$gameDir = 'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger'
$games = 'C:\Users\Didier\Documents\My Games'
$common = "-addonsDir `"$dirs`" -addons `"$addons`" -rpl-timeout-disable -maxFPS 30"

function LatestLog($profile, $since) {
    $root = Join-Path $games "$profile\logs"
    if (-not (Test-Path $root)) { return $null }
    $d = Get-ChildItem $root -Directory | Where-Object { $_.CreationTime -ge $since } | Sort-Object CreationTime | Select-Object -Last 1
    if ($d) { return Join-Path $d.FullName 'console.log' }
    return $null
}
function WaitFor($profile, $since, $pattern, $seconds) {
    $end = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $end) {
        $log = LatestLog $profile $since
        if ($log -and (Test-Path $log)) {
            $hit = Select-String -Path $log -Pattern $pattern -List -ErrorAction SilentlyContinue
            if ($hit) { return $true }
        }
        Start-Sleep -Seconds 3
    }
    return $false
}

$since = (Get-Date).AddSeconds(-2)
$procs = @()
try {
    $server = Start-Process -FilePath "$serverDir\ArmaReforgerServerDiag.exe" -WorkingDirectory $serverDir -PassThru -ArgumentList "-gproj `"C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Server/addons/data/ArmaReforger.gproj`" -server worlds/tanin69/_Tests/GDC_Support_AutoTest.ent -profile GDCAutoServer -nothrow $common"
    $procs += $server
    "server pid $($server.Id)"
    if (-not (WaitFor 'GDCAutoServer' $since 'Starting RPL server|Entered online game state' 240)) { 'SERVER NOT READY'; return }
    'server ready'
    Start-Sleep -Seconds 5

    foreach ($p in 'GDCAutoClient1', 'GDCAutoClient2') {
        $c = Start-Process -FilePath "$gameDir\ArmaReforgerSteamDiag.exe" -WorkingDirectory $gameDir -PassThru -ArgumentList "-gproj `"C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger/addons/data/ArmaReforger.gproj`" -window -screenWidth 800 -screenHeight 450 -forceupdate -nofocus -client -profile $p $common"
        $procs += $c
        "$p pid $($c.Id)"
        Start-Sleep -Seconds 25
    }

    $ok = WaitFor 'GDCAutoClient1' $since '\[GDC_SupportTest\] DONE' 240
    "done marker: $ok"
    Start-Sleep -Seconds 3
}
finally {
    foreach ($proc in $procs) {
        if ($proc -and -not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
    }
}
foreach ($p in 'GDCAutoServer', 'GDCAutoClient1', 'GDCAutoClient2') { "$p log: $(LatestLog $p $since)" }

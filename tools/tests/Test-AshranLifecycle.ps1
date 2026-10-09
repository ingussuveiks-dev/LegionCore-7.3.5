param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/ashran-lifecycle")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $output | Out-Null
function Extract-Block([string]$Text,[string]$Marker) {
    $start=$Text.IndexOf($Marker,[StringComparison]::Ordinal)
    if($start -lt 0){throw "Missing production handler: $Marker"}
    $open=$Text.IndexOf('{',$start);$depth=1;$end=$open+1
    while($depth -gt 0 -and $end -lt $Text.Length){if($Text[$end] -eq '{'){$depth++};if($Text[$end] -eq '}'){$depth--};$end++}
    if($depth){throw "Unbalanced handler: $Marker"}
    return $Text.Substring($start,$end-$start)
}
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$source=Get-Content "$root/src/server/scripts/OutdoorPvP/Ashran/AshranInstance.cpp" -Raw
[IO.File]::WriteAllText("$output/offer.inc",(Extract-Block $source 'void OfferAshranQuests('))
$methods=@('uint32 GetCreatureEntry(', 'uint32 GetGameObjectEntry(', 'void OnPlayerLeave(', 'void InitializeBattle(', 'void Update(')
[IO.File]::WriteAllText("$output/instance.inc",(($methods | ForEach-Object { (Extract-Block $source $_).Replace(' override','') }) -join "`n"))
[IO.File]::WriteAllText("$output/entry.inc",(Extract-Block $source 'void OnUpdate(').Replace(' override',''))
$maps=Get-Content "$root/src/server/game/Maps/MapInstanced.cpp" -Raw
[IO.File]::WriteAllText("$output/shared.inc",(Extract-Block $maps 'if (mapId == 1191)'))
$player=Get-Content "$root/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$output/weekly.inc",(Extract-Block $player 'void Player::ResetWeeklyQuestStatus('))
$outdoor=Get-Content "$root/src/server/game/OutdoorPvP/OutdoorPvP.cpp" -Raw
[IO.File]::WriteAllText("$output/capture-delete.inc",(Extract-Block $outdoor 'bool OPvPCapturePoint::DelCapturePoint(').Replace('ObjectGuid::Empty','0'))
[IO.File]::WriteAllText("$output/criteria-clear.inc",(Extract-Block $player 'if ((quest_id == 38923 || quest_id == 38925)'))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/AshranLifecycleTest.cpp" /Fe"$output/AshranLifecycleTest.exe" /Fo"$output/AshranLifecycleTest.obj"
if errorlevel 1 exit /b 1
"$output/AshranLifecycleTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Ashran lifecycle regression failed'}

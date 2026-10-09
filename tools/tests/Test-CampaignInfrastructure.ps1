param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/campaign-infrastructure")
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
$source=Get-Content "$root/src/server/scripts/OutdoorPvP/Ashran/AshranMgr.cpp" -Raw
$end=Extract-Block $source 'void OutdoorPvPAshran::EndEvent('
$win=(Extract-Block $source 'void OutdoorPvPAshran::SetEventData(').Replace('ObjectGuid::Empty','0')
[IO.File]::WriteAllText("$output/ashran-handlers.inc",$end+"`n"+$win)
$queue=Get-Content "$root/src/server/game/DungeonFinding/LFGQueue.cpp" -Raw
[IO.File]::WriteAllText("$output/trial-queue.inc",(Extract-Block $queue 'if (dungeon->id == 870 && dungeon->map == 1374)'))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/CampaignInfrastructureTest.cpp" /Fe"$output/CampaignInfrastructureTest.exe" /Fo"$output/CampaignInfrastructureTest.obj"
if errorlevel 1 exit /b 1
"$output/CampaignInfrastructureTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Campaign infrastructure regression failed'}

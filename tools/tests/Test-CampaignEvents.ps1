param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/campaign-events")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $output | Out-Null
function Extract-Block([string]$Text,[string]$Marker,[bool]$Semicolon=$false) {
    $start=$Text.IndexOf($Marker,[StringComparison]::Ordinal)
    if($start -lt 0){throw "Missing production handler: $Marker"}
    $open=$Text.IndexOf('{',$start);$depth=1;$end=$open+1
    while($depth -gt 0 -and $end -lt $Text.Length){if($Text[$end] -eq '{'){$depth++};if($Text[$end] -eq '}'){$depth--};$end++}
    if($depth){throw "Unbalanced handler: $Marker"}
    return $Text.Substring($start,$end-$start)+$(if($Semicolon){';'})
}
$root=(Resolve-Path "$PSScriptRoot/../..").Path
$blood=Get-Content "$root/src/server/scripts/Draenor/bloodmaul_campaign_events.cpp" -Raw
$seismic=Get-Content "$root/src/server/scripts/Draenor/seismic_campaign.cpp" -Raw
$exarch=Get-Content "$root/src/server/scripts/Draenor/exarch_campaign.cpp" -Raw
$blocks=@(
    (Extract-Block $blood 'namespace BloodmaulEvents'),
    (Extract-Block $seismic 'namespace SeismicCampaign'),
    (Extract-Block $exarch 'namespace ExarchCampaign'),
    (Extract-Block $blood 'class go_bloodmaul_shadow_gate' $true),
    (Extract-Block $blood 'class npc_bloodmaul_ritual_totem' $true),
    (Extract-Block $blood 'class npc_bloodmaul_campaign_enemy' $true),
    (Extract-Block $blood 'class spell_bloodmaul_purify_soul' $true),
    (Extract-Block $seismic 'class npc_seismic_tremor_tracker' $true),
    (Extract-Block $seismic 'class go_seismic_campaign_object' $true),
    (Extract-Block $exarch 'class npc_exarch_council_trial' $true)
)
[IO.File]::WriteAllText("$output/campaign-handlers.inc",($blocks -join "`n"))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/CampaignEventsTest.cpp" /Fe"$output/CampaignEventsTest.exe" /Fo"$output/CampaignEventsTest.obj"
if errorlevel 1 exit /b 1
"$output/CampaignEventsTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Campaign event regression failed'}

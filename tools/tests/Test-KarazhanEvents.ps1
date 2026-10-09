param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/karazhan-events")
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
$source=Get-Content "$root/src/server/scripts/EasternKingdoms/karazhan_campaign.cpp" -Raw
[IO.File]::WriteAllText("$output/helpers.inc",(Extract-Block $source 'bool HasKarazhanQuest(')+"`n"+(Extract-Block $source 'Creature* FindKarazhanSample('))
$collect=$source.Substring($source.IndexOf('class spell_karazhan_collect_sample'))
$node=$source.Substring($source.IndexOf('class spell_karazhan_disable_node'))
[IO.File]::WriteAllText("$output/collect.inc",((@('SpellCastResult Check(', 'void Select(', 'void Hit(', 'void Register(') | ForEach-Object {(Extract-Block $collect $_).Replace(' override','')}) -join "`n"))
[IO.File]::WriteAllText("$output/node.inc",((@('uint32 Entry(', 'uint32 QuestId(', 'Creature* Target(', 'SpellCastResult Check(', 'void Credit(', 'void Register(') | ForEach-Object {(Extract-Block $node $_).Replace(' override','')}) -join "`n"))
[IO.File]::WriteAllText("$output/arrival.inc",(Extract-Block $source 'void OnMapChanged(').Replace(' override',''))
[IO.File]::WriteAllText("$output/node-reset.inc",(Extract-Block $source 'void Reset(').Replace(' override',''))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/KarazhanEventsTest.cpp" /Fe"$output/KarazhanEventsTest.exe" /Fo"$output/KarazhanEventsTest.obj"
if errorlevel 1 exit /b 1
"$output/KarazhanEventsTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Karazhan event regression failed'}

param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/azsuna-scythe",[switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference = 'Stop'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$native=Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::QuestObjectiveSatisfy('
$native=$native.Replace('QuestObjectiveType type /*= QUEST_OBJECTIVE_MONSTER*/','unsigned type').Replace('ObjectGuid guid /*= ObjectGuid::Empty*/','ObjectGuid guid').Replace('uint8 i','unsigned i')
[IO.File]::WriteAllText("$output/ObjectiveSatisfy.inc",$native)
$definition=''
if($Baseline){$definition='/DSCYTHE_EXIT_BASELINE';$source=(& git -C $repo show '38f88905:src/server/scripts/Legion/azsuna_scythe.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned Scythe source missing'};[IO.File]::WriteAllText("$output/ScytheBaseline.inc",$source)}
if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
foreach ($header in @('ScriptMgr.h','ScriptedCreature.h','Player.h','ObjectAccessor.h','Map.h','TemporarySummon.h','GameObject.h')) {
    [IO.File]::WriteAllText("$output/$header", '#pragma once')
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 $definition /I"$output" "$PSScriptRoot/AzsunaScytheTest.cpp" /Fe"$output/AzsunaScytheTest.exe" /Fo"$output/AzsunaScytheTest.obj" /Fd"$output/AzsunaScytheTest.pdb"
if errorlevel 1 exit /b 1
"$output/AzsunaScytheTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw 'Azsuna scythe regression failed' }

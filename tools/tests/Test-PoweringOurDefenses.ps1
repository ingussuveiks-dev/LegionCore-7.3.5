param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/powering-our-defenses")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$out = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$script = Get-Content "$repo/src/server/scripts/EasternKingdoms/zone_eversong_woods.cpp" -Raw
$start = $script.IndexOf('#define MOB_ENRAGED_WRAITH')
$end = $script.IndexOf('void AddSC_eversong_woods()', $start)
[IO.File]::WriteAllText("$out/CrystalAI.inc", $script.Substring($start, $end - $start))

$map = Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
$start = $map.IndexOf('Player* crystalOwner = nullptr;', $map.IndexOf('TempSummon* Map::SummonCreature('))
$end = $map.IndexOf('if (summoner && entry != 114975', $start)
[IO.File]::WriteAllText("$out/CrystalAdmission.inc", $map.Substring($start, $end - $start))
$binding = $map.IndexOf('crystalOwner->BindQuestObject(8490, summon);', $end)
if ($binding -lt $end -or $binding -gt $map.IndexOf('AddToMap(summon->ToCreature())', $end) -or
    $map.IndexOf('crystalOwner->IncompleteQuest(8490);', $end) -gt $binding) {
    throw 'Defense must recover old progress and bind the current attempt before creature publication'
}

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$out" "$PSScriptRoot/PoweringOurDefensesTest.cpp" /Fe"$out/PoweringOurDefensesTest.exe" /Fo"$out/PoweringOurDefensesTest.obj"
if errorlevel 1 exit /b 1
"$out/PoweringOurDefensesTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Powering our Defenses production-source regression failed' }

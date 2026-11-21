param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Python
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath "$PSScriptRoot/../..").Path
$out = (New-Item -ItemType Directory -Path $OutputDirectory -Force).FullName
$native = @'
from pathlib import Path
import sys

root = Path(sys.argv[1])
sys.dont_write_bytecode = True
sys.path.insert(0, str(root / 'tools/research'))
from audit_spell_mechanics import Data

dbc = root / 'build-extractors/bin/Release/dbc/enUS'
data = Data(dbc, dbc)
spells = (234676, 234678)
effects = {row['SpellID']: row for row in data.rows('SpellEffect').values()
           if row['SpellID'] in spells and not row['DifficultyID']}
assert tuple(effects[234676][key] for key in ('Effect', 'EffectAura', 'ImplicitTarget1')) == (6, 4, 6)
assert tuple(effects[234678][key] for key in ('Effect', 'EffectBasePoints', 'ImplicitTarget1')) == (136, 5, 25)
misc, = (row for row in data.rows('SpellMisc').values() if row['SpellID'] == 234676 and not row['DifficultyID'])
assert data.rows('SpellCastTimes')[misc['CastingTimeIndex']]['Base'] == 2000
assert data.rows('SpellDuration')[misc['DurationIndex']]['Duration'] == 45000
proc, = (row for row in data.rows('SpellAuraOptions').values() if row['SpellID'] == 234676 and not row['DifficultyID'])
assert (proc['ProcTypeMask'], proc['ProcChance'], proc['ProcCharges']) == (2148147880, 100, 0)
categories, = (row for row in data.rows('SpellCategories').values() if row['SpellID'] == 234676 and not row['DifficultyID'])
assert (categories['DefenseType'], categories['PreventionType']) == (1, 3)
print('PASS 26972 normal cast, 45-second debuff, incoming-hit proc data and native 5% maximum-health heal')
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Kruul Twisted Reflection DB2 regression failed' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$production = [regex]::Match($source, '(?ms)^    Unit\* GetHealRecipient\(ProcEventInfo& event\).*?^    \}')
if (!$production.Success) { throw 'Production Twisted Reflection callback is missing' }
[IO.File]::WriteAllText("$out/KruulReflection.inc", $production.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerKruulReflectionTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Kruul Twisted Reflection callback regression failed' }

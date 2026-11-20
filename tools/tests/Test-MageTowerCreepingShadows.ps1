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
sys.dont_write_bytecode=True
root=Path(sys.argv[1])
sys.path.insert(0,str(root/"tools/research"))
from audit_spell_mechanics import Data
dbc=root/"build-extractors/bin/Release/dbc/enUS"
d=Data(dbc,dbc)
ids={231817,232664,232672}
effects={(r["SpellID"],r["EffectIndex"]):r for r in d.rows("SpellEffect").values()
         if r["SpellID"] in ids and not r["DifficultyID"]}
def value(spell, effect, *fields):
    return tuple(effects[spell,effect][key] for key in fields)
assert value(231817,0,"Effect","EffectAura","EffectMiscValue1")== (6,4,3000)
assert value(232664,0,"Effect","EffectMiscValue1")== (179,9346)
assert value(232672,0,"EffectAura","EffectBasePoints","EffectAuraPeriod")== (3,974999,1000)
assert value(232672,1,"EffectAura","EffectBasePoints")== (33,-50)
assert value(232672,2,"EffectAura","EffectMiscValue1")== (260,744)
misc={r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and not r["DifficultyID"]}
duration=d.rows("SpellDuration")
assert {i:duration[r["DurationIndex"]]["Duration"] for i,r in misc.items()} == {231817:3000,232664:600000,232672:300000}
steps={r["OrderIndex"] for r in d.rows("ScenarioStep").values() if r["ScenarioID"]==1204}
assert steps=={0,1,2}
print("PASS 26972 warmup, ground trigger, periodic damage, speed reduction, screen effect and scenario stages.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Creeping Shadows DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$header = [regex]::Match($source, '(?ms)^constexpr float CreepingShadowsInitialRadius.*?^\}\r?\n(?=\r?\nstruct CorruptingShadowsAI)')
if (!$header.Success) { throw 'Production Creeping Shadows geometry is absent' }
[IO.File]::WriteAllText("$out/CreepingShadows.inc", $header.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerCreepingShadowsTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Creeping Shadows callback regression failed' }

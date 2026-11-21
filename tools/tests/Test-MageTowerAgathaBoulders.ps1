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
ids={241473,241475,241502,241523,243570}
all_effects=d.rows("SpellEffect").values()
effects={(r["SpellID"],r["EffectIndex"]):r for r in all_effects
         if r["SpellID"] in ids and not r["DifficultyID"]}
def value(spell, effect, *fields):
    return tuple(effects[spell,effect][key] for key in fields)
assert value(241473,0,"EffectAura","ImplicitTarget1")== (4,1)
assert value(241502,0,"Effect","ImplicitTarget1")== (3,1)
assert value(241475,1,"Effect","ImplicitTarget2","EffectRadiusIndex2")== (2,16,14)
assert value(241475,2,"Effect","EffectTriggerSpell")== (64,243570)
assert value(241523,0,"Effect","EffectBasePoints","ImplicitTarget1")== (2,2849999,25)
assert value(241523,1,"EffectAura","EffectMechanic","ImplicitTarget1")== (12,12,25)
assert value(243570,0,"EffectAura","EffectMiscValue1")== (395,10326)
assert {r["SpellID"] for r in all_effects if r["EffectMiscValue1"]==10326 and (r["Effect"]==179 or r["EffectAura"]==395)} == {243570}
misc={r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and not r["DifficultyID"]}
assert d.rows("SpellCastTimes")[misc[241502]["CastingTimeIndex"]]["Base"]==3000
assert d.rows("SpellDuration")[misc[241523]["DurationIndex"]]["Duration"]==2000
assert d.rows("SpellRadius")[14]["Radius"]==8.0
assert any(r["SpellID"]==241473 and r["SpellVisualID"]==66068 for r in d.rows("SpellXSpellVisual").values())
print("PASS 26972 boulder windup, visual, landing, attachment, rolling damage and stun.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Agatha Felfire Boulder DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$body = [regex]::Match($source, '(?ms)^bool IsInAgathaBoulderPath\(.*?^\}')
if (!$body.Success) { throw 'Production Felfire Boulder contact function is absent' }
[IO.File]::WriteAllText("$out/AgathaBoulders.inc", $body.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerAgathaBouldersTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Agatha Felfire Boulder callback regression failed' }

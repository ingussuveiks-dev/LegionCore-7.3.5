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
ids={1543,132950,164393,231914,231980,231982,232006,232007,234728,241048,241062,242386,242394}
effects={(r["SpellID"],r["EffectIndex"]):r for r in d.rows("SpellEffect").values()
         if r["SpellID"] in ids and not r["DifficultyID"]}
def value(spell, effect, *fields):
    return tuple(effects[spell,effect][key] for key in fields)
assert value(1543,0,"EffectTriggerSpell","EffectRadiusIndex1")== (132950,13)
assert value(132950,0,"Effect","EffectMiscValue1")== (179,510)
assert value(231914,0,"Effect","EffectRadiusIndex2")== (2,29)
assert value(231914,1,"Effect","EffectRadiusIndex2")== (98,29)
assert value(231982,0,"Effect","EffectMiscValue1")== (179,9255)
assert value(231980,0,"EffectAura","EffectBasePoints","EffectAuraPeriod")== (20,15,1000)
assert value(231980,1,"EffectAura","EffectBasePoints")== (69,100)
assert value(231980,2,"EffectAura","EffectBasePoints")== (399,30)
assert value(232006,0,"EffectAura","EffectBasePoints","EffectAuraPeriod")== (226,232007,1000)
assert value(232007,0,"Effect","ImplicitTarget2")== (2,15)
assert value(232007,1,"EffectAura","EffectBasePoints","ImplicitTarget2")== (33,-60,15)
assert value(234728,0,"Effect","ImplicitTarget2")== (2,15)
assert value(234728,1,"Effect","ImplicitTarget2")== (98,15)
assert value(241048,2,"EffectAura","EffectMiscValue1")== (56,116492)
assert value(241048,5,"Effect","EffectTriggerSpell")== (64,241062)
assert all(value(spell,0,"Effect","ImplicitTarget1")== (3,1) for spell in (164393,242386,242394))
misc={r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and not r["DifficultyID"]}
casts=d.rows("SpellCastTimes")
assert casts[misc[234728]["CastingTimeIndex"]]["Base"]==40000
assert casts[misc[231914]["CastingTimeIndex"]]["Base"]==3000
assert d.rows("SpellDuration")[misc[231982]["DurationIndex"]]["Duration"]==10000
assert d.rows("SpellRadius")[29]["Radius"]==6.0 and d.rows("SpellRadius")[13]["Radius"]==10.0
assert d.rows("CreatureDisplayInfo")[73863]["StateSpellVisualKitID"]==74560
print("PASS 26972 intermission cast durations, knockbacks, Flare, image visual, barrier, Darkness, absorption and phase spells.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Xylem intermission DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$names = 'IsCastingAnnihilation','CanHitInIntermission','IsProtected','CanReveal','UpdateEncounter'
$bodies = foreach ($name in $names) {
    $body = [regex]::Match($source, "(?ms)^(?:bool|void) XylemAI::$name\(.*?^\}")
    if (!$body.Success) { throw "Production Xylem intermission function $name is absent" }
    $body.Value
}
$spells = [regex]::Match($source, '(?ms)^enum XylemIntermissionSpells : uint32.*?^};')
if (!$spells.Success) { throw 'Production Xylem spell and NPC IDs are absent' }
[IO.File]::WriteAllLines("$out/XylemIntermissions.inc", @($spells.Value) + $bodies)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerXylemIntermissionsTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Xylem intermission callback regression failed' }

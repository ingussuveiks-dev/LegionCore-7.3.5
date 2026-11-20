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
ids={235259,235261,235263}
effects={(r["SpellID"],r["EffectIndex"]):r for r in d.rows("SpellEffect").values()
         if r["SpellID"] in ids and not r["DifficultyID"]}
def value(spell, effect, *fields):
    return tuple(effects[spell,effect][key] for key in fields)
assert value(235259,0,"Effect","EffectTriggerSpell","ImplicitTarget1")== (32,235261,87)
assert value(235261,0,"Effect","EffectBasePoints","ImplicitTarget2")== (2,284999,16)
assert value(235261,1,"Effect","EffectMiscValue1")== (179,9645)
assert value(235263,0,"EffectAura","EffectAuraPeriod","EffectBasePoints","EffectRadiusIndex2")== (3,1000,94999,29)
assert d.rows("SpellRadius")[29]["Radius"]==6.0
misc={r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and not r["DifficultyID"]}
assert d.rows("SpellDuration")[misc[235261]["DurationIndex"]]["Duration"]==10000
print("PASS 26972 Fel Lava missile, direct impact, six-yard ground boundary, ten-second patch and one-second tick.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Agatha Fel Lava DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$body = [regex]::Match($source, '(?ms)^constexpr uint32 AgathaFelLavaTrigger.*?^\}')
if (!$body.Success) { throw 'Production Agatha Fel Lava geometry is absent' }
[IO.File]::WriteAllText("$out/AgathaFelLava.inc", $body.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerAgathaFelLavaTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Agatha Fel Lava callback regression failed' }

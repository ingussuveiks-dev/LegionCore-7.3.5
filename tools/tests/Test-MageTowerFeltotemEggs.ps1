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
sys.dont_write_bytecode = True
root = Path(sys.argv[1])
sys.path.insert(0, str(root / "tools/research"))
from audit_spell_mechanics import Data
dbc = root / "build-extractors/bin/Release/dbc/enUS"
d = Data(dbc, dbc)
ids = {237950, 242378, 242379, 242384, 242468, 242472, 242566, 243167}
effects = {(r["SpellID"], r["EffectIndex"]): r for r in d.rows("SpellEffect").values() if r["SpellID"] in ids and r["DifficultyID"] == 0}
def values(id, keys, index=0):
    return tuple(effects[(id,index)][key] for key in keys.split())
assert values(242384,"Effect EffectTriggerSpell") == (32,242378)
assert values(242378,"Effect EffectMiscValue1 EffectMiscValue2") == (28,121459,61)
assert values(242379,"Effect EffectAura EffectAuraPeriod") == (6,226,1000)
assert values(243167,"Effect EffectMiscValue1 EffectMiscValue2") == (28,121460,4059)
assert values(242468,"Effect ImplicitTarget2") == (3,15)
assert values(242566,"Effect EffectTriggerSpell") == (32,242472)
assert values(242472,"Effect EffectRadiusIndex2") == (2,15)
assert values(242472,"EffectAura EffectAuraPeriod",1) == (3,3000)
misc = {r["SpellID"]: r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and r["DifficultyID"] == 0}
assert d.rows("SpellCastTimes")[misc[242379]["CastingTimeIndex"]]["Base"] == 12000
assert (misc[242384]["Attributes10"] & 16) and misc[242384]["Speed"] == 1.0
print("PASS native 26972 eggs, 1-second missile, 12-second hatch, acid splash and damage-over-time.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Feltotem egg DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$fragments = [ordered]@{ 'FeltotemEggWave.inc' = 'FinishEggWave\(uint64'; 'FeltotemEggHatch.inc' = 'Hatch\(\)' }
foreach ($name in $fragments.Keys) {
    $match = [regex]::Match($source, "(?ms)^    void $($fragments[$name]).*?^    }")
    if (!$match.Success) { throw "Missing production callback for $name" }
    [IO.File]::WriteAllText("$out/$name", $match.Value)
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerFeltotemEggsTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Feltotem egg wave or hatch regression failed' }

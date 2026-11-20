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
ids = (237950, 238027, 238302, 238471, 238703, 239942, 241395, 241719, 243242, 243674)
effects = {r["SpellID"]: r for r in d.rows("SpellEffect").values() if r["SpellID"] in ids and r["EffectIndex"] == r["DifficultyID"] == 0}
assert (effects[237950]["EffectAura"], effects[237950]["EffectAuraPeriod"]) == (226, 1000)
assert (effects[238027]["Effect"], effects[238027]["EffectRadiusIndex2"]) == (2, 8)
assert (effects[238302]["Effect"], effects[238302]["EffectMiscValue1"]) == (179, 9923)
assert (effects[238471]["EffectAura"], effects[238471]["EffectBasePoints"]) == (87, -10)
assert (effects[238703]["EffectAura"], effects[238703]["EffectMiscValue1"]) == (56, 119900)
assert effects[241395]["Effect"] == 27
assert effects[241719]["EffectAura"] == 12
assert effects[243242]["Effect"] == 3
assert (effects[243674]["Effect"], effects[243674]["EffectMiscValue1"]) == (179, 10535)
misc = {r["SpellID"]: r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and r["DifficultyID"] == 0}
dur = d.rows("SpellDuration")
assert [(i, dur[misc[i]["DurationIndex"]]["Duration"]) for i in (237950, 238302, 239942, 243674)] == [
    (237950, 5000), (238302, 45000), (239942, 30000), (243674, -1)]
print("PASS native 26972 warning, damage, rock, transform, burrow duration, scale and break catalog.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Feltotem DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$patterns = @(
    '(?m)^constexpr (?:uint32 (?:StalactiteBreak|FelHardenedScales)|float RockRadius) = [^;]+;',
    '(?ms)^bool SegmentTouches\(.*?^}',
    '(?ms)^void TugarAI::HitStalactites\(.*?^}'
)
$fragments = foreach ($pattern in $patterns) {
    $found = [regex]::Matches($source, $pattern)
    if (!$found.Count) { throw "Missing production callback or constant matching $pattern" }
    $found.Value.Replace('constexpr uint32', 'constexpr unsigned')
}
[IO.File]::WriteAllLines("$out/FeltotemEarthquake.inc", $fragments)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerFeltotemEarthquakeTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Feltotem native rock contact regression failed' }

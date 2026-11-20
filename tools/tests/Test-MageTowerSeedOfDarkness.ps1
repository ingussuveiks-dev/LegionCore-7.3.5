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
ids={232991,233164,233165,233248}
effects={(r["SpellID"],r["EffectIndex"]):r for r in d.rows("SpellEffect").values()
         if r["SpellID"] in ids and not r["DifficultyID"]}
def value(spell, effect, *fields):
    return tuple(effects[spell,effect][key] for key in fields)
assert value(232991,0,"Effect","EffectBasePoints","ImplicitTarget2")== (136,20,7)
assert value(233164,0,"EffectAura","ImplicitTarget1")== (4,1)
assert value(233165,0,"Effect","EffectMiscValue1","EffectMiscValue2","ImplicitTarget1")== (28,116840,64,73)
assert value(233248,0,"EffectAura","EffectAuraPeriod","ImplicitTarget1")== (226,8000,6)
assert value(233248,1,"EffectAura","EffectAuraPeriod","ImplicitTarget1")== (226,8000,1)
misc={r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and not r["DifficultyID"]}
duration=d.rows("SpellDuration")
assert duration[misc[233248]["DurationIndex"]]["Duration"] == 8000
assert duration[misc[233164]["DurationIndex"]]["Duration"] == 35000
properties=d.rows("SummonProperties")[64]
assert properties["Slot"]==0
print("PASS 26972 Seed duration, hostile player aura, native summon entry and properties, and original 20% Merge.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Seed of Darkness DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$release = [regex]::Match($source, '(?ms)^    void ReleaseSeeds\(Player& player\).*?^    \}')
$expiry = [regex]::Match($source, '(?ms)^    void ReleaseSeeds\(AuraEffect const\*, AuraEffectHandleModes\).*?^    \}')
$merge = [regex]::Match($source, '(?ms)^bool CanMergeDarkness\(.*?^\}')
if (!$release.Success -or !$expiry.Success -or !$merge.Success) { throw 'Production Seed callbacks are absent' }
[IO.File]::WriteAllLines("$out/SeedOfDarkness.inc", @(
    $release.Value.Replace('void ReleaseSeeds', 'void CorruptingShadowsAI::ReleaseSeeds'),
    $expiry.Value.Replace('void ReleaseSeeds', 'void spell_mage_tower_seed_expiry::ReleaseSeeds'),
    $merge.Value
))

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerSeedOfDarknessTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Seed of Darkness callback regression failed' }

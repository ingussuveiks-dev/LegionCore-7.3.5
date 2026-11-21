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
ids = {234440, 234441, 234788, 234789, 234920, 234922}
effects = {(row['SpellID'], row['EffectIndex']): row for row in data.rows('SpellEffect').values()
           if row['SpellID'] in ids and not row['DifficultyID']}
def fields(spell, effect, *names):
    return tuple(effects[spell, effect][name] for name in names)

assert fields(234920, 0, 'Effect', 'ImplicitTarget1') == (3, 1)
assert fields(234788, 0, 'Effect', 'EffectMiscValue1') == (61, 56402)
assert fields(234789, 0, 'Effect', 'ImplicitTarget1') == (77, 1)
assert fields(234922, 0, 'Effect', 'EffectTriggerSpell', 'ImplicitTarget2') == (140, 234440, 7)
assert fields(234922, 1, 'EffectAura', 'ImplicitTarget2') == (4, 7)
assert fields(234440, 0, 'Effect', 'EffectMiscValue1', 'ImplicitTarget1') == (179, 9554, 18)
assert fields(234441, 0, 'EffectAura', 'EffectAuraPeriod', 'ImplicitTarget1') == (3, 1000, 1)
assert fields(234441, 1, 'EffectAura', 'EffectBasePoints') == (33, -50)
assert any(row['SpellID'] == 234440 and row['SpellVisualID'] == 63929
           for row in data.rows('SpellXSpellVisual').values())
print('PASS 26972 Shadow Sweep warning, spawn event, caster-entry filter, force-cast area, visual, damage and slow')
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Kruul Shadow Sweep DB2 regression failed' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$start = [regex]::Match($source, '(?ms)^Position GetShadowSweeperStart\(.*?^\}')
$sweep = [regex]::Match($source, '(?ms)^bool ShadowSweepTouches\(.*?^\}')
if (!$start.Success -or !$sweep.Success) { throw 'Production Shadow Sweep geometry is missing' }
[IO.File]::WriteAllText("$out/KruulShadowSweep.inc", $start.Value + "`n" + $sweep.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerKruulShadowSweepTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Kruul Shadow Sweep geometry regression failed' }

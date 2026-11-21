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
sys.path.insert(0, str(root / 'tools/research'))
from audit_spell_mechanics import Data

dbc = root / 'build-extractors/bin/Release/dbc/enUS'
data = Data(dbc, dbc)
route = data.rows('LFGDungeons')[1446]
assert (route['MapID'], route['ScenarioID']) == (1703, 1265)
share = [row for row in data.rows('SpellEffect').values() if row['SpellID'] == 239056 and not row['DifficultyID']]
assert any((row['EffectIndex'], row['Effect'], row['EffectAura']) == (1, 6, 4) for row in share)
unit = (root / 'src/server/game/Entities/Unit/Unit.cpp').read_text(encoding='utf-8-sig')
damage = unit.split('uint32 Unit::DealDamage(', 1)[1]
assert damage.index('damage / victim->GetHealthMultiplierForTarget(this)') < damage.index('instance->OnCreatureDamageTaken(creature, this, damage)') < damage.index('if (health <= damage)')
print('PASS 26972 solo route and Shared Health effect with the final native damage callback before the death check')
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Sigryn native catalog regression failed' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$instance = [regex]::Match($source, '(?ms)^class instance_mage_tower_god_queen : public InstanceMapScript.*?^\};')
if (!$instance.Success) { throw 'Production God-Queen instance damage callback is missing' }
[IO.File]::WriteAllText("$out/GodQueenInstance.inc", $instance.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerSigrynSurrenderTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Sigryn native surrender damage regression failed' }

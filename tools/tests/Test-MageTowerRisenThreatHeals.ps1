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
dungeon = data.rows('LFGDungeons')[1471]
assert (dungeon['MapID'], dungeon['ScenarioID']) == (1710, 1281)
tree = data.rows('CriteriaTree')[57401]
assert tree['Amount'] == 8 and tree['Parent'] == 57400
criteria = data.rows('Criteria')[tree['CriteriaID']]
assert (criteria['Type'], criteria['Asset']) == (92, 56911)
army = data.rows('CriteriaTree')[57563]
army_criteria = data.rows('Criteria')[army['CriteriaID']]
assert army['Parent'] == 57562 and (army_criteria['Type'], army_criteria['Asset']) == (92, 56980)
effects = [row for row in data.rows('SpellEffect').values() if not row['DifficultyID']]
assert any(row['SpellID'] == 237846 and row['Effect'] == 6 for row in effects)
assert any(row['SpellID'] == 237847 and row['EffectTriggerSpell'] == 237848 for row in effects)
ignite = next(row for row in effects if row['SpellID'] == 237188 and not row['EffectIndex'])
burst = next(row for row in effects if row['SpellID'] == 237189 and not row['EffectIndex'])
assert (ignite['Effect'], ignite['EffectAura']) == (6, 4)
assert (burst['Effect'], burst['ImplicitTarget1'], burst['ImplicitTarget2'], burst['EffectBasePoints']) == (2, 22, 30, 1)
assert data.rows('SpellRadius')[burst['EffectRadiusIndex2']]['Radius'] == 300.0
misc = {row['SpellID']: row for row in data.rows('SpellMisc').values() if not row['DifficultyID']}
assert data.rows('SpellDuration')[misc[237188]['DurationIndex']]['Duration'] == 9000
assert misc[237189]['Attributes3'] & 0x20000000
print('PASS 26972 healer solo route, eight-soul and Risen Army criteria, draw and corruption spells')
print('PASS 26972 nine-second Ignite Soul, native 300-yard ally-area damage effect and no critical hits')
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Risen Threat native healing data regression failed' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$scope = [regex]::Match($source, '(?ms)^bool IsRisenThreatSoulHealingStage\(.*?^bool IsRisenThreatChallenger\(.*?^\}')
$instance = [regex]::Match($source, '(?ms)^class instance_mage_tower_risen_threat : public InstanceMapScript.*?^\};')
$records = [regex]::Match($source, '(?ms)^void RecordRisenThreatWaveHeal\(.*?^void RecordRisenThreatCorruption\(.*?^\}')
$ignite = [regex]::Match($source, '(?ms)^int32 GetIgniteSoulDamage\(.*?^\}')
$spells = [regex]::Match($source, '(?ms)^class spell_mage_tower_erdris_ignite_soul : public AuraScript.*?^class spell_mage_tower_erdris_ignite_soul_damage : public SpellScript.*?^\};')
$receipt = [regex]::Match($source, '(?ms)^    struct RisenThreatSoulAI : SmartAI.*?^    \};')
if (!$scope.Success -or !$instance.Success -or !$records.Success -or !$ignite.Success -or !$spells.Success -or !$receipt.Success) { throw 'Production Risen Threat callbacks are missing' }
[IO.File]::WriteAllText("$out/RisenThreatHeals.inc", (@($scope.Value,$instance.Value,$records.Value,$ignite.Value,$spells.Value,$receipt.Value) -join "`n"))

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerRisenThreatHealsTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Risen Threat healing receipt regression failed' }

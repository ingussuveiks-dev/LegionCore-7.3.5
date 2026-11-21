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
ids={243111,243113,243114}
effects={(r["SpellID"],r["EffectIndex"]):r for r in d.rows("SpellEffect").values()
         if r["SpellID"] in ids and not r["DifficultyID"]}
def value(spell, effect, *fields):
    return tuple(effects[spell,effect][key] for key in fields)
assert value(243114,0,"EffectAura","EffectAuraPeriod","EffectTriggerSpell")== (23,1000,243111)
assert value(243114,1,"Effect","EffectTriggerSpell")== (64,243113)
assert value(243113,0,"EffectAura","EffectBasePoints")== (69,4000000)
assert value(243113,1,"EffectAura","EffectMiscValue1")== (147,2107)
assert value(243111,0,"Effect","EffectMechanic","ImplicitTarget2")== (2,35,15)
assert value(243111,1,"EffectAura","EffectMiscValue1","EffectBasePoints")== (276,35,10)
assert any(r["SpellID"]==243114 and not r["DifficultyID"] and r["Attributes2"] & 64 for r in d.rows("SpellMisc").values())
assert any(r["SpellID"]==243114 and not r["DifficultyID"] and r["PreventionType"]==1 for r in d.rows("SpellCategories").values())
assert any(r["SpellID"]==243111 and not r["DifficultyID"] and r["CumulativeAura"]==99 for r in d.rows("SpellAuraOptions").values())
costs = [(r["PowerType"], r["ManaCost"]) for r in d.rows("SpellPower").values() if r["SpellID"] == 243114]
assert costs == [(3, 100)]
print("PASS 26972 original Dark Fury 100-energy cost, channel, interrupt category, periodic damage, stacking mechanic and shield.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Agatha Dark Fury DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$scope = [regex]::Match($source, '(?ms)^class spell_mage_tower_agatha_dark_fury_channel .*?(?=^class spell_mage_tower_agatha_dark_fury_damage)')
$charge = [regex]::Match($source, '(?ms)^struct AgathaDarkFuryCharge.*?^\};')
$advance = [regex]::Match($source, '(?ms)^        void UpdateDarkFury\(.*?^        \}')
$cycle = [regex]::Match($source, '(?ms)^        void DarkFuryStarted\(.*?^        \}')
$start = [regex]::Match($scope.Value, '(?ms)^    void Start\(.*?^    \}')
$tick = [regex]::Match($scope.Value, '(?ms)^    void OnTick\(.*?^    \}')
$stop = [regex]::Match($scope.Value, '(?ms)^    void Stop\(.*?^    \}')
$shield = [regex]::Match($scope.Value, '(?ms)^    void SetInterruptImmunity\(.*?^    \}')
if (!$charge.Success -or !$advance.Success -or !$cycle.Success -or !$start.Success -or !$tick.Success -or !$stop.Success -or !$shield.Success) { throw 'Production Agatha Dark Fury callbacks are absent' }
[IO.File]::WriteAllText("$out/AgathaDarkFuryCharge.inc", $charge.Value)
[IO.File]::WriteAllLines("$out/AgathaDarkFury.inc", @(
    $advance.Value.Replace('void UpdateDarkFury','void AgathaAI::UpdateDarkFury'),
    $cycle.Value.Replace('void DarkFuryStarted','void AgathaAI::DarkFuryStarted'),
    $start.Value.Replace('void Start','void spell_mage_tower_agatha_dark_fury_channel::Start'),
    $tick.Value.Replace('void OnTick','void spell_mage_tower_agatha_dark_fury_channel::OnTick'),
    $stop.Value.Replace('void Stop','void spell_mage_tower_agatha_dark_fury_channel::Stop'),
    $shield.Value.Replace('void SetInterruptImmunity','void spell_mage_tower_agatha_dark_fury_shield::SetInterruptImmunity')
))

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerAgathaDarkFuryTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Agatha Dark Fury callback regression failed' }

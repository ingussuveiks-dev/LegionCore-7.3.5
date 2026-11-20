param(
    [Parameter(Mandatory)][string]$FixtureOptionsFile,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Python,
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_20_06_mage_tower_summoning_rune.sql"
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath "$PSScriptRoot/../..").Path
$out = (New-Item -ItemType Directory -Path $OutputDirectory -Force).FullName
$dbData = (Resolve-Path -LiteralPath $FixtureDataDirectory).Path
if (!$dbData.StartsWith("$repo\.codex\", [StringComparison]::OrdinalIgnoreCase)) { throw 'Use an isolated copied data directory' }
$options = (Resolve-Path -LiteralPath $FixtureOptionsFile).Path
$maria = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
$identity = & $maria "--defaults-extra-file=$options" --batch --raw --skip-column-names '--execute=SELECT @@datadir'
if ($LASTEXITCODE -or [IO.Path]::GetFullPath($identity).TrimEnd('\','/') -ne $dbData) { throw 'Fixture identity did not match' }

$native = @'
from pathlib import Path
import sys
sys.dont_write_bytecode = True
root = Path(sys.argv[1])
sys.path.insert(0, str(root / "tools/research"))
from audit_spell_mechanics import Data
dbc = root / "build-extractors/bin/Release/dbc/enUS"
d = Data(dbc, dbc)
ids = (236460, 236461, 236468, 236469, 236705)
effects = {r["SpellID"]: r for r in d.rows("SpellEffect").values() if r["SpellID"] in ids and r["EffectIndex"] == r["DifficultyID"] == 0}
assert (effects[236469]["Effect"], effects[236469]["EffectTriggerSpell"]) == (32, 236705)
assert (effects[236705]["Effect"], effects[236705]["EffectTriggerSpell"]) == (32, 236460)
assert (effects[236460]["Effect"], effects[236460]["EffectMiscValue1"]) == (179, 9778)
assert (effects[236461]["Effect"], effects[236461]["EffectMiscValue1"], effects[236461]["EffectMiscValue2"]) == (28, 119098, 3499)
assert effects[236468]["Effect"] == 77
misc = {r["SpellID"]: r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids}
assert d.rows("SpellDuration")[misc[236460]["DurationIndex"]]["Duration"] == 8000
assert misc[236469]["Speed"] == 1.5 and misc[236705]["Speed"] == 2
assert d.rows("SummonProperties")[3499]["Control"] == 0
print("PASS native 26972 warning, two missiles, 8-second trigger and Nightmare summon.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Twins Rune DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$body = [regex]::Match($source, '(?ms)^struct at_mage_tower_summoning_rune : AreaTriggerAI\r?\n.*?(?=^class npc_mage_tower_agatha )')
$duration = [regex]::Match($source, '(?m)^constexpr uint32 SoakDuration = \d+;')
if (!$body.Success -or !$duration.Success) { throw 'Production Twins rune code could not be extracted' }
[IO.File]::WriteAllText("$out/TwinsRuneAreaAI.inc", $body.Value)
[IO.File]::WriteAllText("$out/RuneSoakDuration.inc", $duration.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerRuneSoakTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Twins Rune callback regression failed' }

$migration = Get-Content -LiteralPath $MigrationPath -Raw
$readRows = @'
SELECT CONCAT(AIName, ':', ScriptName) FROM creature_template WHERE entry=116409;
SELECT CONCAT_WS(':', entry, spellId, customEntry, Radius) FROM areatrigger_template
WHERE entry=9778 OR (entry=5061 AND spellId=193998 AND customEntry=9778) ORDER BY entry, spellId;
SELECT CONCAT_WS(':', entry, ScriptName) FROM areatrigger_scripts WHERE entry=236460;
SELECT CONCAT_WS(':', spell_id, ScriptName) FROM spell_script_names
WHERE spell_id IN (236469,236705) ORDER BY spell_id,ScriptName;
SELECT CONCAT_WS(':', faction, minlevel, maxlevel, speed_run, dmg_multiplier) FROM creature_template WHERE entry=119098;
SELECT CONCAT_WS(':', Entry, Type, Displayid1, HpMulti) FROM creature_template_wdb WHERE Entry=119098;
SELECT CONCAT_WS(':', GroupID, ID, Type, BroadcastTextID) FROM creature_text WHERE CreatureID=116409 AND GroupID=7;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=116409;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=116409
  AND ((id=18 AND event_param1=5 AND event_param2=5) OR (id=22 AND action_param2=56198));
'@
$rows = @("$readRows`n$migration`n$readRows`n$migration`n$readRows" |
    & $maria "--defaults-extra-file=$options" --database=legion_world --batch --raw --skip-column-names)
$baseline = @('SmartAI:', '5061:193998:9778:60', '119098:15:75054:100', '24', '2')
$repaired = @(
    ':npc_mage_tower_raest', '5061:193998:9778:60', '9778:236460:236460:3',
    '236460:at_mage_tower_summoning_rune', '236469:spell_mage_tower_rune_missile', '236705:spell_mage_tower_rune_missile',
    '16:111:111:1:24', '119098:15:75054:100', '7:0:41:127537', '24', '2'
)
if ($LASTEXITCODE -or (Compare-Object $rows ($baseline + $repaired + $repaired) -SyncWindow 0)) {
    $rows
    throw 'Twins fixture migration did not preserve the expected encounter and trigger rows'
}
'PASS isolated migration twice, with Raest phase and victory rows and the unrelated trigger untouched.'

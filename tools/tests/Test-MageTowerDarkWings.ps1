param(
    [Parameter(Mandatory)][string]$FixtureOptionsFile,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Python,
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_20_07_mage_tower_dark_wings.sql"
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
ids = (237730, 237759, 237761, 237769, 237772)
effects = {r["SpellID"]: r for r in d.rows("SpellEffect").values() if r["SpellID"] in ids and r["EffectIndex"] == r["DifficultyID"] == 0}
assert (effects[237730]["Effect"], effects[237730]["EffectMiscValue1"], effects[237730]["EffectMiscValue2"]) == (28, 119696, 64)
assert (effects[237759]["Effect"], effects[237759]["EffectMiscValue1"]) == (254, 73)
assert (effects[237761]["Effect"], effects[237761]["ImplicitTarget2"]) == (2, 134)
assert effects[237772]["Effect"] == 3
misc = {r["SpellID"]: r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids}
assert d.rows("SpellCastTimes")[misc[237769]["CastingTimeIndex"]]["Base"] == 3000
assert any(r["SpellID"] == 237761 and r["Width"] == 6.0 for r in d.rows("SpellTargetRestrictions").values())
assert d.rows("SummonProperties")[64]["Title"] != 4
print("PASS native 26972 Dark Wings spawn, 3-second telegraph, motion and 6-yard damage width.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Dark Wings DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$formation = [regex]::Match($source, '(?ms)^    void StartWave\(\).*?(?=^    void StopWaves\(\))')
$releaseCode = [regex]::Match($source, '(?ms)^void ValkyrAI::SpellFinishCast\(.*?(?=^}\r?\n\r?\nclass npc_mage_tower_sigryn)')
if (!$formation.Success -or !$releaseCode.Success) { throw 'Production Dark Wings callbacks could not be extracted' }
[IO.File]::WriteAllText("$out/DarkWingsFormation.inc", $formation.Value)
[IO.File]::WriteAllText("$out/DarkWingsRelease.inc", $releaseCode.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerDarkWingsTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Dark Wings callback regression failed' }

$migration = Get-Content -LiteralPath $MigrationPath -Raw
$readRows = @'
SELECT CONCAT(AIName, ':', ScriptName) FROM creature_template WHERE entry=116484;
SELECT CONCAT_WS(':', entry, faction, unit_flags, ScriptName) FROM creature_template WHERE entry=119696;
SELECT CONCAT_WS(':', Entry, Type, Displayid1) FROM creature_template_wdb WHERE Entry=119696;
SELECT CONCAT_WS(':', spell_id, ScriptName) FROM spell_script_names WHERE spell_id=237761;
SELECT CONCAT_WS(':', GroupID, Type, BroadcastTextID) FROM creature_text
WHERE CreatureID=116484 AND GroupID IN (8,9) ORDER BY GroupID;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=116484;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=116484
  AND ((id=6 AND action_param1=237945) OR (id=10 AND action_param2=56052));
SELECT COUNT(*) FROM spell_script_names WHERE
  (spell_id=237952 AND ScriptName='spell_mage_tower_ancestral_blast')
  OR (spell_id IN(236469,236705) AND ScriptName='spell_mage_tower_rune_missile');
'@
$rows = @("$readRows`n$migration`n$readRows`n$migration`n$readRows" |
    & $maria "--defaults-extra-file=$options" --database=legion_world --batch --raw --skip-column-names)
$baseline = @('SmartAI:', '119696:7:26096', '16', '2', '3')
$repaired = @(':npc_mage_tower_sigryn','119696:16:33554434:npc_mage_tower_dark_valkyr',
    '119696:7:26096','237761:spell_mage_tower_dark_wings','8:14:128424','9:41:128428','16','2','3')
if ($LASTEXITCODE -or (Compare-Object $rows ($baseline + $repaired + $repaired) -SyncWindow 0)) {
    $rows
    throw 'Dark Wings migration did not preserve the expected encounter rows'
}
'PASS isolated migration twice, with Sigryn surrender, Blood of the Father and prior Mage Tower bindings intact.'

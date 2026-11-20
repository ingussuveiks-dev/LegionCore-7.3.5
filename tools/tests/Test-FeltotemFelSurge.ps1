param(
    [Parameter(Mandatory)][string]$FixtureOptionsFile,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Python,
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_20_04_feltotem_fel_surge_totem.sql"
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
effects = {(row["SpellID"], row["EffectIndex"]): row for row in d.rows("SpellEffect").values()
           if row["SpellID"] in (243224, 237903, 242496) and row["DifficultyID"] == 0}
missile, summon, stun = (effects[spell, 0] for spell in (243224, 237903, 242496))
assert tuple(missile[k] for k in ("Effect", "EffectTriggerSpell", "ImplicitTarget1", "EffectRadiusIndex1")) == (32, 237903, 73, 176)
assert tuple(summon[k] for k in ("Effect", "EffectMiscValue1", "EffectMiscValue2", "ImplicitTarget1")) == (28, 121499, 3407, 87)
assert tuple(stun[k] for k in ("Effect", "EffectAura", "ImplicitTarget1", "ImplicitTarget2", "EffectRadiusIndex2")) == (6, 12, 22, 15, 12)
assert effects[242496, 1]["EffectTriggerSpell"] == 242560
properties = d.rows("SummonProperties")[3407]
assert (properties["Control"], properties["Title"], properties["Slot"]) == (1, 4, 15)
misc = {row["SpellID"]: row for row in d.rows("SpellMisc").values() if row["SpellID"] in (243224, 237903, 242496)}
assert misc[243224]["Speed"] == 2.0 and misc[243224].get("LaunchDelay", 0) == 0
assert d.rows("SpellCastTimes")[misc[242496]["CastingTimeIndex"]]["Base"] == 8000
assert d.rows("SpellDuration")[misc[237903]["DurationIndex"]]["Duration"] == 20000
assert d.rows("SpellDuration")[misc[242496]["DurationIndex"]]["Duration"] == 5000
assert d.rows("SpellRadius")[176]["Radius"] == 15 and d.rows("SpellRadius")[12]["Radius"] == 100
print("PASS native 26972 missile, summon, 8-second cast, 100-yard radius and 5-second stun.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Native Fel Surge DB2 catalog mismatch' }

$migration = Get-Content -LiteralPath $MigrationPath -Raw
$readRows = @'
SELECT CONCAT(AIName, ':', ScriptName) FROM creature_template WHERE entry=117230;
SELECT COUNT(*) FROM creature_template WHERE entry=121499;
SELECT CONCAT_WS(':', faction, minlevel, maxlevel, RegenHealth, flags_extra, ScriptName)
FROM creature_template WHERE entry=121499;
SELECT CONCAT_WS(':', Entry, Type, Displayid1, VerifiedBuild) FROM creature_template_wdb WHERE Entry=121499;
SELECT CONCAT_WS(':', spell_id, ScriptName) FROM spell_script_names
WHERE spell_id IN (243224,237903,242496) ORDER BY spell_id,ScriptName;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND
  ((entryorguid=117230 AND id=7 AND action_type=85 AND target_type=130)
  OR (entryorguid=117484 AND id IN(14,15) AND target_type=130));
SELECT COUNT(*) FROM creature WHERE id=121499;
'@
$rows = @("$readRows`n$migration`n$readRows`n$migration`n$readRows" |
    & $maria "--defaults-extra-file=$options" --database=legion_world --batch --raw --skip-column-names)
$baseline = @('SmartAI:', '0', '121499:10:4590:23877', '3', '0')
$repaired = @(
    ':npc_mage_tower_tugar', '1', '2792:110:110:0:64:npc_mage_tower_fel_surge_totem',
    '121499:10:4590:23877', '237903:spell_mage_tower_fel_surge_summon',
    '242496:spell_mage_tower_fel_surge', '243224:spell_mage_tower_fel_surge_missile', '3', '0'
)
if ($LASTEXITCODE -or (Compare-Object $rows ($baseline + $repaired + $repaired) -SyncWindow 0)) {
    $rows
    throw 'Feltotem fixture migration did not preserve the expected templates, hooks and credit'
}

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$body = [regex]::Match($source, '(?ms)^namespace Feltotem\r?\n.*?(?=^class npc_mage_tower_agatha )')
if (!$body.Success) { throw 'Production Feltotem code could not be extracted' }
[IO.File]::WriteAllText("$out/FeltotemFelSurge.inc", $body.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/FeltotemFelSurgeTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Feltotem callback regression failed' }
'PASS isolated migration twice, with the three previous finale actions intact.'

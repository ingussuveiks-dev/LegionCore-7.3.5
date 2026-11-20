param(
    [Parameter(Mandatory)][string]$FixtureOptionsFile,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Python,
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_20_05_mage_tower_ancestral_knowledge.sql"
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
effects = {(r["SpellID"], r["EffectIndex"]): r for r in d.rows("SpellEffect").values()
           if r["SpellID"] in (237949, 237952) and r["DifficultyID"] == 0}
shield, interrupt_effect, interrupt_mechanic, silence, native_trigger, blast = (effects[key] for key in (
    (237949, 0), (237949, 1), (237949, 2), (237949, 3), (237949, 5), (237952, 0)))
assert (shield["EffectAura"], shield["EffectBasePoints"]) == (69, 5000000)
assert (interrupt_effect["EffectAura"], interrupt_effect["EffectMiscValue1"]) == (37, 68)
assert (interrupt_mechanic["EffectAura"], interrupt_mechanic["EffectMiscValue1"]) == (77, 26)
assert (silence["EffectAura"], silence["EffectMiscValue1"]) == (77, 9)
assert (native_trigger["Effect"], native_trigger["EffectTriggerSpell"], native_trigger["ImplicitTarget1"]) == (160, 237952, 1)
assert (blast["Effect"], blast["EffectBasePoints"], blast["ImplicitTarget2"]) == (2, 5000000, 15)
misc = {r["SpellID"]: r for r in d.rows("SpellMisc").values() if r["SpellID"] in (237949, 237952)}
assert d.rows("SpellDuration")[misc[237949]["DurationIndex"]]["Duration"] == 30000
assert d.rows("SpellCastTimes")[misc[237952]["CastingTimeIndex"]]["Base"] == 20000
assert any(r["SpellID"] == 237952 and r["PreventionType"] == 1 for r in d.rows("SpellCategories").values())
print("PASS native 26972 five-million absorb, effect and mechanic immunities, effect160 and 20-second blast.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Ancestral Knowledge DB2 catalog mismatch' }

$migration = Get-Content -LiteralPath $MigrationPath -Raw
$readRows = @'
SELECT CONCAT(AIName, ':', ScriptName) FROM creature_template WHERE entry=116496;
SELECT CONCAT_WS(':', spell_id, ScriptName) FROM spell_script_names
WHERE spell_id IN (237949,237952) ORDER BY spell_id,ScriptName;
SELECT GROUP_CONCAT(map ORDER BY map) FROM creature WHERE id=116496;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=116496;
SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=116496
  AND ((id=3 AND action_param1=237890) OR (id=8 AND event_param1=3 AND event_param2=3)
  OR (id=11 AND action_type=45));
SELECT COUNT(*) FROM spell_script_names
WHERE spell_id IN(243224,237903,242496) AND ScriptName LIKE 'spell_mage_tower_fel_surge%';
'@
$rows = @("$readRows`n$migration`n$readRows`n$migration`n$readRows" |
    & $maria "--defaults-extra-file=$options" --database=legion_world --batch --raw --skip-column-names)
$baseline = @('SmartAI:', '1703', '12', '3', '3')
$repaired = @(':npc_mage_tower_faljar','237952:spell_mage_tower_ancestral_blast','1703','12','3','3')
if ($LASTEXITCODE -or (Compare-Object $rows ($baseline + $repaired + $repaired) -SyncWindow 0)) {
    $rows
    throw 'Faljar fixture migration did not preserve the expected SmartAI and spell bindings'
}

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$body = [regex]::Match($source, '(?ms)^namespace GodQueen\r?\n.*?(?=^class npc_mage_tower_agatha )')
if (!$body.Success) { throw 'Production Faljar code could not be extracted' }
[IO.File]::WriteAllText("$out/MageTowerFaljar.inc", $body.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerFaljarTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Faljar callback regression failed' }
'PASS isolated migration twice, with the original Faljar events and Totem hooks intact.'

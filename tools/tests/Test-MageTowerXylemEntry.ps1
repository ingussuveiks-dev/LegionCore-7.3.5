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
pairs = [
    (235527, 237660, 1417, 1616, 1221),
    (235557, 235194, 1471, 1710, 1281),
    (237622, 237664, 1446, 1703, 1265),
    (229196, 229196, 1406, 1673, 1204),
    (233380, 233379, 1440, 1698, 1259),
    (231857, 237663, 1452, 1684, 1273),
    (233804, 233808, 1445, 1702, 1266),
]
effects = [r for r in d.rows("SpellEffect").values() if r["DifficultyID"] == 0]
reagents = {r["SpellID"]: (r["CurrencyTypesID"], r["CurrencyCount"]) for r in d.rows("SpellReagentsCurrency").values()}
dungeons = d.rows("LFGDungeons")
misc = {r["SpellID"]: r for r in d.rows("SpellMisc").values() if not r["DifficultyID"]}
for paid, free, dungeon_id, map_id, scenario_id in pairs:
    for spell in (paid, free):
        teleports = [r for r in effects if r["SpellID"] == spell and r["Effect"] == 227]
        assert len(teleports) == 1 and teleports[0]["EffectMiscValue2"] == dungeon_id
        assert misc[spell]["Speed"] == misc[spell]["LaunchDelay"] == 0
    assert (dungeons[dungeon_id]["MapID"], dungeons[dungeon_id]["ScenarioID"]) == (map_id, scenario_id)
    assert free not in reagents
    assert reagents.get(paid) == ((1226, 100) if paid != free else None)
assert any(r["SpellID"] == 233804 and r["Effect"] == 16 and r["EffectMiscValue1"] == 45636 for r in effects)
assert not any(r["SpellID"] == 237634 for r in effects)
smart_src = (root / 'src/server/game/AI/SmartScripts/SmartScript.cpp').read_text(encoding='utf-8-sig')
invoker_cast = smart_src.split('case SMART_ACTION_INVOKER_CAST:', 1)[1].split('case SMART_ACTION_ADD_AURA:', 1)[0]
assert 'AddDelayedEvent(200,' in invoker_cast
spell_src = (root / 'src/server/game/Spells/Spell.cpp').read_text(encoding='utf-8-sig')
assert spell_src.index('CallScriptAfterCastHandlers();') < spell_src.index('TakeReagents();')
assert 'CurrencyID != 0 && !_preventCurrencyReagentConsumption' in spell_src
print("PASS Legion 26972 seven route pairs, native reagents, delayed SmartAI dispatch and currency debit hook order")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Xylem admission DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$callback = [regex]::Match($source, '(?ms)^namespace MageTowerAdmission.*?(?=^class npc_mage_tower_victoria)')
$spell = [regex]::Match($source, '(?ms)^class spell_mage_tower_victoria_admission : public SpellScript.*?^\};')
if (!$callback.Success -or !$spell.Success) { throw 'Mage Tower production entry callbacks are absent' }
[IO.File]::WriteAllText("$out/XylemEntry.inc", $callback.Value + $spell.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerXylemEntryTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Xylem admission callback regression failed' }

param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Python,
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_21_01_mage_tower_feltotem_rupture.sql"
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
ids = {241664, 241665, 241666, 241667, 241668, 241669}
effects = {(r["SpellID"], r["EffectIndex"]): r for r in d.rows("SpellEffect").values() if r["SpellID"] in ids and r["DifficultyID"] == 0}
assert effects[241664,0]["Effect"] == 3
assert effects[241666,0]["Effect"] == 2 and effects[241666,0]["EffectBasePoints"] == 1899999
assert effects[241666,1]["Effect"] == 98 and effects[241666,1]["EffectMiscValue1"] == 100
for spell, entries in [(241665,(10341,10342)), (241667,(10343,7562)), (241668,(10344,10345)), (241669,(10346,10347))]:
    assert tuple(effects[spell,i]["EffectMiscValue1"] for i in (0,1)) == entries
    assert all(effects[spell,i]["Effect"] == 179 for i in (0,1))
misc = {r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in ids and r["DifficultyID"] == 0}
assert d.rows("SpellCastTimes")[misc[241667]["CastingTimeIndex"]]["Base"] == 2500
assert d.rows("SpellDuration")[misc[241665]["DurationIndex"]]["Duration"] == 2000
assert {r["SpellID"]: r["SpellVisualID"] for r in d.rows("SpellXSpellVisual").values() if r["SpellID"] in ids} == {
    241665:57911, 241667:57910, 241668:57911, 241669:57934}
print("PASS native 26972 Fel Rupture damage, knockback, trigger pairs, warning and impact visuals.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Feltotem rupture DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$callback = [regex]::Match($source, '(?ms)^bool IsInRuptureCross\(.*?^}')
$dimensions = [regex]::Matches($source, '(?m)^constexpr float RuptureHalf(?:Length|Width|Height) = [^;]+;')
if (!$callback.Success -or $dimensions.Count -ne 3) { throw 'Feltotem rupture production geometry is absent' }
$loader = Get-Content -LiteralPath "$repo/src/server/game/Globals/AreaTriggerData.cpp" -Raw
$migration = Get-Content -LiteralPath $MigrationPath -Raw
$templates = [regex]::Matches($migration, '(?m)^    \((\d+), 2416\d\d, (10\d+),')
if ($templates.Count -ne 8 -or @($templates | Where-Object { $_.Groups[1].Value -ne '0' }).Count -or
    $loader -notmatch [regex]::Escape('id ? _areaTriggerData[id] : _areaTriggerDataByEntry[customEntry]') -or
    $loader -notmatch [regex]::Escape('float halfX = info.Radius / 2.0f;') -or
    $source -notmatch [regex]::Escape('ObjectGuid::Empty, RuptureTemplateOffset + entries[i]')) {
    throw 'Fel Rupture must use the custom template store without overwriting a shared native entry'
}
'PASS all eight rupture templates load under private custom keys.'
[IO.File]::WriteAllLines("$out/FeltotemRupture.inc", @($dimensions.Value) + @($callback.Value))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerFeltotemRuptureTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Feltotem rupture geometry regression failed' }

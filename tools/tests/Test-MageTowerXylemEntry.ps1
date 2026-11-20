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
entry = [r for r in d.rows("SpellEffect").values() if r["SpellID"] == 229196 and r["DifficultyID"] == 0]
assert len(entry) == 1 and entry[0]["EffectIndex"] == 0 and entry[0]["Effect"] == 227 and entry[0]["EffectMiscValue2"] == 1406
assert not any(r["SpellID"] == 229196 for r in d.rows("SpellReagentsCurrency").values())
assert not any(r["Effect"] == 16 and r["EffectMiscValue1"] == 45635 for r in d.rows("SpellEffect").values())
dungeon = d.rows("LFGDungeons")[1406]
assert (dungeon["MapID"], dungeon["ScenarioID"]) == (1673, 1204)
print("PASS Legion 26972 Xylem spell, destination and absence of built-in currency or attempt credit.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Xylem admission DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$callback = [regex]::Match($source, '(?ms)^namespace ClosingTheEye.*?(?=^class npc_mage_tower_victoria)')
if (!$callback.Success) { throw 'Xylem production entry callback is absent' }
[IO.File]::WriteAllText("$out/XylemEntry.inc", $callback.Value)
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

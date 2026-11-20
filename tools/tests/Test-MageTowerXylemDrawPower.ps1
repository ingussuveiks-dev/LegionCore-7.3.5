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
effects = {(r["SpellID"],r["EffectIndex"]):r for r in d.rows("SpellEffect").values()
           if r["SpellID"] in (231522,231610) and r["DifficultyID"] == 0}
channel = effects[231522,1]
assert (channel["Effect"],channel["EffectAura"],channel["EffectAuraPeriod"],channel["EffectTriggerSpell"],channel["ImplicitTarget1"]) == (6,23,1000,0,38)
stack = effects[231610,0]
assert (stack["Effect"],stack["EffectAura"],stack["EffectBasePoints"],stack["ImplicitTarget1"]) == (6,79,5,1)
misc = {r["SpellID"]:r for r in d.rows("SpellMisc").values() if r["SpellID"] in (231522,231610) and r["DifficultyID"] == 0}
duration=d.rows("SpellDuration")
assert duration[misc[231522]["DurationIndex"]]["Duration"] == 10000 and misc[231522]["Attributes2"] & 4
assert duration[misc[231610]["DurationIndex"]]["Duration"] == -1
assert any(r["SpellID"] == 231610 and r["CumulativeAura"] == 99 for r in d.rows("SpellAuraOptions").values())
assert any(r["SpellID"] == 231522 and r["InterruptFlags"] == 41 for r in d.rows("SpellInterrupts").values())
print("PASS Legion 26972 10-second interruptible channel, one-second blank trigger, and native 5% stack with its 99 cap.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Xylem Draw Power DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$callback = [regex]::Match($source, '(?ms)^namespace ClosingTheEye\r?\n\{\r?\nconstexpr uint32 Xylem =.*?(?=^class npc_mage_tower_xylem)')
if (!$callback.Success) { throw 'Xylem production Draw Power controller is absent' }
[IO.File]::WriteAllText("$out/XylemDrawPower.inc", $callback.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerXylemDrawPowerTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Xylem Draw Power callback regression failed' }

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
ids={242987,242989}
effects=[r for r in d.rows("SpellEffect").values() if r["SpellID"] in ids and not r["DifficultyID"]]
assert len(effects)==2
assert all((r["EffectIndex"],r["Effect"],r["ImplicitTarget1"],r["ImplicitTarget2"])==(0,252,1,46) for r in effects)
casts=d.rows("SpellCastTimes")
assert all(casts[r["CastingTimeIndex"]]["Base"]==2000 for r in d.rows("SpellMisc").values()
           if r["SpellID"] in ids and not r["DifficultyID"])
print("PASS 26972 native two-second Translocate variants and matching nearby-entry teleport effects.")
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Agatha Translocate DB2 catalog mismatch' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$positions = [regex]::Match($source, '(?ms)^enum class AgathaPosition.*?^};')
$choice = [regex]::Match($source, '(?ms)^uint32 GetAgathaTranslocate\(.*?^\}')
$failed = [regex]::Match($source, '(?ms)^        void TranslocateFailed\(.*?^        \}')
$update = [regex]::Match($source, '(?ms)^        void UpdateTranslocate\(.*?^        \}')
if (!$positions.Success -or !$choice.Success -or !$failed.Success -or !$update.Success) { throw 'Production Agatha Translocate callbacks are absent' }
[IO.File]::WriteAllLines("$out/AgathaTranslocateChoice.inc", @($positions.Value, $choice.Value))
[IO.File]::WriteAllLines("$out/AgathaTranslocateUpdate.inc", @(
    $failed.Value.Replace('void TranslocateFailed', 'void AgathaAI::TranslocateFailed'),
    $update.Value.Replace('void UpdateTranslocate', 'void AgathaAI::UpdateTranslocate')
))

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerAgathaTranslocateTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Agatha Translocate callback regression failed' }

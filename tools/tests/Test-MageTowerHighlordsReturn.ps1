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

root = Path(sys.argv[1])
sys.dont_write_bytecode = True
sys.path.insert(0, str(root / 'tools/research'))
from audit_spell_mechanics import Data

dbc = root / 'build-extractors/bin/Release/dbc/enUS'
data = Data(dbc, dbc)
dungeon = data.rows('LFGDungeons')[1440]
assert (dungeon['MapID'], dungeon['ScenarioID']) == (1698, 1259)
trees = data.rows('CriteriaTree')
criteria = data.rows('Criteria')
for root_id, assets in ((56886, {56480, 56553}), (56889, {56552, 56796})):
    assert trees[root_id]['Operator'] == 8
    leaves = [criteria[row['CriteriaID']] for row in trees.values() if row['Parent'] == root_id]
    assert len(leaves) == 2 and all(row['Type'] == 92 for row in leaves)
    assert {row['Asset'] for row in leaves} == assets
print('PASS 26972 solo LFG route and both native kill-plus-survival criteria trees')
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Highlord survival DB2 regression failed' }

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$production = [regex]::Match($source, '(?ms)^constexpr uint32 HighlordsReturnMap.*?^\};')
if (!$production.Success) { throw 'Production Highlord survival controller is missing' }
[IO.File]::WriteAllText("$out/HighlordsReturn.inc", "namespace`n{`n" + $production.Value)

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerHighlordsReturnTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd", $cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Highlord survival callback regression failed' }

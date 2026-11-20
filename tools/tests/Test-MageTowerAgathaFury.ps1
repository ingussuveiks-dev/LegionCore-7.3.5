param(
    [Parameter(Mandatory)][string]$FixtureOptionsFile,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$MigrationPath = "$PSScriptRoot/../../sql/updates/world/2026_11_20_03_mage_tower_agatha_fury.sql"
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath "$PSScriptRoot/../..").Path
$out = (New-Item -ItemType Directory -Path $OutputDirectory -Force).FullName
$migration = Get-Content -LiteralPath $MigrationPath -Raw
$maria = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
$sql = @"
CREATE TEMPORARY TABLE scoped_creatures AS SELECT * FROM creature_template WHERE entry IN (115638,115640,115641,115642,115643,115719);
CREATE TEMPORARY TABLE scoped_scripts AS SELECT * FROM smart_scripts WHERE entryorguid IN (115638,115640,115641,115642,115643,115719);
CREATE TEMPORARY TABLE creature_template LIKE scoped_creatures;
CREATE TEMPORARY TABLE smart_scripts LIKE scoped_scripts;
INSERT INTO creature_template SELECT * FROM scoped_creatures;
INSERT INTO smart_scripts SELECT * FROM scoped_scripts;
SELECT CONCAT(AIName, ':', ScriptName) FROM creature_template WHERE entry=115638;
SELECT CONCAT(event_type, ':', action_type, ':', action_param1) FROM smart_scripts WHERE entryorguid=115638 AND source_type=0 AND id=14;
$migration
$migration
SELECT CONCAT(AIName, ':', ScriptName) FROM creature_template WHERE entry=115638;
SELECT (SELECT COUNT(*) FROM scoped_scripts) - COUNT(*) FROM smart_scripts;
SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=115638 AND source_type=0 AND id=14;
SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=115638 AND source_type=0 AND id=15 AND event_type=6 AND action_param2=55521;
SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=115642 AND source_type=0 AND id=2 AND event_type=6 AND action_param1=243027;
"@
$options = (Resolve-Path -LiteralPath $FixtureOptionsFile).Path
$rows = @($sql | & $maria "--defaults-extra-file=$options" --database=legion_world --batch --raw --skip-column-names)
$expected = @('SmartAI:', '0:11:242879', ':npc_mage_tower_agatha', '1', '0', '1', '1')
if ($LASTEXITCODE -or (Compare-Object $rows $expected -SyncWindow 0)) {
    $rows
    throw 'Agatha rebinding, timer retirement or preserved death rows mismatch'
}

$source = Get-Content -LiteralPath "$repo/src/server/scripts/Legion/zone_broken_shore.cpp" -Raw
$body = [regex]::Match($source,'(?ms)^        void SummonedCreatureDies\(Creature\* summon, Unit\* /\*killer\*/\) override.*?^        }')
if (!$body.Success) { throw 'Agatha production death callback could not be extracted' }
[IO.File]::WriteAllText("$out/AgathaFury.inc", $body.Value)
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd"$out/test.pdb" /I"$out" "$PSScriptRoot/MageTowerAgathaFuryTest.cpp" /Fo"$out/test.obj" /Fe"$out/test.exe" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if ($LASTEXITCODE) { throw 'Agatha callback regression failed' }
'PASS: Agatha binding replaces one timer and preserves native victory and Umbral shield cleanup.'

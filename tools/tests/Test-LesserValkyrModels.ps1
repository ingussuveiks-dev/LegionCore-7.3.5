$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/lesser-valkyr-models"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_23_lesser_valkyr_invalid_spawn_model.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_creature LIKE creature;
INSERT INTO saved_creature SELECT * FROM creature WHERE id=45239;
CREATE TEMPORARY TABLE creature LIKE saved_creature;
INSERT INTO creature SELECT * FROM saved_creature;
UPDATE creature SET modelid=26101 WHERE guid IN (239816,239817,239818,239820,239822,239823,239824,239826);
UPDATE creature SET modelid=33997 WHERE guid=239813;
CREATE TEMPORARY TABLE original_creature LIKE creature;
INSERT INTO original_creature SELECT * FROM creature;
UPDATE creature SET id=45240 WHERE guid=239816;
$migration
SELECT 'spawn',o.modelid,c.modelid FROM creature c JOIN original_creature o USING(guid) WHERE c.id=45239 ORDER BY guid;
SELECT 'neighbour',modelid FROM creature WHERE id=45240;
SELECT 'preserved',COUNT(*) FROM creature c JOIN original_creature o USING(guid) WHERE c.map=o.map AND c.position_x=o.position_x AND c.position_y=o.position_y AND c.position_z=o.position_z AND c.spawnMask=o.spawnMask AND c.phaseMask=o.phaseMask;
SELECT 'model',DisplayID,BoundingRadius,CombatReach,DisplayID_Other_Gender,hostileId FROM creature_model_info WHERE DisplayID IN(25517,33997,26101) ORDER BY DisplayID;
SELECT 'template',Displayid1,Displayid2,Displayid3,Displayid4 FROM creature_template_wdb WHERE Entry=45239;
SELECT 'remaining',COUNT(*) FROM creature WHERE id=45239 AND modelid=26101;
$migration
SELECT 'repeat',o.modelid,c.modelid FROM creature c JOIN original_creature o USING(guid) WHERE c.id=45239 ORDER BY guid;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Temporary Val kyr fixture failed'}
} finally {$env:MYSQL_PWD=$old}
$spawn=@($rows | Where-Object {$_ -like "spawn`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
$repeat=@($rows | Where-Object {$_ -like "repeat`t*"} | ForEach-Object {($_ -split "`t",2)[1]})
if(($spawn -join '|') -ne ($repeat -join '|') -or "neighbour`t26101" -notin $rows -or "preserved`t15" -notin $rows -or "remaining`t0" -notin $rows -or "template`t25517`t33997`t0`t0" -notin $rows){throw 'Val kyr migration changed unrelated data or was not idempotent'}
# Test all fifteen actual NPCs, including the unchanged valid override.
$spawn += "26101`t0"
[IO.File]::WriteAllLines("$out/spawns.tsv",[string[]]$spawn)
[IO.File]::WriteAllLines("$out/models.tsv",[string[]]@($rows | Where-Object {$_ -like "model`t*"} | ForEach-Object {($_ -split "`t",2)[1]}))
$hotfix=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try {
 $env:MYSQL_PWD=$hotfix[3]
 $displayRows=@(& C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($hotfix[0]) --port=$($hotfix[1]) --user=$($hotfix[2]) --database=$($hotfix[4]) --batch --raw --execute='SELECT * FROM creature_display_info WHERE ID IN(25517,26101,33997)')
 if($LASTEXITCODE){throw 'Display hotfix read failed'}
 $links=@(& C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($hotfix[0]) --port=$($hotfix[1]) --user=$($hotfix[2]) --database=$($hotfix[4]) --batch --raw --execute='SELECT Id,TableHash,RecordID,Deleted FROM hotfix_data WHERE RecordID IN(25517,26101,33997) ORDER BY Id')
 if($LASTEXITCODE){throw 'Display hotfix deletion read failed'}
} finally {$env:MYSQL_PWD=$old}
# Empty mysql results may omit the header; Data only needs an overlay when rows exist.
if($displayRows.Count -gt 1){[IO.File]::WriteAllLines("$out/mechanics-creature_display_info.tsv",[string[]]$displayRows)}
elseif(Test-Path "$out/mechanics-creature_display_info.tsv"){Remove-Item -LiteralPath "$out/mechanics-creature_display_info.tsv"}
if($links.Count -gt 1){[IO.File]::WriteAllLines("$out/mechanics-link-hotfixes.tsv",[string[]]$links)}
elseif(Test-Path "$out/mechanics-link-hotfixes.tsv"){Remove-Item -LiteralPath "$out/mechanics-link-hotfixes.tsv"}
& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe -X utf8 "$PSScriptRoot/lesser_valkyr_native_data_test.py" "$repo" "$out"
if($LASTEXITCODE){throw 'Native 7.3.5 display check failed'}
function Export-Function($source,$signature,$name){
 $a=$source.IndexOf($signature);if($a -lt 0){throw "Missing native function $signature"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
$source=Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw
Export-Function $source 'int32 ObjectMgr::ChooseDisplayId(' 'ChooseDisplayId'
Export-Function $source 'uint32 ObjectMgr::GetCreatureDisplay(' 'GetCreatureDisplay'
Export-Function $source 'CreatureModelInfo const* ObjectMgr::GetCreatureModelRandomGender(' 'GetCreatureModelRandomGender'
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
Export-Function $source 'int32 CreatureTemplate::GetRandomValidModelId() const' 'GetRandomValidModelId'
Export-Function $source 'int32 CreatureTemplate::GetFirstValidModelId() const' 'GetFirstValidModelId'
$a=$source.IndexOf('    // Cancel load if no model defined',$source.IndexOf('bool Creature::InitEntry('))
$b=$source.IndexOf('    // Load creature equipment',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native InitEntry model branch missing'}
[IO.File]::WriteAllText("$out/InitEntryModel.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/LesserValkyrModelTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/models.tsv`" `"$out/spawns.tsv`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Val kyr model initialization regression failed'}
'PASS: isolated migration preserves valid overrides, template models, unrelated entries and spawn coordinates; repeat application is stable.'

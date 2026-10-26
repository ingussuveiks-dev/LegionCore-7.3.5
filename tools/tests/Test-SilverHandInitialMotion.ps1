param([switch]$VerifyDatabase,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-initial-motion"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$waypoint=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
$header=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h" -Raw
[IO.File]::WriteAllText("$out/WaypointConstructor.inc",(Block $header 'explicit WaypointMovementGenerator(uint32 pathId = 0'))
[IO.File]::WriteAllText("$out/MotionTimer.inc",(Block (Get-Content "$repo/src/common/Utilities/Timer.h" -Raw) 'struct TimeTrackerSmall')+';')
[IO.File]::WriteAllText("$out/SelectMovement.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAISelector.cpp" -Raw) 'MovementGenerator* SelectMovementGenerator('))
$motion=Get-Content "$repo/src/server/game/Movement/MotionMaster.cpp" -Raw
$idle=Get-Content "$repo/src/server/game/Movement/MovementGenerators/IdleMovementGenerator.cpp" -Raw
$methods=@((Block $waypoint 'void WaypointMovementGenerator<Creature>::LoadPath('),(Block $waypoint 'void WaypointMovementGenerator<Creature>::DoInitialize('),(Block $waypoint 'bool WaypointMovementGenerator<Creature>::CanMove('),(Block $motion 'void MotionMaster::InitDefault('),(Block $motion 'void MotionMaster::MovePath(uint32'),(Block $idle 'void IdleMovementGenerator::Initialize('),(Block $idle 'void IdleMovementGenerator::Reset('))
[IO.File]::WriteAllText("$out/InitialMotionMethods.inc",($methods -join "`n"))
$migration=Get-Content "$repo/sql/updates/world/2026_10_26_00_silver_hand_initial_motion.sql" -Raw
$targets='361893,361894,361895,361986,361987,361991,362006'
$all="$targets,362008,362009,362044,362046,362048"
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$sql){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Initial motion SQL check failed'};if(!$result.Count){throw "Initial motion SQL returned no rows despite exit0; query length=$($sql.Length), inspect retained fixture SQL and native stderr"};$result}finally{$env:MYSQL_PWD=$previous}}
$snapshot=@(Query "SET SESSION TRANSACTION READ ONLY;SELECT guid,id,map,PhaseId,MovementType FROM creature WHERE guid IN ($all) ORDER BY guid;SELECT 'path',id,COUNT(*) FROM waypoint_data_script WHERE id BETWEEN 10993804 AND 10993807 GROUP BY id;SELECT 'zero',COUNT(*) FROM waypoint_data WHERE id=0;")
[IO.File]::WriteAllLines("$out/live-before.tsv",[string[]]$snapshot)
$fixture=@"
CREATE TEMPORARY TABLE creature SELECT * FROM creature WHERE guid IN ($all);
CREATE TEMPORARY TABLE creature_template SELECT * FROM creature_template WHERE entry IN (106370,106371,106429,106639,106676,106719,106774,106777);
CREATE TEMPORARY TABLE creature_addon SELECT * FROM creature_addon WHERE guid IN ($all);
CREATE TEMPORARY TABLE creature_template_addon SELECT * FROM creature_template_addon WHERE entry IN (106370,106371,106429,106639,106676,106719,106774,106777);
UPDATE creature SET MovementType=2 WHERE guid IN ($targets);
SELECT 'before',spawn.guid,spawn.id,spawn.MovementType,COALESCE(addon.path_id,inherited.path_id,0) FROM creature spawn LEFT JOIN creature_addon addon ON addon.guid=spawn.guid LEFT JOIN creature_template_addon inherited ON inherited.entry=spawn.id WHERE spawn.guid IN ($targets) ORDER BY spawn.guid;
$migration
SELECT 'changed',ROW_COUNT();
SELECT 'after',spawn.guid,spawn.id,spawn.MovementType,COALESCE(addon.path_id,inherited.path_id,0) FROM creature spawn LEFT JOIN creature_addon addon ON addon.guid=spawn.guid LEFT JOIN creature_template_addon inherited ON inherited.entry=spawn.id WHERE spawn.guid IN ($targets) ORDER BY spawn.guid;
$migration
SELECT 'idempotent',ROW_COUNT();
SELECT 'other',guid,id,MovementType FROM creature WHERE guid NOT IN ($targets) ORDER BY guid;
UPDATE creature SET MovementType=2 WHERE guid=362006;
UPDATE creature SET map=1 WHERE guid=362006;
$migration
SELECT 'wrong-map',ROW_COUNT();
UPDATE creature SET map=1539,id=106639 WHERE guid=362006;
$migration
SELECT 'wrong-entry',ROW_COUNT();
UPDATE creature SET id=106676,PhaseId='7000' WHERE guid=362006;
$migration
SELECT 'wrong-phase',ROW_COUNT();
UPDATE creature SET PhaseId='7002',MovementType=1 WHERE guid=362006;
$migration
SELECT 'other-motion',ROW_COUNT();
UPDATE creature SET MovementType=2,spawndist=5 WHERE guid=362006;
$migration
SELECT 'custom-radius',ROW_COUNT();
UPDATE creature SET spawndist=0 WHERE guid=362006;
UPDATE creature_template SET MovementType=1 WHERE entry=106676;
$migration
SELECT 'template-motion',ROW_COUNT();
UPDATE creature_template SET MovementType=0,ScriptName='unrelated' WHERE entry=106676;
$migration
SELECT 'other-script',ROW_COUNT();
UPDATE creature_template SET ScriptName='npc_travar_third' WHERE entry=106676;
UPDATE creature_template_addon SET path_id=42 WHERE entry=106676;
$migration
SELECT 'inherited-path',ROW_COUNT();
INSERT INTO creature_addon (guid,path_id) VALUES (362006,43);
$migration
SELECT 'spawn-path',ROW_COUNT();
UPDATE creature_addon SET path_id=0 WHERE guid=362006;
$migration
SELECT 'zero-override',ROW_COUNT();
"@
[IO.File]::WriteAllText("$out/fixture.sql",$fixture)
$rows=@(Query $fixture)
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]($snapshot.Where({$_ -like "path`t*"})+$rows))
foreach($expected in @("changed`t7","idempotent`t0","wrong-map`t0","wrong-entry`t0","wrong-phase`t0","other-motion`t0","custom-radius`t0","template-motion`t0","other-script`t0","inherited-path`t0","spawn-path`t0","zero-override`t1","other`t362008`t106371`t0","other`t362009`t106370`t0","other`t362044`t106719`t2","other`t362046`t106774`t0","other`t362048`t106777`t0")){if($rows -notcontains $expected){throw "Initial motion fixture mismatch: $expected"}}
foreach($expected in @("path`t10993804`t11","path`t10993805`t19","path`t10993806`t15","path`t10993807`t22","zero`t0")){if($snapshot -notcontains $expected){throw "Native path metadata changed: $expected"}}
$after=@(Query "SET SESSION TRANSACTION READ ONLY;SELECT guid,id,map,PhaseId,MovementType FROM creature WHERE guid IN ($all) ORDER BY guid;SELECT 'path',id,COUNT(*) FROM waypoint_data_script WHERE id BETWEEN 10993804 AND 10993807 GROUP BY id;SELECT 'zero',COUNT(*) FROM waypoint_data WHERE id=0;")
if(Compare-Object $snapshot $after){throw 'Temporary fixtures changed persistent movement data'}
if($VerifyDatabase){
 $expected=@("361893`t106371`t1539`t5738 7005`t0","361894`t106370`t1539`t5738 7005`t0","361895`t106429`t1539`t5738 7005`t0","361986`t106371`t1539`t7000`t0","361987`t106370`t1539`t7000`t0","361991`t106639`t1539`t7000`t0","362006`t106676`t1539`t7002`t0")
 foreach($row in $expected){if($snapshot -notcontains $row){throw "Deployed initial motion mismatch: $row"}}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/SilverHandInitialMotionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/fixture.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native initial motion regression failed'}
Write-Host 'PASS guarded migration 7/0; ten negative/override cases; five unrelated phase/boss spawns unchanged; persistent data untouched by fixtures.'
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_initial_motion_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed initial-motion implementation mismatch'}}finally{$env:PYTHONPATH=$previous}
}

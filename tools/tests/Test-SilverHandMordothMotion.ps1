param([switch]$VerifyDatabase,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-mordoth-motion"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Test-SilverHandInitialMotion.ps1" -VerifyDatabase -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Shared native motion regression failed'}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
if($smart -notmatch 'mCanCombatMove = true;'){throw 'Native initial SmartAI combat-movement policy changed'}
[IO.File]::WriteAllText("$out/MordothSmartMethods.inc",(Block $smart 'void SmartAI::AttackStart(')+"`n"+(Block $smart 'void SmartAI::JustReachedHome('))
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$sql){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE -or !$rows.Count){throw "Mordoth SQL probe failed/empty; exit=$LASTEXITCODE length=$($sql.Length), retained fixture.sql"};$rows}finally{$env:MYSQL_PWD=$previous}}
$liveSql="SELECT 'spawn',guid,id,map,PhaseId,MovementType,spawndist FROM creature WHERE id=106719;SELECT 'template',entry,MovementType,AIName,ScriptName,unit_flags,unit_flags2,faction FROM creature_template WHERE entry=106719;SELECT 'addon',entry,path_id,auras FROM creature_template_addon WHERE entry=106719;SELECT 'script',entryorguid,source_type,id,event_type,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid IN (106719,-362044) ORDER BY source_type,id;SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2 FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=8;SELECT 'no-spawn-addon',COUNT(*) FROM creature_addon WHERE guid=362044;SELECT 'no-ordinary-path',COUNT(*) FROM waypoint_data WHERE id IN (0,106719,362044);SELECT 'path',id,COUNT(*) FROM waypoint_data_script WHERE id BETWEEN 10993804 AND 10993807 GROUP BY id;"
$snapshot=@(Query ("SET SESSION TRANSACTION READ ONLY;"+$liveSql))
[IO.File]::WriteAllLines("$out/live-before.tsv",[string[]]$snapshot)
foreach($expected in @("template`t106719`t0`tSmartAI`t`t33536`t4196352`t16","addon`t106719`t0`t212300","script`t106719`t0`t0`t0`t2000`t2000`t2000`t2000`t11`t215690`t2","phase`t7001`t44`t1092`t7","no-spawn-addon`t0","no-ordinary-path`t0")){if($snapshot -notcontains $expected){throw "Native Mordoth context changed: $expected"}}
if(@($snapshot.Where({$_ -like "spawn`t*"})).Count -ne 1){throw 'Mordoth DB spawn scope changed'}
if($VerifyDatabase -and $snapshot -notcontains "spawn`t362044`t106719`t1539`t7001`t0`t0"){throw 'Canonical Mordoth movement repair absent'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_26_01_silver_hand_mordoth_initial_motion.sql" -Raw
$fixture=@"
CREATE TEMPORARY TABLE creature SELECT * FROM creature WHERE guid=362044;
CREATE TEMPORARY TABLE creature_template SELECT * FROM creature_template WHERE entry=106719;
CREATE TEMPORARY TABLE creature_addon SELECT * FROM creature_addon WHERE guid=362044;
CREATE TEMPORARY TABLE creature_template_addon SELECT * FROM creature_template_addon WHERE entry=106719;
UPDATE creature SET MovementType=2 WHERE guid=362044;
INSERT INTO creature SELECT * FROM creature WHERE guid=362044;
UPDATE creature SET guid=9000001 WHERE guid=362044 LIMIT 1;
SELECT 'before',spawn.guid,spawn.id,spawn.MovementType,COALESCE(addon.path_id,inherited.path_id,0) FROM creature spawn LEFT JOIN creature_addon addon ON addon.guid=spawn.guid LEFT JOIN creature_template_addon inherited ON inherited.entry=spawn.id WHERE spawn.guid=362044;
$migration
SELECT 'changed',ROW_COUNT();
SELECT 'after',spawn.guid,spawn.id,spawn.MovementType,COALESCE(addon.path_id,inherited.path_id,0) FROM creature spawn LEFT JOIN creature_addon addon ON addon.guid=spawn.guid LEFT JOIN creature_template_addon inherited ON inherited.entry=spawn.id WHERE spawn.guid=362044;
SELECT 'positive-template',MovementType,AIName,ScriptName,unit_flags,unit_flags2,faction FROM creature_template WHERE entry=106719;
SELECT 'positive-addon',path_id,auras FROM creature_template_addon WHERE entry=106719;
$migration
SELECT 'idempotent',ROW_COUNT();
SELECT 'shared-entry',guid,MovementType FROM creature WHERE guid=9000001;
UPDATE creature SET MovementType=2,map=1 WHERE guid=362044;
$migration
SELECT 'wrong-map',ROW_COUNT();
UPDATE creature SET map=1539,id=106676 WHERE guid=362044;
$migration
SELECT 'wrong-entry',ROW_COUNT();
UPDATE creature SET id=106719,PhaseId='7003' WHERE guid=362044;
$migration
SELECT 'wrong-phase',ROW_COUNT();
UPDATE creature SET PhaseId='7001',MovementType=1 WHERE guid=362044;
$migration
SELECT 'other-motion',ROW_COUNT();
UPDATE creature SET MovementType=2,spawndist=5 WHERE guid=362044;
$migration
SELECT 'custom-radius',ROW_COUNT();
UPDATE creature SET spawndist=0 WHERE guid=362044;
UPDATE creature_template SET MovementType=1 WHERE entry=106719;
$migration
SELECT 'template-motion',ROW_COUNT();
UPDATE creature_template SET MovementType=0,AIName='other' WHERE entry=106719;
$migration
SELECT 'other-ai',ROW_COUNT();
UPDATE creature_template SET AIName='SmartAI',ScriptName='unrelated' WHERE entry=106719;
$migration
SELECT 'other-script',ROW_COUNT();
UPDATE creature_template SET ScriptName='' WHERE entry=106719;
UPDATE creature_template_addon SET path_id=42 WHERE entry=106719;
$migration
SELECT 'inherited-path',ROW_COUNT();
INSERT INTO creature_addon (guid,path_id) VALUES (362044,43);
$migration
SELECT 'spawn-path',ROW_COUNT();
UPDATE creature_addon SET path_id=0 WHERE guid=362044;
$migration
SELECT 'zero-override',ROW_COUNT();
SELECT 'template-retained',MovementType,AIName,ScriptName,unit_flags,unit_flags2,faction FROM creature_template WHERE entry=106719;
"@
[IO.File]::WriteAllText("$out/fixture.sql",$fixture)
$rows=@(Query $fixture)
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]($snapshot.Where({$_ -like "path`t*"})+$rows))
foreach($expected in @("changed`t1","idempotent`t0","positive-template`t0`tSmartAI`t`t33536`t4196352`t16","positive-addon`t0`t212300","shared-entry`t9000001`t2","wrong-map`t0","wrong-entry`t0","wrong-phase`t0","other-motion`t0","custom-radius`t0","template-motion`t0","other-ai`t0","other-script`t0","inherited-path`t0","spawn-path`t0","zero-override`t1","template-retained`t0`tSmartAI`t`t33536`t4196352`t16")){if($rows -notcontains $expected){throw "Mordoth fixture mismatch: $expected"}}
$after=@(Query ("SET SESSION TRANSACTION READ ONLY;"+$liveSql))
if(Compare-Object $snapshot $after){throw 'Mordoth temporary fixture changed persistent data'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$repo/build-extractors/runtime-test/silver-hand-initial-motion`" `"$PSScriptRoot/SilverHandMordothMotionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/fixture.tsv`" 1`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Mordoth initial motion/SmartAI regression failed'}
Write-Host 'PASS one guarded static-spawn update/idempotence; shared-entry and eleven negative/override cases; template/phase/aura/combat rows retained; no persistent fixture writes.'
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_mordoth_motion_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed SmartAI movement implementation mismatch'}}finally{$env:PYTHONPATH=$previous}
}

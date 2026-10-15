param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-final-judgment"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_40_stormheim_final_judgment.sql" -Raw
$sql=@"
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38818;
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=93234 AND quest=38818;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=97319 AND quest=38818;
SELECT 'next-offer',COUNT(*) FROM creature_queststarter s JOIN quest_template_addon a ON a.ID=s.quest WHERE s.id=97319 AND s.quest=39837 AND a.PrevQuestID=38818;
SELECT 'handoff',q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,q.AllowableRaces FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=38818;
SELECT 'faljar-spawn',COUNT(*) FROM creature WHERE guid=377348 AND id=93093 AND map=1220 AND zoneId=7541 AND PhaseId='4005' AND modelid=0 AND ABS(position_x-3626.52)<0.01 AND ABS(position_y-791.312)<0.01 AND ABS(position_z-38.7806)<0.01;
SELECT 'spirit-spawn',COUNT(*) FROM creature WHERE guid=292001 AND id=93428 AND map=1220 AND ABS(position_x-3639.2)<0.01 AND ABS(position_y-777.75)<0.01 AND ABS(position_z-40.2274)<0.01;
SELECT 'helheim',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid=11546745 AND c.id=97319 AND c.map=1463 AND t.npcflag=2 AND ABS(c.position_x-341.621)<0.01 AND ABS(c.position_y-356.556)<0.01;
SELECT 'scaling',LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry=93093;
SELECT 'models',COUNT(*) FROM creature_model_info WHERE DisplayID IN(64593,68635) AND BoundingRadius>0 AND CombatReach>0;
SELECT 'phase',ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=151 ORDER BY ElseGroup,ConditionValue1;
SELECT 'detection',COUNT(*) FROM spell_area WHERE spell=100044 AND area=7653 AND quest_start=38818 AND quest_start_status=8 AND autocast=1;
SELECT 'poses',COUNT(*) FROM creature_template_addon WHERE (entry=93093 AND auras='209164 209363 89319') OR (entry=93428 AND bytes1=8 AND auras='209164');
SELECT 'instance',COUNT(*) FROM instance_template WHERE map=1463;
"@
if(!$Live){$sql+=@"
CREATE TEMPORARY TABLE saved_template LIKE creature_template;
INSERT INTO saved_template SELECT * FROM creature_template WHERE entry IN(93093,93428,97319,117199,116496);
CREATE TEMPORARY TABLE creature_template LIKE saved_template;
INSERT INTO creature_template SELECT * FROM saved_template;
UPDATE creature_template SET ScriptName='' WHERE entry=93093;
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(93093,97319,117199,116496)) OR (source_type=9 AND entryorguid=9309300);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
-- Seed the retired rows even after live deployment; use their real old shapes.
DELETE FROM smart_scripts WHERE (source_type=0 AND entryorguid=93093 AND id IN(0,1,2)) OR (source_type=9 AND entryorguid=9309300 AND id BETWEEN 0 AND 7) OR (source_type=0 AND entryorguid=97319 AND id=0);
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES
(93093,0,0,2,80,9309300,1,'fixture'),(93093,0,1,25,42,35,1,'fixture'),(93093,0,2,4,42,1,1,'fixture'),
(9309300,9,0,0,1,4,1,'fixture'),(9309300,9,1,0,69,0,8,'fixture'),(9309300,9,2,0,11,209310,11,'fixture'),(9309300,9,3,0,11,208544,24,'fixture'),
(9309300,9,4,0,62,1463,24,'fixture'),(9309300,9,5,0,2,16,1,'fixture'),(9309300,9,6,0,33,93442,24,'fixture'),(9309300,9,7,0,41,0,1,'fixture'),(97319,0,0,60,33,93442,18,'fixture');
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES (93093,0,99,25,1,99,1,'fixture'),(9309300,9,99,0,1,99,1,'fixture');
$migration
$migration
UPDATE saved_template SET ScriptName='npc_stormheim_final_judgment' WHERE entry=93093;
SELECT 'template-actual',t.* FROM creature_template t ORDER BY entry;
SELECT 'template-expected',t.* FROM saved_template t ORDER BY entry;
SELECT 'other-actual',s.* FROM smart_scripts s WHERE source_type=0 AND entryorguid IN(117199,116496) ORDER BY entryorguid,id;
SELECT 'other-expected',s.* FROM saved_sai s WHERE source_type=0 AND entryorguid IN(117199,116496) ORDER BY entryorguid,id;
SELECT 'extra',COUNT(*) FROM smart_scripts WHERE id=99 AND entryorguid IN(93093,9309300);
"@}
$sql+=@"
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=93093 AND AIName='SmartAI' AND ScriptName='npc_stormheim_final_judgment' AND unit_flags=33024 AND faction=16;
SELECT 'legacy',COUNT(*) FROM smart_scripts WHERE (source_type=0 AND entryorguid=93093 AND id IN(0,1,2)) OR (source_type=9 AND entryorguid=9309300 AND id BETWEEN 0 AND 7) OR (source_type=0 AND entryorguid=97319 AND id=0 AND action_type=33 AND action_param1=93442);
"@
if(!$Live){$sql+=@"
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES (93093,0,0,2,80,9309300,1,'fixture'),(9309300,9,0,0,1,4,1,'fixture'),(97319,0,0,60,33,93442,18,'fixture');
UPDATE creature_template SET ScriptName='custom_faljar' WHERE entry=93093;
$migration
SELECT 'custom-binding',ScriptName FROM creature_template WHERE entry=93093;
SELECT 'custom-sai',COUNT(*) FROM smart_scripts WHERE (entryorguid=93093 AND id=0) OR (entryorguid=9309300 AND id=0) OR (entryorguid=97319 AND id=0);
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final Judgment SQL check failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("objective`t278476`t0`t0`t93442`t1`t0","starter`t1","ender`t1","next-offer`t1","handoff`t0`t38815`t0`t18446744073709551615","faljar-spawn`t1","spirit-spawn`t1","helheim`t1","scaling`t98`t110","models`t2","detection`t1","poses`t2","instance`t0","binding`t1","legacy`t0","phase`t0`t9`t38818`t0","phase`t0`t9`t45522`t1","phase`t1`t28`t38818`t0","phase`t1`t9`t45522`t1")){if($rows -notcontains $line){throw "Missing: $line"}}
if(!$Live){
 foreach($tag in @('template','other')){
  $a="$tag-actual`t";$b="$tag-expected`t";$actual=@($rows|Where-Object {$_ -like "$a*"}|ForEach-Object {$_.Substring($a.Length)});$expected=@($rows|Where-Object {$_ -like "$b*"}|ForEach-Object {$_.Substring($b.Length)})
  if(!$actual.Count -or (Compare-Object $actual $expected)){throw "Unexpected $tag change"}
 }
 foreach($line in @("extra`t2","custom-binding`tcustom_faljar","custom-sai`t3")){if($rows -notcontains $line){throw "Missing: $line"}}
}
$rows|Set-Content "$out/data$(if($Live){'-live'}).tsv"
'PASS: actual38818 objective/actors/scaling/phases/detection/Helheim menu route; migration twice, exact legacy removal, shared other Faljars and custom rows/binding preserved.'

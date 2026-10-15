param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-reformation"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_39_stormheim_reformation.sql" -Raw
$sql=@"
SELECT 'current-objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38815;
SELECT 'rune',COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.guid=115183 AND g.id=244450 AND g.map=1220 AND g.zoneId=7541 AND g.phaseMask=1 AND g.PhaseId='' AND t.type=10 AND t.Data0=1691 AND t.Data1=38815 AND t.Data3=3000 AND t.Data10=192069 AND t.flags=262148 AND t.ScriptName='' AND t.AIName='';
SELECT 'phase',ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=20 ORDER BY ElseGroup;
SELECT 'phase-definition',COUNT(*) FROM phase_definitions WHERE zoneId=7541 AND entry=20 AND phaseId='6776' AND phasemask=0 AND flags=16;
SELECT 'ashildir',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_questender e ON e.id=c.id AND e.quest=38815 JOIN creature_queststarter s ON s.id=c.id AND s.quest=38818 WHERE c.id=93234 AND c.guid IN(338671,341220) AND c.map=1220 AND c.PhaseId='6776' AND c.modelid=0 AND t.npcflag=2 AND t.ScriptName='';
SELECT 'next',q.RewardNextQuest,a.PrevQuestID,a.NextQuestID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=38815;
SELECT 'helper-other',COUNT(*) FROM creature WHERE id=97281;
SELECT 'helper-sai',COUNT(*) FROM smart_scripts WHERE entryorguid=97281 AND source_type=0;
SELECT 'helper-addon',COUNT(*) FROM creature_template_addon WHERE entry=97281;
SELECT 'helper-models',COUNT(*) FROM creature_model_info WHERE DisplayID IN(328,27823,68635) AND BoundingRadius>0;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(192069,192062)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry=38815);
"@
if(!$Live){$sql+=@"
CREATE TEMPORARY TABLE saved_template LIKE creature_template;
INSERT INTO saved_template SELECT * FROM creature_template WHERE entry IN(97281,93234,93428);
CREATE TEMPORARY TABLE creature_template LIKE saved_template;
INSERT INTO creature_template SELECT * FROM saved_template;
CREATE TEMPORARY TABLE saved_objectives LIKE quest_objectives;
INSERT INTO saved_objectives SELECT * FROM quest_objectives WHERE QuestID IN(38815,38818);
CREATE TEMPORARY TABLE quest_objectives LIKE saved_objectives;
INSERT INTO quest_objectives SELECT * FROM saved_objectives;
UPDATE quest_objectives SET Type=2,ObjectID=244450 WHERE ID=278471;
UPDATE creature_template SET ScriptName='' WHERE entry=97281;
CREATE TEMPORARY TABLE spell_target_position (id MEDIUMINT UNSIGNED PRIMARY KEY,target_map SMALLINT UNSIGNED,target_position_x FLOAT,target_position_y FLOAT,target_position_z FLOAT,target_orientation FLOAT);
INSERT INTO spell_target_position VALUES(184582,1220,3116.57,1247.91,125.037,0);
$migration
$migration
UPDATE saved_template SET ScriptName='npc_stormheim_reformation_bones' WHERE entry=97281;
SELECT 'actual',t.* FROM creature_template t ORDER BY entry;
SELECT 'expected',t.* FROM saved_template t ORDER BY entry;
UPDATE saved_objectives SET Type=0,ObjectID=97281 WHERE ID=278471;
SELECT 'objective-actual',o.* FROM quest_objectives o ORDER BY ID;
SELECT 'objective-expected',o.* FROM saved_objectives o ORDER BY ID;
SELECT 'unrelated',COUNT(*) FROM spell_target_position WHERE id=184582 AND ABS(target_position_x-3116.57)<0.001;
"@}
$sql+=@"
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38815;
SELECT 'destination',id,target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id=192069;
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=97281 AND AIName='' AND ScriptName='npc_stormheim_reformation_bones' AND npcflag=0 AND unit_flags=33555200 AND unit_flags2=2048;
"@
if(!$Live){$sql+=@"
UPDATE spell_target_position SET target_position_x=123 WHERE id=192069;
UPDATE creature_template SET ScriptName='custom_helper' WHERE entry=97281;
UPDATE quest_objectives SET ObjectID=9999 WHERE ID=278471;
$migration
SELECT 'custom',target_position_x FROM spell_target_position WHERE id=192069;
SELECT 'custom-binding',ScriptName FROM creature_template WHERE entry=97281;
SELECT 'custom-objective',ObjectID FROM quest_objectives WHERE ID=278471;
DELETE FROM spell_target_position WHERE id=192069;
CREATE TEMPORARY TABLE saved_go LIKE gameobject;
CREATE TEMPORARY TABLE gameobject LIKE saved_go;
$migration
SELECT 'missing-source',COUNT(*) FROM spell_target_position WHERE id=192069;
"@}
# Configuration is read privately; do not place credentials in process arguments.
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Reformation SQL check failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("objective`t278471`t0`t1`t97281`t1`t0","rune`t1","phase`t0`t28`t38815`t0","phase`t1`t8`t38815`t0","phase-definition`t1","ashildir`t2","next`t38818`t38816`t38818","helper-other`t0","helper-sai`t0","helper-addon`t0","helper-models`t3","conditions`t0","destination`t192069`t1220`t3532.88`t887.547`t53.4053`t2.82809","binding`t1")){if($rows -notcontains $line){throw "Missing: $line"}}
if(!$Live){
 $actual=@($rows | Where-Object {$_ -like "actual`t*"} | ForEach-Object {$_.Substring(7)})
 $expected=@($rows | Where-Object {$_ -like "expected`t*"} | ForEach-Object {$_.Substring(9)})
 if($actual.Count -ne 3 -or (Compare-Object $actual $expected)){throw 'Unexpected creature template change'}
 $actual=@($rows | Where-Object {$_ -like "objective-actual`t*"} | ForEach-Object {$_.Substring(17)})
 $expected=@($rows | Where-Object {$_ -like "objective-expected`t*"} | ForEach-Object {$_.Substring(19)})
 if($actual.Count -ne 2 -or (Compare-Object $actual $expected)){throw 'Unexpected objective change'}
 foreach($line in @("unrelated`t1","custom`t123","custom-binding`tcustom_helper","custom-objective`t9999","missing-source`t0")){if($rows -notcontains $line){throw "Missing: $line"}}
}
$rows | Set-Content "$out/data$(if($Live){'-live'}).tsv"
($rows | Where-Object {$_ -like "destination`t*"}) -replace '^destination\t','' | Set-Content "$out/destination.tsv"
'PASS: actual rune/objective/phase/endgiver handoff; repeat migration, scoped binding/destination, custom/unrelated preservation and missing-source refusal. Client appearance remains unverified.'

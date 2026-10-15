param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-helheim-disguise"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_44_stormheim_helheim_disguise.sql" -Raw
$sql=@"
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38347;
SELECT 'quests',q.ID,q.StartScript,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38347,39848) ORDER BY q.ID;
SELECT 'ender-next',COUNT(*) FROM creature_questender e JOIN creature_queststarter s ON s.id=e.id AND s.quest=39848 WHERE e.id IN(97419,97480) AND e.quest=38347;
SELECT 'fallback',COUNT(*) FROM creature WHERE id=97480 AND guid=11546796 AND map=1463 AND PhaseId='' AND ABS(position_x-327.851)<0.001 AND ABS(position_y-574.399)<0.001;
SELECT 'guard',COUNT(*) FROM creature WHERE id=91386 AND guid=11546644 AND map=1463 AND PhaseId='' AND modelid=0 AND ABS(position_x-455.686)<0.001 AND ABS(position_y-526.597)<0.001;
SELECT 'later-guard',COUNT(*) FROM creature WHERE id=91386 AND guid=12743029 AND map=1463 AND PhaseId='8306' AND modelid=0;
SELECT 'model-info',COUNT(*) FROM creature_model_info WHERE DisplayID IN(70104,68635) AND BoundingRadius>0 AND CombatReach>0;
SELECT 'exit',COUNT(*) FROM eventobject WHERE guid=400326 AND id=334 AND map=1463 AND PhaseId='' AND ABS(position_x-454.412)<0.001 AND ABS(position_y-565.988)<0.001 AND ABS(position_z-0.083255)<0.001;
SELECT 'phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7588 AND SourceEntry=4 AND ConditionTypeOrReference=9 AND ConditionValue1=38347 AND NegativeCondition=0;
"@
if(!$Live){
 foreach($table in @('creature_template','eventobject_template','smart_scripts','conditions')){
  $filter=switch($table){creature_template{'entry IN(91386,91531,97419,97480)'}eventobject_template{'entry=334'}smart_scripts{'entryorguid IN(91386,91531,97419,97480) OR (entryorguid=334 AND source_type=13)'}conditions{'(SourceTypeOrReferenceId=22 AND SourceEntry IN(334,91386)) OR (SourceTypeOrReferenceId=23 AND SourceGroup=7588)'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(91386,91531,97419);
UPDATE eventobject_template SET ScriptName='SmartEventObject' WHERE entry=334;
DELETE FROM smart_scripts WHERE (entryorguid=91531 AND source_type=0 AND id IN(0,1)) OR (entryorguid=91386 AND source_type=0 AND id=0) OR (entryorguid=97419 AND source_type=0 AND id=1) OR (entryorguid=334 AND source_type=13 AND id=0);
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1,target_param2,comment) VALUES
(91531,0,0,1,19,100,38347,0,85,192299,0,7,0,0,'legacy controller'),
(91531,0,1,0,61,100,0,0,85,181705,0,7,0,0,'legacy linked disguise'),
(91386,0,0,0,10,100,1,4,33,91386,0,7,0,0,'legacy near guard credit'),
(97419,0,1,0,38,100,1,1,33,91386,0,21,10,0,'legacy nearest player credit'),
(334,13,0,0,89,100,0,0,45,1,1,11,97419,50,'legacy relay');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND ((SourceEntry=334 AND SourceId=10 AND SourceGroup=1) OR (SourceEntry=91386 AND SourceId=0 AND SourceGroup=1));
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionTarget,ConditionValue1) VALUES(22,1,334,10,9,0,38347),(22,1,91386,0,1,0,181705);
$migration
$migration
UPDATE saved_creature_template SET AIName='',ScriptName='npc_stormheim_helheim_ashildir' WHERE entry=97419;
UPDATE saved_creature_template SET AIName='' WHERE entry IN(91386,91531);
UPDATE saved_eventobject_template SET ScriptName='eventobject_stormheim_helheim_passage' WHERE entry=334;
DELETE FROM saved_smart_scripts WHERE (entryorguid=91531 AND source_type=0 AND id IN(0,1)) OR (entryorguid=91386 AND source_type=0 AND id=0) OR (entryorguid=97419 AND source_type=0 AND id=1) OR (entryorguid=334 AND source_type=13 AND id=0);
DELETE FROM saved_conditions WHERE SourceTypeOrReferenceId=22 AND ((SourceEntry=334 AND SourceId=10 AND SourceGroup=1) OR (SourceEntry=91386 AND SourceId=0 AND SourceGroup=1));
"@
 foreach($table in @('creature_template','eventobject_template','smart_scripts','conditions')){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'actor-binding',COUNT(*) FROM creature_template WHERE entry=97419 AND AIName='' AND ScriptName='npc_stormheim_helheim_ashildir' AND npcflag=2 AND faction=35 AND unit_flags=33600;
SELECT 'exit-binding',COUNT(*) FROM eventobject_template WHERE entry=334 AND ScriptName='eventobject_stormheim_helheim_passage' AND radius=4 AND SpellID=0 AND WorldSafeLocID=0 AND Flags=0;
SELECT 'retired',COUNT(*) FROM smart_scripts WHERE (entryorguid=91531 AND source_type=0 AND id IN(0,1)) OR (entryorguid=91386 AND source_type=0 AND id=0) OR (entryorguid=97419 AND source_type=0 AND id=1) OR (entryorguid=334 AND source_type=13 AND id=0);
SELECT 'native-ash-actions',COUNT(*) FROM smart_scripts WHERE entryorguid=97419 AND source_type=0 AND id IN(0,2,3,4);
SELECT 'empty-ai',COUNT(*) FROM creature_template WHERE entry IN(91386,91531) AND AIName='' AND ScriptName='';
SELECT 'old-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND ((SourceEntry=334 AND SourceId=10 AND SourceGroup=1) OR (SourceEntry=91386 AND SourceId=0 AND SourceGroup=1));
"@
if(!$Live){$sql+=@"
UPDATE creature_template SET AIName='SmartAI',ScriptName='custom_ashildir' WHERE entry=97419;
UPDATE eventobject_template SET ScriptName='custom_passage' WHERE entry=334;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_param1,action_type,action_param1,target_type,comment) VALUES(91531,0,0,1,19,100,38347,85,192299,7,'custom retained event');
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionTarget,ConditionValue1) VALUES(22,1,334,10,9,0,38347);
$migration
SELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=97419 AND ScriptName='custom_ashildir';
SELECT 'custom-exit',COUNT(*) FROM eventobject_template WHERE entry=334 AND ScriptName='custom_passage';
SELECT 'custom-event',COUNT(*) FROM smart_scripts WHERE entryorguid=91531 AND source_type=0 AND id=0;
SELECT 'custom-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=334 AND SourceId=10;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Disguise SQL failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("objective`t277732`t0`t1`t91386`t1`t0","quests`t38347`t0`t39848`t18446744073709551615`t38324`t39848`t0","quests`t39848`t0`t39857`t18446744073709551615`t38347`t39857`t0","ender-next`t2","fallback`t1","guard`t1","later-guard`t1","model-info`t2","exit`t1","phase`t1","actor-binding`t1","exit-binding`t1","retired`t0","native-ash-actions`t4","empty-ai`t2","old-condition`t0")){if($rows -notcontains $line){throw "Missing: $line"}}
if(!$Live){
 foreach($table in @('creature_template','eventobject_template','smart_scripts','conditions')){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if(!$actual.Count -or (Compare-Object $actual $expected)){throw "Unexpected $table change"}
 }
 foreach($line in @("custom-actor`t1","custom-exit`t1","custom-event`t1","custom-condition`t1")){if($rows -notcontains $line){throw "Missing: $line"}}
}
$rows | Set-Content "$out/data$(if($Live){'-live'}).tsv"
'PASS: real MySQL repeat migration, full template/SAI/condition preservation, five retired shortcuts, native dialogue/next quest/phase, exact base and later guard, fallback and custom bindings. Client interaction remains unverified.'

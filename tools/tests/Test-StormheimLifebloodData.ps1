param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-lifeblood"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_04_stormheim_lifeblood.sql" -Raw
$sql=@"
SELECT 'quest',q.ID,q.QuestPackageID,q.Flags,q.FlagsEx,q.StartItem,q.StartScript,q.CompleteScript,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SourceSpellID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39855;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39855;
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE quest=39855 AND id IN(97480,97558);
SELECT 'ender',COUNT(*) FROM creature_questender WHERE quest=39855 AND id=92539;
SELECT 'havi',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.id=92539 AND c.guid=291280 AND c.map=1220 AND c.PhaseId='' AND c.modelid=0 AND ABS(c.position_x-3214.38)<0.001 AND ABS(c.position_y-1543.61)<0.001 AND ABS(c.position_z-180.715)<0.001 AND t.npcflag=3 AND t.AIName='SmartAI' AND w.Displayid1=76630;
SELECT 'altar',COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.id=244559 AND g.guid=127713 AND g.map=1463 AND g.PhaseId='' AND t.type=2 AND t.Data3=18790 AND t.AIName='' AND t.ScriptName='go_stormheim_helheim_bargain';
SELECT 'later-altar',COUNT(*) FROM gameobject WHERE id=244559 AND guid=25210114 AND PhaseId='8306';
SELECT 'later-marker',COUNT(*) FROM creature WHERE id=97487 AND guid=12743041 AND PhaseId='8306';
SELECT 'menu',COUNT(*) FROM gossip_menu_option g JOIN conditions c ON c.SourceTypeOrReferenceId=15 AND c.SourceGroup=g.MenuID AND c.SourceEntry=g.OptionID WHERE g.MenuID=18790 AND g.OptionID=1 AND g.OptionNpc=0 AND g.OptionBroadcastTextID=99516 AND c.ConditionTypeOrReference=9 AND c.ConditionValue1=39855;
SELECT 'return-spawn',COUNT(*) FROM eventobject WHERE id=333 AND guid=400325 AND map=1463 AND PhaseId='' AND ABS(position_x-336.092)<0.001 AND ABS(position_y-345.864)<0.001 AND ABS(position_z-30.6906)<0.001;
SELECT 'return-destination',target_map,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id=241140;
SELECT 'native-destination',target_map,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id=192670;
SELECT 'actor',entry,actorId,creatureId,displayId,duration FROM conversation_actor WHERE entry IN(2257,2258,2259) ORDER BY entry;
SELECT 'native-actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry IN(2257,2258,2259) ORDER BY entry;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(2257,2258,2259) ORDER BY entry;
SELECT 'ashildir-text',COUNT(*) FROM creature_text WHERE CreatureID=97558 AND GroupID=3 AND BroadcastTextID=98494 AND Sound=53005 AND Emote=1;
SELECT 'no-controller',COUNT(*) FROM spell_area WHERE spell IN(192469,192471,217358) OR quest_start=39855 OR quest_end=39855;
SELECT 'no-spawn',COUNT(*) FROM creature WHERE id=97558;
"@
$tables=@{
 creature_template='entry IN(97558,92539,97480,97487)';eventobject_template='entry=333';
 smart_scripts='(source_type=1 AND entryorguid=244559) OR (source_type=0 AND entryorguid IN(97558,92539,97480,97487)) OR (source_type=9 AND entryorguid IN(9253900,9253901))';
 quest_template='ID=39855';quest_template_addon='ID=39855';quest_objectives='QuestID=39855';
 creature='id IN(97558,92539,97480,97487)';gameobject='id=244559';gameobject_template='entry=244559';
 eventobject='id=333';conditions='(SourceTypeOrReferenceId=15 AND SourceGroup=18790) OR (SourceTypeOrReferenceId=22 AND SourceEntry=333 AND SourceId=10)';
 creature_queststarter='quest=39855';creature_questender='quest=39855';gossip_menu_option='MenuID=18790'
}
if(!$Live){
 foreach($table in ($tables.Keys | Sort-Object)){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
UPDATE creature_template SET ScriptName='' WHERE entry=97558;
UPDATE eventobject_template SET ScriptName='SmartEventObject' WHERE entry=333;
DELETE FROM smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id IN(3,4);
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_param1,event_param2,action_type,action_param1,target_type,target_x,target_y,target_z,target_o,comment) VALUES
(244559,1,3,4,62,100,18790,1,33,97487,7,0,0,0,0,'legacy group credit'),
(244559,1,4,0,61,100,0,0,62,1220,7,3152.87,1536.12,181.33,6.09,'legacy teleport');
$migration
$migration
UPDATE saved_creature_template SET ScriptName='npc_stormheim_lifeblood_ashildir' WHERE entry=97558;
UPDATE saved_eventobject_template SET ScriptName='eventobject_stormheim_lifeblood_return' WHERE entry=333;
DELETE FROM saved_smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id IN(3,4);
"@
 foreach($table in ($tables.Keys | Sort-Object)){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'binding',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=97558 AND t.ScriptName='npc_stormheim_lifeblood_ashildir' AND t.AIName='' AND t.npcflag=2 AND t.faction=35 AND t.unit_flags=33088 AND w.Displayid1=68635;
SELECT 'gate-binding',COUNT(*) FROM eventobject_template WHERE entry=333 AND SpellID=241140 AND WorldSafeLocID=0 AND radius=2 AND Flags=0 AND ScriptName='eventobject_stormheim_lifeblood_return';
SELECT 'legacy-retired',COUNT(*) FROM smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id IN(3,4);
SELECT 'native-accept',COUNT(*) FROM smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id=0 AND event_type=19 AND event_param1=39857 AND action_type=206 AND action_param1=2252;
"@
if(!$Live){$sql+=@"
UPDATE creature_template SET ScriptName='custom_ashildir' WHERE entry=97558;
UPDATE eventobject_template SET ScriptName='custom_return' WHERE entry=333;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,comment) VALUES(244559,1,3,62,100,1,123,'custom row3'),(244559,1,4,61,100,1,456,'custom row4');
$migration
SELECT 'custom',COUNT(*) FROM creature_template t JOIN eventobject_template e ON e.entry=333 WHERE t.entry=97558 AND t.ScriptName='custom_ashildir' AND e.ScriptName='custom_return';
SELECT 'custom-rows',COUNT(*) FROM smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id IN(3,4) AND action_type=1;
UPDATE gameobject_template SET ScriptName='custom_altar' WHERE entry=244559;
UPDATE smart_scripts SET link=4,event_type=62,event_param1=18790,event_param2=1,action_type=33,action_param1=97487,target_type=7 WHERE entryorguid=244559 AND source_type=1 AND id=3;
UPDATE smart_scripts SET link=0,event_type=61,action_type=62,action_param1=1220,target_type=7,target_x=3152.87,target_y=1536.12,target_z=181.33 WHERE entryorguid=244559 AND source_type=1 AND id=4;
$migration
SELECT 'custom-altar-rows',COUNT(*) FROM smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id IN(3,4);
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Lifeblood World query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("quest`t39855`t9690`t35651592`t262144`t0`t0`t0`t0`t39853`t0`t0`t0","objective`t280035`t0`t0`t97487`t1`t0","starters`t2","ender`t1","havi`t1","altar`t1","later-altar`t1","later-marker`t1","menu`t1","return-spawn`t1","return-destination`t1220`t3641.49`t774.516`t-3.08297","native-destination`t1220`t3155.89`t1541.77`t180.31","actor`t2257`t53867`t91387`t65043`t8823","actor`t2258`t50552`t91387`t65043`t4978","actor`t2259`t50552`t91387`t65043`t6396","line`t2257`t4753`t0`t0`t620`t256","line`t2258`t4754`t0`t0`t620`t0","line`t2259`t4755`t0`t0`t620`t0","ashildir-text`t1","no-controller`t0","no-spawn`t0","binding`t1","gate-binding`t1","legacy-retired`t0","native-accept`t1")){if($rows -notcontains $line){throw "Missing Lifeblood data $line"}}
if(!$Live){
 foreach($table in $tables.Keys){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $table"}
 }
 if($rows -notcontains "custom`t1" -or $rows -notcontains "custom-rows`t2" -or $rows -notcontains "custom-altar-rows`t2"){throw 'Custom bindings/rows overwritten'}
}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(192470,192471,212916,212917,212918,216818,192670,192671,192672,241140); SELECT 'visuals',COUNT(*) FROM spell_x_spell_visual WHERE SpellID IN(216818,192670,241140); SELECT 'displays',COUNT(*) FROM creature_display_info WHERE ID IN(65043,68635,76630); SELECT 'lines',COUNT(*) FROM conversation_line WHERE ID IN(4753,4754,4755); SELECT 'voices',ID,SoundEntriesID1,SoundEntriesID2 FROM broadcast_text WHERE ID IN(98494,98498,98499,98501) ORDER BY ID;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Lifeblood Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("effects`t0","visuals`t0","displays`t0","lines`t0","voices`t98494`t0`t53005","voices`t98498`t0`t52988","voices`t98499`t0`t52989","voices`t98501`t0`t52991")){if($hotfix -notcontains $line){throw "Missing Lifeblood Hotfix evidence $line"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: repeated real MySQL39855 migration/full rows/custom preservation, original quest/relations/menu/altar/later spawns, native dialogue records/durations/voice overlays, original Havi and both distinct exit destinations.'

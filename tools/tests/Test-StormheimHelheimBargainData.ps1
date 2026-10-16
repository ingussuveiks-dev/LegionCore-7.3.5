param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-helheim-bargain"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_00_stormheim_helheim_bargain.sql" -Raw
$sql=@"
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39848 ORDER BY ID;
SELECT 'quest',q.ID,q.StartScript,q.CompleteScript,q.Flags,q.FlagsEx,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39848;
SELECT 'enders',COUNT(*) FROM gameobject_questender e JOIN gameobject_queststarter s ON s.id=e.id AND s.quest=39857 WHERE e.id=244559 AND e.quest=39848;
SELECT 'next',COUNT(*) FROM quest_template_addon WHERE ID=39857 AND PrevQuestID=39848 AND NextQuestID=39849;
SELECT 'markers',COUNT(*) FROM creature WHERE id=97696 AND guid IN(338828,340857) AND map=1463 AND PhaseId='' AND modelid=0 AND ABS(position_x-484.451)<0.001 AND ABS(position_y-717.054)<0.001 AND ABS(position_z-23.3577)<0.001;
SELECT 'altar',COUNT(*) FROM gameobject WHERE id=244559 AND guid=127713 AND map=1463 AND PhaseId='' AND ABS(position_x-490.429)<0.001 AND ABS(position_y-721.396)<0.001 AND ABS(position_z-23.2744)<0.001;
SELECT 'later-altar',COUNT(*) FROM gameobject WHERE id=244559 AND guid=25210114 AND PhaseId='8306';
SELECT 'later-marker',COUNT(*) FROM creature WHERE id=97696 AND guid=12743040 AND PhaseId='8306';
SELECT 'gossip',COUNT(*) FROM gossip_menu_option WHERE MenuID=18790 AND OptionNpc=0 AND ((OptionID=0 AND OptionBroadcastTextID=99443) OR (OptionID=1 AND OptionBroadcastTextID=99516));
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18790 AND ConditionTypeOrReference=9 AND ((SourceEntry=0 AND ConditionValue1=39848) OR (SourceEntry=1 AND ConditionValue1=39855));
SELECT 'conversation',COUNT(*) FROM conversation_actor a JOIN conversation_data d ON d.entry=a.entry WHERE a.entry=2251 AND a.id=0 AND a.actorId=50552 AND a.creatureId=91387 AND a.displayId=65043 AND a.duration=5898 AND d.id=4747;
"@
if(!$Live){
 foreach($table in @('creature_template','gameobject_template','smart_scripts')){
  $filter=switch($table){creature_template{'entry IN(97695,97696)'}gameobject_template{'entry=244559'}smart_scripts{'(source_type=0 AND entryorguid IN(97695,97696)) OR (source_type=1 AND entryorguid=244559)'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry=97696;
UPDATE gameobject_template SET AIName='SmartGameObjectAI',ScriptName='' WHERE entry=244559;
DELETE FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(97695,97696) AND id=0) OR (source_type=1 AND entryorguid=244559 AND id IN(1,2));
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_param1,event_param2,action_type,action_param1,target_type,target_param1,comment) VALUES
(97695,0,0,0,60,100,1000,1000,33,97696,18,5,'legacy credit'),
(97696,0,0,0,60,100,1000,1000,33,97696,18,5,'legacy credit'),
(244559,1,1,2,62,100,18790,0,85,212898,7,0,'legacy cast'),
(244559,1,2,0,61,100,0,0,33,97479,7,0,'legacy credit');
$migration
$migration
UPDATE saved_creature_template SET AIName='',ScriptName='npc_stormheim_helheim_throne' WHERE entry=97696;
UPDATE saved_gameobject_template SET AIName='',ScriptName='go_stormheim_helheim_bargain' WHERE entry=244559;
DELETE FROM saved_smart_scripts WHERE (source_type=0 AND entryorguid IN(97695,97696) AND id=0) OR (source_type=1 AND entryorguid=244559 AND id IN(1,2));
"@
 foreach($table in @('creature_template','gameobject_template','smart_scripts')){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'bindings',COUNT(*) FROM creature_template t JOIN gameobject_template g ON g.entry=244559 WHERE t.entry=97696 AND t.AIName='' AND t.ScriptName='npc_stormheim_helheim_throne' AND g.AIName='' AND g.ScriptName='go_stormheim_helheim_bargain' AND g.type=2 AND g.Data1=19547 AND g.Data3=18790 AND g.Data10=0 AND g.flags=0 AND g.displayId=7786 AND g.size=1.25;
SELECT 'retired',COUNT(*) FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(97695,97696) AND id=0) OR (source_type=1 AND entryorguid=244559 AND id IN(1,2));
SELECT 'native-altar',COUNT(*) FROM smart_scripts WHERE entryorguid=244559 AND source_type=1 AND id IN(0,3,4);
SELECT 'native-sylvanas',COUNT(*) FROM smart_scripts WHERE entryorguid=97695 AND source_type=0 AND id IN(1,2);
"@
if(!$Live){$sql+=@"
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry=97696;
UPDATE gameobject_template SET AIName='SmartGameObjectAI',ScriptName='custom_altar' WHERE entry=244559;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_param1,event_param2,action_type,action_param1,target_type,target_param1,comment) VALUES(97696,0,0,0,60,100,1000,1000,33,97696,18,5,'custom retained');
$migration
SELECT 'custom',COUNT(*) FROM gameobject_template g JOIN creature_template t ON t.entry=97696 WHERE g.entry=244559 AND g.ScriptName='custom_altar' AND t.ScriptName='' AND t.AIName='SmartAI';
SELECT 'custom-event',COUNT(*) FROM smart_scripts WHERE entryorguid=97696 AND source_type=0 AND id=0;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bargain SQL failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("objective`t280017`t0`t2`t97479`t1`t2","objective`t280092`t0`t0`t97696`t1`t0","quest`t39848`t0`t0`t0`t0`t39857`t18446744073709551615`t38347`t39857`t0","enders`t1","next`t1","markers`t2","altar`t1","later-altar`t1","later-marker`t1","gossip`t2","conditions`t2","conversation`t1","bindings`t1","retired`t0","native-altar`t3","native-sylvanas`t2")){if($rows -notcontains $line){throw "Missing: $line"}}
if(!$Live){
 foreach($table in @('creature_template','gameobject_template','smart_scripts')){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if(Compare-Object $actual $expected){throw "Full-row preservation failed: $table"}
 }
 if($rows -notcontains "custom`t1" -or $rows -notcontains "custom-event`t1"){throw 'Custom binding was overwritten'}
}
$rows | Set-Content "$out/data$(if($Live){'-live'}).tsv"
'PASS: repeat real MySQL bargain migration, full-row preservation, two native objectives, questgiver altar/next relations, original gossip, conversation2251, later Warrior spawns and custom bindings.'

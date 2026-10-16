param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-eternal-victory"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_03_stormheim_geir.sql" -Raw
$sql=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.StartItem,q.StartScript,q.CompleteScript,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39853;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39853;
SELECT 'group',COUNT(*) FROM quest_template_addon WHERE ID IN(39850,39851) AND ExclusiveGroup=-39850 AND NextQuestID=39853;
SELECT 'relations',COUNT(*) FROM creature_questender e JOIN creature_queststarter s ON s.id=e.id AND s.quest=39855 WHERE e.quest=39853 AND e.id IN(97480,97558);
SELECT 'giver',COUNT(*) FROM creature_queststarter WHERE id=97480 AND quest=39853;
SELECT 'ashildir',COUNT(*) FROM creature WHERE id=97480 AND guid=11546796 AND map=1463 AND PhaseId='';
SELECT 'no-alternate-spawn',COUNT(*) FROM creature WHERE id=97558;
SELECT 'spawn',COUNT(*) FROM creature WHERE id=91948 AND guid=11546719 AND map=1463 AND PhaseId='' AND modelid=0 AND spawntimesecs=180 AND ABS(position_x-209.854)<0.001 AND ABS(position_y-587.158)<0.001 AND ABS(position_z+4.0533)<0.001;
SELECT 'later-spawns',COUNT(*) FROM creature WHERE id=91948 AND map=1463 AND PhaseId='8306' AND guid IN(12742855,12742860) AND modelid=0;
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=91948 AND LevelScalingMin=98 AND LevelScalingMax=110 AND LevelScalingDeltaMin=0 AND LevelScalingDeltaMax=0;
SELECT 'death',event_type,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=91948 AND source_type=0 AND id=2;
SELECT 'combat',id,event_param1,event_param2,event_param3,event_param4,action_param1,target_type FROM smart_scripts WHERE entryorguid=91948 AND source_type=0 AND event_type=0 ORDER BY id;
SELECT 'all-events',COUNT(*) FROM smart_scripts WHERE entryorguid=91948 AND source_type=0 AND Difficulties='' AND event_flags=0 AND link=0 AND event_chance=100;
SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry=2256;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=2256;
SELECT 'model',COUNT(*) FROM creature_model_info WHERE DisplayID=24301 AND BoundingRadius>0 AND CombatReach>0;
SELECT 'next',COUNT(*) FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39855 AND a.PrevQuestID=39853 AND q.StartScript=0 AND q.CompleteScript=0;
"@
if(!$Live){
 foreach($table in @('creature_template','smart_scripts','creature')){
  $filter=switch($table){creature_template{'entry IN(91948,97480,97558)'}smart_scripts{'source_type=0 AND entryorguid IN(91948,97480,97558)'}creature{'id IN(91948,97480,97558)'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry=91948;`n"+$migration+"`n"+$migration+"`n"
 $sql+="UPDATE saved_creature_template SET AIName='SmartAI',ScriptName='npc_stormheim_geir' WHERE entry=91948;`n"
 foreach($table in @('creature_template','smart_scripts','creature')){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'binding',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=91948 AND t.AIName='SmartAI' AND t.ScriptName='npc_stormheim_geir' AND t.npcflag=0 AND t.faction=1814 AND t.unit_flags=32768 AND t.unit_flags2=4196352 AND t.lootid=91948 AND w.Displayid1=24301 AND w.HpMulti=10 AND w.KillCredit1=0 AND w.KillCredit2=0;
"@
if(!$Live){$sql+="UPDATE creature_template SET ScriptName='custom_geir' WHERE entry=91948;`n"+$migration+"`nSELECT 'custom',COUNT(*) FROM creature_template WHERE entry=91948 AND AIName='SmartAI' AND ScriptName='custom_geir';`n"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Eternal Victory World query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("quest`t39853`t34603016`t0`t0`t0`t0`t39855`t39850`t39855`t0","objective`t280031`t0`t1`t91948`t1`t0","group`t2","relations`t2","giver`t1","ashildir`t1","no-alternate-spawn`t0","spawn`t1","later-spawns`t2","scaling`t1","death`t6`t206`t2256`t7","combat`t0`t7000`t8000`t14000`t16000`t182866`t1","combat`t1`t5000`t6000`t10000`t12000`t182877`t2","all-events`t3","actor`t2256`t0`t50552`t91387`t65043`t0`t1`t0`t6422","line`t2256`t4752`t0`t0`t620`t0","model`t1","next`t1","binding`t1")){if($rows -notcontains $line){throw "Missing Geir data $line"}}
if(!$Live){
 foreach($table in @('creature_template','smart_scripts','creature')){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $table"}
 }
 if($rows -notcontains "custom`t1"){throw 'Custom Geir binding overwritten'}
}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(182866,182868,182877); SELECT 'visual-links',COUNT(*) FROM spell_x_spell_visual WHERE SpellID IN(182866,182868,182877); SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID=24301; SELECT 'conversation-lines',COUNT(*) FROM conversation_line WHERE ID=4752; SELECT 'voice',COUNT(*) FROM broadcast_text WHERE ID=98495 AND SoundEntriesID1=0 AND SoundEntriesID2=53006;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Eternal Victory Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("effects`t0","visual-links`t0","display`t0","conversation-lines`t0","voice`t1")){if($hotfix -notcontains $line){throw "Missing Geir Hotfix evidence $line"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: real MySQL repeated Geir migration/full-row/custom preservation, native kill objective and negative-group prerequisites, three unchanged SmartAI events, model/scaling/base/later spawns, conversation2256 and actual Ashildir39855 handoff.'

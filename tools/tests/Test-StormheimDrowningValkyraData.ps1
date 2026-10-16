param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-drowning-valkyra"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_02_stormheim_drowning_valkyra.sql" -Raw
$sql=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.StartItem,q.StartScript,q.CompleteScript,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39851;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39851;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.id=s.id AND e.quest=s.quest WHERE s.id=97480 AND s.quest=39851;
SELECT 'group',COUNT(*) FROM quest_template_addon WHERE ID IN(39850,39851) AND PrevQuestID=39849 AND NextQuestID=39853 AND ExclusiveGroup=-39850;
SELECT 'click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=97469 AND spell_id=192387 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1;
SELECT 'condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=97469 AND SourceEntry=192387 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=39851 AND NegativeCondition=0;
SELECT 'all-click-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=97469;
SELECT 'pose',COUNT(*) FROM creature_template_addon WHERE entry=97469 AND bytes1=0 AND bytes2=0 AND auras='192388';
SELECT 'base-spawn',guid,position_x,position_y,position_z,orientation FROM creature WHERE id=97469 AND map=1463 AND PhaseId='' AND modelid=0 AND spawntimesecs=180 ORDER BY guid;
SELECT 'later-spawns',COUNT(*) FROM creature WHERE id=97469 AND map=1463 AND PhaseId='8306' AND dynamicflags=256;
SELECT 'native-event',COUNT(*) FROM smart_scripts WHERE entryorguid=97469 AND source_type=0 AND id=0 AND link=0 AND Difficulties='' AND event_type=8 AND event_flags=0 AND event_param1=192387 AND event_param3=10000 AND event_param4=10000 AND action_type=80 AND action_param1=9746900 AND action_param2=2 AND target_type=1;
SELECT 'native-tal',id,event_param1,event_param2,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=9746900 AND source_type=9 ORDER BY id;
SELECT 'dialogue',ID,BroadcastTextID FROM creature_text WHERE CreatureID=97469 AND GroupID=0 ORDER BY ID;
SELECT 'model',COUNT(*) FROM creature_model_info WHERE DisplayID=68954 AND BoundingRadius>0 AND CombatReach>0;
"@
if(!$Live){
 foreach($table in @('creature_template','spell_script_names','smart_scripts','creature','conditions','creature_template_addon')){
  $filter=switch($table){creature_template{'entry=97469'}spell_script_names{'spell_id=192387'}smart_scripts{'(source_type=0 AND entryorguid=97469) OR (source_type=9 AND entryorguid=9746900)'}creature{'id=97469'}conditions{'SourceTypeOrReferenceId=18 AND SourceGroup=97469'}creature_template_addon{'entry=97469'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry=97469; DELETE FROM spell_script_names WHERE spell_id=192387 AND ScriptName='spell_stormheim_drowning_valkyra';`n"+$migration+"`n"+$migration+"`n"
 $sql+="UPDATE saved_creature_template SET AIName='SmartAI',ScriptName='npc_stormheim_drowning_valkyra' WHERE entry=97469; INSERT IGNORE INTO saved_spell_script_names VALUES(192387,'spell_stormheim_drowning_valkyra');`n"
 foreach($table in @('creature_template','spell_script_names','smart_scripts','creature','conditions','creature_template_addon')){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'binding',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=97469 AND t.AIName='SmartAI' AND t.ScriptName='npc_stormheim_drowning_valkyra' AND t.npcflag=16777216 AND t.faction=190 AND t.unit_flags=33536 AND t.unit_flags2=2048 AND w.Displayid1=68954 AND w.TypeFlags=0;
SELECT 'spell',COUNT(*) FROM spell_script_names WHERE spell_id=192387 AND ScriptName='spell_stormheim_drowning_valkyra';
"@
if(!$Live){$sql+="UPDATE creature_template SET ScriptName='custom_valkyra' WHERE entry=97469;`n"+$migration+"`nSELECT 'custom',COUNT(*) FROM creature_template WHERE entry=97469 AND AIName='SmartAI' AND ScriptName='custom_valkyra';`n"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Drowning Valkyra World query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("quest`t39851`t33554440`t0`t0`t0`t0`t39853`t39849`t39853`t-39850`t0","objective`t280026`t0`t1`t97472`t8`t0","relations`t1","group`t2","click`t1","condition`t1","all-click-conditions`t1","pose`t1","later-spawns`t13","native-event`t1","native-tal`t0`t0`t0`t1`t0`t7","native-tal`t1`t0`t0`t33`t97472`t7","native-tal`t2`t4500`t4500`t47`t0`t1","native-tal`t3`t14000`t14000`t47`t1`t1","dialogue`t0`t99583","dialogue`t1`t99581","dialogue`t2`t99582","dialogue`t3`t24833","model`t1","binding`t1","spell`t1")){if($rows -notcontains $line){throw "Missing Valkyra data $line"}}
if(@($rows | Where-Object {$_ -like "base-spawn`t*"}).Count -ne 10){throw 'Expected ten base spawns'}
if(!$Live){
 foreach($table in @('creature_template','spell_script_names','smart_scripts','creature','conditions','creature_template_addon')){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $table"}
 }
 if($rows -notcontains "custom`t1"){throw 'Custom binding overwritten'}
}
$rows | Set-Content "$out/data$(if($Live){'-live'}).tsv"
($rows | Where-Object {$_ -like "base-spawn`t*"}) -replace '^base-spawn\t','' | Set-Content "$out/spawns.tsv"
$sql="SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(192387,192388); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID=192387; SELECT 'visuals',COUNT(*) FROM spell_x_spell_visual WHERE SpellID IN(192387,192388); SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID=68954;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Drowning Valkyra Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("effects`t0","misc`t0","visuals`t0","display`t0")){if($hotfix -notcontains $line){throw "Missing Valkyra Hotfix evidence $line"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: repeated real MySQL Valkyra migration, full-row/custom preservation, native TAL/SmartAI admission, ten base/thirteen later spawns, source18 quest condition, pose, dialogue and negative-group prerequisites.'

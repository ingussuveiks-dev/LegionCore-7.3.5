param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-sundered"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_01_stormheim_sundered.sql" -Raw
$sql=@"
SELECT 'objectives',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39850 ORDER BY ID;
SELECT 'group',COUNT(*) FROM quest_template_addon WHERE ID IN(39850,39851) AND PrevQuestID=39849 AND NextQuestID=39853 AND ExclusiveGroup=-39850;
SELECT 'next',COUNT(*) FROM quest_template_addon WHERE ID=39853 AND PrevQuestID=39850 AND NextQuestID=39855;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.id=s.id AND e.quest=s.quest WHERE s.id=97480 AND s.quest IN(39850,39851);
SELECT 'spawns',COUNT(*) FROM creature WHERE map=1463 AND PhaseId='' AND modelid=0 AND spawntimesecs=7200 AND ((id=97356 AND guid=146728276 AND ABS(position_x-231.622)<0.001 AND ABS(position_y-378.858)<0.001 AND ABS(position_z-6.28104)<0.001) OR(id=97570 AND guid=146728280 AND ABS(position_x-109.45)<0.001 AND ABS(position_y-449.408)<0.001 AND ABS(position_z+14.6794)<0.001) OR(id=97571 AND guid=146728281 AND ABS(position_x-112.366)<0.001 AND ABS(position_y-704.08)<0.001 AND ABS(position_z-17.823)<0.001));
SELECT 'clicks',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(97356,97570,97571) AND spell_id=192493 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1;
SELECT 'click-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN(97356,97570,97571);
SELECT 'poses',COUNT(*) FROM creature_template_addon WHERE bytes1=50331648 AND bytes2=1 AND ((entry=97356 AND auras='192214') OR(entry=97570 AND auras='192485') OR(entry=97571 AND auras='192486'));
SELECT 'helpers',COUNT(*) FROM smart_scripts WHERE source_type=0 AND event_type=8 AND event_param1=192500 AND action_type=11 AND target_type=7 AND ((entryorguid=97357 AND action_param1=192195) OR(entryorguid=97575 AND action_param1=192196) OR(entryorguid=97577 AND action_param1=192200) OR(entryorguid=97578 AND action_param1=192198));
SELECT 'conversations',COUNT(*) FROM conversation_data WHERE entry IN(2253,2254,2255);
"@
if(!$Live){
 foreach($table in @('creature_template','smart_scripts','quest_template','spell_script_names','quest_start_scripts')){
  $filter=switch($table){creature_template{'entry IN(97356,97570,97571,97357,97575,97577,97578)'}smart_scripts{'source_type=0 AND entryorguid IN(97356,97570,97571,97357,97575,97577,97578)'}quest_template{'ID IN(39849,39850,39851,39853)'}spell_script_names{'spell_id=192493'}quest_start_scripts{'id=39850'}}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(97356,97570,97571); UPDATE quest_template SET StartScript=39850 WHERE ID=39850; DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid IN(97356,97570,97571) AND id BETWEEN 0 AND 5; DELETE FROM spell_script_names WHERE spell_id=192493 AND ScriptName='spell_stormheim_sundered';`n"
 foreach($f in @(@(97356,97482,212902),@(97570,97483,212903),@(97571,97484,212904))){
  $sql+="INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_flags,event_param1,action_type,action_param1,target_type,comment) VALUES ($($f[0]),0,0,0,60,100,1,0,11,192500,1,'legacy chains'),($($f[0]),0,1,2,8,100,1,192493,33,$($f[1]),7,'legacy credit'),($($f[0]),0,2,3,61,100,1,0,11,192495,1,'legacy release'),($($f[0]),0,3,4,61,100,1,0,85,$($f[2]),1,'legacy conversation'),($($f[0]),0,4,5,61,100,1,0,81,0,1,'legacy flags'),($($f[0]),0,5,0,61,100,1,0,41,5000,1,'legacy despawn');`n"
 }
 $sql+=$migration+"`n"+$migration+"`n"
 $sql+="UPDATE saved_creature_template SET AIName='',ScriptName='npc_stormheim_sundered' WHERE entry IN(97356,97570,97571); DELETE FROM saved_smart_scripts WHERE source_type=0 AND entryorguid IN(97356,97570,97571) AND id BETWEEN 0 AND 5; UPDATE saved_quest_template SET StartScript=0 WHERE ID=39850; INSERT IGNORE INTO saved_spell_script_names VALUES(192493,'spell_stormheim_sundered');`n"
 foreach($table in @('creature_template','smart_scripts','quest_template','spell_script_names','quest_start_scripts')){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'bindings',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(97356,97570,97571) AND t.AIName='' AND t.ScriptName='npc_stormheim_sundered' AND t.npcflag=16777216 AND t.faction=190 AND t.unit_flags=0 AND w.Displayid1=65186;
SELECT 'start',StartScript FROM quest_template WHERE ID=39850;
SELECT 'retired',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(97356,97570,97571) AND id BETWEEN 0 AND 5;
SELECT 'spell',COUNT(*) FROM spell_script_names WHERE spell_id=192493 AND ScriptName='spell_stormheim_sundered';
"@
if(!$Live){$sql+=@"
UPDATE creature_template SET AIName='SmartAI',ScriptName='custom_fragment' WHERE entry=97356;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,target_type,comment) VALUES(97356,0,0,60,100,11,192500,1,'custom retained');
$migration
SELECT 'custom',COUNT(*) FROM creature_template WHERE entry=97356 AND AIName='SmartAI' AND ScriptName='custom_fragment';
SELECT 'custom-row',COUNT(*) FROM smart_scripts WHERE entryorguid=97356 AND source_type=0 AND id=0;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Sundered SQL failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("objectives`t280023`t0`t1`t97482`t1`t0","objectives`t280024`t0`t2`t97483`t1`t0","objectives`t280025`t0`t3`t97484`t1`t0","group`t2","next`t1","relations`t2","spawns`t3","clicks`t3","click-conditions`t0","poses`t3","helpers`t4","conversations`t3","bindings`t3","start`t0","retired`t0","spell`t1")){if($rows -notcontains $line){throw "Missing Sundered data $line"}}
if(!$Live){
 foreach($table in @('creature_template','smart_scripts','quest_template','spell_script_names','quest_start_scripts')){
  $a="actual-$table`t";$e="expected-$table`t"
  $actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object)
  $expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object)
  if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $table"}
 }
 if($rows -notcontains "custom`t1" -or $rows -notcontains "custom-row`t1"){throw 'Custom binding overwritten'}
}
$rows | Set-Content "$out/data$(if($Live){'-live'}).tsv"
$sql="SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(192493,192495,192500,192214,192485,192486,212902,212903,212904); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID=192493; SELECT 'visual-links',COUNT(*) FROM spell_x_spell_visual WHERE SpellID IN(192493,192495,192214,192485,192486); SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID=65186;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Sundered Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("effects`t0","misc`t0","visual-links`t0","display`t0")){if($hotfix -notcontains $line){throw "Missing Hotfix evidence $line"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: real MySQL repeated Sundered migration, full-row preservation/custom binding, actual click/pose/chain data, three native objectives, both negative-group prerequisites and original conversations.'

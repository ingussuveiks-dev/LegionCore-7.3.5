param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-familiar-fate"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_06_stormheim_familiar_fate.sql" -Raw
$tables=@{creature_template='entry IN(91743,97890,97979,97986)';creature='id IN(91743,97890,97979,97986)';creature_template_addon='entry IN(91743,97890,97979,97986)';smart_scripts='entryorguid IN(91743,97890,97979,97986)';gossip_menu='MenuID IN(18828,12618)';gossip_menu_option='MenuID=18828';conditions='SourceTypeOrReferenceId=15 AND SourceGroup=18828';creature_loot_template='Entry=97890';quest_template='ID IN(40001,40002,40003)';quest_template_addon='ID IN(40001,40002,40003)';quest_objectives='QuestID IN(40002,40003)';creature_queststarter='quest IN(40002,40003)';creature_questender='quest IN(40002,40003)';spell_script_names='spell_id IN(193253,193262)'}
$sql=''
if(!$Live){
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="CREATE TEMPORARY TABLE saved_$t LIKE $t; INSERT INTO saved_$t SELECT * FROM $t WHERE $($tables[$t]); CREATE TEMPORARY TABLE $t LIKE saved_$t; INSERT INTO $t SELECT * FROM saved_$t;`n"}
 $sql+="UPDATE creature_template SET ScriptName='' WHERE entry=91743; DELETE FROM spell_script_names WHERE spell_id=193253 AND ScriptName='spell_stormheim_familiar_shard';`n$migration`n$migration`n"
 $sql+="UPDATE saved_creature_template SET ScriptName='npc_stormheim_familiar_binding' WHERE entry=91743; INSERT INTO saved_spell_script_names(spell_id,ScriptName) SELECT 193253,'spell_stormheim_familiar_shard' WHERE NOT EXISTS(SELECT 1 FROM saved_spell_script_names WHERE spell_id=193253 AND ScriptName='spell_stormheim_familiar_shard');`n"
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="SELECT 'actual-$t',t.* FROM $t t; SELECT 'expected-$t',t.* FROM saved_$t t;`n"}
}
$sql+=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.StartScript,q.CompleteScript,q.ItemDrop1,q.ItemDropQuantity1,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40002;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=40002 ORDER BY ID;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.quest=s.quest AND e.id=s.id WHERE s.id=97979 AND s.quest=40002;
SELECT 'next',COUNT(*) FROM creature_queststarter s JOIN quest_template_addon a ON a.ID=s.quest WHERE s.id=97979 AND s.quest=40003 AND a.PrevQuestID=40002;
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=91743 AND gossip_menu_id=18828 AND npcflag=1 AND faction=190 AND AIName='SmartAI' AND ScriptName='npc_stormheim_familiar_binding';
SELECT 'spell-binding',COUNT(*) FROM spell_script_names WHERE spell_id=193253 AND ScriptName='spell_stormheim_familiar_shard';
SELECT 'source-drop',COUNT(*) FROM creature_loot_template WHERE Entry=97890 AND Item=129106 AND QuestRequired=1 AND ABS(Chance-75.985)<0.001 AND MinCount=1 AND MaxCount=1;
SELECT 'menu',COUNT(*) FROM gossip_menu_option WHERE MenuID=18828 AND OptionID=0 AND OptionNpc=0 AND OptionBroadcastTextID=99850;
SELECT 'item-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18828 AND SourceEntry=0 AND ConditionTypeOrReference=2 AND ConditionValue1=129106 AND ConditionValue2=1 AND NegativeCondition=0;
SELECT 'old-action',COUNT(*) FROM smart_scripts WHERE entryorguid=91743 AND source_type=0 AND id=0 AND event_type=62 AND event_param1=18828 AND event_param2=0 AND action_type=33 AND action_param1=97981 AND target_type=7 AND link=1;
SELECT 'spawn',guid,id,position_x,position_y,position_z FROM creature WHERE guid IN(291388,291392,291393) ORDER BY guid;
"@
if(!$Live){$sql+="UPDATE creature_template SET ScriptName='custom_binding' WHERE entry=91743;`n$migration`nSELECT 'custom',COUNT(*) FROM creature_template WHERE entry=91743 AND ScriptName='custom_binding';"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Familiar Fate World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($e in @("quest`t40002`t33554440`t8192`t0`t0`t129106`t1`t40003`t40001`t40003`t0","objective`t280169`t0`t0`t97982`t1`t0","objective`t280170`t0`t1`t97981`t1`t0","relations`t1","next`t1","binding`t1","spell-binding`t1","source-drop`t1","menu`t1","item-condition`t1","old-action`t1","spawn`t291388`t97890`t2946.9`t1239.42`t184.415","spawn`t291392`t97979`t2875.26`t1208.98`t188.763","spawn`t291393`t91743`t2873.21`t1207.08`t188.763")){if($rows -notcontains $e){throw "Missing Familiar Fate World evidence $e"}}
if(!$Live){
 foreach($t in $tables.Keys){$a="actual-$t`t";$e="expected-$t`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object);$expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object);if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $t"}}
 if($rows -notcontains "custom`t1"){throw 'Custom binding overwritten'}
}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'item',COUNT(*) FROM item_sparse WHERE ID IN(129106,129161); SELECT 'use',COUNT(*) FROM item_effect WHERE ItemID IN(129106,129161); SELECT 'spell',COUNT(*) FROM spell_effect WHERE SpellID IN(193253,193262);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Familiar Fate Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('item','use','spell')){if($rows -notcontains "$name`t0"){throw "Unexpected Hotfix $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: repeated migration/full-row preservation, custom binding, native Dravax source loot/quest drop, Circle menu/condition/action and unchanged Vethir/40003 handoff.'

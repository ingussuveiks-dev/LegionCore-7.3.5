param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-stem-tide"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_07_stormheim_stem_tide.sql" -Raw
$tables=@{creature_template='entry IN(97979,97983,97984,97985,97986,98039,98040,98041)';creature='id IN(97979,97983,97984,97985,97986)';creature_template_addon='entry IN(97979,97983,97984,97985,97986)';smart_scripts='entryorguid IN(97983,97984,97985,9798300,9798400,9798500,97986)';gameobject_template='entry=244681';gameobject='id=244681';quest_template='ID=40003';quest_template_addon='ID=40003';quest_objectives='QuestID=40003';conditions='SourceTypeOrReferenceId=13 AND SourceEntry=193327';spell_script_names='spell_id IN(193327,193328,193329,193330,196084)';creature_queststarter='quest=40003';creature_questender='quest=40003'}
$sql=''
if(!$Live){
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="CREATE TEMPORARY TABLE saved_$t LIKE $t; INSERT INTO saved_$t SELECT * FROM $t WHERE $($tables[$t]); CREATE TEMPORARY TABLE $t LIKE saved_$t; INSERT INTO $t SELECT * FROM saved_$t;`n"}
 $sql+="UPDATE creature_template SET ScriptName='' WHERE entry IN(97983,97984,97985); UPDATE gameobject_template SET ScriptName='' WHERE entry=244681; DELETE FROM spell_script_names WHERE spell_id=193327 AND ScriptName='spell_stormheim_stem_horn';`n$migration`n$migration`n"
 $sql+="UPDATE saved_creature_template SET ScriptName='npc_stormheim_stem_marker' WHERE entry IN(97983,97984,97985); UPDATE saved_gameobject_template SET ScriptName='go_stormheim_stem_portal' WHERE entry=244681; INSERT INTO saved_spell_script_names(spell_id,ScriptName) SELECT 193327,'spell_stormheim_stem_horn' WHERE NOT EXISTS(SELECT 1 FROM saved_spell_script_names WHERE spell_id=193327 AND ScriptName='spell_stormheim_stem_horn');`n"
 foreach($t in ($tables.Keys | Sort-Object)){$sql+="SELECT 'actual-$t',t.* FROM $t t; SELECT 'expected-$t',t.* FROM saved_$t t;`n"}
}
$sql+=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.StartItem,q.ItemDrop1,q.ItemDropQuantity1,q.RewardNextQuest,q.StartScript,q.CompleteScript,a.PrevQuestID,a.ProvidedItemCount,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40003;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=40003 ORDER BY ID;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.quest=s.quest WHERE s.quest=40003 AND s.id=97979 AND e.id=97986;
SELECT 'markers',COUNT(*) FROM creature_template WHERE entry IN(97983,97984,97985) AND AIName='SmartAI' AND ScriptName='npc_stormheim_stem_marker' AND unit_flags=33555200;
SELECT 'portal',COUNT(*) FROM gameobject_template WHERE entry=244681 AND type=8 AND displayId=26244 AND Data4=40003 AND Data0=1887 AND Data1=10 AND flags=4 AND size=0.75 AND ScriptName='go_stormheim_stem_portal';
SELECT 'spell-binding',COUNT(*) FROM spell_script_names WHERE spell_id=193327 AND ScriptName='spell_stormheim_stem_horn';
SELECT 'conditions',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=193327 ORDER BY ElseGroup;
SELECT 'spawn',guid,id,position_x,position_y,position_z FROM creature WHERE guid IN(291595,291537,291523,338656) ORDER BY id;
SELECT 'objects',guid,id,position_x,position_y,position_z FROM gameobject WHERE guid IN(115097,115080,115070) ORDER BY guid;
SELECT 'base-markers',COUNT(*) FROM creature WHERE guid IN(291595,291537,291523) AND map=1220 AND zoneId=7541 AND PhaseId='' AND spawnMask=1 AND phaseMask=1 AND modelid=0;
SELECT 'base-objects',COUNT(*) FROM gameobject WHERE guid IN(115097,115080,115070) AND map=1220 AND PhaseId='' AND spawnMask=1 AND phaseMask=1;
SELECT 'native-events',COUNT(*) FROM event_scripts WHERE id IN(46837,46839,46840);
SELECT 'old-lists',COUNT(*) FROM smart_scripts WHERE source_type=9 AND entryorguid IN(9798300,9798400,9798500) AND ((id=0 AND action_type=11 AND action_param1=196084 AND event_param1=5000) OR (id=1 AND action_type=33 AND action_param1=entryorguid DIV 100 AND target_type=7));
SELECT 'dragons',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(98039,98040,98041) AND t.VehicleId=0 AND t.ScriptName='' AND t.AIName='' AND w.Displayid1=68213;
"@
if(!$Live){$sql+="UPDATE creature_template SET ScriptName='custom_marker' WHERE entry=97983; UPDATE gameobject_template SET ScriptName='custom_portal' WHERE entry=244681;`n$migration`nSELECT 'custom',COUNT(*) FROM creature_template c JOIN gameobject_template g ON g.entry=244681 WHERE c.entry=97983 AND c.ScriptName='custom_marker' AND g.ScriptName='custom_portal';"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Stem World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($e in @("quest`t40003`t38928392`t8192`t129161`t129161`t1`t0`t0`t0`t40002`t1`t0","objective`t280172`t0`t0`t97983`t1`t0","objective`t280173`t0`t1`t97984`t1`t0","objective`t280174`t0`t2`t97985`t1`t0","objective`t280263`t0`t3`t98186`t3`t28","relations`t1","markers`t3","portal`t1","spell-binding`t1","conditions`t0`t31`t0`t3`t97983","conditions`t1`t31`t0`t3`t97984","conditions`t2`t31`t0`t3`t97985","native-events`t0","old-lists`t6","dragons`t3","base-markers`t3","base-objects`t3","spawn`t291595`t97983`t2713.4`t970.118`t205.88","spawn`t291537`t97984`t2601.57`t981.316`t216.77","spawn`t291523`t97985`t2467.32`t1060.44`t216.77","spawn`t338656`t97986`t2490.15`t948.437`t241.446","objects`t115070`t244681`t2469.83`t1058.2`t216.179","objects`t115080`t244681`t2601.84`t981.429`t215.86","objects`t115097`t244681`t2714.29`t969.302`t205.287")){if($rows -notcontains $e){throw "Missing Stem World evidence $e"}}
if(!$Live){foreach($t in $tables.Keys){$a="actual-$t`t";$e="expected-$t`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object);$expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object);if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed: $t"}};if($rows -notcontains "custom`t1"){throw 'Custom scripts overwritten'}}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql="SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=129161; SELECT 'use',COUNT(*) FROM item_effect WHERE ItemID=129161; SELECT 'spell',COUNT(*) FROM spell_effect WHERE SpellID IN(193327,196084); SELECT 'visual',COUNT(*) FROM spell_x_spell_visual WHERE SpellID=196084; SELECT 'models',COUNT(*) FROM creature_display_info WHERE ID=68213; SELECT 'portal-model',COUNT(*) FROM gameobject_display_info WHERE ID=26244; SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(193327,196084); SELECT 'cooldown',COUNT(*) FROM spell_cooldowns WHERE SpellID=193327; SELECT 'model-data',COUNT(*) FROM creature_model_data WHERE ID=8673; SELECT 'casting',COUNT(*) FROM spell_casting_requirements WHERE SpellID=193327;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Stem Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($n in @('item','use','spell','visual','models','portal-model','misc','cooldown','model-data','casting')){if($rows -notcontains "$n`t0"){throw "Unexpected Hotfix $n"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: repeated Stem migration/full-row/custom-script preservation; native horn, all4 objectives, markers/portals/conditions, old timed lists, Vethir templates and turn-in. No spawn/native visual-data edits.'

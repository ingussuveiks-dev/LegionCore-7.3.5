param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_20_stormheim_cry_thunder.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(91767,92218,92302,92312,92367,92384,92342)';quest_template='ID IN(38624,39803)';quest_template_addon='ID=38624';quest_start_scripts='id=38624';smart_scripts='source_type=0 AND entryorguid IN(92218,-146706157)';spell_script_names='spell_id IN(182921,183058,183065,183042,183471,183072)'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE quest_template SET StartScript=38624 WHERE ID=38624; UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=38624; DELETE FROM smart_scripts WHERE entryorguid=-146706157; DELETE FROM quest_start_scripts WHERE id=38624; INSERT INTO quest_start_scripts VALUES(38624,1,8,91767,0,0,0,0,0,0),(38624,5,7,38624,0,0,0,0,0,0),(38624,10,8,91768,0,0,0,0,0,0);`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'start-detached',COUNT(*) FROM quest_template WHERE ID=38624 AND StartScript=0 AND RewardNextQuest=39803;
SELECT 'legacy-removed',COUNT(*) FROM quest_start_scripts WHERE id=38624;
SELECT 'explore-bit-removed',COUNT(*) FROM quest_template_addon WHERE ID=38624 AND (SpecialFlags & 2)=0 AND NextQuestID=39803;
SELECT 'native-stages',COUNT(*) FROM quest_objectives WHERE QuestID=38624 AND ((ID=278235 AND Type=3 AND ObjectID=91767 AND Amount=1 AND StorageIndex=0 AND Flags=0) OR(ID=278236 AND Type=0 AND ObjectID=92360 AND Amount=150 AND StorageIndex=3 AND Flags=2) OR(ID=278237 AND Type=0 AND ObjectID=91768 AND Amount=1 AND StorageIndex=1 AND Flags=2));
SELECT 'scoped-list',COUNT(*) FROM smart_scripts WHERE entryorguid=-146706157 AND source_type=0 AND id IN(0,3) AND comment LIKE '%Cry Thunder38624%';
SELECT 'scoped-no-teleport',COUNT(*) FROM smart_scripts WHERE entryorguid=-146706157 AND source_type=0 AND event_type=19 AND event_param1=38624;
SELECT 'shared-list-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=92218 AND source_type=0 AND id IN(0,1,3);
SELECT 'later-accept-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=-146706157 AND source_type=0 AND id=3 AND event_type=19 AND event_param1=43560 AND action_type=12 AND action_param1=110990;
SELECT 'targets',COUNT(*) FROM creature_template WHERE entry IN(92312,92367,92384) AND AIName='' AND ScriptName='npc_stormheim_cry_thunder_target';
SELECT 'tempest',COUNT(*) FROM creature_template WHERE entry=92342 AND AIName='' AND ScriptName='npc_stormheim_cry_thunder_tempest';
SELECT 'shared-bar-untouched',COUNT(*) FROM creature_template WHERE entry=92302 AND ScriptName='npc_stormheim_dragons' AND VehicleId=4133 AND spell1=0 AND spell2=0 AND spell3=0 AND spell4=0;
SELECT 'native-click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=91767 AND spell_id=182921 AND cast_flags=1 AND add_npc_flag=1;
SELECT 'source',COUNT(*) FROM creature WHERE guid=388111 AND id=91767 AND map=1220 AND ABS(position_x-2080.96)<0.01 AND ABS(position_y-2639.05)<0.01 AND ABS(position_z-676.364)<0.01;
SELECT 'receiver',COUNT(*) FROM creature WHERE guid=290836 AND id=97074 AND map=1220 AND ABS(position_x-3066.67)<0.01 AND ABS(position_y-2246.5)<0.01;
SELECT 'spells',COUNT(*) FROM spell_script_names WHERE (spell_id=182921 AND ScriptName='spell_stormheim_cry_thunder_board') OR(spell_id IN(183058,183065,183042) AND ScriptName='spell_stormheim_cry_thunder_attack') OR(spell_id=183471 AND ScriptName='spell_stormheim_cry_thunder_end') OR(spell_id=183072 AND ScriptName='spell_stormheim_cry_thunder_tempest');
SELECT 'relations',COUNT(*) FROM creature_questender WHERE id=97074 AND quest=38624;
SELECT 'next-relation',COUNT(*) FROM creature_queststarter WHERE id=97074 AND quest=39803;
SELECT 'storm-visual',COUNT(*) FROM areatrigger_template WHERE entry=4045 AND spellId=183073 AND VisualID=46719 AND Radius=12;
'@
$expected=@("start-detached`t1","legacy-removed`t0","explore-bit-removed`t1","native-stages`t3","scoped-list`t2","scoped-no-teleport`t0","shared-list-preserved`t3","later-accept-preserved`t1","targets`t3","tempest`t1","shared-bar-untouched`t1","native-click`t1","source`t1","receiver`t1","spells`t6","relations`t1","next-relation`t1","storm-visual`t1")
$route=Get-Content "$repo/src/server/scripts/Legion/stormheim_cry_thunder_route.h" -Raw
$seeds=[regex]::Matches($route,'\{(\d+), (\d+), \{([\d.]+)f,([\d.]+)f,([\d.]+)f,([\d.]+)f\}')
if($seeds.Count -ne 30){throw 'Unexpected target seed count'}
$values=$seeds | ForEach-Object {$sourceEntry=@{'92312'=91205;'92367'=91204;'92384'=91417}[$_.Groups[2].Value];"($($_.Groups[1].Value),$sourceEntry,$($_.Groups[3].Value),$($_.Groups[4].Value),$($_.Groups[5].Value))"}
$sql+="CREATE TEMPORARY TABLE expected_seeds(guid BIGINT,entry INT,x FLOAT,y FLOAT,z FLOAT); INSERT INTO expected_seeds VALUES "+($values -join ',')+"; SELECT 'original-seeds',COUNT(*) FROM expected_seeds s JOIN creature c ON c.guid=s.guid AND c.id=s.entry AND c.map=1220 AND ABS(c.position_x-s.x)<0.01 AND ABS(c.position_y-s.y)<0.01 AND ABS(c.position_z-s.z)<0.01;"
$expected+="original-seeds`t30"
if(!$Live){
 $reset="DELETE FROM smart_scripts WHERE entryorguid=-146706157; UPDATE quest_template SET StartScript=38624 WHERE ID=38624; DELETE FROM quest_start_scripts WHERE id=38624; INSERT INTO quest_start_scripts VALUES(38624,1,8,91767,0,0,0,0,0,0),(38624,5,7,38624,0,0,0,0,0,0),(38624,10,8,91768,0,0,0,0,0,0); DELETE FROM spell_script_names WHERE ScriptName LIKE 'spell_stormheim_cry_thunder%'; UPDATE creature_template SET ScriptName='',AIName='' WHERE entry IN(92312,92367,92384,92342);"
 $sql+=@"
$reset
UPDATE creature_template SET ScriptName='custom' WHERE entry=92312;
$migration
SELECT 'custom-target',COUNT(*) FROM creature_template WHERE entry=92312 AND ScriptName='custom';
SELECT 'custom-pipeline-preserved',COUNT(*) FROM quest_template WHERE ID=38624 AND StartScript=38624;
UPDATE creature_template SET ScriptName='' WHERE entry=92312;
INSERT INTO quest_start_scripts VALUES(38624,9,8,999,0,0,0,0,0,0);
$migration
SELECT 'custom-start-preserved',COUNT(*) FROM quest_start_scripts WHERE id=38624;
$reset
CREATE TEMPORARY TABLE custom_guid AS SELECT * FROM smart_scripts WHERE entryorguid=92218 AND id=0;
UPDATE custom_guid SET entryorguid=-146706157,id=42,comment='custom';
INSERT INTO smart_scripts SELECT * FROM custom_guid;
$migration
SELECT 'custom-guid-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=-146706157 AND id=42 AND comment='custom';
SELECT 'custom-guid-not-merged',COUNT(*) FROM smart_scripts WHERE entryorguid=-146706157;
SELECT 'custom-guid-start-preserved',COUNT(*) FROM quest_template WHERE ID=38624 AND StartScript=38624;
SELECT 'custom-no-spells',COUNT(*) FROM spell_script_names WHERE ScriptName LIKE 'spell_stormheim_cry_thunder%';
"@
 $expected+=@("custom-target`t1","custom-pipeline-preserved`t1","custom-start-preserved`t4","custom-guid-preserved`t1","custom-guid-not-merged`t1","custom-guid-start-preserved`t1","custom-no-spells`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Cry Thunder SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Cry Thunder SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: installed Cry Thunder stages/click/bindings/owned GUID list/native abilities and30 original target anchors.'}else{'PASS: migration twice, native stages/next/vehicles/click/shared AI retained, no shared bar mutation,30 original anchors; custom scripts/start/GUID preserved.'}

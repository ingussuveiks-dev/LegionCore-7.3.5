param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_05_stormheim_valor.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(96122,96121,96078,96144)';gameobject_template='entry IN(243808,243801,243802)';quest_objectives='QuestID IN(39590,39591,39592)';gameobject='guid IN(114823,25658799)'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE quest_objectives SET Type=0,ObjectID=96078 WHERE ID=279720;`n"
 $sql+="UPDATE gameobject SET position_x=3479.78,position_y=2130.51,position_z=233.305,orientation=5.4982,rotation0=0,rotation1=0,rotation2=0,rotation3=1 WHERE guid=114823;`n"
 $sql+="$migration`n$migration`n"
}
$sql+=@'
SELECT 'heads',COUNT(*) FROM quest_objectives WHERE QuestID=39590 AND Type=1 AND Amount=1 AND ((ObjectID=128508 AND StorageIndex=0) OR (ObjectID=128509 AND StorageIndex=1));
SELECT 'missing-champion',COUNT(*) FROM quest_objectives WHERE ID=279716 AND QuestID=39590 AND Type=0 AND ObjectID=96255 AND StorageIndex=2 AND Amount=1;
SELECT 'native-record-objective',COUNT(*) FROM quest_objectives WHERE ID=279720 AND QuestID=39591 AND Type=2 AND ObjectID=243801 AND StorageIndex=3 AND Amount=1;
SELECT 'head-loot',COUNT(*) FROM creature_loot_template WHERE ((Entry=90747 AND Item=128508) OR (Entry=90734 AND Item=128509)) AND Chance>94 AND QuestRequired=1 AND LootMode=0 AND MinCount=1 AND MaxCount=1;
SELECT 'missing-location',COUNT(*) FROM creature WHERE guid=290585 AND id=96255 AND map=1220 AND PhaseId='';
SELECT 'missing-quest-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=96255 AND ConditionTypeOrReference=9 AND ConditionValue1=39590;
SELECT 'havi-starter',COUNT(*) FROM creature_queststarter WHERE id=96254 AND quest=39590;
SELECT 'head-handin-record-starter',COUNT(*) FROM gameobject_questender WHERE id=243836 AND quest=39590;
SELECT 'record-starter',COUNT(*) FROM gameobject_queststarter WHERE id=243836 AND quest=39591;
SELECT 'record-handin',COUNT(*) FROM gameobject_questender WHERE id=243836 AND quest=39591;
SELECT 'record-gate',COUNT(*) FROM quest_template_addon WHERE ID=39591 AND PrevQuestID=39590;
SELECT 'console-bindings',COUNT(*) FROM gameobject_template WHERE entry IN(243808,243801) AND ScriptName='go_stormheim_valor_console';
SELECT 'actor-bindings',COUNT(*) FROM creature_template WHERE entry IN(96122,96121,96078) AND ScriptName='npc_stormheim_valor_actor';
SELECT 'native-access-spell',COUNT(*) FROM gameobject_template WHERE entry=243801 AND Data0=1691 AND Data10=190557;
SELECT 'record-data',COUNT(*) FROM conversation_data WHERE entry IN(776,777,778);
SELECT 'record-actors',COUNT(*) FROM conversation_actor WHERE entry IN(776,777,778) AND actorId=50369 AND creatureId=96257 AND displayId=25217 AND duration>0;
SELECT 'projector-native-destination',COUNT(*) FROM gameobject g JOIN spell_target_position p ON p.id=190557 WHERE g.guid=114823 AND g.id=243818 AND g.map=p.target_map AND ABS(g.position_x-p.target_position_x)<0.01 AND ABS(g.position_y-p.target_position_y)<0.01 AND ABS(g.position_z-p.target_position_z)<0.01;
'@
$expected=@("heads`t2","missing-champion`t1","native-record-objective`t1","head-loot`t2","missing-location`t1","missing-quest-condition`t1","havi-starter`t1","head-handin-record-starter`t1","record-starter`t1","record-handin`t1","record-gate`t1","console-bindings`t2","actor-bindings`t3","native-access-spell`t1","record-data`t3","record-actors`t3","projector-native-destination`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-actor-flags',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.npcflag<>b.npcflag OR a.unit_flags<>b.unit_flags OR a.VehicleId<>b.VehicleId;
SELECT 'preserved-object-flags',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.flags<>b.flags OR a.Data0<>b.Data0 OR a.Data10<>b.Data10;
SELECT 'other-trial',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.QuestID=39592 AND (a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.StorageIndex<>b.StorageIndex);
UPDATE creature_template SET ScriptName='custom' WHERE entry=96122;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=243801;
UPDATE quest_objectives SET Type=0,ObjectID=999 WHERE ID=279720;
UPDATE gameobject SET position_z=200 WHERE guid=114823;
$migration
SELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=96122 AND ScriptName='custom';
SELECT 'custom-object',COUNT(*) FROM gameobject_template WHERE entry=243801 AND ScriptName='custom';
SELECT 'custom-objective',COUNT(*) FROM quest_objectives WHERE ID=279720 AND Type=0 AND ObjectID=999;
SELECT 'custom-projector',COUNT(*) FROM gameobject WHERE guid=114823 AND position_z=200;
"@
 $expected+=@("preserved-actor-flags`t0","preserved-object-flags`t0","other-trial`t0","custom-actor`t1","custom-object`t1","custom-objective`t1","custom-projector`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Valor SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Valor SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, guarded native slot restoration, both head drops, missing champion condition, native hand-ins/start/gate, three recording actors, console/actor bindings and custom/unrelated preservation.'

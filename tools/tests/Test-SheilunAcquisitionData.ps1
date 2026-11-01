param([string]$OutputDirectory='',[string]$Migration='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-acquisition-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
if(!$Migration){
 $Migration="$repo/sql/pending/world/2026_11_01_01_sheilun_manual_acquisition.sql"
 if(!(Test-Path $Migration)){$Migration="$repo/sql/updates/world/2026_11_01_01_sheilun_manual_acquisition.sql"}
}
$migrationText=[IO.File]::ReadAllText($Migration)
$tables=[ordered]@{
 creature_template='entry IN (101880,102479,102648,103151)'
 instance_template='map IN (1541,1533)'
 scenario_data='ScenarioID IN (1007,988)'
 lfg_entrances='dungeonId IN (1168,26)'
 spell_target_position='id IN (202742,202831)'
 gameobject_template='entry IN (256913,246464)'
 gameobject_loot_template='Entry IN (256913,254087)'
 npc_spellclick_spells='npc_entry IN (102479,102648,102590)'
 smart_scripts='entryorguid=101880 AND source_type=0'
 creature_text='CreatureID IN (101881,101880)'
 spell_script_names='spell_id IN (202435,202437,202828,202831,202438) OR ScriptName IN (''spell_sheilun_manual_claim'',''spell_sheilun_claim_scene'',''spell_sheilun_return_boarding'',''spell_sheilun_return_transfer'')'
}
$withoutComments=$migrationText -replace '(?m)^\s*--.*$',''
if($withoutComments -match '(?i)\b(ALTER|TRUNCATE|RENAME|GRANT|CALL|USE|DELIMITER)\b' -or
 $withoutComments -match '(?i)\bCREATE\s+(?!TEMPORARY\s+TABLE)' -or $withoutComments -match '(?i)\bDROP\s+(?!TEMPORARY\s+TABLE)'){
 throw 'Review migration DDL before executing temporary shadows'
}
foreach($match in [regex]::Matches($withoutComments,'(?im)(?:^|;)\s*(?:UPDATE|INSERT(?:\s+IGNORE)?\s+INTO|DELETE\s+FROM)\s+([a-z_]+)')){
 if($match.Groups[1].Value -notin (@($tables.Keys)+@('sheilun_acquisition_admission'))){throw 'Migration writes outside the declared temporary shadows'}
}
# MariaDB refuses CREATE TEMPORARY TABLE ... AS SELECT in READ ONLY mode.
# Every migration target is first shadowed by its connection-local temporary
# table; the static target/DDL certificate above refuses permanent operations.
$seed="SET SESSION group_concat_max_len=1048576;`n"
foreach($table in $tables.Keys){
 $seed+="CREATE TEMPORARY TABLE native_seed_$table LIKE $table;`nINSERT INTO native_seed_$table SELECT * FROM $table WHERE $($tables[$table]);`n"
 $seed+="CREATE TEMPORARY TABLE $table LIKE native_seed_$table;`nINSERT INTO $table SELECT * FROM native_seed_$table;`n"
}
$seed+=@'
CREATE TEMPORARY TABLE sheilun_test_assert (ok TINYINT NOT NULL CHECK (ok=1));
SELECT 'ADMISSION_ACTORS',entry,AIName,ScriptName,npcflag,VehicleId,gossip_menu_id FROM creature_template ORDER BY entry;
SELECT 'ADMISSION_GO',entry,type,Data1,Data3,ScriptName FROM gameobject_template WHERE entry=256913;
SELECT 'ADMISSION_CLICKS',npc_entry,spell_id,cast_flags FROM npc_spellclick_spells ORDER BY npc_entry;
SELECT 'ADMISSION_SMART',event_type,COUNT(*) FROM smart_scripts GROUP BY event_type;
SELECT 'ADMISSION_BINDINGS',spell_id,ScriptName FROM spell_script_names ORDER BY spell_id;
SELECT 'ADMISSION_TEXT',CreatureID,GroupID,ID,BroadcastTextID FROM creature_text WHERE CreatureID=101881 AND GroupID BETWEEN 13 AND 18 OR CreatureID=101880 AND GroupID=13;
SET @original_taran=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID) ORDER BY GroupID,ID)) FROM creature_text WHERE CreatureID=101881 AND GroupID<13);
SET @original_shared=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',entry,AIName,ScriptName) ORDER BY entry)) FROM creature_template WHERE entry=103151);
SET @original_gossip=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type) ORDER BY id)) FROM smart_scripts);
SET @original_routes=(SELECT COALESCE(MD5(GROUP_CONCAT(CONCAT_WS(':',ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID) ORDER BY ScenarioID,MapID)),'') FROM scenario_data WHERE NOT(ScenarioID=1007 AND MapID=1541));
SET @original_targets=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',id,target_map,target_position_x,target_position_y,target_position_z,target_orientation) ORDER BY id)) FROM spell_target_position);

'@
$assert=@'
INSERT INTO sheilun_test_assert SELECT COUNT(*)=3 FROM creature_template WHERE (entry=101880 AND AIName='SmartAI' AND ScriptName='npc_sheilun_return_guide') OR (entry=102479 AND ScriptName='npc_sheilun_owned_staff') OR (entry=102648 AND ScriptName='npc_sheilun_return_kite');
INSERT INTO sheilun_test_assert SELECT Data1=256913 FROM gameobject_template WHERE entry=256913;
INSERT INTO sheilun_test_assert SELECT COUNT(*)=1 FROM scenario_data WHERE ScenarioID=1007 AND MapID=1541 AND DifficultyID=12 AND Team=0 AND Class=10 AND LfgDungeonID=0;
INSERT INTO sheilun_test_assert SELECT COUNT(*)=1 FROM lfg_entrances e JOIN spell_target_position p ON p.id=202742 WHERE e.dungeonId=1168 AND e.position_x=p.target_position_x AND e.position_y=p.target_position_y AND e.position_z=p.target_position_z AND e.orientation=p.target_orientation;
INSERT INTO sheilun_test_assert SELECT COUNT(*)=4 FROM spell_script_names WHERE (spell_id=202435 AND ScriptName='spell_sheilun_manual_claim') OR (spell_id=202437 AND ScriptName='spell_sheilun_claim_scene') OR (spell_id=202828 AND ScriptName='spell_sheilun_return_boarding') OR (spell_id=202831 AND ScriptName='spell_sheilun_return_transfer');
INSERT INTO sheilun_test_assert SELECT COUNT(*)=7 FROM creature_text WHERE CreatureID=101881 AND GroupID BETWEEN 13 AND 18 OR CreatureID=101880 AND GroupID=13;
INSERT INTO sheilun_test_assert SELECT @original_taran=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID) ORDER BY GroupID,ID)) FROM creature_text WHERE CreatureID=101881 AND GroupID<13);
INSERT INTO sheilun_test_assert SELECT @original_shared=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',entry,AIName,ScriptName) ORDER BY entry)) FROM creature_template WHERE entry=103151);
INSERT INTO sheilun_test_assert SELECT @original_gossip=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type) ORDER BY id)) FROM smart_scripts);
INSERT INTO sheilun_test_assert SELECT @original_routes=(SELECT COALESCE(MD5(GROUP_CONCAT(CONCAT_WS(':',ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID) ORDER BY ScenarioID,MapID)),'') FROM scenario_data WHERE NOT(ScenarioID=1007 AND MapID=1541));
INSERT INTO sheilun_test_assert SELECT @original_targets=(SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',id,target_map,target_position_x,target_position_y,target_position_z,target_orientation) ORDER BY id)) FROM spell_target_position);
SELECT 'PASS','native shadow migration/idempotence/shared actors/text/gossip';

'@
$cases=[ordered]@{
 native_idempotence=''
 custom_source_script="UPDATE creature_template SET ScriptName='custom_source' WHERE entry=102479;"
 custom_guide_script="UPDATE creature_template SET ScriptName='custom_guide' WHERE entry=101880;"
 custom_kite_script="UPDATE creature_template SET ScriptName='custom_kite' WHERE entry=102648;"
 wrong_native_vehicle='UPDATE creature_template SET VehicleId=0 WHERE entry=102648;'
 wrong_shared_menu='UPDATE creature_template SET gossip_menu_id=0 WHERE entry=101880;'
 custom_loot_link='UPDATE gameobject_template SET Data1=42 WHERE entry=256913;'
 consumable_source='UPDATE gameobject_template SET Data3=1 WHERE entry=256913;'
 wrong_artifact='UPDATE gameobject_loot_template SET Item=2589 WHERE Entry=256913;'
 duplicate_quantity='UPDATE gameobject_loot_template SET MaxCount=2 WHERE Entry=256913;'
 quest_filtered_loot='UPDATE gameobject_loot_template SET QuestRequired=1 WHERE Entry=256913;'
 wrong_click_caster='UPDATE npc_spellclick_spells SET cast_flags=2 WHERE npc_entry=102479;'
 missing_native_click='DELETE FROM npc_spellclick_spells WHERE npc_entry=102648;'
 missing_shared_branch='DELETE FROM smart_scripts WHERE id=(SELECT MIN(id) FROM native_seed_smart_scripts);'
 wrong_instance_binding="UPDATE instance_template SET script='custom_instance' WHERE map=1541;"
 missing_native_route='DELETE FROM scenario_data WHERE ScenarioID=1007 AND MapID=1541;'
 wrong_route_map='UPDATE scenario_data SET MapID=1542 WHERE ScenarioID=1007 AND MapID=1541;'
 wrong_route_class='UPDATE scenario_data SET Class=9 WHERE ScenarioID=1007 AND MapID=1541;'
 wrong_route_difficulty='UPDATE scenario_data SET DifficultyID=23 WHERE ScenarioID=1007 AND MapID=1541;'
 wrong_route_dungeon='UPDATE scenario_data SET LfgDungeonID=1 WHERE ScenarioID=1007 AND MapID=1541;'
 missing_native_entrance='DELETE FROM spell_target_position WHERE id=202742;'
 custom_native_entrance='UPDATE spell_target_position SET target_position_z=28 WHERE id=202742;'
 custom_resume_entrance='REPLACE INTO lfg_entrances (dungeonId,position_x,position_y,position_z,orientation) VALUES(1168,1,2,3,4);'
 extra_spell_script="INSERT INTO spell_script_names VALUES (202435,'custom_script');"
 misbound_new_script="INSERT INTO spell_script_names VALUES (1,'spell_sheilun_manual_claim');"
 custom_native_text="REPLACE INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID) VALUES (101881,13,0,'custom line',12,0,100,0,0,0,104773,0,0,0);"
 custom_native_sound="REPLACE INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID) VALUES (101881,13,0,'It is done! You have done a great service for Pandaria this day.',12,0,100,0,0,1,104773,0,0,0);"
 wrong_native_text_group="REPLACE INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID) VALUES (101881,13,0,'It is done! You have done a great service for Pandaria this day.',12,0,100,0,0,0,104774,0,0,0);"
 extra_native_text="REPLACE INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID) VALUES (101881,13,1,'It is done! You have done a great service for Pandaria this day.',12,0,100,0,0,0,104773,0,0,0);"
}
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$prior=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$db[3]
 foreach($name in $cases.Keys){
  $path="$out/$name.sql"
  $sql=$seed+$cases[$name]+"`n"+$migrationText+"`n"
  if($name -eq 'native_idempotence'){$sql+=$assert+$migrationText+"`n"+$assert+"SELECT 'EOF','sheilun-data';`n"}
  [IO.File]::WriteAllText($path,$sql,(New-Object Text.UTF8Encoding($false)))
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names') -InputFile $path -OutputFile "$out/$name.stdout" -ErrorFile "$out/$name.stderr"
  $stderr=[IO.File]::ReadAllText("$out/$name.stderr")
  if($name -eq 'native_idempotence'){
   if($code -or $stderr.Length -or @(Get-Content "$out/$name.stdout")[-1] -cne "EOF`tsheilun-data"){throw "Native shadow migration failed; inspect $name stdout/stderr"}
  }elseif(!$code -or $stderr -notmatch 'CONSTRAINT.*sheilun_acquisition_admission.*failed'){
   throw "Malformed native scope was not refused by the admission constraint: $name"
  }
  "CASE $name PASS"
 }
}finally{$env:MYSQL_PWD=$prior}
@{migration=(Get-FileHash $Migration).Hash;cases=$cases.Count;temporary_shadow_tables=$true;static_target_ddl_certificate=$true}|ConvertTo-Json|Set-Content "$out/inputs.json"
"PASS $($cases.Count) native MariaDB shadow migration cases; no permanent table writes, credentials or character data"

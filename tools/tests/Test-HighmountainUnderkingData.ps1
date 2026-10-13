param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_01_highmountain_underking.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature','creature_template','smart_scripts','spell_script_names')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 # Recreate the known defects so a previously deployed database still exercises
 # the migration rather than merely checking its already-correct final state.
 $sql+=@"
UPDATE creature_template SET AIName='',ScriptName='' WHERE entry=97266;
REPLACE INTO creature(guid,id,map,position_x,position_y,position_z,PhaseId) VALUES(388320,97266,1220,3630.05,3928.34,906.824,''),(388319,97352,1220,3662.03,3928.39,906.681,'');
DELETE FROM smart_scripts WHERE entryorguid IN(93846,97352) AND source_type=0 AND id=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES(93846,0,0,60,33,97352,18,'old polling credit'),(97352,0,0,64,33,97352,18,'old gossip credit');
DELETE FROM spell_script_names WHERE (spell_id=192270 AND ScriptName='spell_highmountain_underking_breath') OR (spell_id=213577 AND ScriptName='spell_highmountain_underking_smash');
"@
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'boss',COUNT(*) FROM creature_template WHERE entry=97266 AND AIName='' AND ScriptName='npc_highmountain_underking';
SELECT 'spells',COUNT(*) FROM spell_script_names WHERE (spell_id=192270 AND ScriptName='spell_highmountain_underking_breath') OR (spell_id=213577 AND ScriptName='spell_highmountain_underking_smash');
SELECT 'duplicates',COUNT(*) FROM creature WHERE guid IN(388319,388320);
SELECT 'retained',COUNT(*) FROM creature WHERE map=1220 AND PhaseId='9180' AND ((guid=340672 AND id=97266 AND ABS(position_x-3611.37)<0.01 AND ABS(position_z-907.282)<0.01) OR (guid=340587 AND id=93846 AND ABS(position_x-3660.78)<0.01 AND ABS(position_z-906.65)<0.01));
SELECT 'premature-meeting',COUNT(*) FROM smart_scripts WHERE entryorguid IN(93846,97352) AND source_type=0 AND action_type=33 AND action_param1=97352;
SELECT 'achievement',COUNT(*) FROM smart_scripts WHERE entryorguid=93846 AND source_type=0 AND id=1 AND event_type=20 AND event_param1=39780 AND action_type=205 AND action_param1=27 AND action_param2=39780;
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID=39780 AND Type=0 AND Amount=1 AND ((ID=279965 AND StorageIndex=0 AND ObjectID=97266 AND Flags=3) OR (ID=280018 AND StorageIndex=1 AND ObjectID=97352 AND Flags=1));
SELECT 'start',COUNT(*) FROM creature_queststarter WHERE id=97407 AND quest=39780;
SELECT 'end',COUNT(*) FROM creature_questender WHERE id=93846 AND quest=39780;
SELECT 'follow-up',COUNT(*) FROM creature_queststarter WHERE id=93846 AND quest=39781;
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE (ID=39780 AND PrevQuestID=40594) OR (ID=39781 AND PrevQuestID=39780);
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=97266 AND LevelScalingMin=98 AND LevelScalingMax=110;
"@
$expected=@("boss`t1","spells`t2","duplicates`t0","retained`t2","premature-meeting`t0","achievement`t1","objectives`t2","start`t1","end`t1","follow-up`t1","chain`t2","scaling`t1")
if(!$Live){
 $sql+=@"
SELECT 'other-spawns',(SELECT COUNT(*) FROM saved_creature WHERE guid NOT IN(388319,388320))-(SELECT COUNT(*) FROM creature WHERE guid NOT IN(388319,388320));
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b ON a.entry=b.entry WHERE a.entry<>97266 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-actions',(SELECT COUNT(*) FROM saved_smart_scripts WHERE NOT(source_type=0 AND id=0 AND entryorguid IN(93846,97352)))-(SELECT COUNT(*) FROM smart_scripts WHERE NOT(source_type=0 AND id=0 AND entryorguid IN(93846,97352)));
UPDATE creature_template SET ScriptName='custom_underking' WHERE entry=97266;
DELETE FROM smart_scripts WHERE entryorguid=93846 AND source_type=0 AND id=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES(93846,0,0,60,33,123,18,'custom');
INSERT INTO creature(guid,id,map,position_x,position_y,position_z,PhaseId) VALUES(388320,97266,1220,3630.05,3928.34,987,'');
"@
 $sql+="$migration`nSELECT 'custom-ai',COUNT(*) FROM creature_template WHERE entry=97266 AND ScriptName='custom_underking'; SELECT 'custom-spawn',COUNT(*) FROM creature WHERE guid=388320 AND position_z=987; SELECT 'custom-event',COUNT(*) FROM smart_scripts WHERE entryorguid=93846 AND source_type=0 AND id=0 AND action_param1=123;"
 $expected+=@("other-spawns`t0","other-ai`t0","other-actions`t0","custom-ai`t1","custom-spawn`t1","custom-event`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Underking SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Underking SQL mismatch: $($actual -join '|')"}
'PASS: Underking SQL bindings, unique encounter, cave Mayla, native objectives, scaling, reward achievement and forward chain; temporary mode also checks idempotence/custom preservation.'

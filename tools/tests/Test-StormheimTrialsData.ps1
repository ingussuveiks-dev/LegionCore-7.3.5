param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_04_stormheim_trials.sql" -Raw
$sql=''
if(!$Live){
 $filters=@{
  creature_template='entry IN(96254,97305,97306,110604,110605,110608,110609,91571)'
  gameobject_template='entry IN(252074,252076,252080,240235)'
  creature='id IN(96254,97305,97306) OR guid=340422'
  smart_scripts='entryorguid IN(97305,97306,110604,110605,110608,110609,11060500,11060800,11060900)'
  quest_objectives='QuestID IN(38210,38459,38331,39590)'
  spell_script_names='spell_id IN(219924,219936,219947,52391,180737)'
 }
 foreach($table in $filters.Keys){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $temporaryMigration=$migration.Replace('JOIN creature original','JOIN saved_creature original')
 $sql+="CREATE TEMPORARY TABLE duplicate_seed LIKE creature; INSERT INTO duplicate_seed SELECT * FROM saved_creature WHERE guid=340423; UPDATE duplicate_seed SET guid=341181; INSERT INTO creature SELECT * FROM duplicate_seed ON DUPLICATE KEY UPDATE guid=VALUES(guid);`n$temporaryMigration`n$temporaryMigration`n"
}
$sql+=@"
SELECT 'empty-travel-objectives',COUNT(*) FROM quest_objectives WHERE QuestID IN(38210,38459);
SELECT 'native-three-ingredients',COUNT(*) FROM quest_objectives WHERE QuestID=38331 AND Type=0 AND Amount=1 AND Flags=0 AND ((ID=285317 AND StorageIndex=0 AND ObjectID=110610) OR(ID=285318 AND StorageIndex=1 AND ObjectID=110611) OR(ID=285319 AND StorageIndex=2 AND ObjectID=110612));
SELECT 'old-stir-definition',COUNT(*) FROM quest_objectives WHERE ID=279982 AND QuestID=38331;
SELECT 'unique-ingredient-slots',COUNT(DISTINCT StorageIndex) FROM quest_objectives WHERE QuestID=38331;
SELECT 'raven-starters',COUNT(*) FROM creature_queststarter WHERE(id=97306 AND quest=38210)OR(id=97305 AND quest=38459);
SELECT 'havi-enders',COUNT(*) FROM creature_questender WHERE id=96254 AND quest IN(38210,38459,38331);
SELECT 'havi-next-starters',COUNT(*) FROM creature_queststarter WHERE id=96254 AND quest IN(38331,39590);
SELECT 'alternative-trial-gates',COUNT(*) FROM quest_template_addon WHERE ((ID=38210 AND PrevQuestID=38060)OR(ID=38459 AND PrevQuestID=38362)) AND NextQuestID=38331 AND ExclusiveGroup=38459;
SELECT 'common-next-gate',COUNT(*) FROM quest_template_addon WHERE ID=38331 AND PrevQuestID=38210 AND NextQuestID=39590;
SELECT 'raven-invitations',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=0 AND event_type=19 AND action_type=85 AND action_param1=183614 AND target_type=7 AND link=0 AND ((entryorguid=97306 AND event_param1=38210)OR(entryorguid=97305 AND event_param1=38459));
SELECT 'raven-flags',COUNT(*) FROM creature_template WHERE entry IN(97305,97306) AND npcflag=2 AND AIName='SmartAI';
SELECT 'havi-flags',COUNT(*) FROM creature_template WHERE entry=96254 AND (npcflag & 2)<>0;
SELECT 'original-havi',COUNT(*) FROM creature WHERE guid=340423 AND id=96254 AND map=1220 AND PhaseId='';
SELECT 'duplicate-havi',COUNT(*) FROM creature WHERE guid=341181;
SELECT 'phase-specific-havi',COUNT(*) FROM creature WHERE guid=338597 AND id=96254 AND PhaseId='6364';
SELECT 'alliance-raven-location',COUNT(*) FROM creature WHERE guid=146722448 AND id=97306 AND PhaseId='4589';
SELECT 'horde-raven-location',COUNT(*) FROM creature WHERE guid=340419 AND id=97305 AND PhaseId='6375';
SELECT 'ingredient-go-scripts',COUNT(*) FROM gameobject_template WHERE entry IN(252074,252076,252080) AND ScriptName='go_stormheim_havi_ingredient' AND type=10 AND Data0=1691 AND Data1=38331;
SELECT 'actor-scripts',COUNT(*) FROM creature_template WHERE entry IN(110604,110605,110608,110609) AND ScriptName='npc_stormheim_havi_actor' AND AIName='SmartAI';
SELECT 'impact-scripts',COUNT(*) FROM spell_script_names WHERE spell_id IN(219924,219936,219947) AND ScriptName='spell_stormheim_havi_impact';
SELECT 'native-vehicles',COUNT(*) FROM creature_template WHERE entry IN(110605,110608,110609) AND VehicleId=4888 AND npcflag=16777216 AND ((entry=110605 AND spell1=219921)OR(entry=110608 AND spell1=219935)OR(entry=110609 AND spell1=219946));
SELECT 'native-boarding-delay',COUNT(*) FROM smart_scripts WHERE source_type=9 AND entryorguid IN(11060500,11060800,11060900) AND id=0 AND event_param1=2000 AND event_param2=2000 AND action_type=85 AND action_param1=52391;
SELECT 'native-pot-hit-actions',COUNT(*) FROM smart_scripts WHERE entryorguid=110604 AND source_type=0 AND event_type=8 AND action_type=33 AND target_type=7 AND((event_param1=219924 AND action_param1=110610)OR(event_param1=219936 AND action_param1=110611)OR(event_param1=219947 AND action_param1=110612));
SELECT 'native-credit-wdb',COUNT(*) FROM creature_template_wdb WHERE Entry IN(110610,110611,110612) AND Type=10;
SELECT 'shared-marker-other-quest',COUNT(*) FROM quest_objectives WHERE ObjectID IN(110610,110611,110612) AND QuestID<>38331;
"@
$expected=@("empty-travel-objectives`t0","native-three-ingredients`t3","old-stir-definition`t0","unique-ingredient-slots`t3","raven-starters`t2","havi-enders`t3","havi-next-starters`t2","alternative-trial-gates`t2","common-next-gate`t1","raven-invitations`t2","raven-flags`t2","havi-flags`t1","original-havi`t1","duplicate-havi`t0","phase-specific-havi`t1","alliance-raven-location`t1","horde-raven-location`t1","ingredient-go-scripts`t3","actor-scripts`t4","impact-scripts`t3","native-vehicles`t3","native-boarding-delay`t3","native-pot-hit-actions`t3","native-credit-wdb`t3","shared-marker-other-quest`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-actor',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry=91571 AND(a.ScriptName<>b.ScriptName OR a.lootid<>b.lootid);
SELECT 'native-actor-flags',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.npcflag<>b.npcflag OR a.unit_flags<>b.unit_flags OR a.VehicleId<>b.VehicleId OR a.spell1<>b.spell1;
SELECT 'unrelated-object',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry=240235 AND a.ScriptName<>b.ScriptName;
SELECT 'preserved-native-actions',COUNT(*) FROM smart_scripts a JOIN saved_smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.action_type<>b.action_type OR a.action_param1<>b.action_param1;
UPDATE creature_template SET ScriptName='custom' WHERE entry=110605;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=252074;
UPDATE smart_scripts SET action_param1=12345 WHERE entryorguid=97306 AND source_type=0 AND id=0;
INSERT INTO creature SELECT * FROM duplicate_seed;
UPDATE creature SET PhaseId='999' WHERE guid=341181;
"@
 $sql+="$temporaryMigration`nSELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=110605 AND ScriptName='custom'; SELECT 'custom-object',COUNT(*) FROM gameobject_template WHERE entry=252074 AND ScriptName='custom'; SELECT 'custom-invitation',COUNT(*) FROM smart_scripts WHERE entryorguid=97306 AND source_type=0 AND id=0 AND action_param1=12345; SELECT 'custom-spawn',COUNT(*) FROM creature WHERE guid=341181 AND PhaseId='999';"
 $expected+=@("unrelated-actor`t0","native-actor-flags`t0","unrelated-object`t0","preserved-native-actions`t0","custom-actor`t1","custom-object`t1","custom-invitation`t1","custom-spawn`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Trials SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Trials SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, both raven/Havi travel routes, three native ingredient slots, guarded Havi duplicate removal, scoped bindings, retained vehicle/boarding/pot actions, WDB credit definitions, next-stage gates and custom/unrelated preservation.'

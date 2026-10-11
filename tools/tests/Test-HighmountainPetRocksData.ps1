param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_09_highmountain_pet_rocks.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-pet-rocks-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('smart_scripts','conditions') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'spell-hit-handlers',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=0 AND link=0 AND event_type=8 AND event_chance=100 AND event_flags=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND action_type=33 AND target_type=7 AND action_param1=entryorguid AND ((entryorguid=95272 AND event_param1=189687) OR (entryorguid IN(95273,95276) AND event_param1=189686));
SELECT 'no-shared-click-credit',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(95272,95273,95276) AND event_type=73;
SELECT 'matching-click-spells',COUNT(*) FROM npc_spellclick_spells WHERE cast_flags=1 AND add_npc_flag=1 AND ((npc_entry=95272 AND spell_id=189687) OR (npc_entry IN(95273,95276) AND spell_id=189686));
SELECT 'quest-only-clicks',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=39391 AND NegativeCondition=0 AND ((SourceGroup=95272 AND SourceEntry=189687) OR (SourceGroup IN(95273,95276) AND SourceEntry=189686));
SELECT 'clickable-smart-actors',COUNT(*) FROM creature_template WHERE entry IN(95272,95273,95276) AND AIName='SmartAI' AND ScriptName='' AND (npcflag & 16777216)<>0;
SELECT 'native-models',COUNT(*) FROM creature_template_wdb WHERE (entry=95272 AND Displayid1=64366) OR (entry IN(95273,95276) AND Displayid1=60098);
SELECT 'frag-remains-fallen',COUNT(*) FROM creature_template WHERE entry=95272 AND (unit_flags2 & 1)=1;
SELECT 'actual-spawns',COUNT(*) FROM creature WHERE map=1220 AND PhaseId='' AND ((guid=340572 AND id=95272 AND ABS(position_x-5662.88)<0.02 AND ABS(position_y-4485.67)<0.02) OR (guid=340594 AND id=95273 AND ABS(position_x-5624.23)<0.02 AND ABS(position_y-4612.62)<0.02) OR (guid=340591 AND id=95276 AND ABS(position_x-5376.99)<0.02 AND ABS(position_y-4610.71)<0.02));
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=95421 AND quest=39391;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=95421 AND quest=39391;
SELECT 'parallel-gate',COUNT(*) FROM quest_template_addon WHERE ID=39391 AND PrevQuestID=39381 AND NextQuestID=39426 AND ExclusiveGroup=-39391;
SELECT 'no-disable',COUNT(*) FROM disables WHERE sourceType=1 AND entry=39391;
"@
$expected=@("native-objectives`t0","spell-hit-handlers`t3","no-shared-click-credit`t0","matching-click-spells`t3","quest-only-clicks`t3","clickable-smart-actors`t3","native-models`t3","frag-remains-fallen`t1","actual-spawns`t3","starter`t1","ender`t1","parallel-gate`t1","no-disable`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-smart',COUNT(*) FROM saved_smart_scripts a JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT(a.entryorguid=95272 AND a.source_type=0 AND a.id=0) AND (a.event_type<>b.event_type OR a.event_param1<>b.event_param1 OR a.event_param2<>b.event_param2 OR a.event_param3<>b.event_param3 OR a.event_param4<>b.event_param4 OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
SELECT 'smart-count',(SELECT COUNT(*) FROM saved_smart_scripts)-(SELECT COUNT(*) FROM smart_scripts);
"@
 $expected+=@("unrelated-smart`t0","smart-count`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Pet Rocks SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Pet Rocks SQL mismatch: $($actual -join '|')"}
'PASS: repeated migration, all native objectives/models, matching cast/hit spells, zero shared cooldown, quest-only interactions, Frag presentation, actual spawns/turn-in and untouched unrelated SmartAI.'

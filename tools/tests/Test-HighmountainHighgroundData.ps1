param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_02_highmountain_highground.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-highground-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','quest_template','quest_template_addon','eventobject_template','spell_script_names') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'jale-ai',COUNT(*) FROM creature_template WHERE entry=95078 AND AIName='' AND ScriptName='npc_highground_jale';
SELECT 'old-credit',COUNT(*) FROM smart_scripts WHERE (entryorguid=95078 AND source_type=0) OR (entryorguid=94547 AND source_type=0 AND id=0 AND action_type=33 AND action_param1=94547) OR (entryorguid=93826 AND source_type=0 AND id=0 AND action_type=33 AND action_param1=95078) OR (entryorguid=1501 AND source_type=13 AND action_type=45 AND target_param1=95078);
SELECT 'event-binding',COUNT(*) FROM eventobject_template WHERE entry=1501 AND ScriptName='';
SELECT 'summon-script',COUNT(*) FROM spell_script_names WHERE spell_id=188583 AND ScriptName='spell_highground_jale';
SELECT 'accept-summon',COUNT(*) FROM smart_scripts WHERE entryorguid=93805 AND source_type=0 AND event_type=19 AND event_param1=38909 AND action_type=85 AND action_param1=188583;
SELECT 'other-mayla-quest',COUNT(*) FROM smart_scripts WHERE entryorguid=93826 AND source_type=0 AND event_type=19 AND event_param1=49773 AND action_type=206 AND action_param1=6483;
SELECT 'parallel-branches',COUNT(*) FROM quest_template_addon WHERE ID IN(38912,38913,40515) AND PrevQuestID=38909 AND ExclusiveGroup=0;
SELECT 'native-next',COUNT(*) FROM quest_template WHERE ID=38909 AND RewardNextQuest=38912;
SELECT 'prior-quest',COUNT(*) FROM quest_template_addon WHERE ID=38909 AND PrevQuestID=39027 AND NextQuestID=38912;
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=93805 AND quest=38909) OR (id=93826 AND quest IN(38912,38913,40515));
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=93826 AND quest=38909;
SELECT 'bridge',COUNT(*) FROM creature WHERE guid=376762 AND id=94547 AND map=1220 AND ABS(position_x-4020.24)<0.02 AND ABS(position_y-4568.14)<0.02 AND ABS(position_z-662.637)<0.02;
SELECT 'mayla',COUNT(*) FROM creature WHERE guid=11349875 AND id=93826 AND map=1220 AND ABS(position_x-4083.05)<0.02 AND ABS(position_y-4385.5)<0.02 AND ABS(position_z-670.709)<0.02;
SELECT 'no-static-jale',COUNT(*) FROM creature WHERE id=95078;
"@
$expected=@("native-objectives`t0","jale-ai`t1","old-credit`t0","event-binding`t1","summon-script`t1","accept-summon`t1","other-mayla-quest`t1","parallel-branches`t3","native-next`t1","prior-quest`t1","starters`t4","ender`t1","bridge`t1","mayla`t1","no-static-jale`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>95078 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'unrelated-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id WHERE b.id IS NULL AND NOT ((a.entryorguid=95078 AND a.source_type=0) OR (a.entryorguid=94547 AND a.source_type=0 AND a.id=0 AND a.action_type=33 AND a.action_param1=94547) OR (a.entryorguid=93826 AND a.source_type=0 AND a.id=0 AND a.action_type=33 AND a.action_param1=95078) OR (a.entryorguid=1501 AND a.source_type=13 AND a.action_type=45 AND a.target_param1=95078));
SELECT 'unrelated-chain',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b USING(ID) WHERE (a.ID NOT IN(38912,38913,40515) AND a.PrevQuestID<>b.PrevQuestID) OR a.NextQuestID<>b.NextQuestID OR a.ExclusiveGroup<>b.ExclusiveGroup;
SELECT 'unrelated-reward',COUNT(*) FROM saved_quest_template a JOIN quest_template b USING(ID) WHERE a.ID<>38909 AND a.RewardNextQuest<>b.RewardNextQuest;
SELECT 'unrelated-event',COUNT(*) FROM saved_eventobject_template a JOIN eventobject_template b USING(entry) WHERE a.entry<>1501 AND a.ScriptName<>b.ScriptName;
"@
 $expected+=@("unrelated-ai`t0","unrelated-smart`t0","unrelated-chain`t0","unrelated-reward`t0","unrelated-event`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Highground SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Highground SQL mismatch: $($actual -join '|')"}
'PASS: native objectives/slots, repeated migration, companion binding, removed unowned credit, actual endpoint spawns, parallel branch prerequisites, native reward link and preserved unrelated data.'

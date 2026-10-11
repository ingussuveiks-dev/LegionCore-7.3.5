param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_05_highmountain_blood_trial.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-blood-trial-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('creature_template','gameobject_template','creature_text') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'altar-binding',COUNT(*) FROM gameobject_template WHERE entry=244564 AND AIName='' AND ScriptName='go_blood_trial_altar' AND type=10 AND Data0=1691 AND Data10=192443 AND Data20=1;
SELECT 'duel-binding',COUNT(*) FROM creature_template WHERE entry=97544 AND AIName='' AND ScriptName='npc_blood_trial_warmonger' AND HealthScalingExpansion=6 AND faction=16;
SELECT 'valid-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=97544 AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'burning-slash',COUNT(*) FROM creature_template_spell WHERE entry=97544 AND spell=195789 AND difficultyMask=0;
SELECT 'witch-visual',COUNT(*) FROM creature_template c JOIN creature_template_wdb w ON c.entry=w.entry WHERE c.entry=97539 AND w.Displayid1=64303 AND (c.unit_flags2 & 1)=1;
SELECT 'altar-position',COUNT(*) FROM gameobject WHERE guid=112983 AND id=244564 AND map=1220 AND phaseId='' AND ABS(position_x-5307.56)<0.02 AND ABS(position_y-4997.42)<0.02 AND ABS(position_z-627.498)<0.02;
SELECT 'torok-position',COUNT(*) FROM creature WHERE guid=283367 AND id=93836 AND map=1220 AND phaseId='' AND ABS(position_x-5293.96)<0.02 AND ABS(position_y-4965.95)<0.02 AND ABS(position_z-626.776)<0.02;
SELECT 'native-dialogue',COUNT(*) FROM creature_text WHERE (CreatureID=97544 AND GroupID=1 AND BroadcastTextID=99485) OR (CreatureID=93836 AND GroupID=0 AND BroadcastTextID=99570);
SELECT 'dialogue-fallback',COUNT(*) FROM creature_text WHERE CreatureID=97544 AND GroupID=1 AND ID=0 AND Text='A challenger? Let''s see what this one is capable of!';
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=93836 AND quest=39455;
SELECT 'next-starter',COUNT(*) FROM creature_queststarter WHERE id=93836 AND quest=39860;
SELECT 'next-gate',COUNT(*) FROM quest_template_addon WHERE ID=39860 AND PrevQuestID=39455;
SELECT 'next-offer',COUNT(*) FROM quest_template WHERE ID=39455 AND RewardNextQuest=39860;
SELECT 'next-script',COUNT(*) FROM smart_scripts WHERE entryorguid=93836 AND source_type=0 AND event_type=19 AND event_param1=39860 AND action_type=85 AND action_param1=190902;
SELECT 'no-static-duel',COUNT(*) FROM creature WHERE id=97544;
SELECT 'no-disable',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(39455,39860);
"@
$expected=@("native-objectives`t0","altar-binding`t1","duel-binding`t1","valid-scaling`t1","burning-slash`t1","witch-visual`t1","altar-position`t1","torok-position`t1","native-dialogue`t2","dialogue-fallback`t1","ender`t1","next-starter`t1","next-gate`t1","next-offer`t1","next-script`t1","no-static-duel`t0","no-disable`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-creatures',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE (a.entry<>97544 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName OR a.HealthScalingExpansion<>b.HealthScalingExpansion)) OR a.faction<>b.faction OR a.unit_flags<>b.unit_flags OR a.minlevel<>b.minlevel OR a.maxlevel<>b.maxlevel;
SELECT 'unrelated-objects',COUNT(*) FROM saved_gameobject_template a JOIN gameobject_template b USING(entry) WHERE (a.entry<>244564 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName OR a.Data20<>b.Data20)) OR a.Data10<>b.Data10 OR a.flags<>b.flags OR a.type<>b.type OR a.displayId<>b.displayId;
"@
 $expected+=@("unrelated-creatures`t0","unrelated-objects`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Blood Trial SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Blood Trial SQL mismatch: $($actual -join '|')"}
'PASS: repeated migration, native objectives, personal bindings/scaling, retained combat spell/visual, actual altar/Torok, next quest gate and preserved unrelated data.'

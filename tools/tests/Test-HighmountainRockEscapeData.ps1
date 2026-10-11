param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_07_highmountain_rock_escape.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','conditions','quest_template','quest_template_addon') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'guide-binding',COUNT(*) FROM creature_template WHERE entry=96286 AND AIName='' AND ScriptName='npc_rock_escape_navarrogg' AND gossip_menu_id=18673 AND npcflag=3;
SELECT 'preserved-summon',COUNT(*) FROM smart_scripts WHERE entryorguid=96286 AND source_type=0 AND event_type=54 AND ((action_type=207 AND target_type=23) OR (action_type=29 AND target_type=23) OR (action_type=85 AND action_param1=212762 AND target_type=23));
SELECT 'old-timeline',COUNT(*) FROM smart_scripts WHERE (entryorguid=9628600 AND source_type=9) OR (entryorguid=96286 AND source_type=0 AND event_type=62 AND event_param1=18673 AND event_param2=0);
SELECT 'retry-menu',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18673 AND SourceEntry=0 AND ConditionValue1=39381 AND NegativeCondition=0 AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28));
SELECT 'option',COUNT(*) FROM gossip_menu_option WHERE MenuID=18673 AND OptionID=0 AND OptionNpc=0 AND OptionBroadcastTextID=86628;
SELECT 'native-objective',COUNT(*) FROM quest_objectives WHERE ID=280082 AND QuestID=39381 AND Type=0 AND ObjectID=96286 AND Amount=1 AND Flags=0 AND Flags2=0;
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=96286 AND quest=39381;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=95421 AND quest=39381;
SELECT 'refuge-actor',COUNT(*) FROM creature WHERE id=95421 AND guid=340571 AND map=1220 AND PhaseId='' AND ABS(position_x-5429.69)<0.02 AND ABS(position_y-4491.97)<0.02 AND ABS(position_z-641.583)<0.02;
SELECT 'refuge-questgiver',COUNT(*) FROM creature_template WHERE entry=95421 AND (npcflag & 2)=2;
SELECT 'parallel-offers',COUNT(*) FROM quest_template_addon WHERE ID IN(39391,39425,39588) AND PrevQuestID=39381 AND NextQuestID=39426 AND ExclusiveGroup=-39391;
SELECT 'native-next',COUNT(*) FROM quest_template WHERE ID IN(39391,39425,39588) AND RewardNextQuest=39426;
SELECT 'branch-starters',COUNT(*) FROM creature_queststarter WHERE id=95421 AND quest IN(39391,39425,39588);
SELECT 'join',COUNT(*) FROM quest_template_addon WHERE ID=39426 AND PrevQuestID=39391 AND NextQuestID=40229;
SELECT 'previous',COUNT(*) FROM quest_template_addon WHERE ID=39381 AND PrevQuestID=39860 AND NextQuestID=39391;
SELECT 'no-disable',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(39381,39391,39425,39588,39426);
"@
$expected=@("guide-binding`t1","preserved-summon`t3","old-timeline`t0","retry-menu`t2","option`t1","native-objective`t1","starter`t1","ender`t1","refuge-actor`t1","refuge-questgiver`t1","parallel-offers`t3","native-next`t3","branch-starters`t3","join`t1","previous`t1","no-disable`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-guide',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE (a.entry<>96286 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName)) OR a.npcflag<>b.npcflag OR a.unit_flags<>b.unit_flags;
SELECT 'unrelated-gates',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b USING(ID) WHERE (a.ID NOT IN(39391,39425,39588,39426) AND (a.PrevQuestID<>b.PrevQuestID OR a.NextQuestID<>b.NextQuestID OR a.ExclusiveGroup<>b.ExclusiveGroup)) OR a.SpecialFlags<>b.SpecialFlags;
SELECT 'unrelated-next',COUNT(*) FROM saved_quest_template a JOIN quest_template b USING(ID) WHERE a.ID NOT IN(39391,39425,39588) AND a.RewardNextQuest<>b.RewardNextQuest;
"@
 $expected+=@("unrelated-guide`t0","unrelated-gates`t0","unrelated-next`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Rock escape SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Rock escape SQL mismatch: $($actual -join '|')"}
'PASS: twice-applied SQL, native objective, incomplete/complete menu, preserved guide summon/conversation, refuge endpoint and three parallel offers/join gate; unrelated records preserved.'

param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_19_highmountain_snowblind_parallel.sql" -Raw
$sql=''
if(!$Live){
 $sql="CREATE TEMPORARY TABLE saved_addon LIKE quest_template_addon; INSERT INTO saved_addon SELECT * FROM quest_template_addon; CREATE TEMPORARY TABLE quest_template_addon LIKE saved_addon; INSERT INTO quest_template_addon SELECT * FROM saved_addon; CREATE TEMPORARY TABLE saved_creatures LIKE creature_template; INSERT INTO saved_creatures SELECT * FROM creature_template; CREATE TEMPORARY TABLE creature_template LIKE saved_creatures; INSERT INTO creature_template SELECT * FROM saved_creatures;`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'parallel',COUNT(*) FROM quest_template_addon WHERE ID IN(39776,39777,39862) AND PrevQuestID=38915 AND NextQuestID=42088 AND ExclusiveGroup=-39776;
SELECT 'group-members',COUNT(*) FROM quest_template_addon WHERE ExclusiveGroup=-39776;
SELECT 'evacuation',COUNT(*) FROM quest_template_addon WHERE ID=42088 AND PrevQuestID=39776 AND NextQuestID=42512 AND ExclusiveGroup=0;
SELECT 'incoming-edges',COUNT(*) FROM quest_template_addon WHERE NextQuestID IN(39776,39777,39862) AND ID<>38915;
SELECT 'native-objectives',COUNT(*) FROM quest_objectives WHERE (QuestID=39777 AND Type=0 AND ObjectID=96690 AND Amount=10 AND StorageIndex=0) OR (QuestID=39862 AND Type=0 AND ObjectID=97546 AND Amount=1 AND StorageIndex=0);
SELECT 'slinger-alias',COUNT(*) FROM creature_template_wdb WHERE Entry=96691 AND KillCredit1=0 AND KillCredit2=96690;
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=97344 AND quest=39777) OR (id=97301 AND quest=39862);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE (id=97344 AND quest=39777) OR (id=97301 AND quest=39862);
SELECT 'field-givers',COUNT(*) FROM creature WHERE guid IN(11354582,11354584) AND id IN(97344,97301) AND map=1220 AND phaseId='4104';
SELECT 'phase',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND entry=203 AND phaseId=4104;
SELECT 'phase-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry=203 AND ConditionValue1=38915 AND NegativeCondition=0 AND ((ElseGroup=0 AND ConditionTypeOrReference=28) OR (ElseGroup=1 AND ConditionTypeOrReference=8));
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(96690,96691,97546) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'spawns',COUNT(*) FROM creature WHERE id IN(96690,96691,97546) AND map=1220 AND phaseId='';
SELECT 'combat-ai',COUNT(*) FROM creature_template WHERE entry IN(96690,96691,97546) AND AIName='' AND ScriptName='npc_highmountain_mightstone';
"@
$expected=@("parallel`t3","group-members`t3","evacuation`t1","incoming-edges`t0","native-objectives`t2","slinger-alias`t1","starters`t2","enders`t2","field-givers`t2","phase`t1","phase-conditions`t2","scaling`t3","spawns`t22")
$expected+="combat-ai`t3"
if(!$Live){
 $sql+=@"
SELECT 'other-links',COUNT(*) FROM saved_addon a JOIN quest_template_addon b USING(ID) WHERE a.ID NOT IN(39776,39777,39862,42088) AND (a.PrevQuestID<>b.PrevQuestID OR a.NextQuestID<>b.NextQuestID OR a.ExclusiveGroup<>b.ExclusiveGroup);
SELECT 'other-ai',COUNT(*) FROM saved_creatures a JOIN creature_template b USING(entry) WHERE a.entry NOT IN(96690,96691,97546) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
UPDATE quest_template_addon SET ExclusiveGroup=12345 WHERE ID=39777;
"@
 $sql+="`n$migration`nSELECT 'custom-group-preserved',COUNT(*) FROM quest_template_addon WHERE ID=39777 AND ExclusiveGroup=12345;"
 $expected+=@("other-links`t0","other-ai`t0","custom-group-preserved`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Snowblind parallel SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Snowblind parallel SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: deployed parallel branches/all-reward gate, native kill aliases/objectives, questgiver phases and scaled spawns.'}
else{'PASS: repeated migration, parallel branches/all-reward gate, native content, unrelated links and custom group preservation.'}

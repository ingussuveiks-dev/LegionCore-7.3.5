param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_03_highmountain_bloodtotem.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-bloodtotem-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('quest_template_addon','quest_objectives') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'parallel',COUNT(*) FROM quest_template_addon WHERE ID IN(39372,39373,39873) AND PrevQuestID=38912 AND NextQuestID=39374 AND ExclusiveGroup=-39372;
SELECT 'group-members',COUNT(*) FROM quest_template_addon WHERE ExclusiveGroup=-39372;
SELECT 'entry',COUNT(*) FROM quest_template_addon WHERE ID=38912 AND PrevQuestID=38909 AND NextQuestID=39372;
SELECT 'audience',COUNT(*) FROM quest_template_addon WHERE ID=39374 AND PrevQuestID=39372 AND NextQuestID=39455;
SELECT 'incoming-bypass',COUNT(*) FROM quest_template_addon WHERE (NextQuestID=39372 AND ID<>38912) OR (NextQuestID=39374 AND ID NOT IN(39372,39373,39873));
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(38912,39372,39373,39873,39374);
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=93826 AND quest=38912) OR (id=95256 AND quest IN(39372,39373,39374)) OR (id=97667 AND quest=39873);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=95256 AND quest IN(38912,39372,39373,39374);
SELECT 'cage-ender',COUNT(*) FROM gameobject_questender WHERE id=243402 AND quest=39873;
SELECT 'witch',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=95311 AND c.map=1220 AND c.phaseId='' AND t.faction=16;
SELECT 'hags',COUNT(*)>0 FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=95310 AND c.map=1220 AND t.lootid=95310;
SELECT 'feather-loot',COUNT(*) FROM creature_loot_template WHERE Entry=95310 AND Item=128380 AND QuestRequired=1 AND Chance>0 AND MinCount=3 AND MaxCount=7;
SELECT 'cup',COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.id=244625 AND g.map=1220 AND g.phaseId='' AND t.type=10;
SELECT 'cage',COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.id=243402 AND g.map=1220 AND g.phaseId='' AND t.type=2;
SELECT 'consume-feathers',COUNT(*) FROM quest_template WHERE ID=39373 AND FlagsEx=0;
"@
$expected=@("native-objectives`t0","parallel`t3","group-members`t3","entry`t1","audience`t1","incoming-bypass`t0","enabled`t0","starters`t5","enders`t4","cage-ender`t1","witch`t1","hags`t1","feather-loot`t1","cup`t1","cage`t1","consume-feathers`t1")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-chain',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b USING(ID) WHERE (a.ID NOT IN(39372,39373,39374,39873) AND a.PrevQuestID<>b.PrevQuestID) OR (a.ID NOT IN(38912,39371,39372,39373,39873) AND a.NextQuestID<>b.NextQuestID) OR (a.ID NOT IN(39372,39373,39873) AND a.ExclusiveGroup<>b.ExclusiveGroup);
SELECT 'unrelated-objectives',COUNT(*) FROM saved_quest_objectives a JOIN quest_objectives b USING(ID) WHERE (a.ID<>279309 AND (a.Amount<>b.Amount OR a.Flags2<>b.Flags2)) OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.StorageIndex<>b.StorageIndex OR a.Flags<>b.Flags;
"@
 $expected+=@("unrelated-chain`t0","unrelated-objectives`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Bloodtotem SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Bloodtotem SQL mismatch: $($actual -join '|')"}
'PASS: native objectives, repeated migration, parallel quests/all-three gate, no alternative incoming link, enabled quests, real targets/loot/turn-ins and preserved unrelated data.'

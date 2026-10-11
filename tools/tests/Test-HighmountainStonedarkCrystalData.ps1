param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_10_highmountain_stonedark_crystal.sql" -Raw
$sql=''
if(!$Live){
 $sql="CREATE TEMPORARY TABLE saved_objectives LIKE quest_objectives; INSERT INTO saved_objectives SELECT * FROM quest_objectives; CREATE TEMPORARY TABLE quest_objectives LIKE saved_objectives; INSERT INTO quest_objectives SELECT * FROM saved_objectives;`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE ID=279391 AND QuestID=39425 AND Type=1 AND ObjectID=128343 AND Amount=1 AND StorageIndex=0 AND Flags=0 AND Flags2=1;
SELECT 'only-objective',COUNT(*) FROM quest_objectives WHERE QuestID=39425;
SELECT 'loot-sources',COUNT(*) FROM creature_loot_template l JOIN creature_template t ON t.entry=l.Entry AND t.lootid=l.Entry WHERE l.Item=128343 AND l.Entry IN(95290,96307,96423,96615,104323,104328) AND l.QuestRequired=1 AND l.Chance>0 AND l.Chance<=100 AND l.Reference=0 AND l.Currency=0 AND l.LootMode=0 AND l.GroupId=0 AND l.MinCount=1 AND l.MaxCount=1;
SELECT 'local-loot-spawns',COUNT(DISTINCT c.id) FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_loot_template l ON l.Entry=t.lootid WHERE l.Item=128343 AND c.map=1220 AND c.areaId=7725 AND c.phaseMask=1 AND c.phaseId='' AND t.faction=2792 AND (t.unit_flags & 33554690)=0;
SELECT 'loot-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=1 AND SourceEntry=128343;
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=95421 AND quest=39425;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=95421 AND quest=39425;
SELECT 'refuge',COUNT(*) FROM creature WHERE guid=340571 AND id=95421 AND map=1220 AND phaseId='';
SELECT 'consume-focus',COUNT(*) FROM quest_template WHERE ID=39425 AND FlagsEx=0;
SELECT 'parallel-gate',COUNT(*) FROM quest_template_addon WHERE ID=39425 AND PrevQuestID=39381 AND NextQuestID=39426 AND ExclusiveGroup=-39391;
SELECT 'no-disable',COUNT(*) FROM disables WHERE sourceType=1 AND entry=39425;
"@
$expected=@("objective`t1","only-objective`t1","loot-sources`t6","local-loot-spawns`t4","loot-conditions`t0","starter`t1","ender`t1","refuge`t1","consume-focus`t1","parallel-gate`t1","no-disable`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-objectives',COUNT(*) FROM saved_objectives a JOIN quest_objectives b USING(ID) WHERE (a.ID<>279391 AND a.Flags2<>b.Flags2) OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.StorageIndex<>b.StorageIndex OR a.Flags<>b.Flags;
SELECT 'row-count',(SELECT COUNT(*) FROM saved_objectives)-(SELECT COUNT(*) FROM quest_objectives);
"@
 $expected+=@("unrelated-objectives`t0","row-count`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Stonedark Crystal SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Stonedark Crystal SQL mismatch: $($actual -join '|')"}
'PASS: native objective, six valid quest-only loot sources, four locally spawned sources, refuge turn-in, item consumption, parallel gate and repeated migration isolation.'

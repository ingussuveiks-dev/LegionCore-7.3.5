param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-helheim-supplies"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_43_stormheim_cursed_bones.sql" -Raw
$sql=@"
SELECT 'objective',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID IN(38324,38339) ORDER BY QuestID;
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.RewardNextQuest,q.StartScript,q.CompleteScript,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38324,38339,38347) ORDER BY q.ID;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.id=s.id AND e.quest=s.quest WHERE s.id=91531 AND s.quest IN(38324,38339);
SELECT 'next-giver',COUNT(*) FROM creature_queststarter WHERE id=91531 AND quest=38347;
SELECT 'colborn',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=91531 AND c.guid=11546819 AND c.map=1463 AND c.PhaseId='5577' AND t.npcflag=2 AND t.ScriptName='';
SELECT 'pile',entry,type,displayId,flags,size,Data0,Data8,Data15,Data30,Data31,AIName,ScriptName FROM gameobject_template WHERE entry=240586;
SELECT 'piles',COUNT(*),SUM(map=1463 AND zoneId=7588 AND phaseMask=1 AND PhaseId='' AND spawntimesecs=60) FROM gameobject WHERE id=240586;
SELECT 'position',guid,id,position_x,position_y,position_z FROM gameobject WHERE id=240586 ORDER BY guid;
SELECT 'drops',Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Shared FROM creature_loot_template WHERE Item=122619 ORDER BY Entry;
SELECT 'sources',COUNT(*) FROM creature_template WHERE entry IN(91575,93377) AND lootid=entry AND faction=16 AND (unit_flags & 33555200)=0 AND ScriptName='';
SELECT 'kvaldir',id,COUNT(*),SUM(map=1463 AND phaseMask=1 AND modelid=0),SUM(PhaseId='8306') FROM creature WHERE id IN(91575,93377) GROUP BY id ORDER BY id;
SELECT 'model-info',COUNT(*) FROM creature_model_info WHERE DisplayID IN(25633,70917,63385) AND BoundingRadius>0 AND CombatReach>0;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=240586) OR (SourceTypeOrReferenceId=1 AND SourceGroup IN(91575,93377) AND SourceEntry=122619) OR (SourceTypeOrReferenceId=30 AND SourceEntry IN(122611,122619)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38324,38339,38347));
"@
if(!$Live){$sql+=@"
CREATE TEMPORARY TABLE saved_template LIKE gameobject_template;
INSERT INTO saved_template SELECT * FROM gameobject_template WHERE entry IN(240586,241778);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_template;
INSERT INTO gameobject_template SELECT * FROM saved_template;
UPDATE gameobject_template SET Data1=0 WHERE entry=240586;
SELECT 'before',Data1 FROM gameobject_template WHERE entry=240586;
$migration
$migration
UPDATE saved_template SET Data1=240586 WHERE entry=240586;
SELECT 'actual',t.* FROM gameobject_template t ORDER BY entry;
SELECT 'expected',t.* FROM saved_template t ORDER BY entry;
"@}
$sql+=@"
SELECT 'loot',g.entry,g.Data1,l.Item,l.Chance,l.QuestRequired,l.LootMode,l.GroupId,l.MinCount,l.MaxCount FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry=240586;
"@
if(!$Live){$sql+=@"
UPDATE gameobject_template SET Data1=12345,Data30=999,Data15=1 WHERE entry=240586;
$migration
SELECT 'custom',Data1,Data30,Data15 FROM gameobject_template WHERE entry=240586;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=240586;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
UPDATE gameobject_template SET Data1=0 WHERE entry=240586;
$migration
SELECT 'missing',Data1 FROM gameobject_template WHERE entry=240586;
INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
UPDATE gameobject_loot_template SET Item=999999;
$migration
SELECT 'wrong-item',Data1 FROM gameobject_template WHERE entry=240586;
SELECT 'loot-unchanged',COUNT(*) FROM gameobject_loot_template WHERE Item=999999;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Helheim supplies SQL failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @(
 "objective`t277700`t38324`t1`t0`t122611`t8`t0","objective`t277717`t38339`t1`t0`t122619`t20`t0",
 "quest`t38324`t37748744`t8192`t38347`t0`t0`t18446744073709551615`t39837`t38347`t-38324`t0",
 "quest`t38339`t37748744`t8192`t38347`t0`t0`t18446744073709551615`t39837`t38347`t-38324`t0",
 "quest`t38347`t38797320`t12288`t39848`t0`t0`t18446744073709551615`t38324`t39848`t0`t0",
 "relations`t2","next-giver`t1","colborn`t1","pile`t240586`t3`t6328`t2113540`t1.75`t2160`t38324`t0`t0`t0`t`t",
 "piles`t16`t16","drops`t91575`t122619`t0`t0`t67.0591`t1`t0`t0`t1`t5`t0","drops`t93377`t122619`t0`t0`t59.4284`t1`t0`t0`t1`t5`t0",
 "sources`t2","kvaldir`t91575`t23`t23`t2","kvaldir`t93377`t10`t10`t1","model-info`t3","conditions`t0","loot`t240586`t240586`t122611`t100`t1`t1`t0`t1`t1"
)){if($rows -notcontains $line){throw "Missing: $line"}}
if(@($rows | Where-Object {$_ -like "position`t*"}).Count -ne 16){throw 'Missing pile positions'}
if(!$Live){
 foreach($line in @("before`t0","custom`t12345`t999`t1","missing`t0","wrong-item`t0","loot-unchanged`t1")){if($rows -notcontains $line){throw "Missing: $line"}}
 $actual=@($rows | Where-Object {$_ -like "actual`t*"} | ForEach-Object {$_.Substring(7)})
 $expected=@($rows | Where-Object {$_ -like "expected`t*"} | ForEach-Object {$_.Substring(9)})
 if($actual.Count -ne 2 -or (Compare-Object $actual $expected)){throw 'Whole-row template preservation failed'}
}
$suffix=if($Live){'live'}else{'fixture'}
$rows | Set-Content "$out/data-$suffix.tsv"
($rows | Where-Object {$_ -like "loot`t*"}) -replace '^loot\t','' | Set-Content "$out/loot-$suffix.tsv"
($rows | Where-Object {$_ -like "drops`t*"}) -replace '^drops\t','' | Set-Content "$out/drops-$suffix.tsv"
($rows | Where-Object {$_ -like "position`t*"}) -replace '^position\t','' | Set-Content "$out/positions-$suffix.tsv"
'PASS: bone selector, sixteen piles, two unchanged seaweed tables/33 sources and later warrior spawns, native item objectives and parallel handoff. Replay/custom/missing/wrong loot and unrelated cage boundaries checked in fixture mode.'

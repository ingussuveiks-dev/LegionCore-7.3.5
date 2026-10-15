param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-regal-remains"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_38_stormheim_regal_remains.sql" -Raw
$sql=@"
SELECT 'loot',Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM gameobject_loot_template WHERE Entry IN(241870,241871,241872,241873,241874) ORDER BY Entry;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38817;
SELECT 'spawns',id,COUNT(*),SUM(map=1220 AND zoneId=7541 AND spawnMask=1 AND phaseMask=1 AND spawntimesecs=180) FROM gameobject WHERE id IN(241870,241871,241872,241873,241874) GROUP BY id ORDER BY id;
SELECT 'position',guid,id,position_x,position_y,position_z FROM gameobject WHERE id IN(241870,241871,241872,241873,241874) ORDER BY guid;
SELECT 'model-info',COUNT(*) FROM creature_model_info WHERE DisplayID=69637 AND BoundingRadius>0 AND CombatReach>0;
SELECT 'template',entry,displayId,size FROM gameobject_template WHERE entry IN(241870,241871,241872,241873,241874) AND type=3 AND flags=4 AND questItem1=124512 AND Data0=1690 AND Data8=38817 AND Data15=0 AND Data30=0 AND ScriptName='' AND AIName='' ORDER BY entry;
SELECT 'giver',COUNT(*) FROM creature_queststarter WHERE id=93446 AND quest=38817;
SELECT 'ender-next',COUNT(*) FROM creature_questender e JOIN creature_queststarter s ON s.id=e.id WHERE e.id=97270 AND e.quest=38817 AND s.quest=38815;
SELECT 'handoff',q.ID,q.Flags,q.FlagsEx,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38817,38815) ORDER BY q.ID;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(4,14,22) AND SourceGroup IN(241870,241871,241872,241873,241874)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38817,38815));
SELECT 'iounn',guid,modelid,map,phaseMask,position_x,position_y,position_z FROM creature WHERE id=97270 ORDER BY guid;
"@
if(!$Live){$sql+=@"
CREATE TEMPORARY TABLE saved_template LIKE gameobject_template;
INSERT INTO saved_template SELECT * FROM gameobject_template WHERE entry IN(241870,241871,241872,241873,241874,241866);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_template;
INSERT INTO gameobject_template SELECT * FROM saved_template;
UPDATE gameobject_template SET Data1=0 WHERE entry IN(241870,241871,241872,241873,241874);
SELECT 'before',COUNT(*) FROM gameobject_template WHERE entry IN(241870,241871,241872,241873,241874) AND Data1=0;
$migration
$migration
UPDATE saved_template SET Data1=entry WHERE entry IN(241870,241871,241872,241873,241874);
SELECT 'actual',g.* FROM gameobject_template g ORDER BY entry;
SELECT 'expected',g.* FROM saved_template g ORDER BY entry;
"@}
$sql+=@"
SELECT 'selector',g.entry,g.Data1,l.Item,l.Chance FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry IN(241870,241871,241872,241873,241874) ORDER BY g.entry;
"@
if(!$Live){$sql+=@"
UPDATE gameobject_template SET Data1=987654,Data0=777,flags=42,Data30=123 WHERE entry=241870;
$migration
SELECT 'custom',Data1,Data0,flags,Data30 FROM gameobject_template WHERE entry=241870;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry IN(241870,241871,241872,241873,241874);
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
INSERT INTO gameobject_loot_template SELECT * FROM saved_loot WHERE Entry!=241874;
UPDATE gameobject_template SET Data1=0 WHERE entry=241874;
$migration
SELECT 'missing',Data1 FROM gameobject_template WHERE entry=241874;
SELECT 'loot-actual',l.* FROM gameobject_loot_template l ORDER BY Entry;
SELECT 'loot-expected',l.* FROM saved_loot l WHERE Entry!=241874 ORDER BY Entry;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Regal Remains SQL check failed'}
}finally{$env:MYSQL_PWD=$old}
foreach($value in @(
 "objective`t278485`t1`t1`t124512`t8`t0","giver`t1","ender-next`t1","conditions`t0","model-info`t1",
 "handoff`t38815`t33554440`t0`t38818`t18446744073709551615`t38816`t38818`t0",
 "handoff`t38817`t33554440`t0`t38815`t18446744073709551615`t39791`t38815`t0",
 "iounn`t338670`t0`t1220`t1`t3416.96`t977.193`t101.426","iounn`t341219`t0`t1220`t1`t3416.96`t977.193`t101.426",
 "spawns`t241870`t1`t1","spawns`t241871`t2`t2","spawns`t241872`t1`t1","spawns`t241873`t2`t2","spawns`t241874`t7`t7",
 "template`t241870`t20764`t3","template`t241871`t17934`t2","template`t241872`t18974`t3","template`t241873`t25607`t2","template`t241874`t18988`t0.5"
)){if($rows -notcontains $value){throw "Regal data mismatch: $value"}}
foreach($entry in 241870..241874){$chance=if($entry -eq 241872){'100.185'}else{'100'}
 foreach($value in @("loot`t$entry`t124512`t0`t0`t$chance`t1`t1`t0`t1`t1","selector`t$entry`t$entry`t124512`t$chance")){if($rows -notcontains $value){throw "Regal loot mismatch: $value"}}
}
if(@($rows | Where-Object {$_ -like "position`t*"}).Count -ne 13){throw 'Expected all thirteen bone positions'}
if(@($rows | Where-Object {$_ -like "selector`t*"}).Count -ne 5 -or @($rows | Where-Object {$_ -like "loot`t*"}).Count -ne 5){throw 'Unexpected bone loot/selector count'}
if(!$Live){
 foreach($value in @("before`t5","custom`t987654`t777`t42`t123","missing`t0")){if($rows -notcontains $value){throw "Missing migration boundary: $value"}}
 foreach($check in @(@('actual','expected',6),@('loot-actual','loot-expected',4))){
  $actual=@($rows | Where-Object {$_ -like "$($check[0])`t*"} | ForEach-Object {$_.Substring($check[0].Length+1)})
  $expected=@($rows | Where-Object {$_ -like "$($check[1])`t*"} | ForEach-Object {$_.Substring($check[1].Length+1)})
  if($actual.Count -ne $check[2] -or (Compare-Object $actual $expected)){throw 'Full-row preservation/replay mismatch'}
 }
}
$suffix=if($Live){'live'}else{'fixture'}
[IO.File]::WriteAllLines("$out/positions-$suffix.tsv",[string[]]@($rows | Where-Object {$_ -like "position`t*"} | ForEach-Object {$_.Substring(9)}))
[IO.File]::WriteAllLines("$out/data-$suffix.tsv",[string[]]$rows)
[IO.File]::WriteAllLines("$out/loot-$suffix.tsv",[string[]]@($rows | Where-Object {$_ -like "selector`t*"} | ForEach-Object {$_.Substring(9)}))
"PASS: $suffix five bone loot links/unchanged chances, thirteen spawns, slot1 item objective, Iounn turn-in/next, both existing ender spawns; replay/custom/missing loot and unrelated Soulthorn preserved in fixture mode."

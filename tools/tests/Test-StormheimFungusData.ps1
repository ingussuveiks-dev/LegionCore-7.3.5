param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-fungus"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_36_stormheim_dreaming_fungus.sql" -Raw
$sql=@"
SELECT 'loot',Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM gameobject_loot_template WHERE Entry=241866;
SELECT 'objective',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38810;
SELECT 'spawns',COUNT(*),SUM(map=1220 AND spawnMask=1 AND phaseMask=1 AND spawntimesecs=180) FROM gameobject WHERE id=241866;
SELECT 'template',entry,type,displayId,flags,questItem1,Data0,Data8,Data15,Data30,AIName,ScriptName FROM gameobject_template WHERE entry=241866;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.quest=s.quest AND e.id=s.id WHERE s.id=93231 AND s.quest=38810;
SELECT 'nextstarter',COUNT(*) FROM creature_queststarter WHERE id=93231 AND quest=38811;
SELECT 'nextender',COUNT(*) FROM creature_questender WHERE id=93446 AND quest=38811;
SELECT 'handoff',q.ID,q.Flags,q.FlagsEx,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38778,39788,38810,38811) ORDER BY q.ID;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(4,14,22) AND SourceGroup=241866) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38810,38811));
"@
if($Live){
 $sql+="SELECT 'live',Data1 FROM gameobject_template WHERE entry=241866;"
}else{
 $sql+=@"
CREATE TEMPORARY TABLE saved_template LIKE gameobject_template;
INSERT INTO saved_template SELECT * FROM gameobject_template WHERE entry IN(241866,241763);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_template;
INSERT INTO gameobject_template SELECT * FROM saved_template;
UPDATE gameobject_template SET Data1=0 WHERE entry=241866;
SELECT 'before',Data1 FROM gameobject_template WHERE entry=241866;
$migration
$migration
UPDATE saved_template SET Data1=241866 WHERE entry=241866;
SELECT 'actual',g.* FROM gameobject_template g ORDER BY entry;
SELECT 'expected',g.* FROM saved_template g ORDER BY entry;
UPDATE gameobject_template SET Data1=987654,Data0=777,flags=42,Data30=123 WHERE entry=241866;
$migration
SELECT 'custom',Data1,Data0,flags,Data30 FROM gameobject_template WHERE entry=241866;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
UPDATE gameobject_template SET Data1=0 WHERE entry=241866;
$migration
SELECT 'missing-loot',Data1 FROM gameobject_template WHERE entry=241866;
"@
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Fungus SQL regression failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @(
 "loot`t241866`t124504`t0`t0`t100`t1`t1`t0`t1`t1",
 "objective`t278463`t38810`t1`t0`t124504`t8`t0",
 "spawns`t19`t19", "template`t241866`t3`t28223`t4`t124504`t1690`t38810`t0`t0`t`t",
 "relations`t1","nextstarter`t1","nextender`t1","conditions`t0",
 "handoff`t38778`t39845896`t270336`t38810`t18446744073709551615`t39796`t38810`t0`t0",
 "handoff`t39788`t37748744`t8192`t38810`t18446744073709551615`t39796`t38810`t0`t0",
 "handoff`t38810`t33554440`t0`t38811`t18446744073709551615`t39788`t38811`t0`t0",
 "handoff`t38811`t33554440`t0`t38817`t18446744073709551615`t38810`t38817`t0`t0"
)) {if($rows -notcontains $expected){throw "Native data mismatch: $expected"}}
if(@($rows | Where-Object {$_ -like "loot`t*"}).Count -ne 1 -or @($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 1){throw 'Unexpected extra loot/objective rows'}
if($Live){if($rows -notcontains "live`t241866"){throw 'Installed chest still has no loot selector'}}else{
 foreach($expected in @("before`t0","custom`t987654`t777`t42`t123","missing-loot`t0")){if($rows -notcontains $expected){throw "Missing isolation check: $expected"}}
 $actual=@($rows | Where-Object {$_ -like "actual`t*"} | ForEach-Object {$_.Substring(7)})
 $expected=@($rows | Where-Object {$_ -like "expected`t*"} | ForEach-Object {$_.Substring(9)})
 if($actual.Count -ne 2 -or (Compare-Object $actual $expected)){throw 'Migration changed unrelated fields/object or replay was not idempotent'}
}
$suffix=if($Live){'live'}else{'fixture'}
[IO.File]::WriteAllLines("$output/data-$suffix.tsv",[string[]]$rows)
"PASS: $suffix Soulthorn loot selector, one guaranteed quest item, eight-item objective, all19 native spawns, original lock/model/flags/shared settings, prerequisites and actual Vydhar->38811 relations; replay/custom/missing-loot isolation checked in fixture mode."

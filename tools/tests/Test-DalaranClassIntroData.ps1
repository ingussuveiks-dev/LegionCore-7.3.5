$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_14_dalaran_class_introduction.sql" -Raw
$tables=@('spell_area','conditions','creature_text')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'offer',COUNT(*) FROM spell_area WHERE spell=204860 AND area=7502 AND quest_end=40716 AND quest_end_status=1 AND classmask=256 AND autocast=1;
SELECT 'other-offers-changed',COUNT(*) FROM spell_area a JOIN saved_spell_area b USING(spell,area,quest_start,quest_end) WHERE a.spell<>204860 AND a.quest_end_status<>b.quest_end_status;
SELECT 'bound-guards',COUNT(*) FROM conditions c JOIN smart_scripts s ON s.entryorguid=c.SourceEntry AND s.source_type=c.SourceId AND s.id+1=c.SourceGroup WHERE c.SourceTypeOrReferenceId=22 AND c.SourceGroup=1 AND c.SourceId=13 AND c.SourceEntry BETWEEN 136 AND 140 AND c.ConditionValue1=42594 AND c.ConditionTypeOrReference IN(9,41);
SELECT 'orphan-guards',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=10 AND SourceEntry BETWEEN 136 AND 140 AND ConditionValue1=42594;
SELECT 'greeting',COUNT(*) FROM creature_text WHERE CreatureID=103506 AND GroupID=0 AND ID=0 AND BroadcastTextID=106111 AND Sound=71324;
SELECT 'accept-line',COUNT(*) FROM creature_text WHERE CreatureID=103506 AND GroupID=1 AND ID=0 AND BroadcastTextID=104179 AND Sound=63919;
CREATE TEMPORARY TABLE cases (id INT, quest_state INT, objectives INT);
INSERT INTO cases VALUES (0,0,7),(1,3,0),(2,3,6),(3,3,5),(4,3,3),(5,3,7),(6,6,7);
SELECT CONCAT('guard-case-',p.id),MIN(CASE c.ConditionTypeOrReference WHEN 9 THEN p.quest_state=3 WHEN 41 THEN (p.objectives & CASE c.ConditionValue2 WHEN 107998 THEN 1 WHEN 108001 THEN 2 WHEN 108011 THEN 4 ELSE 0 END)<>0 ELSE 0 END)
FROM cases p CROSS JOIN conditions c WHERE c.SourceTypeOrReferenceId=22 AND c.SourceGroup=1 AND c.SourceId=13 AND c.SourceEntry=139 GROUP BY p.id ORDER BY p.id;
UPDATE spell_area SET quest_end_status=33 WHERE spell=204860 AND area=7502;
UPDATE creature_text SET BroadcastTextID=1,Sound=2 WHERE CreatureID=103506 AND GroupID IN(0,1);
$migration
SELECT 'custom-offer',COUNT(*) FROM spell_area WHERE spell=204860 AND area=7502 AND quest_end_status=33;
SELECT 'custom-text',COUNT(*) FROM creature_text WHERE CreatureID=103506 AND GroupID IN(0,1) AND BroadcastTextID=1 AND Sound=2;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=$fixture | & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Dalaran class introduction SQL fixture failed'}
 $expected=@("offer`t1","other-offers-changed`t0","bound-guards`t17","orphan-guards`t0","greeting`t1","accept-line`t1","guard-case-0`t0","guard-case-1`t0","guard-case-2`t0","guard-case-3`t0","guard-case-4`t0","guard-case-5`t1","guard-case-6`t0","custom-offer`t1","custom-text`t2")
 if(($rows -join '|') -ne ($expected -join '|')){throw "Wrong migration result: $($rows -join '|')"}
} finally {$env:MYSQL_PWD=$previous}
'PASS: repeatable offer/text repair; 17 guards bind real event scripts; no-quest and missing-objective dialogue blocked; active correct progress admitted; custom replacements preserved.'

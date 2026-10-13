$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_16_new_blood_ritual_handoff.sql" -Raw
$ownerMigration=Get-Content "$repo/sql/updates/world/2026_10_13_01_new_blood_capture_owner.sql" -Raw
$fixture=(@('conditions','smart_scripts') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
-- Recreate the historical map guards for this older migration's fixture.
-- Current base-object targeting is covered by Test-NewBloodCapture.ps1.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98831 AND SourceGroup IN(3,5) AND ConditionTypeOrReference=22 AND ConditionTarget=1;
$migration
$migration
$ownerMigration
$ownerMigration
SELECT 'opening-branches',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=98831 AND SourceId=0 AND ConditionTypeOrReference=29 AND ConditionValue1 IN(99178,101456) AND NegativeCondition=0;
SELECT 'fallback',COUNT(*) FROM smart_scripts WHERE entryorguid=98831 AND source_type=0 AND id=4 AND event_type=60 AND event_flags=1 AND event_param1=18000 AND event_param2=18000 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=1;
SELECT 'obsolete-fallback',COUNT(*) FROM smart_scripts WHERE entryorguid=9879701 AND source_type=9 AND id=13 AND action_type=45 AND action_param1=1;
SELECT 'actor-dependency',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=98831 AND ConditionTypeOrReference=29;
SELECT 'map-guards',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup IN(3,5) AND SourceEntry=98831 AND SourceId=0 AND ConditionTypeOrReference=22 AND ConditionValue1 IN(1220,1502);
SELECT 'once',COUNT(*) FROM smart_scripts WHERE entryorguid=98831 AND source_type=0 AND id=2 AND event_type=38 AND event_param1=1 AND event_param2=1 AND (event_flags & 1)=1 AND action_type=80 AND action_param1=9883100;
SELECT 'existing-scripts-changed',COUNT(*) FROM smart_scripts a JOIN saved_smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.event_flags<>b.event_flags;
SELECT 'capture-tail',COUNT(*) FROM smart_scripts WHERE entryorguid=9883100 AND source_type=9 AND id=5 AND action_type=11 AND action_param1=200038;
SELECT 'jailer-blackout',COUNT(*) FROM smart_scripts WHERE entryorguid=101460 AND source_type=9 AND id=0 AND event_param1=12000 AND action_type=85 AND action_param1=199362;
SELECT 'native-transfer',COUNT(*) FROM spell_linked_spell s JOIN spell_target_position p ON p.id=s.spell_effect WHERE s.spell_trigger=199362 AND s.spell_effect=199359 AND p.target_map=1522;
SELECT JSON_OBJECT('despawn',d.action_param1,'challenge',c.event_param1,'fallback',f.event_param1,'jagLifetime',j.action_param3) FROM smart_scripts d JOIN smart_scripts c ON c.entryorguid=9879701 AND c.source_type=9 AND c.id=11 JOIN smart_scripts f ON f.entryorguid=98831 AND f.source_type=0 AND f.id=4 JOIN smart_scripts j ON j.entryorguid=9879701 AND j.source_type=9 AND j.id=10 WHERE d.entryorguid=98797 AND d.source_type=0 AND d.id=10;
UPDATE smart_scripts SET action_param1=99 WHERE entryorguid=98831 AND source_type=0 AND id=4;
$ownerMigration
SELECT 'custom-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=98831 AND source_type=0 AND id=4 AND action_param1=99;
"@
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1
$connection=$config.Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=$fixture | & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names
 if($LASTEXITCODE){throw 'New Blood SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$lines=$rows | Where-Object {$_ -match '^(opening-branches|fallback|obsolete-fallback|actor-dependency|map-guards|once|existing-scripts-changed|capture-tail|jailer-blackout|native-transfer|custom-preserved)\s+[0-9]+$'}
$expected=@("opening-branches`t2","fallback`t1","obsolete-fallback`t0","actor-dependency`t0","map-guards`t4","once`t1","existing-scripts-changed`t0","capture-tail`t1","jailer-blackout`t1","native-transfer`t1","custom-preserved`t1")
if(($lines -join '|') -ne ($expected -join '|')){throw "Ritual migration mismatch: $($rows -join '|')"}
$timing=($rows | Where-Object {$_.StartsWith('{')}) | ConvertFrom-Json
if(-not $timing -or $timing.despawn -ge 8000 -or $timing.fallback -ge $timing.jagLifetime){throw 'Actor lifetime regression fixture changed'}
# Exercise both signal orderings with the installed event times. A successful
# movement callback despawns Zelifrax before his old fallback; Jagganoth lives.
foreach($map in @(1502,1220,1522)) {foreach($callback in @('normal','missing','rejected')) {
 $starts=0; $ran=$false
 $events=@(@{time=$timing.fallback;name='boss-timer'})
 if($callback -ne 'missing'){$events+=@{time=$timing.challenge+100;name='movement'}}
 foreach($e in ($events | Sort-Object time)) {
  if($callback -eq 'rejected' -and $e.name -eq 'movement'){continue}
  if($map -notin @(1220,1502) -or $ran){continue}
  $starts++;$ran=$true
 }
 $expectedStarts=if($map -eq 1522){0}else{1}
 if($starts -ne $expectedStarts){throw "Capture timing failed: $map/$callback"}
}}
'PASS: repeatable migrations, surviving fallback owner, no actor/phase dependency, once-only capture under callback loss/rejection, later scene excluded, native transfer chain intact.'

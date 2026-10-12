$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_16_new_blood_ritual_handoff.sql" -Raw
$fixture=(@('conditions','smart_scripts') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'opening-branches',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=98831 AND SourceId=0 AND ConditionTypeOrReference=29 AND ConditionValue1 IN(99178,101456) AND NegativeCondition=0;
SELECT 'fallback',COUNT(*) FROM smart_scripts WHERE entryorguid=9879701 AND source_type=9 AND id=13 AND event_param1=8000 AND event_param2=8000 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=98831 AND target_param2=30;
SELECT 'once',COUNT(*) FROM smart_scripts WHERE entryorguid=98831 AND source_type=0 AND id=2 AND event_type=38 AND event_param1=1 AND event_param2=1 AND (event_flags & 1)=1 AND action_type=80 AND action_param1=9883100;
SELECT 'existing-scripts-changed',COUNT(*) FROM smart_scripts a JOIN saved_smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.event_flags<>b.event_flags;
SELECT 'capture-tail',COUNT(*) FROM smart_scripts WHERE entryorguid=9883100 AND source_type=9 AND id=5 AND action_type=11 AND action_param1=200038;
SELECT 'jailer-blackout',COUNT(*) FROM smart_scripts WHERE entryorguid=101460 AND source_type=9 AND id=0 AND event_param1=12000 AND action_type=85 AND action_param1=199362;
SELECT 'native-transfer',COUNT(*) FROM spell_linked_spell s JOIN spell_target_position p ON p.id=s.spell_effect WHERE s.spell_trigger=199362 AND s.spell_effect=199359 AND p.target_map=1522;
UPDATE smart_scripts SET action_param1=99 WHERE entryorguid=9879701 AND source_type=9 AND id=13;
$migration
SELECT 'custom-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=9879701 AND source_type=9 AND id=13 AND action_param1=99;
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
$lines=$rows | Where-Object {$_ -match '^(opening-branches|fallback|once|existing-scripts-changed|capture-tail|jailer-blackout|native-transfer|custom-preserved)\s+[0-9]+$'}
$expected=@("opening-branches`t2","fallback`t1","once`t1","existing-scripts-changed`t0","capture-tail`t1","jailer-blackout`t1","native-transfer`t1","custom-preserved`t1")
if(($lines -join '|') -ne ($expected -join '|')){throw "Ritual migration mismatch: $($rows -join '|')"}
'PASS: repeatable opening and fallback repair, original scripts/custom replacements preserved, capture gated once, existing jailer/blackout/teleport chain intact.'

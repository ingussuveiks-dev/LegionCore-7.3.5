param([switch]$SkipMigration)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_03_new_blood_personal_stage.sql" -Raw
$fixture=(@('creature','conditions','smart_scripts') | ForEach-Object {
 "CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"
}) -join "`n"
if(-not $SkipMigration){$fixture+="`n$migration`n$migration`n"}
$fixture+=@"
SELECT 'static-portals',COUNT(*) FROM creature WHERE id=101433 AND map IN(1220,1502);
SELECT 'unphased-council',COUNT(*) FROM creature WHERE id IN(101456,98797,101454,101455,101457) AND map IN(1220,1502) AND PhaseId='';
SELECT 'waiting-council',COUNT(*) FROM creature WHERE id IN(101456,98797,101454,101455,101457) AND map IN(1220,1502) AND PhaseId='5436';
SELECT 'questgivers',COUNT(*) FROM creature WHERE id=101456 AND map IN(1220,1502) AND PhaseId='5436';
SELECT 'portal-count',COUNT(*) FROM smart_scripts WHERE entryorguid=98797 AND source_type=0 AND action_type=12 AND action_param1=101433;
SELECT 'persistent-portal',COUNT(*) FROM smart_scripts WHERE entryorguid=98797 AND source_type=0 AND id=2 AND action_type=12 AND action_param1=101433 AND action_param2=8 AND action_param3=0 AND link IN(3,4);
SELECT 'persistent-visual',COUNT(*) FROM smart_scripts WHERE action_type=50 AND action_param1=245958 AND action_param2=0 AND source_type=0 AND ((entryorguid=98797 AND id=3 AND link=4) OR (entryorguid=101433 AND id=6));
SELECT 'independent-reset',COUNT(*) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND id>=3;
SELECT 'click-gates',COUNT(*) FROM conditions WHERE ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=40729 AND NegativeCondition=0 AND
 ((SourceTypeOrReferenceId=18 AND SourceGroup=101433 AND SourceEntry=199966) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=101433));
SELECT 'phase-definition',COUNT(*) FROM phase_definitions WHERE zoneId=8392 AND entry=200 AND phaseId='5436';
SELECT 'other-spawns-changed',COUNT(*) FROM saved_creature a LEFT JOIN creature b USING(guid) WHERE (b.guid IS NULL OR a.PhaseId<>b.PhaseId) AND a.guid NOT IN (146813192,146813194,146813198,146813200,146890500,146890497,146890506,146890507,146890499,146813196,146813190,146890511);
SELECT JSON_OBJECT('kind','phase','group',ElseGroup,'type',ConditionTypeOrReference,'quest',ConditionValue1,'negative',NegativeCondition) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=8392 AND SourceEntry=200;
SELECT JSON_OBJECT('kind','click','id',id,'link',link,'event',event_type,'flags',event_flags,'action',action_type,'p1',action_param1,'p2',action_param2,'target',target_type,'entry',target_param1,'range',target_param2) FROM smart_scripts WHERE entryorguid=101433 AND source_type=0 ORDER BY id;
SELECT JSON_OBJECT('kind','timer','entry',entryorguid,'id',id,'delay',event_param1,'action',action_type,'arg',action_param1) FROM smart_scripts WHERE source_type=9 AND entryorguid IN(10145600,9879701,9883100,101460) ORDER BY entryorguid,id;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=$fixture | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names
 if($LASTEXITCODE){throw 'SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$counts=@($rows | Where-Object {$_ -notmatch '^\{'})
$expected=@('static-portals:0','unphased-council:0','waiting-council:10','questgivers:2','portal-count:1','persistent-portal:1','persistent-visual:1','independent-reset:0','click-gates:2','phase-definition:1','other-spawns-changed:0') | ForEach-Object {$_.Replace(':',"`t")}
if(($counts -join '|') -ne ($expected -join '|')){throw "Stage regression: $($counts -join '|')"}
$data=@($rows | Where-Object {$_.StartsWith('{')} | ForEach-Object {$_ | ConvertFrom-Json})
# Evaluate the actual existing phase conditions through accept/abandon/reaccept.
# A second player's unchanged status must keep their waiting council visible.
function Test-WaitingPhase([bool]$active,[bool]$previousRewarded) {
 foreach($group in ($data | Where-Object {$_.kind -eq 'phase'} | Group-Object group)) {
  $pass=$true
  foreach($condition in $group.Group) {
   $ok=switch([int]$condition.type) {
    9 {if($condition.quest -eq 40729){$active}else{$false}}
    8 {$condition.quest -eq 40716 -and $previousRewarded}
    28 {$false}
    default {throw 'Unexpected phase condition'}
   }
   if($condition.negative){$ok=-not $ok}
   $pass=$pass -and $ok
  }
  if($pass){return $true}
 }
 return $false
}
foreach($active in @($false,$true,$false,$true)) {
 if((Test-WaitingPhase $active $true) -eq $active){throw 'Waiting council does not follow quest state'}
 if(-not (Test-WaitingPhase $false $true)){throw 'Other player lost their questgiver'}
}
$click=@($data | Where-Object {$_.kind -eq 'click'})
if($click.Count -notin @(6,9) -or $click[0].event -ne 73 -or -not ($click[0].flags -band 1)){throw 'Portal must start exactly once'}
$actors=@(101457,101455,101454,101456,98797)
for($i=0;$i -lt 5;$i++) {
 if($click[$i].id -ne $i -or $click[$i].link -ne ($i+1) -or $click[$i].action -ne 45 -or $click[$i].target -ne 19 -or $click[$i].entry -ne $actors[$i]){throw 'Broken portal council signal chain'}
}
if($click[4].p1 -ne 3 -or $click[4].p2 -ne 3 -or $click[5].action -ne 81 -or $click[5].p1 -ne 0){throw 'Portal does not start ritual then disable repeat clicks'}
# Delayed clicks used to lose their clickable portal at t=155 seconds after
# accepting. Even immediate clicks near that deadline lost it during casting.
$timers=@($data | Where-Object {$_.kind -eq 'timer'})
$intro=@($timers | Where-Object {$_.entry -eq 10145600})
if($intro.Count -ne 3 -or ($intro | Measure-Object delay -Sum).Sum -ne 15000){throw 'Intro dialogue changed'}
if(-not ($timers | Where-Object {$_.entry -eq 9879701 -and $_.action -eq 12 -and $_.arg -eq 98831})){throw 'Ritual cannot summon Jagganoth'}
if(-not ($timers | Where-Object {$_.entry -eq 9883100 -and $_.arg -eq 200038})){throw 'Capture lost jailer forcecast'}
if(-not ($timers | Where-Object {$_.entry -eq 101460 -and $_.arg -eq 199362})){throw 'Capture lost native teleport blackout'}
'PASS: duplicate static actors removed/phased, two questgivers retained, accept/abandon/reaccept and player isolation, persistent portal, once-only click chain, native ritual/capture continuation, migration idempotence and unrelated spawn preservation.'

$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_02_new_blood_knockback_reset.sql" -Raw
# Temporary copies shadow production tables for this connection only.
$fixture=(@('conditions','smart_scripts') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=200011;
DELETE FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND id>=3;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_chance,event_param1,event_param2,action_type,action_param1,action_param2,target_type,comment)
 VALUES(10145600,9,3,100,120000,120000,80,10145601,2,1,'Original nested reset');
SELECT JSON_OBJECT('kind','old','id',id,'delay',event_param1,'action',action_type,'arg',action_param1,'target',target_type) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 ORDER BY id;
$migration
$migration
SELECT 'condition-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=200011;
SELECT 'reset-count',COUNT(*) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9;
SELECT 'nested-self-call',COUNT(*) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND action_type=80 AND target_type=1;
SELECT 'invalid-template',COUNT(*) FROM conditions c LEFT JOIN creature_template t ON t.entry=c.ConditionValue2 WHERE c.SourceTypeOrReferenceId=13 AND c.SourceEntry=200011 AND c.ConditionTypeOrReference=31 AND c.ConditionValue1=3 AND t.entry IS NULL;
SELECT JSON_OBJECT('kind','condition','group',ElseGroup,'type',ConditionTypeOrReference,'target',ConditionTarget,'v1',ConditionValue1,'v2',ConditionValue2,'negative',NegativeCondition) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=200011 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT JSON_OBJECT('kind','new','id',id,'delay',event_param1,'action',action_type,'arg',action_param1,'target',target_type) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 ORDER BY id;
SELECT JSON_OBJECT('kind','tail','id',id,'delay',event_param1,'action',action_type,'arg',action_param1,'target',target_type) FROM smart_scripts WHERE entryorguid=10145601 AND source_type=9 ORDER BY id;
-- Do not replace a site's unrelated customization at the old call site.
DELETE FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND id>=4;
UPDATE smart_scripts SET action_type=80,action_param1=999999,event_param1=120000,event_param2=120000 WHERE entryorguid=10145600 AND source_type=9 AND id=3;
$migration
SELECT 'custom-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND id=3 AND action_param1=999999;
SELECT 'custom-tail',COUNT(*) FROM smart_scripts WHERE entryorguid=10145600 AND source_type=9 AND id>=4;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=$fixture | & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names
 if($LASTEXITCODE){throw 'New Blood knockback/reset SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$counts=@($rows | Where-Object {$_ -notmatch '^\{'})
$expected=@("condition-count`t8","reset-count`t7","nested-self-call`t0","invalid-template`t0","custom-preserved`t1","custom-tail`t0")
if(($counts -join '|') -ne ($expected -join '|')){throw "SQL fixture mismatch: $($counts -join '|')"}
$data=@($rows | Where-Object {$_.StartsWith('{')} | ForEach-Object {$_ | ConvertFrom-Json})
$conditions=@($data | Where-Object {$_.kind -eq 'condition'})
function Test-Target($map,$type,$entry,$quest) {
 foreach($group in ($conditions | Group-Object group)) {
  $pass=$true
  foreach($c in $group.Group) {
   $ok=switch([int]$c.type) {
    22 {if($c.target -ne 1){throw 'Map must be tested on caster'}; $map -eq $c.v1}
    31 {if($c.target -ne 0){throw 'Entry must be tested on candidate'}; $type -eq $c.v1 -and ($c.v2 -eq 0 -or $entry -eq $c.v2)}
    9 {if($c.target -ne 0 -or $c.v1 -ne 40729){throw 'Unexpected quest gate'}; $type -eq 4 -and $quest}
    default {throw "Unexpected condition type $($c.type)"}
   }
   if($c.negative){$ok=-not $ok}
   if(-not $ok){$pass=$false}
  }
  if($pass){return $true}
 }
 return $false
}
foreach($map in @(1220,1502)) {
 foreach($entry in @(101456,101454,101455,101457)) {
  if(-not (Test-Target $map 3 $entry $false)){throw "Council rejected: $map/$entry"}
 }
 foreach($entry in @(101433,98797,98831,101460,416,12345)) {
  if(Test-Target $map 3 $entry $false){throw "Nonparticipant hit: $map/$entry"}
 }
 if(-not (Test-Target $map 4 0 $true)){throw 'Quest player rejected'}
 if(Test-Target $map 4 0 $false){throw 'Bystander player hit'}
 if(Test-Target $map 5 245958 $false){throw 'Portal object hit'}
}
# The second use of 200011 (Calydus escape) and other maps retain no extra filter.
foreach($map in @(1522,0)) {
 foreach($entry in @(99178,99145,99147,98797,101433)) {
  if(-not (Test-Target $map 3 $entry $false)){throw "Other scene changed: $map/$entry"}
 }
 if(-not (Test-Target $map 4 0 $false)){throw 'Other scene player changed'}
}
$old=@($data | Where-Object {$_.kind -eq 'old'})
$tail=@($data | Where-Object {$_.kind -eq 'tail'})
$new=@($data | Where-Object {$_.kind -eq 'new'})
if($old[-1].action -ne 80 -or $old[-1].target -ne 1){throw 'Original self-replacement regression not reproduced'}
# Compare the intended nested timeline with the flattened executable timeline.
# This is a data/timing check, not an in-game SmartAI simulation.
$wanted=@(); $time=0
foreach($e in $old) {
 $time+=$e.delay
 if($e.action -eq 80) {
  foreach($t in $tail){$time+=$t.delay; $wanted+="$time/$($t.action)/$($t.arg)/$($t.target)"}
 } else {$wanted+="$time/$($e.action)/$($e.arg)/$($e.target)"}
}
$actual=@(); $time=0
foreach($e in $new){$time+=$e.delay; $actual+="$time/$($e.action)/$($e.arg)/$($e.target)"}
if(($actual -join '|') -ne ($wanted -join '|')){throw "Reset timeline changed: $($actual -join '|') vs $($wanted -join '|')"}
'PASS: migration twice, Dalaran target matrix, portal/Zelifrax excluded, later escape preserved, no nested self-call, original recovery timing retained, custom call preserved.'

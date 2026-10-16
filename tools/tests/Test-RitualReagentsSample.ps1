$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_16_12_ritual_reagents_sample.sql" -Raw
$snapshot=@'
SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition
FROM conditions WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=19586 AND SourceEntry IN(0,1))
 OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106581 AND SourceId=0)
ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=19586 AND ConditionTypeOrReference=2 AND ConditionValue1=136986)
 OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106581 AND SourceId=0);
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'OTHER',COUNT(*) FROM saved_conditions s LEFT JOIN conditions c
 ON c.SourceTypeOrReferenceId=s.SourceTypeOrReferenceId AND c.SourceGroup=s.SourceGroup AND c.SourceEntry=s.SourceEntry AND c.SourceId=s.SourceId AND c.ElseGroup=s.ElseGroup AND c.ConditionTypeOrReference=s.ConditionTypeOrReference AND c.ConditionTarget=s.ConditionTarget AND c.ConditionValue1=s.ConditionValue1 AND c.ConditionValue2=s.ConditionValue2 AND c.ConditionValue3=s.ConditionValue3 AND c.NegativeCondition=s.NegativeCondition
 WHERE NOT ((s.SourceTypeOrReferenceId=15 AND s.SourceGroup=19586 AND s.SourceEntry IN(0,1) AND s.ConditionTypeOrReference=2 AND s.ConditionValue1=136986) OR (s.SourceTypeOrReferenceId=22 AND s.SourceGroup=1 AND s.SourceEntry=106581 AND s.SourceId=0)) AND c.SourceTypeOrReferenceId IS NULL;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Sample SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$end=[Array]::IndexOf($rows,'END')
if(($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($end-1)] -join "`n")){throw 'Sample migration is not idempotent'}
$rules=@($rows[($a+1)..($b-1)] | ForEach-Object { ,($_.Split("`t") | ForEach-Object {[int]$_}) })
if($rules.Count -ne 6 -or $rows -notcontains "OTHER`t0"){throw 'Unexpected scope or lost original quest gates'}
# Exercise the installed AND conditions for both gossip choices and the action.
# The native CONDITION_ITEM uses HasItemCount(value1,value2,value3!=0).
foreach($state in @('none','incomplete','complete','rewarded')) {
 foreach($bag in @(0,1,3)) { foreach($bank in @(0,1)) {
  foreach($route in @(0,1,2)) {
   $selected=@($rules | Where-Object {if($route -eq 2){$_[0] -eq 22}else{$_[0] -eq 15 -and $_[2] -eq $route}})
   $allowed=$true
   foreach($r in $selected) {
    if($r[4] -ne 0){throw 'Sample guard is not evaluated against player invoker'}
    switch($r[3]) {
     2 {if($r[5] -ne 136986 -or $r[6] -ne 1 -or $r[7] -ne 1 -or $r[8] -ne 1){throw 'Sample count/bank/negative guard changed'}; $pass=($bag+$bank -ge 1)}
     9 {if($r[5] -ne 42128){throw 'Wrong quest'}; $pass=($state -eq 'incomplete')}
     default {throw 'Unexpected condition type'}
    }
    if($r[8]){$pass=!$pass};$allowed=$allowed -and $pass
   }
   $expected=($state -eq 'incomplete' -and $bag+$bank -eq 0)
   if($allowed -ne $expected){throw "Admission mismatch: $state bag=$bag bank=$bank route=$route"}
  }
 } }
}
'PASS: both gossip choices and grant guard; 72 quest/inventory/bank cases, repeated migration and unrelated conditions preserved. Client interaction pending.'

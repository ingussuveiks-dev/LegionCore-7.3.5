$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_00_dreadscar_claim.sql" -Raw
$tables=@('conditions','smart_scripts','quest_template_addon','gossip_menu_option')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'intro',COUNT(*) FROM quest_template_addon WHERE ID=40731 AND PrevQuestID=40729;
SELECT 'demo-lock',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=42125 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(42125,40712,43254);
SELECT 'pet-kill',COUNT(*) FROM smart_scripts WHERE entryorguid=101468 AND source_type=0 AND id=4 AND link=7 AND action_type=205 AND action_param1=92 AND action_param2=48186 AND target_type=18 AND target_param1=100;
SELECT 'heart-grant',COUNT(*) FROM event_scripts WHERE id=48173 AND command=17 AND datalong=132880 AND datalong2=1;
SELECT 'reentry',COUNT(*) FROM smart_scripts a JOIN smart_scripts b ON b.entryorguid=a.entryorguid AND b.source_type=a.source_type AND b.id=a.link WHERE a.entryorguid=101097 AND a.source_type=0 AND a.id=5 AND a.event_type=62 AND a.event_param1=19326 AND a.event_param2=2 AND a.action_type=204 AND a.action_param1=998 AND b.action_type=85 AND b.action_param1=200172;
SELECT 'recovery',COUNT(*) FROM smart_scripts WHERE entryorguid=101470 AND source_type=0 AND id=2 AND event_type=10 AND action_type=205 AND action_param1=36 AND action_param2=132880 AND action_param3=1 AND target_type=7;
SELECT 'final-events',COUNT(*) FROM smart_scripts WHERE entryorguid=246148 AND action_type=205 AND action_param1=92 AND action_param2 IN(48182,48187);
SELECT 'transfer',COUNT(*) FROM spell_target_position WHERE id=201205 AND target_map=1107;
SELECT JSON_OBJECT('type',SourceTypeOrReferenceId,'grp',SourceGroup,'entry',SourceEntry,'source',SourceId,'branch',ElseGroup,'cond',ConditionTypeOrReference,'target',ConditionTarget,'v1',ConditionValue1,'v2',ConditionValue2,'negative',NegativeCondition) FROM conditions WHERE (SourceTypeOrReferenceId=19 AND SourceEntry=40731) OR (SourceTypeOrReferenceId=18 AND SourceGroup IN(101468,101470)) OR (SourceTypeOrReferenceId=22 AND ((SourceEntry=246148 AND SourceGroup=1 AND SourceId=1) OR (SourceEntry=101097 AND SourceGroup=6) OR (SourceEntry=101470 AND SourceGroup=3))) OR (SourceTypeOrReferenceId=15 AND SourceGroup=19326 AND SourceEntry=2);
"@
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1
$c=$config.Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=$fixture | & $mysql --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names
 if($LASTEXITCODE){throw 'Dreadscar SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$expected=@("intro`t1","demo-lock`t0","pet-kill`t1","heart-grant`t1","reentry`t1","recovery`t1","final-events`t2","transfer`t1")
if((@($rows | Where-Object {!$_.StartsWith('{')}) -join '|') -ne ($expected -join '|')){throw "Data assertions failed: $($rows -join '|')"}
$conditions=@($rows | Where-Object {$_.StartsWith('{')} | ForEach-Object {$_ | ConvertFrom-Json})
function Meets($rules,$state) {
 foreach($branch in ($rules | Group-Object branch)) {
  $pass=$true
  foreach($r in $branch.Group) {
   $value=switch([int]$r.cond) {
    2 {$state.heart -ge $r.v2}
    8 {$state.rewarded -contains $r.v1}
    9 {$state.active -eq $r.v1}
    22 {$state.map -eq $r.v1}
    36 {$state.bossAlive}
    44 {$state.scenario -eq $r.v1 -and $state.step -eq $r.v2}
    default {throw "Unsupported fixture condition $($r.cond)"}
   }
   if($r.negative){$value=-not $value}
   if(-not $value){$pass=$false}
  }
  if($pass){return $true}
 }
 return $false
}
function Check($actual,$expected,$label){if($actual -ne $expected){throw "Failed: $label"}}
$accept=@($conditions | Where-Object type -eq 19)
foreach($artifact in @(40712,41156,42125,43254)) {Check (Meets $accept @{rewarded=@($artifact)}) $true "artifact return $artifact"}
Check (Meets $accept @{rewarded=@(40716,40729)}) $false 'intro alone must not offer claim'
$altar=@($conditions | Where-Object {$_.type -eq 22 -and $_.entry -eq 246148})
$corpse=@($conditions | Where-Object {$_.type -eq 18 -and $_.grp -eq 101468})
$heart=@($conditions | Where-Object {$_.type -eq 18 -and $_.grp -eq 101470})
$recovery=@($conditions | Where-Object {$_.type -eq 22 -and $_.entry -eq 101470})
foreach($active in @(0,40729,40731)) {foreach($scenario in @(962,998)) {foreach($step in @(0,1,2)) {foreach($count in @(0,1)) {
 $s=@{active=$active;scenario=$scenario;step=$step;heart=$count;bossAlive=$true}
 $valid=$active -eq 40731 -and $scenario -eq 998
 Check (Meets $altar $s) ($valid -and $step -eq 2 -and $count -eq 1) 'altar cannot skip boss or heart'
 Check (Meets $corpse $s) $false 'living Jagganoth cannot give heart'
 $s.bossAlive=$false
 Check (Meets $corpse $s) ($valid -and $step -eq 1) 'corpse collection stage'
 Check (Meets $heart $s) ($valid -and $step -eq 1) 'heart remains retryable after full bags'
 Check (Meets $recovery $s) ($valid -and $step -eq 1 -and $count -eq 1) 're-entry restores only an owned heart'
}}}}
foreach($type in @(15,22)) {
 $returnRules=@($conditions | Where-Object {$_.type -eq $type -and (($_.type -eq 15) -or ($_.entry -eq 101097))})
 foreach($map in @(1220,1502,1522,1107)) {foreach($active in @(0,40729,40731)) {
  Check (Meets $returnRules @{map=$map;active=$active}) ($active -eq 40731 -and $map -in @(1220,1502)) 'return option and handler scope'
 }}
}
'PASS: Dreadscar migration idempotence, all artifact alternatives, pet kill target, stage/heart gates, inventory recovery, return restrictions, and final transfer chain.'

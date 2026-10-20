param([switch]$Baseline, [switch]$QuestTurnIn, [switch]$RaidFragments, [switch]$TrialCriteria, [switch]$TrialTurnIn, [switch]$TrialInstance, [switch]$TrialExit, [switch]$TrialChest, [switch]$TrialReentry, [switch]$TrialGarm, [switch]$TrialHelya, [switch]$TrialHelyaPhase3, [switch]$TrialHelyaPhase1, [switch]$TrialHelyaTaint, [switch]$TrialHelyaGrimelord, [switch]$TrialHelyaMariner, [string]$AdminUser='root')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$connection=[regex]::Match($config,'(?m)^CharacterDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
if($connection.Count -ne 5 -or $connection[4] -notmatch '^\w+$'){throw 'Unexpected character connection/database format'}
$nonce=[guid]::NewGuid().ToString('N').Substring(0,16)
$scratch='codex_roll_lifecycle_'+$nonce
$authScratch='codex_roll_auth_'+$nonce
$scratchUser='codex_roll_'+$nonce;$scratchPassword=[guid]::NewGuid().ToString('N')
$original=$connection[4];$previous=$env:MYSQL_PWD
$authConnection=[regex]::Match($config,'(?m)^LoginDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
if($authConnection.Count -ne 5 -or $authConnection[4] -notmatch '^\w+$' -or $authConnection[0] -ne $connection[0] -or $authConnection[1] -ne $connection[1]){throw 'Scratch lifecycle requires local auth/character schemas on one server'}
function Invoke-ScratchSql([string]$sql){
 $result=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$AdminUser --batch --raw --skip-column-names --execute=$sql
 if($LASTEXITCODE){throw 'Scratch database operation failed'}
 return $result
}
$created=@();$userCreated=$false;$tableCount=0
try{
 $env:MYSQL_PWD=$env:CODEX_MYSQL_ADMIN_PASSWORD
 Invoke-ScratchSql ("CREATE USER '$scratchUser'@'localhost' IDENTIFIED BY '$scratchPassword'") | Out-Null;$userCreated=$true
 # Copy schema only. Native character persistence runs against this disposable
 # database; no personal rows are copied and no live table receives a write.
 foreach($pair in @(@($scratch,$original),@($authScratch,$authConnection[4]))){
  $destination=$pair[0];$source=$pair[1]
  $tables=@(Invoke-ScratchSql ("SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA='$source' AND TABLE_TYPE='BASE TABLE' ORDER BY TABLE_NAME"))
  if(!$tables.Count -or @($tables | Where-Object {$_ -notmatch '^\w+$'}).Count){throw 'Unexpected source table catalog'}
  Invoke-ScratchSql ('CREATE DATABASE `{0}` CHARACTER SET utf8mb4' -f $destination) | Out-Null;$created+=$destination
  Invoke-ScratchSql ('GRANT ALL PRIVILEGES ON `{0}`.* TO ''{1}''@''localhost''' -f $destination,$scratchUser) | Out-Null
  for($i=0;$i -lt $tables.Count;$i+=25){
   $sql=($tables[$i..([Math]::Min($i+24,$tables.Count-1))] | ForEach-Object {'CREATE TABLE `{0}`.`{1}` LIKE `{2}`.`{1}`;' -f $destination,$_,$source}) -join "`n"
   Invoke-ScratchSql $sql | Out-Null
  }
  $tableCount+=$tables.Count
 }
 $mode=if($TrialHelyaMariner){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true;TrialHelya=$true;TrialHelyaMariner=$true}}elseif($TrialHelyaGrimelord){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true;TrialHelya=$true;TrialHelyaGrimelord=$true}}elseif($TrialHelyaTaint){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true;TrialHelya=$true;TrialHelyaTaint=$true}}elseif($TrialHelyaPhase1){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true;TrialHelya=$true;TrialHelyaPhase1=$true}}elseif($TrialHelyaPhase3){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true;TrialHelya=$true;TrialHelyaPhase3=$true}}elseif($TrialHelya){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true;TrialHelya=$true}}elseif($TrialGarm){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true;TrialGarm=$true}}elseif($TrialReentry){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialReentry=$true}}elseif($TrialChest){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialChest=$true}}elseif($TrialExit){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true;TrialExit=$true}}elseif($TrialInstance){@{TrialCriteria=$true;TrialTurnIn=$true;TrialInstance=$true}}elseif($TrialTurnIn){@{TrialCriteria=$true;TrialTurnIn=$true}}elseif($TrialCriteria){@{TrialCriteria=$true}}elseif($RaidFragments){@{RaidFragments=$true}}elseif($QuestTurnIn){@{QuestTurnIn=$true}}else{@{GroupLifecycle=$true}}
 & "$PSScriptRoot/Test-RockawayChannelNative.ps1" @mode -CharacterScratch $scratch -AuthScratch $authScratch -ScratchUser $scratchUser -ScratchPassword $scratchPassword -Baseline:$Baseline
}finally{
 try{
  foreach($database in $created){
   if($database -notmatch '^codex_roll_(lifecycle|auth)_[a-f0-9]{16}$' -or $database -eq $original -or $database -eq $authConnection[4]){throw 'Refusing unexpected scratch cleanup target'}
   Invoke-ScratchSql ('DROP DATABASE `{0}`' -f $database) | Out-Null
  }
  if($created.Count){"PASS: schema-only character/auth scratch databases ($tableCount tables) removed; no live character/account rows copied or modified."}
 }finally{
  try{if($userCreated){Invoke-ScratchSql ("DROP USER '$scratchUser'@'localhost'") | Out-Null}}
  finally{$env:MYSQL_PWD=$previous}
 }
}

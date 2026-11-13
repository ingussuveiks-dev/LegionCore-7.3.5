param([string]$AdminUser='root',[switch]$PendingBaseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/pet-logout-2026-11-13/save-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory("$out/logs")
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw;$connection=[regex]::Match($config,'(?m)^CharacterDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';');$auth=[regex]::Match($config,'(?m)^LoginDatabaseInfo\s*=\s*"([^"]+)"').Groups[1].Value.Split(';')
 if($connection.Count -ne 5 -or $auth.Count -ne 5 -or $connection[4] -notmatch '^\w+$' -or $auth[4] -notmatch '^\w+$' -or $auth[0] -ne $connection[0] -or $auth[1] -ne $connection[1]){throw 'Unexpected local scratch connection format'}
 $nonce=[guid]::NewGuid().ToString('N').Substring(0,16);$characterScratch='codex_pet_logout_'+$nonce;$authScratch='codex_pet_auth_'+$nonce;$scratchUser='codex_pet_'+$nonce;$scratchPassword=[guid]::NewGuid().ToString('N');$created=@();$userCreated=$false
 $oldPassword=$env:MYSQL_PWD;$oldCompiler=$env:CL;$oldConfig=$env:LEGION_DIALOGUE_CONFIG
 function Sql([string]$sql){$result=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$AdminUser --batch --raw --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Scratch schema operation failed'};return $result}
 try {
  $env:MYSQL_PWD=$env:CODEX_MYSQL_ADMIN_PASSWORD;Sql "CREATE USER '$scratchUser'@'localhost' IDENTIFIED BY '$scratchPassword'"|Out-Null;$userCreated=$true
  foreach($pair in @(@($characterScratch,$connection[4]),@($authScratch,$auth[4]))){$destination=$pair[0];$source=$pair[1];$tables=@(Sql "SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA='$source' AND TABLE_TYPE='BASE TABLE' ORDER BY TABLE_NAME");if(!$tables.Count -or @($tables|Where-Object {$_ -notmatch '^\w+$'}).Count){throw 'Unexpected source schema'};Sql ('CREATE DATABASE `{0}` CHARACTER SET utf8mb4' -f $destination)|Out-Null;$created+=$destination;Sql ('GRANT ALL PRIVILEGES ON `{0}`.* TO ''{1}''@''localhost''' -f $destination,$scratchUser)|Out-Null
   for($i=0;$i -lt $tables.Count;$i+=25){Sql (($tables[$i..([Math]::Min($i+24,$tables.Count-1))]|ForEach-Object {'CREATE TABLE `{0}`.`{1}` LIKE `{2}`.`{1}`;' -f $destination,$_,$source})-join "`n")|Out-Null}
  }
  $characterNew=@($connection);$characterNew[2]=$scratchUser;$characterNew[3]=$scratchPassword;$characterNew[4]=$characterScratch;$authNew=@($auth);$authNew[2]=$scratchUser;$authNew[3]=$scratchPassword;$authNew[4]=$authScratch
  $config=[regex]::Replace($config,'(?m)^CharacterDatabaseInfo\s*=.*$',('CharacterDatabaseInfo = "'+($characterNew-join ';')+'"'));$config=[regex]::Replace($config,'(?m)^LoginDatabaseInfo\s*=.*$',('LoginDatabaseInfo = "'+($authNew-join ';')+'"'));$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));$config+="`nPetLogoutScratchCharacter = `"$characterScratch`"`nPetLogoutScratchAuth = `"$authScratch`"`n";[IO.File]::WriteAllText("$out/worldserver.conf",$config)
  & "$PSScriptRoot/Prepare-PetBattleOwnedNative.ps1" -Out $out;$env:CL="$oldCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`"";$env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
  $source='../tests/PetBattleLogoutSaveNative.cpp';$probe='PetBattleLogoutSaveNative'
  if($PendingBaseline){
   $raw=Get-Content "$repo/src/server/game/PetBattle/PetBattleSystem.cpp" -Raw
   $pattern='(?m)^        // SaveToDB uses the pending destination during a far transfer\. Keep\r?\n        // the real loading state, but save the same validated return pose\.\r?\n        if \(player->IsBeingTeleportedFar\(\)\)\r?\n            player->GetTeleportDest\(\)\.WorldRelocate\(player->m_recallLoc\);\r?\n'
   if([regex]::Matches($raw,$pattern).Count -ne 1){throw 'Exact pending-save counterfactual projection missing'}
   [IO.File]::WriteAllText("$out/CounterfactualSystem.cpp",[regex]::Replace($raw,$pattern,''))
   [IO.File]::WriteAllText("$out/Counterfactual.cpp",('#include "'+$repo.Replace('\','/')+'/src/server/game/PrecompiledHeaders/gamePCH.h"'+"`n"+'#include "'+$out.Replace('\','/')+'/CounterfactualSystem.cpp"'+"`n"+'#include "'+$repo.Replace('\','/')+'/tools/tests/PetBattleLogoutSaveNative.cpp"'+"`n"))
   $source='../../'+[IO.Path]::GetRelativePath($repo,"$out/Counterfactual.cpp").Replace('\','/');$probe='PetBattleLogoutSaveBaselineNative'
  }
  $failure=$null;try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $source -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($PendingBaseline){if(!$failure -or !$failure.Exception.Message.Contains('failed with exit 1') -or !(Get-Content "$out/build.log" -Raw).Contains('Actual logout must save matchmaking recall rather than arena pose')){Get-Content "$out/build.log" -Tail 18;throw 'Unexpected pending-save counterfactual failure'};'PASS expected-broken pending-save projection: actual logout/SaveToDB persists the old far-transfer destination;exit1;paired repaired24 run required'}
  else{if($failure){Get-Content "$out/build.log" -Tail 20;throw $failure};if(!(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual full-session logout/save cases=24;')){throw 'Scratch full logout/save did not complete'};Get-Content "$out/native.tsv" -Tail 1}
 }finally{
  try{foreach($database in $created){if($database -notmatch '^codex_pet_(logout|auth)_[a-f0-9]{16}$' -or $database -eq $connection[4] -or $database -eq $auth[4]){throw 'Refusing unexpected scratch cleanup target'};Sql ('DROP DATABASE `{0}`' -f $database)|Out-Null};if($created.Count){'PASS schema-only scratch databases removed;no personal rows copied or live character/account tables modified'}}
  finally{try{if($userCreated){Sql "DROP USER '$scratchUser'@'localhost'"|Out-Null}}finally{$env:MYSQL_PWD=$oldPassword;$env:CL=$oldCompiler;$env:LEGION_DIALOGUE_CONFIG=$oldConfig}}
 }
}
$global:LASTEXITCODE=0

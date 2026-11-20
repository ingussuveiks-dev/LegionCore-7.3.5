param([string]$Migration="$PSScriptRoot/../../sql/updates/world/2026_11_19_12_restore_legion_world_quest_rotation.sql",
 [switch]$Progression,[switch]$VerifyGarrisonPlacements,[string]$RuntimeConfig)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migrationPath=(Resolve-Path -LiteralPath $Migration).Path
$out="$repo/build-extractors/runtime-test/legion-world-quest-rotation/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
if($VerifyGarrisonPlacements -and !$Progression){throw 'Garrison placement verification requires -Progression'}
if(!$RuntimeConfig){$RuntimeConfig="$repo/build-extractors/bin/Release/worldserver.conf"}
$config=Get-Content -LiteralPath $RuntimeConfig -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
$previous=$env:LEGION_DIALOGUE_CONFIG
$previousMigration=$env:LEGION_ROTATION_MIGRATION
$previousRewardSchema=$env:LEGION_REWARD_SCHEMA
$compiler=$env:CL
$link=$env:_LINK_
$private="$out/character-client.cnf"
$isolatedClient="$out/isolated-client.cnf"
$schemaDump="$out/character-schema-only.sql"
$datadir="$out/character-db"
$testSchema=$null
$databaseProcess=$null
$databaseVerified=$false
$inputs=@($migrationPath,"$PSScriptRoot/LegionWorldQuestRotationNative.cpp","$PSScriptRoot/LegionAreaDialogueNative.cpp",
 "$repo/src/server/game/Globals/QuestData.cpp","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/World/World.cpp",
 "$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 if($Progression){
  $connection=[regex]::Match($config,'(?m)^\s*CharacterDatabaseInfo\s*=\s*"([^"]+)"')
  if(!$connection.Success){throw 'Missing character connection for isolated reward fixture'}
  $parts=$connection.Groups[1].Value.Split(';')
  if($parts.Count -ne 5){throw 'Unexpected character connection format'}
  $testSchema='codex_wq_reward_'+[Guid]::NewGuid().ToString('N')
  $mariaDirectory='C:/wamp64/bin/mariadb/mariadb11.4.9/bin'
  function QuoteClient([string]$value){'"'+$value.Replace('\','\\').Replace('"','\"')+'"'}
  $options="[client]`nhost=$(QuoteClient $parts[0])`nport=$($parts[1])`nuser=$(QuoteClient $parts[2])`npassword=$(QuoteClient $parts[3])`n"
  [IO.File]::WriteAllText($private,$options)
  & "$mariaDirectory/mariadb-dump.exe" "--defaults-extra-file=$private" --no-data --skip-triggers --skip-lock-tables --skip-add-drop-table "--result-file=$schemaDump" $parts[4]
  if($LASTEXITCODE){throw 'Could not copy empty character table definitions'}
  $listener=[Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback,0)
  try{$listener.Start();$testPort=$listener.LocalEndpoint.Port}finally{$listener.Stop()}
  if($testPort -eq [int]$parts[1]){throw 'Isolated database selected the live port'}
  $testPassword=[Guid]::NewGuid().ToString('N')
  [IO.File]::WriteAllText($isolatedClient,"[client]`nhost=127.0.0.1`nport=$testPort`nuser=root`npassword=$testPassword`nprotocol=tcp`n")
  & "$mariaDirectory/mariadb-install-db.exe" "--datadir=$datadir" "--password=$testPassword" --silent *> "$out/character-init.log"
  if($LASTEXITCODE){throw 'Could not initialize disposable MariaDB data directory'}
  $arguments=@('--no-defaults','--console','--bind-address=127.0.0.1',"--port=$testPort","--datadir=`"$datadir`"", "--pid-file=`"$out/character-db.pid`"",'--innodb-buffer-pool-size=64M')
  $databaseProcess=Start-Process -FilePath "$mariaDirectory/mariadbd.exe" -ArgumentList $arguments -WorkingDirectory $out -WindowStyle Hidden -PassThru `
   -RedirectStandardOutput "$out/character-db-stdout.log" -RedirectStandardError "$out/character-db-stderr.log"
  $deadline=[DateTime]::UtcNow.AddSeconds(20)
  while(!$databaseProcess.HasExited -and [DateTime]::UtcNow -lt $deadline){
   $path=& "$mariaDirectory/mariadb.exe" "--defaults-extra-file=$isolatedClient" --connect-timeout=1 --batch --raw --skip-column-names '--execute=SELECT @@datadir' 2>$null
   if(!$LASTEXITCODE){
    $actual=[IO.Path]::GetFullPath(($path|Select-Object -First 1)).TrimEnd('\','/')
    if($actual -ne [IO.Path]::GetFullPath($datadir)){throw 'Database listener belongs to a different data directory'}
    $databaseVerified=$true
    break
   }
   Start-Sleep -Milliseconds 100
  }
  if(!$databaseVerified){throw 'Could not verify the isolated MariaDB listener'}
  & "$mariaDirectory/mariadb.exe" "--defaults-extra-file=$isolatedClient" "--execute=CREATE DATABASE $testSchema"
  if($LASTEXITCODE){throw 'Could not create the isolated character schema'}
  Get-Content -LiteralPath $schemaDump -Raw | & "$mariaDirectory/mariadb.exe" "--defaults-extra-file=$isolatedClient" "--database=$testSchema"
  if($LASTEXITCODE){throw 'Could not initialize the isolated character schema'}
  $isolated=@('127.0.0.1',$testPort,'root',$testPassword,$testSchema)
  $line='CharacterDatabaseInfo = "'+($isolated -join ';')+'"'
  $config=$config.Substring(0,$connection.Index)+$line+$config.Substring($connection.Index+$connection.Length)
  $env:LEGION_REWARD_SCHEMA=$testSchema
 }
 [IO.File]::WriteAllText("$out/worldserver.conf",$config)
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:LEGION_ROTATION_MIGRATION=$migrationPath
 $env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 if($Progression){$env:CL+=' /DLEGION_ROTATION_PROGRESSION'}
 if($VerifyGarrisonPlacements){$env:CL+=' /DLEGION_ROTATION_GARRISON_PLACEMENTS'}
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionWorldQuestRotationNative.cpp -ProbeName LegionWorldQuestRotationNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|ROTATION|PROGRESSION|GARRISON)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous
 $env:LEGION_ROTATION_MIGRATION=$previousMigration
 $env:LEGION_REWARD_SCHEMA=$previousRewardSchema
 $env:CL=$compiler
 $env:_LINK_=$link
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 try{
  if($databaseProcess){
   if(!$databaseProcess.HasExited -and $databaseVerified){
    & "$mariaDirectory/mariadb-admin.exe" "--defaults-extra-file=$isolatedClient" --connect-timeout=3 shutdown
   }
   if(!$databaseProcess.WaitForExit(10000)){
    $databaseProcess.Kill()
    $databaseProcess.WaitForExit()
    throw 'The owned MariaDB test process required forced shutdown; inspect captured diagnostics'
   }
   if($databaseProcess.ExitCode){throw "Owned MariaDB exited $($databaseProcess.ExitCode); inspect captured diagnostics"}
   if(Select-String -LiteralPath "$out/character-db-stdout.log","$out/character-db-stderr.log" -Pattern '\[(warning|error)\]' -Quiet){
    throw 'The isolated MariaDB listener logged a warning or error'
   }
   'PASS isolated MariaDB shutdown and exit 0'
  }
 }finally{
  foreach($file in @($private,$isolatedClient,$schemaDump)){if(Test-Path -LiteralPath $file){Remove-Item -LiteralPath $file -Force}}
 }
 foreach($item in $sealed){if((Get-FileHash -LiteralPath $item.Path).Hash -ne $item.Hash){throw 'Rotation verification input changed'}}
}
"WORLD_QUEST_ROTATION_OUTPUT $out"

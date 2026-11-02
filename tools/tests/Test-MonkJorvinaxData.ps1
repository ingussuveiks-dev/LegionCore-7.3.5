param([switch]$VerifyNativeCatalog,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-jorvinax-data"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $sql=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'boss',t.entry,t.AIName,t.ScriptName FROM creature_template t WHERE t.entry=98217;
SELECT 'spawn',guid,id,map,phaseId FROM creature WHERE id=98217 ORDER BY guid;
SELECT 'death',entryorguid,source_type,id,link,event_type,event_chance,event_flags,action_type,action_param1,action_param2,action_param3,action_param4,target_type FROM smart_scripts WHERE (entryorguid=98217 OR entryorguid=-362203) AND event_type=6 ORDER BY entryorguid,source_type,id;
SELECT 'condition',SourceTypeOrReferenceId,SourceGroup,SourceEntry FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(98217,-362203);
SELECT 'END-JORVINAX';
'@
 [IO.File]::WriteAllText("$out/check.sql",$sql)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $prior=$env:MYSQL_PWD
 Push-Location $out
 try{
  $env:MYSQL_PWD=$connection[3]
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/check.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
  if($code -or (Get-Item "$out/world.stderr").Length){throw 'Read-only Jorvinax data check failed; captured stderr retained'}
 }finally{$env:MYSQL_PWD=$prior;Pop-Location}
 $rows=Get-Content "$out/world.tsv"
 $expected=@("boss`t98217`tSmartAI`t","spawn`t362203`t98217`t1014`t7000","death`t98217`t0`t2`t0`t6`t100`t1`t205`t92`t46951`t0`t0`t7",'END-JORVINAX')
 if(($rows -join "`n") -cne ($expected -join "`n")){throw 'Native boss identity, actual death205 target7, condition scope or global spawn changed'}
 'PASS live read-only98217/362203 map1014 phase7000, unchanged native SmartAI DEATH205/92/46951 invoker7/runOnce; no added source22 gate'
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  Push-Location $out
  try{
   $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MonkFinalCatalog.cpp -ProbeName MonkFinalCatalog -OutputFile "$out/native.tsv"
   if(!$? -or $LASTEXITCODE -or !(Test-Path "$out/native.tsv")){throw 'Matching native final catalog failed; output retained'}
  }finally{$env:LEGION_DIALOGUE_CONFIG=$prior;Pop-Location}
  $native=Get-Content "$out/native.tsv"
  foreach($line in @("TREE`t1900`t45909`t45908`t0`t1`t29071","CRITERION`t1900`t29071`t92`t46951`t0`t0","TREE`t1901`t45911`t45910`t0`t1`t29072","CRITERION`t1901`t29072`t92`t46952`t0`t0","MAP_DIFFICULTY`t3448`t12`t1","COMPLETE`t3`t7")){
   if($native -notcontains $line){throw 'Native defeat/portal distinction, original scenario/private admission or catalog termination changed'}
  }
  'PASS matching native943 stage5/29071/46951 defeat before stage6/29072/46952 portal, seven scenario steps and private admission; native assets distinct from actual client traversal'
 }
}

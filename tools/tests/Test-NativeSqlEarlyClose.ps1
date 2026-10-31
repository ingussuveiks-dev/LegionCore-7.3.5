param([switch]$Baseline,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/native-sql-early-close$(if($Baseline){'-baseline'})"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$helper="$PSScriptRoot/Invoke-NativeSqlFiles.ps1"
if($Baseline){
 $old=& git -C $repo show ab99b2d2d:tools/tests/Invoke-NativeSqlFiles.ps1
 if($LASTEXITCODE){throw 'Pinned actual SQL helper missing'}
 $helper="$out/pinned-ab99b2d2d.ps1";[IO.File]::WriteAllText($helper,($old -join "`n"),[Text.UTF8Encoding]::new($false))
}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -RuntimeDirectories @($out) -Test {
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$c[3]
  # The expected1054 exits the native client while its stdin pipe still has
  # 8MiB to consume. Neither the trailing query nor a permanent write runs.
  $sql="SET SESSION TRANSACTION READ ONLY;SELECT 'BEFORE_EXPECTED_EARLY_EXIT';SELECT __codex_early_close_missing_column__;`n/*"+('x'*8388608)+"*/`nSELECT 'UNREACHED';`n"
  [IO.File]::WriteAllText("$out/input.sql",$sql,[Text.UTF8Encoding]::new($false))
  foreach($client in @('C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe','C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe')){
   $prefix=if($client -like '*mariadb*'){'mariadb'}else{'mysql'}
   foreach($index in 1..3){
    $thrown=$null;$code=$null
    try{$code=& $helper -Executable $client -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/input.sql" -OutputFile "$out/$prefix-$index.stdout" -ErrorFile "$out/$prefix-$index.stderr"}catch{$thrown=$_;[IO.File]::WriteAllText("$out/$prefix-$index.exception.txt",$_.Exception.ToString())}
    $stderr=[IO.File]::ReadAllText("$out/$prefix-$index.stderr")
    if(@([regex]::Matches($stderr,'ERROR 1054')).Count -ne 1 -or @([regex]::Matches($stderr,'ERROR [0-9]+')).Count -ne 1){throw 'Native early-exit SQL diagnostic missing/changed'}
    $rows=@(Get-Content "$out/$prefix-$index.stdout")
    if($rows.Count -ne 1 -or $rows[0] -cne 'BEFORE_EXPECTED_EARLY_EXIT'){throw 'Native pre-error stdout was lost/altered'}
    if($Baseline){
     if(!$thrown -or $thrown.Exception.GetBaseException() -isnot [IO.IOException]){throw 'Pinned helper did not reproduce its broken pipe masking the native exit'}
    }elseif($thrown -or $code -isnot [int] -or $code -eq 0){throw 'Helper did not return the actual native failing exit after draining both pipes'}
    "CASE $prefix/$index expected SQL1054/early stdin close PASS"
   }
  }
 }finally{$env:MYSQL_PWD=$previous}
 $(if($Baseline){'EXPECTED_BROKEN pinned ab99b2d2d:6 native early exits masked by stdin IO exceptions, original stdout/stderr retained'}else{'PASS6 native early exits: actual failing scalar exit and complete pre-error stdout/SQL1054 stderr retained; no database mutation'})
}

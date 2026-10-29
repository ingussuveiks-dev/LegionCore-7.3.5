$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/native-sql-files"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3]
  foreach($client in @('C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe','C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe')){
  $prefix=if($client -like '*mariadb*'){'mariadb'}else{'mysql'}
  foreach($case in @('clean','error-before-payload','tail-after-error','native-failure','metadata')){
   $payload="SELECT JSON_OBJECT('payload',REPEAT('x',1048576),'tail','EOF_SENTINEL');"
   $failure='SELECT __codex_capture_missing_column__;'
   $sql='SET SESSION TRANSACTION READ ONLY;'+$(switch($case){'clean'{$payload};'error-before-payload'{$failure+$payload};'tail-after-error'{$payload+$failure+"SELECT JSON_OBJECT('final','NATIVE_EOF');"};'native-failure'{$failure};'metadata'{"SELECT JSON_OBJECT('database',DATABASE(),'columns',(SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='playerchoice_response'));"}})
   $inputFile="$out/$prefix-$case.sql";$stdout="$out/$prefix-$case.stdout";$stderr="$out/$prefix-$case.stderr"
   [IO.File]::WriteAllText($inputFile,$sql,[Text.UTF8Encoding]::new($false))
   $arguments=@("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names')
   if($case -ne 'native-failure'){$arguments+='--force'}
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable $client -Arguments $arguments -InputFile $inputFile -OutputFile $stdout -ErrorFile $stderr
   if($code -isnot [int] -or ($case -eq 'native-failure' -and $code -eq 0) -or ($case -ne 'native-failure' -and $code -ne 0)){throw "Native SQL exit is not the actual scalar exit: $case/$code"}
   $errors=Get-Content $stderr -Raw -Encoding UTF8
   if($case -in @('clean','metadata')){if($errors){throw 'Unexpected clean stderr'}}elseif(@([regex]::Matches($errors,'ERROR 1054')).Count -ne 1 -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Expected SQL counterfactual lost or altered: $case"}
   $rows=@(Get-Content $stdout -Encoding UTF8)
   if($case -eq 'native-failure'){if($rows.Count){throw 'Unexpected output for failed native query'};continue}
   if($case -eq 'metadata'){$data=$rows[0]|ConvertFrom-Json;if($rows.Count -ne 1 -or $data.database -cne $c[4] -or $data.columns -lt 5){throw "Native database/metadata query incomplete: $prefix"};continue}
   $first=$rows[0]|ConvertFrom-Json
   if($first.payload.Length -ne 1048576 -or $first.payload -cne ('x'*1048576) -or $first.tail -cne 'EOF_SENTINEL'){throw "Native stdout payload truncated: $case"}
   if($case -eq 'tail-after-error'){if($rows.Count -ne 2 -or ($rows[1]|ConvertFrom-Json).final -cne 'NATIVE_EOF'){throw 'Final post-error native row lost'}}elseif($rows.Count -ne 1){throw 'Unexpected extra native output'}
  }
  }
 }finally{$env:MYSQL_PWD=$previous}
 'PASS native stdin/stdout/stderr EOF: complete1MiB clean/error-before/post-error tail, real scalar native failure exit and exactly retained expected SQL1054 counterfactuals; read-only queries, no database mutation'
}

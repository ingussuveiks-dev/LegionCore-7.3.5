param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/native-sql-prepared-refusal"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$c[3]
  # No tables or routines are created. Deliberate session-local prepared
  # SIGNAL is a counterfactual, followed by two complete native result sets.
  $sql="SET SESSION TRANSACTION READ ONLY;SELECT CONCAT('before-',seq,':',REPEAT('x',105)) FROM seq_1_to_36;`nSET @refusal='SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''expected prepared refusal''';PREPARE native_refusal FROM @refusal;EXECUTE native_refusal;`nDEALLOCATE PREPARE native_refusal;SELECT CONCAT('after-',seq,':',REPEAT('x',105)) FROM seq_1_to_36;SELECT '__NATIVE_FLUSH_COMPLETE__';`n"
  [IO.File]::WriteAllText("$out/input.sql",$sql,[Text.UTF8Encoding]::new($false))
  $expected=@(1..36|ForEach-Object {"before-$($_):"+('x'*105)})+@(1..36|ForEach-Object {"after-$($_):"+('x'*105)})+@('__NATIVE_FLUSH_COMPLETE__')
  foreach($client in @('C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe','C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe')){
   $prefix=if($client -like '*mariadb*'){'mariadb'}else{'mysql'}
   foreach($index in 1..40){
    $stdout="$out/$prefix-$index.stdout";$stderr="$out/$prefix-$index.stderr"
    $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable $client -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$out/input.sql" -OutputFile $stdout -ErrorFile $stderr
    $rows=@(Get-Content $stdout -Encoding UTF8);$errors=Get-Content $stderr -Raw -Encoding UTF8
    if($code -isnot [int] -or $code -ne 0 -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1 -or $errors -notmatch 'ERROR 1644 .*expected prepared refusal'){throw "Unexpected prepared counterfactual exit/stderr: $prefix/$index"}
    if($rows.Count -ne 73 -or ($rows -join "`n") -cne ($expected -join "`n")){throw "Incomplete native prepared-refusal result: $stdout"}
   }
  }
 }finally{$env:MYSQL_PWD=$previous}
 'PASS80 prepared-SIGNAL counterfactuals across MariaDB/MySQL clients: actual scalar0 exit, exactly one retained1644, all72 before/after rows byte-content checked and final EOF receipt present. Queries read-only; no persistent data writes.'
}

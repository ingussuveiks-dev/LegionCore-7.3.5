param([Parameter(Mandatory=$true)][string]$NativeOutput)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$sqlTaskDirectory="$repo/build-extractors/runtime-test/sixth-dump-sql/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $sqlTaskDirectory|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$sqlTaskDirectory/evidence" -RuntimeDirectories @($sqlTaskDirectory) -Test {
 $nativeText=[IO.File]::ReadAllText($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($NativeOutput))
 if(!$nativeText.Contains('PASS native AP field-width control')){throw 'A passing immutable native transform run is required'}
 $statements=@($nativeText -split "`r?`n"|Where-Object {$_ -like "SQL`t*"}|ForEach-Object {$_.Substring(4)})
 if($statements.Count -ne 4 -or @($statements|Where-Object {$_ -notmatch '^INSERT INTO `codex_sixth_(item|pet|declined|child)` VALUES \('}).Count){throw 'Unexpected generated synthetic SQL'}
 $sql=@'
CREATE TEMPORARY TABLE codex_sixth_item(id BIGINT UNSIGNED PRIMARY KEY);
CREATE TEMPORARY TABLE codex_sixth_pet(id INT UNSIGNED PRIMARY KEY,entry MEDIUMINT UNSIGNED,owner BIGINT UNSIGNED);
CREATE TEMPORARY TABLE codex_sixth_declined(id INT UNSIGNED PRIMARY KEY,owner BIGINT UNSIGNED,genitive VARCHAR(20));
CREATE TEMPORARY TABLE codex_sixth_child(id INT UNSIGNED,spell INT UNSIGNED);
'@+"`n"+($statements -join "`n")+"`n"+@'
SELECT CONCAT('ID=',id) FROM codex_sixth_item;
SELECT CONCAT('PET=',p.id,':',p.owner,':',d.owner,':',d.genitive,':',c.id) FROM codex_sixth_pet p JOIN codex_sixth_declined d ON p.id=d.id JOIN codex_sixth_child c ON c.id=p.id;
DROP TEMPORARY TABLE codex_sixth_item,codex_sixth_pet,codex_sixth_declined,codex_sixth_child;
'@
 [IO.File]::WriteAllText("$sqlTaskDirectory/statements.sql",$sql)
 $dbFields=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $savedAdminSecret=$env:CODEX_MYSQL_ADMIN_PASSWORD
 try{
  if($dbFields[2] -eq 'root'){$env:CODEX_MYSQL_ADMIN_PASSWORD=$dbFields[3]}
  & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$sqlTaskDirectory/reader-private" -Test {
   $sqlCheckExit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe' -Arguments @("--host=$($dbFields[0])","--port=$($dbFields[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($dbFields[4])",'--batch','--raw','--skip-column-names') -InputFile "$sqlTaskDirectory/statements.sql" -OutputFile "$sqlTaskDirectory/roundtrip.tsv" -ErrorFile "$sqlTaskDirectory/roundtrip.stderr"
   if($sqlCheckExit -or (Get-Item "$sqlTaskDirectory/roundtrip.stderr").Length){throw 'Restricted TEMP round trip failed'}
   $actual=@(Get-Content "$sqlTaskDirectory/roundtrip.tsv")
   if($actual.Count -ne 2 -or $actual[0] -ne 'ID=1099511627993' -or $actual[1] -ne "PET=2147483648:1099511627793:1099511627793:O'Brien:2147483648"){throw 'Native transform SQL readback mismatch'}
  }
 }finally{$env:CODEX_MYSQL_ADMIN_PASSWORD=$savedAdminSecret}
 "PASS native-generated64bit item/pet owner/name/child SQL in connection-local TEMP tables;SELECT+CREATE TEMP-only test user,denied persistent-DML canary,task user dropped;no actual character import/data writes ($sqlTaskDirectory)"
}

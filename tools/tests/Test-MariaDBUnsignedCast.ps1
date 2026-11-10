param([switch]$Baseline,[string]$NamedPipe='',[string]$ClientExecutable='C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/mariadb-unsigned-cast/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"}
$out=[IO.Path]::GetFullPath($OutputDirectory);[void][IO.Directory]::CreateDirectory($out)
if($NamedPipe -and !$NamedPipe.StartsWith('CodexCast')){throw 'This test only owns explicitly named CodexCast isolation pipes'}
$values=@('0','1','2147483647','2147483648','4294967295','4294967296','4294967297','5000000000','9999999999','10000000000','1099511627793','9223372036854775807','9223372036854775808','18446744073709551615')
$sql="SET SESSION TRANSACTION READ ONLY;`n"
foreach($value in $values){$sql+="SELECT 'VALUE', $value, CAST($value AS UNSIGNED), HEX(CAST($value AS UNSIGNED)), CAST($value AS UNSIGNED)=$value;`n"}
$sql+="SELECT 'STRING', CAST('4294967296' AS UNSIGNED), CAST('-1' AS UNSIGNED);`n"
$sql+="PREPARE cast_probe FROM 'SELECT ''PREPARED'', CAST(4294967296 AS UNSIGNED)';`nEXECUTE cast_probe;`nDEALLOCATE PREPARE cast_probe;`n"
if(!$Baseline){$sql+="SET SESSION TRANSACTION READ WRITE;`nCREATE TEMPORARY TABLE codex_cast_probe AS SELECT CAST(4294967296 AS UNSIGNED) AS value;`nSELECT 'TEMP',value FROM codex_cast_probe;`nSHOW COLUMNS FROM codex_cast_probe;`nDROP TEMPORARY TABLE codex_cast_probe;`nSET SESSION TRANSACTION READ ONLY;`n"}
[IO.File]::WriteAllText("$out/probe.sql",$sql)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $prior=$env:MYSQL_PWD
 try{
  if($NamedPipe){$args=@('--no-defaults','--protocol=PIPE',"--socket=$NamedPipe",'--user=root','--database=mysql');$env:MYSQL_PWD=$null}
  else{$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$args=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])");$env:MYSQL_PWD=$db[3]}
  $args+=@('--batch','--raw','--skip-column-names')
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable $ClientExecutable -Arguments $args -InputFile "$out/probe.sql" -OutputFile "$out/probe.tsv" -ErrorFile "$out/probe.stderr"
  if($code -or (Get-Item "$out/probe.stderr").Length){throw 'Unsigned CAST query failed'}
 }finally{$env:MYSQL_PWD=$prior}
 $rows=@(Get-Content "$out/probe.tsv"|ForEach-Object {,@($_.Split("`t"))});$numbers=@($rows|Where-Object {$_[0] -ceq 'VALUE'})
 if($numbers.Count -ne $values.Count){throw 'Incomplete CAST boundary results'}
 foreach($row in $numbers){
  if($row[4] -ne '1'){throw 'Numeric unsigned CAST equality failed'}
  if(!$Baseline -and $row[1] -ne $row[2]){throw "Unsigned result truncated at $($row[1])"}
 }
 $control=$numbers|Where-Object {$_[1] -eq '4294967296'}
 $stringControl=$rows|Where-Object {$_[0] -ceq 'STRING'}
 if($stringControl[1] -ne '4294967296' -or $stringControl[2] -ne '18446744073709551615'){throw 'String/negative unsigned conversion control changed'}
 $prepared=$rows|Where-Object {$_[0] -eq 'PREPARED'}
 if($Baseline){if($control[2] -ne '0' -or $control[3] -ne '100000000' -or $prepared[1] -ne '0'){throw 'Expected original metadata/result-width counterfactual missing'};'PASS expected-broken MariaDB: bare/prepared4294967296 serializes0 while numeric equality andHEX retain64bits'}
 else{
  if($prepared[1] -ne '4294967296'){throw 'Prepared result narrowed'}
  $temp=$rows|Where-Object {$_[0] -ceq 'TEMP'};$column=$rows|Where-Object {$_[0] -ceq 'value'}
  if($temp[1] -ne '4294967296' -or $column[1] -ne 'bigint(10) unsigned'){throw 'Native TEMP CTAS result or metadata narrowed'}
  'PASS MariaDB unsigned CAST:14 signed32/unsigned32/signed64/unsigned64 boundaries,bare/prepared text result,comparison/HEX/string controls and actual TEMP CTAS bigint storage'
 }
}
$global:LASTEXITCODE=0

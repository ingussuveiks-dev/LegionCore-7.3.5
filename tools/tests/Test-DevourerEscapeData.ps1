param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_11_05_02_devourer_escape_casts.sql",[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-escape-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=Get-Content -LiteralPath $MigrationPath -Raw
 $live=if($VerifyLiveData){"SELECT 'live',COUNT(*) FROM spell_script_names WHERE spell_id IN(211687,211689) AND ScriptName='spell_devourer_escape';"}else{''}
 $sql=@"
$live
CREATE TEMPORARY TABLE escape_seed LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE escape_seed;
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(211687,'escape_unrelated'),(211689,'escape_unrelated'),(999999,'escape_sentinel');
$migration
SELECT 'first',spell_id,ScriptName FROM spell_script_names ORDER BY spell_id,ScriptName;
$migration
SELECT 'second',spell_id,ScriptName FROM spell_script_names ORDER BY spell_id,ScriptName;
"@
 [IO.File]::WriteAllText("$out/fixture.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$db[3]
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/fixture.sql" -OutputFile "$out/fixture.tsv" -ErrorFile "$out/stderr.log"
  if($code -or (Get-Item "$out/stderr.log").Length){throw 'Escape TEMP-only SQL replay failed'}
 }finally{$env:MYSQL_PWD=$prior}
 $rows=Get-Content "$out/fixture.tsv"
 foreach($label in @('first','second')){
  $expected=@("$label`t211687`tescape_unrelated","$label`t211687`tspell_devourer_escape","$label`t211689`tescape_unrelated","$label`t211689`tspell_devourer_escape","$label`t999999`tescape_sentinel")
  if(@($rows|Where-Object {$_ -like "$label`t*"}).Count -ne 5){throw 'Unexpected escape replay rows'}
  foreach($row in $expected){if($rows -notcontains $row){throw 'Escape repeat/custom binding preservation failed'}}
 }
 if($VerifyLiveData -and $rows -notcontains "live`t2"){throw 'Actual live escape bindings missing'}
 'PASS TEMP-only repeatable two-spell binding migration; unrelated shared bindings and sentinel preserved. Live binding readback requested='+$VerifyLiveData
}

param([string]$SqlFile="$PSScriptRoot/../../sql/updates/world/2026_11_15_00_fuzan_return_reconstruction.sql",[string]$OutputDirectory="$PSScriptRoot/../../.codex/fuzan-return-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $SqlFile))
 $sql=@'
CREATE TEMPORARY TABLE return_actor_seed LIKE creature_template;
INSERT INTO return_actor_seed SELECT * FROM creature_template WHERE entry IN(109440,109441);
CREATE TEMPORARY TABLE creature_template LIKE return_actor_seed;
INSERT INTO creature_template SELECT * FROM return_actor_seed;
UPDATE creature_template SET ScriptName='';
CREATE TEMPORARY TABLE return_spell_seed LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE return_spell_seed;
SET SESSION TRANSACTION READ ONLY;
'@
 $checks=@'
SELECT @fuzan_return_safe,(SELECT COUNT(*) FROM creature_template WHERE entry=109440 AND ScriptName='npc_fuzan_return_guide'),(SELECT COUNT(*) FROM creature_template WHERE entry=109441 AND ScriptName='npc_fuzan_return_vehicle'),(SELECT COUNT(*) FROM spell_script_names);
'@
 $sql+="`n$migration`n$checks`n$migration`n$checks`nUPDATE creature_template SET ScriptName='unrelated_custom_actor' WHERE entry=109440;`n$migration`nSELECT @fuzan_return_safe,ScriptName FROM creature_template WHERE entry=109440;`nSELECT 'EOF';"
 [IO.File]::WriteAllText("$out/shadow.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $prior=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$db[3]
  $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/shadow.sql" -OutputFile "$out/shadow.tsv" -ErrorFile "$out/shadow.stderr"
  if($code -or (Get-Item "$out/shadow.stderr").Length){throw 'Shadow migration failed; inspect captured stderr'}
 }finally{$env:MYSQL_PWD=$prior}
 $lines=@(Get-Content "$out/shadow.tsv")
 if($lines.Count -ne 4 -or $lines[0] -cne "1`t1`t1`t2" -or $lines[1] -cne $lines[0] -or $lines[2] -cne "0`tunrelated_custom_actor" -or $lines[3] -cne 'EOF'){throw 'Shadow reconstruction migration/update/idempotence/custom-cohort guard differs'}
 @{migration_sha256=(Get-FileHash -LiteralPath $SqlFile).Hash;cases=3;persistent_writes=$false;read_only=$true}|ConvertTo-Json|Set-Content "$out/result.json"
 'PASS native MariaDB temporary shadow: install, idempotent replay, unrelated custom binding refused; permanent tables read only'
}

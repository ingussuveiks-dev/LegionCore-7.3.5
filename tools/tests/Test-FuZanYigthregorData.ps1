param([string]$SqlFile="$PSScriptRoot/../../sql/updates/world/2026_11_15_01_fuzan_yigthregor.sql",[string]$OutputDirectory="$PSScriptRoot/../../.codex/yigthregor-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $SqlFile))
 $sql=''
 foreach($pair in @(@('creature','guid=14670030'),@('creature_template','entry=109820'),@('creature_movement_override','SpawnId=14670030'),@('spell_script_names','spell_id=217787'))){
  $table=$pair[0];$where=$pair[1];$sql+="CREATE TEMPORARY TABLE yig_seed_$table LIKE $table;`nINSERT INTO yig_seed_$table SELECT * FROM $table WHERE $where;`nCREATE TEMPORARY TABLE $table LIKE yig_seed_$table;`nINSERT INTO $table SELECT * FROM yig_seed_$table;`n"
 }
 $sql+="UPDATE creature SET MovementType=2;UPDATE creature_template SET ScriptName='';DELETE FROM creature_movement_override;DELETE FROM spell_script_names;SET SESSION TRANSACTION READ ONLY;`n"
 $check="SELECT @yigthregor_safe,(SELECT MovementType FROM creature WHERE guid=14670030),(SELECT COUNT(*) FROM creature_template WHERE entry=109820 AND ScriptName='npc_fuzan_yigthregor'),(SELECT COUNT(*) FROM creature_movement_override WHERE SpawnId=14670030 AND Ground=0 AND Flight=1),(SELECT COUNT(*) FROM spell_script_names WHERE spell_id=217787 AND ScriptName='spell_fuzan_felfire_bomb');"
 $sql+="$migration`n$check`n$migration`n$check`nUPDATE creature_movement_override SET Flight=2;UPDATE creature SET MovementType=2;`n$migration`nSELECT @yigthregor_safe,MovementType FROM creature WHERE guid=14670030;SELECT 'EOF';"
 [IO.File]::WriteAllText("$out/shadow.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/shadow.sql" -OutputFile "$out/shadow.tsv" -ErrorFile "$out/shadow.stderr";if($code -or (Get-Item "$out/shadow.stderr").Length){throw 'Yigthregor shadow SQL failed'}}finally{$env:MYSQL_PWD=$prior}
 $lines=@(Get-Content "$out/shadow.tsv");if($lines.Count -ne 4 -or $lines[0] -cne "1`t0`t1`t1`t1" -or $lines[1] -cne $lines[0] -or $lines[2] -cne "0`t2" -or $lines[3] -cne 'EOF'){throw 'Yigthregor install/replay/custom override guard failed'}
 'PASS Yigthregor temporary shadow install/idempotence/custom movement refusal; permanent data unchanged'
}

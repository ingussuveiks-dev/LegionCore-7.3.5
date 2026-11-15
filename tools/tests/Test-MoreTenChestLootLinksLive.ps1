param([string]$OutputDirectory="$PSScriptRoot/../../.codex/more-ten-ten-chest-live-readback")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $plan=Get-Content "$PSScriptRoot/MoreTenTen20261115ChestRepairs.json" -Raw|ConvertFrom-Json;$rows=@($plan.repairs);$ids=($rows|ForEach-Object entry) -join ','
 if($rows.Count -ne 100 -or @($rows|ForEach-Object entry|Sort-Object -Unique).Count -ne 100){throw '100 distinct source records required'}
 $fields=@($rows[0].PSObject.Properties.Name|Where-Object {$_ -notin @('spawn_count','legion_spawn_count','loot_rows','batch','fix')})
 $json=($fields|ForEach-Object {"'$_',g.``$_``"}) -join ','
 $sql="SET SESSION TRANSACTION READ ONLY;SELECT JSON_OBJECT($json,'spawn_count',(SELECT COUNT(*) FROM gameobject s WHERE s.id=g.entry),'legion_spawn_count',(SELECT COUNT(*) FROM gameobject s WHERE s.id=g.entry AND s.map IN(1220,1669)),'loot_rows',(SELECT COUNT(*) FROM gameobject_loot_template l WHERE l.Entry=g.entry)) FROM gameobject_template g WHERE entry IN($ids) ORDER BY entry;SELECT 'EOF';"
 [IO.File]::WriteAllText("$out/readback.sql",$sql)
 [IO.File]::WriteAllText("$out/readback.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/readback.sql" -OutputFile "$out/readback.tsv" -ErrorFile "$out/readback.stderr";if($code -or (Get-Item "$out/readback.stderr").Length){throw 'Actual100 live template readback failed'}}finally{$env:MYSQL_PWD=$prior}
 $lines=@(Get-Content "$out/readback.tsv" -Encoding UTF8)
 if($lines.Count -ne 101 -or $lines[-1] -cne 'EOF'){throw 'Actual100 readback incomplete'}
 $seen=@{}
 foreach($line in $lines[0..99]){
  $actual=$line|ConvertFrom-Json;$expected=@($rows|Where-Object entry -eq $actual.entry)
  if($expected.Count -ne 1 -or $seen.ContainsKey($actual.entry)){throw 'Unexpected/repeated live source'};$seen[$actual.entry]=$true
  foreach($property in $actual.PSObject.Properties){
   $value=if($property.Name -eq 'Data1'){$actual.entry}else{$expected[0].($property.Name)}
   if($property.Value -cne $value){throw "Actual template differs outside declared loot delta: $($actual.entry)/$($property.Name)"}
  }
 }
 @{entries=100;batches=10;other_fields_preserved=$true;live_readback_sha256=(Get-FileHash "$out/readback.tsv").Hash;plan_sha256=(Get-FileHash "$PSScriptRoot/MoreTenTen20261115ChestRepairs.json").Hash}|ConvertTo-Json|Set-Content "$out/result.json"
 'PASS100 actual live loot links and original other template fields/spawn counts/loot row counts preserved; no permanent writes'
}

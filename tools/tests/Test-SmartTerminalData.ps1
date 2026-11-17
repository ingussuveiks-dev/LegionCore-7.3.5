#requires -Version 7.0
param([Parameter(Mandatory)][string]$ContextFile,[Parameter(Mandatory)][string]$FixtureFile,[Parameter(Mandatory)][string]$OutputDirectory,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
  $context=Get-Content $ContextFile -Raw|ConvertFrom-Json -AsHashtable;$fixture=Get-Content $FixtureFile -Raw|ConvertFrom-Json -AsHashtable
  $path="$repo/sql/updates/world/$($context.migration_name)";if(!(Test-Path $path)){$path="$repo/sql/pending/world/$($context.migration_name)"};$migration=Get-Content $path -Raw
  function Value($v){if($null -eq $v){return 'NULL'};if($v -is [string]){if(!$v.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($v))+' USING utf8mb4)'};[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
  $setup='';$baseline=''
  foreach($table in $context.filters.Keys){
   if($fixture[$table].Count -ne $context.expected_rows[$table]){throw "Pinned row count differs: $table"}
   $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($context.filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
   $baseline+="DELETE FROM $table;`n";$cols=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ','
   foreach($row in $fixture[$table]){$values=($context.columns[$table]|ForEach-Object{Value $row[$_]}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
  }
  function Snapshot($tag){$sql='';foreach($table in $context.filters.Keys){$order=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ',';$sql+="SELECT '$tag-$table',$table.* FROM $table WHERE $($context.filters[$table]) ORDER BY $order;`n"};$sql}
  $edit=$context.edit
  $expectedEdit="UPDATE $($edit.table) SET ``$($edit.field)``=$($edit.after) WHERE $($edit.where) AND ``$($edit.field)``=$($edit.before);`n"
  $sql=$setup+$baseline+(Snapshot original)+$expectedEdit+(Snapshot expected)+$baseline+$migration+(Snapshot first)+$migration+(Snapshot second)
  for($i=0;$i -lt $context.refusals.Count;++$i){$sql+=$baseline+$context.refusals[$i]+(Snapshot "refuse-$i-before")+$migration+(Snapshot "refuse-$i-after")}
  if($VerifyLiveData){foreach($table in $context.filters.Keys){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"};$sql+=(Snapshot live);$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))));$sql+="SELECT 'release',hash,state FROM updates WHERE name='$($context.migration_name)';"}
  $sql+="SELECT '__SMART_TERMINAL_COMPLETE__';";[IO.File]::WriteAllText("$out/replay.sql",$sql)
  $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names','--force','--default-character-set=utf8mb4') -InputFile "$out/replay.sql" -OutputFile "$out/replay.tsv" -ErrorFile "$out/replay.stderr"}finally{$env:MYSQL_PWD=$prior}
  [IO.File]::WriteAllText("$out/replay.exit.txt",[string]$code);$errors=[IO.File]::ReadAllLines("$out/replay.stderr");$pattern='^ERROR 1644 .*'+[regex]::Escape($context.refusal_message)
  if($code -notin 0,1 -or @($errors|Where-Object{$_ -match $pattern}).Count -ne $context.refusals.Count -or @($errors|Where-Object{$_ -match '^ERROR ' -and $_ -notmatch $pattern}).Count){throw 'Unexpected SQL/refusal result; evidence retained'}
  $rows=[IO.File]::ReadAllLines("$out/replay.tsv");if($rows[-1] -cne '__SMART_TERMINAL_COMPLETE__'){throw 'Incomplete terminal replay'}
  $groups=@{};foreach($line in $rows){$tab=$line.IndexOf("`t");if($tab -lt 1){continue};$tag=$line.Substring(0,$tab);if(!$groups.ContainsKey($tag)){$groups[$tag]=[Collections.Generic.List[string]]::new()};$groups[$tag].Add($line.Substring($tab+1))}
  function Signature($tag){if(!$groups.ContainsKey($tag)){return ''};[string]::Join("`n",$groups[$tag])}
  foreach($table in $context.filters.Keys){
   if((Signature "first-$table") -cne (Signature "expected-$table") -or (Signature "first-$table") -cne (Signature "second-$table")){throw "Wrong edit or non-idempotent: $table"}
   if($table -ne $edit.table -and (Signature "original-$table") -cne (Signature "first-$table")){throw "Unrelated table changed: $table"}
   for($i=0;$i -lt $context.refusals.Count;++$i){if((Signature "refuse-$i-before-$table") -cne (Signature "refuse-$i-after-$table")){throw "Refused state mutated: $i/$table"}}
   if($VerifyLiveData -and (Signature "first-$table") -cne (Signature "live-$table")){throw "Live data differs: $table"}
  }
  if((Signature "original-$($edit.table)") -ceq (Signature "expected-$($edit.table)")){throw 'Expected edit did not alter the pinned input'}
  if($VerifyLiveData -and (Signature release) -cne "$hash`tRELEASED"){throw 'Actual updater identity/state differs'}
  [IO.File]::WriteAllLines("$out/original-smart.tsv",$groups['original-smart_scripts']);[IO.File]::WriteAllLines("$out/terminal-smart.tsv",$groups['first-smart_scripts'])
  "PASS full-schema $($context.filters.Count)-table terminal replay, exact intended field edit, repeat/live identity and $($context.refusals.Count) foreign/partial refusals; persistent DML denied"
 }
}

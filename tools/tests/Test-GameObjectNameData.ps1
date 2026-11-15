param([ValidateRange(0,10)][int]$Batch=0,[string]$SqlDirectory='', [switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");if(!$SqlDirectory){$SqlDirectory="$repo/sql/updates/world"}
$out="$repo/build-extractors/runtime-test/gameobject-name-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $plan=[IO.File]::ReadAllText("$PSScriptRoot/GameObjectName20261115Repairs.json")|ConvertFrom-Json;$all=@($plan.repairs);if($all.Count -ne 100 -or @($all.entry|Sort-Object -Unique).Count -ne 100){throw 'Expected100 distinct authored names'}
 $meta=@('before_hex','after_hex','after_name','other_fingerprint','spawn_count','maps','batch','fix');$fields=@($all[0].PSObject.Properties.Name|Where-Object {$_ -notin $meta});$ids=($all.entry+239692)-join ','
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:goNameQueryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $i=$script:goNameQueryIndex++;$inputPath="$out/query-$i.sql";$outputPath="$out/query-$i.tsv";$errorPath="$out/query-$i.stderr";[IO.File]::WriteAllText($inputPath,$sql);$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$arguments=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($Refusal){$arguments+='--force'};$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile $inputPath -OutputFile $outputPath -ErrorFile $errorPath}finally{$env:MYSQL_PWD=$prior}
  $errors=[IO.File]::ReadAllText($errorPath);$lines=@([IO.File]::ReadAllLines($outputPath));
  @{exit=$code;expected_refusal=[bool]$Refusal}|ConvertTo-Json|Set-Content "$out/query-$i-status.json";if($Refusal){$failures=@($errors -split "`n"|Where-Object {$_ -match '^ERROR '});if($code -or $failures.Count -ne 1 -or $failures[0] -notmatch '^ERROR 1644 \(45000\).*GameObject name batch\d+ differs; review before updating' -or ($errors -notmatch '\A--------------\r?\nEXECUTE go_name_guard\r?\n--------------\r?\n\r?\nERROR 1644 \(45000\) at line \d+: GameObject name batch\d+ differs; review before updating\r?\n?\z')){throw 'Foreign-state refusal differs; inspect captured evidence'}}elseif($code -or $errors.Length){throw 'Name SQL failed; inspect captured evidence'}
  if(!$lines.Count -or $lines[-1] -cne 'EOF'){throw 'Name SQL output incomplete'};,$lines
 }
 function Case($rows,[string]$key){'CASE g.entry '+(($rows|ForEach-Object {"WHEN $($_.entry) THEN '$($_.$key)'"}) -join ' ')+' END'}
 $allFp="SHA2(CONCAT_WS('|',"+(($fields|ForEach-Object {"COALESCE(HEX(CAST(g.``$_`` AS CHAR CHARACTER SET utf8mb4)),'NULL')"}) -join ',')+'),256)'
 function Snapshot([string]$tag){"SELECT '$tag',g.entry,$allFp FROM gameobject_template g ORDER BY g.entry;"}
 $batches=if($Batch){@($Batch)}else{@(1..10)}
 foreach($n in $batches){
  $rows=@($all|Where-Object batch -eq $n);if($rows.Count -ne 10){throw 'Expected ten names per batch'};$cohort=$rows.entry-join ',';$old=Case $rows before_hex;$new=Case $rows after_hex;$first=$rows[0].entry
  $path=Join-Path $SqlDirectory "2026_11_15_$($n+69)_gameobject_name_batch$n.sql";$migration=[IO.File]::ReadAllText((Resolve-Path $path))
  $setup="CREATE TEMPORARY TABLE name_seed LIKE gameobject_template;INSERT INTO name_seed SELECT * FROM gameobject_template WHERE entry IN($ids);CREATE TEMPORARY TABLE gameobject_template LIKE name_seed;INSERT INTO gameobject_template SELECT * FROM name_seed;UPDATE gameobject_template g SET name=CONVERT(UNHEX($old) USING utf8mb4) WHERE g.entry IN($cohort) AND HEX(g.name)=$new;SET SESSION TRANSACTION READ ONLY;"
  $equal=($fields|Where-Object {$_ -ne 'name'}|ForEach-Object {"g.``$_`` <=> s.``$_``"}) -join ' AND '
  $query=$setup+"SELECT 'baseline',COUNT(*) FROM gameobject_template g WHERE g.entry IN($cohort) AND HEX(g.name)=$old;"+$migration+"SELECT 'fixed',COUNT(*) FROM gameobject_template g WHERE g.entry IN($cohort) AND HEX(g.name)=$new;"+$migration+"SELECT 'replay',COUNT(*) FROM gameobject_template g WHERE g.entry IN($cohort) AND HEX(g.name)=$new;SELECT 'preserved',COUNT(*) FROM gameobject_template g JOIN name_seed s USING(entry) WHERE $equal;SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN name_seed s USING(entry) WHERE g.entry NOT IN($cohort) AND BINARY g.name=BINARY s.name;SELECT 'EOF';"
  $actual=Query $query;$expected=@("baseline`t10","fixed`t10","replay`t10","preserved`t101","unrelated`t91",'EOF');if(($actual-join "`n") -cne ($expected-join "`n")){throw "Batch$n install/replay/preservation failed"}
  foreach($variant in @("UPDATE gameobject_template SET name='UserCustomName' WHERE entry=$first;","UPDATE gameobject_template SET flags=flags^1 WHERE entry=$first;","DELETE FROM gameobject_template WHERE entry=$first;")){
   $actual=Query ($setup+$variant+(Snapshot 'custom')+$migration+(Snapshot 'after')+"SELECT 'EOF';") -Refusal
   $before=@($actual|Where-Object {$_ -like "custom`t*"}|ForEach-Object {$_.Substring(7)});$after=@($actual|Where-Object {$_ -like "after`t*"}|ForEach-Object {$_.Substring(6)});if(($before-join "`n") -cne ($after-join "`n")){throw "Batch$n refused state changed"}
  }
  if($VerifyLiveData){
   $json=($fields|ForEach-Object {"'$_',g.``$_``"})-join ',';$actual=Query "SET SESSION TRANSACTION READ ONLY;SELECT JSON_OBJECT($json) FROM gameobject_template g WHERE entry IN($cohort) ORDER BY entry;SELECT 'EOF';";if($actual.Count -ne 11){throw 'Ten live templates missing'}
   foreach($line in $actual[0..9]){$live=$line|ConvertFrom-Json;$expected=@($rows|Where-Object entry -eq $live.entry);if($expected.Count -ne 1){throw 'Unexpected live entry'};foreach($property in $live.PSObject.Properties){$value=if($property.Name -eq 'name'){$expected[0].after_name}else{$expected[0].($property.Name)};if($property.Value -cne $value){throw "Live batch$n changed outside declared name delta: $($live.entry)/$($property.Name)"}}}
   $sha1=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha1.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha1.Dispose()};$file=[IO.Path]::GetFileName($path);$history=Query "SET SESSION TRANSACTION READ ONLY;SELECT name,hash,state FROM updates WHERE name='$file';SELECT 'EOF';";if($history.Count -ne 2 -or $history[0] -cne "$file`t$hash`tRELEASED"){throw 'Updater does not certify exact live migration'}
  }
  "PASS batch${n}: ten names restored/replayed; all other fields on101 templates and91 unrelated names preserved; custom name/metadata/missing-row refusals unchanged; live=$VerifyLiveData"
 }
 @{batches=$batches;live=[bool]$VerifyLiveData;permanent_writes=$false;plan_sha256=(Get-FileHash "$PSScriptRoot/GameObjectName20261115Repairs.json").Hash}|ConvertTo-Json|Set-Content "$out/result.json"
}

param([ValidateRange(1,10)][int]$Batch=1,[string]$SqlFile='',[string]$OutputDirectory="$PSScriptRoot/../../.codex/ten-ten-chest-shadow")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$SqlFile){$SqlFile="$repo/sql/updates/world/2026_11_15_$('{0:00}' -f ($Batch+36))_legion_chest_links_ten_batch$Batch.sql"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $plan=Get-Content "$PSScriptRoot/TenTen20261115ChestRepairs.json" -Raw|ConvertFrom-Json;$all=@($plan.repairs|ForEach-Object entry);$entries=@($plan.repairs|Where-Object batch -eq $Batch|ForEach-Object entry)
 if($all.Count -ne 100 -or $entries.Count -ne 10){throw 'Expected100 distinct entries/ten10 cohorts'}
 $ids=$entries -join ',';$allids=($all+245924) -join ',';$migration=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $SqlFile))
 $sql=@"
CREATE TEMPORARY TABLE chest_seed LIKE gameobject_template;
INSERT INTO chest_seed SELECT * FROM gameobject_template WHERE entry IN($allids);
CREATE TEMPORARY TABLE gameobject_template LIKE chest_seed;
INSERT INTO gameobject_template SELECT * FROM chest_seed;
UPDATE gameobject_template SET Data1=0 WHERE entry IN($ids);
SET SESSION TRANSACTION READ ONLY;
$migration
SELECT 'fixed',COUNT(*) FROM gameobject_template WHERE entry IN($ids) AND Data1=entry;
$migration
SELECT 'replay',COUNT(*) FROM gameobject_template WHERE entry IN($ids) AND Data1=entry;
SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN chest_seed s ON g.entry=s.entry WHERE g.entry NOT IN($ids) AND g.Data1=s.Data1;
SET @chest_preserved=(SELECT GROUP_CONCAT(CONCAT('g.',CHAR(96),COLUMN_NAME,CHAR(96),' <=> s.',CHAR(96),COLUMN_NAME,CHAR(96)) ORDER BY ORDINAL_POSITION SEPARATOR ' AND ') FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='gameobject_template' AND COLUMN_NAME<>'Data1');
SET @chest_sql=CONCAT('SELECT ',CHAR(39),'preserved',CHAR(39),',COUNT(*) FROM gameobject_template g JOIN chest_seed s ON g.entry=s.entry WHERE ',@chest_preserved);
PREPARE chest_stmt FROM @chest_sql;EXECUTE chest_stmt;DEALLOCATE PREPARE chest_stmt;
UPDATE gameobject_template SET Data1=123456 WHERE entry=$($entries[0]);
$migration
SELECT 'custom',Data1 FROM gameobject_template WHERE entry=$($entries[0]);
SELECT 'EOF';
"@
 [IO.File]::WriteAllText("$out/shadow.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/shadow.sql" -OutputFile "$out/shadow.tsv" -ErrorFile "$out/shadow.stderr";if($code -or (Get-Item "$out/shadow.stderr").Length){throw 'Chest shadow SQL failed; inspect captured stderr'}}finally{$env:MYSQL_PWD=$prior}
 $lines=@(Get-Content "$out/shadow.tsv");$expected=@("fixed`t10","replay`t10","unrelated`t91","preserved`t101","custom`t123456",'EOF')
 if(($lines -join "`n") -cne ($expected -join "`n")){throw 'Chest exact install/replay/isolation/all-column preservation/custom-link guard differs'}
 @{batch=$Batch;entries=$entries;migration_sha256=(Get-FileHash -LiteralPath $SqlFile).Hash;permanent_writes=$false}|ConvertTo-Json -Depth 3|Set-Content "$out/result.json"
 "PASS batch${Batch}:10 exact loot links, idempotence,91 unrelated entries, all101 other columns preserved and foreign link refused; temporary tables only"
}

param([Parameter(Mandatory=$true)][string]$SqlFile,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $SqlFile))
 $sql=@"
CREATE TEMPORARY TABLE chest_seed LIKE gameobject_template;
INSERT INTO chest_seed SELECT * FROM gameobject_template WHERE entry IN(251959,257290,257291,266851,245924);
CREATE TEMPORARY TABLE gameobject_template LIKE chest_seed;
INSERT INTO gameobject_template SELECT * FROM chest_seed;
UPDATE gameobject_template SET Data1=0 WHERE entry IN(251959,257290,257291,266851);
UPDATE gameobject_template SET Data26=219461 WHERE entry=251959;
SET SESSION TRANSACTION READ ONLY;
$migration
SELECT 'fixed',COUNT(*) FROM gameobject_template WHERE entry IN(251959,257290,257291,266851) AND Data1=entry AND Data26=IF(entry=251959,0,81040);
$migration
SELECT 'replay',COUNT(*) FROM gameobject_template WHERE entry IN(251959,257290,257291,266851) AND Data1=entry AND Data26=IF(entry=251959,0,81040);
SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN chest_seed s USING(entry) WHERE g.entry=245924 AND g.Data1=s.Data1 AND g.Data26=s.Data26;
SET @preserved=(SELECT GROUP_CONCAT(CONCAT('g.',CHAR(96),COLUMN_NAME,CHAR(96),' <=> s.',CHAR(96),COLUMN_NAME,CHAR(96)) ORDER BY ORDINAL_POSITION SEPARATOR ' AND ') FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='gameobject_template' AND COLUMN_NAME NOT IN('Data1','Data26'));
SET @statement=CONCAT('SELECT ',CHAR(39),'preserved',CHAR(39),',COUNT(*) FROM gameobject_template g JOIN chest_seed s USING(entry) WHERE ',@preserved);
PREPARE chest_stmt FROM @statement;EXECUTE chest_stmt;DEALLOCATE PREPARE chest_stmt;
UPDATE gameobject_template SET Data1=123456,Data26=219461 WHERE entry=251959;
$migration
SELECT 'custom',Data1,Data26 FROM gameobject_template WHERE entry=251959;
SELECT 'EOF';
"@
 [IO.File]::WriteAllText("$out/shadow.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/shadow.sql" -OutputFile "$out/shadow.tsv" -ErrorFile "$out/shadow.stderr";if($code -or (Get-Item "$out/shadow.stderr").Length){throw 'Four-chest shadow SQL failed'}}finally{$env:MYSQL_PWD=$prior}
 $lines=@(Get-Content "$out/shadow.tsv");$expected=@("fixed`t4","replay`t4","unrelated`t1","preserved`t5","custom`t123456`t219461",'EOF')
 if(($lines -join "`n") -cne ($expected -join "`n")){throw 'Four-chest install/replay/column preservation/custom-link refusal differs'}
 'PASS four chest data deltas, idempotence/all other columns/unrelated source and custom-link refusal; temporary tables only'
}

$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"
$c=($config | Where-Object {$_ -match '^CharacterDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$migration=Get-Content "$repo/sql/updates/characters/2026_10_15_00_scenario_resume.sql" -Raw
$source=Get-Content "$repo/src/server/database/Database/Implementation/CharacterDatabase.cpp" -Raw
function Statement($name){
 $m=[regex]::Match($source,('PrepareStatement\('+[regex]::Escape($name)+',\s*((?:"[^"]*"\s*)+), CONNECTION_ASYNC\);'))
 if(!$m.Success){throw "Missing native statement $name"}
 return ([regex]::Matches($m.Groups[1].Value,'"([^"]*)"') | ForEach-Object {$_.Groups[1].Value}) -join ''
}
$select=Statement 'CHAR_SEL_CHARACTER'
$update=Statement 'CHAR_UPD_CHARACTER'
if(!$select.Contains('killPoints, scenarioResume FROM characters') -or !$update.Contains('killPoints=?,scenarioResume=? WHERE guid=?')){throw 'Native character persistence wiring missing'}
$sql=@"
CREATE TEMPORARY TABLE resume_character_fixture LIKE characters;
ALTER TABLE resume_character_fixture RENAME TO characters;
$migration
$migration
INSERT IGNORE INTO characters(guid,account,name) VALUES(4294967000,1,'ResumeFixture');
UPDATE characters SET map=9000,instance_id=42,position_x=50,scenarioResume='1 9000 777 42 1000 10 20 30 1' WHERE guid=4294967000;
SELECT JSON_OBJECT('map',map,'instance',instance_id,'position',position_x,'checkpoint',scenarioResume) FROM characters WHERE guid=4294967000;
"@
$old=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Temporary character checkpoint migration failed'}
 $r=$rows[-1] | ConvertFrom-Json
 if([int]$r.map -ne 9000 -or [int]$r.instance -ne 42 -or [float]$r.position -ne 50 -or $r.checkpoint -ne '1 9000 777 42 1000 10 20 30 1'){throw 'Checkpoint/position roundtrip failed'}
 'PASS: actual migration twice in a temporary character table, checkpoint/current-position separation and native SELECT/UPDATE wiring'
}finally{$env:MYSQL_PWD=$old}

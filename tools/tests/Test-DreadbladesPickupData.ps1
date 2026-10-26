param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_09_dreadblades_pickup.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dreadblades-pickup-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $sql=@'
CREATE TEMPORARY TABLE saved_template LIKE gameobject_template;
INSERT INTO saved_template SELECT * FROM gameobject_template WHERE entry IN(254087,249824);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_template;
INSERT INTO gameobject_template SELECT * FROM saved_template;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=254087;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
UPDATE gameobject_template SET Data1=0 WHERE entry=254087;
'@
 $sql+="`n$migration`n$migration`nSELECT 'fixed',Data1 FROM gameobject_template WHERE entry=254087; SELECT 'same-loot',COUNT(*) FROM gameobject_loot_template l JOIN saved_loot s ON l.Entry=s.Entry AND l.Item=s.Item AND l.Reference=s.Reference AND l.Chance=s.Chance AND l.MinCount=s.MinCount AND l.MaxCount=s.MaxCount; SELECT 'unrelated',COUNT(*) FROM gameobject_template t JOIN saved_template s USING(entry) WHERE t.entry=249824 AND t.Data1=s.Data1 AND t.ScriptName=s.ScriptName;"
 foreach($variant in @("UPDATE gameobject_template SET ScriptName='custom_pickup',Data1=0 WHERE entry=254087;","UPDATE gameobject_template SET ScriptName='go_scenario_dreadblades',Data1=123 WHERE entry=254087;","UPDATE gameobject_template SET Data1=0 WHERE entry=254087; UPDATE gameobject_loot_template SET Item=1 WHERE Entry=254087;")){$sql+="`n$variant`n$migration`nSELECT 'refused',Data1 FROM gameobject_template WHERE entry=254087;"}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Dreadblades TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("fixed`t254087","same-loot`t1","unrelated`t1","refused`t0","refused`t123","refused`t0")
 if(($rows -join '|') -ne ($expected -join '|')){throw "Pickup data mismatch: $($rows -join '|')"}
 [IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$rows)
 'PASS native loot binding in TEMP only, repeat-safe; existing loot/unrelated data and custom binding/table preserved. This test does not apply persistent SQL.'
}

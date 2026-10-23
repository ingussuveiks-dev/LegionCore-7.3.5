$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_06_priest_dalaran_messenger.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_templates LIKE creature_template;
INSERT INTO saved_templates SELECT * FROM creature_template WHERE entry=101344;
CREATE TEMPORARY TABLE creature_template LIKE saved_templates;
INSERT INTO creature_template SELECT * FROM saved_templates;
DELETE FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id BETWEEN 1 AND 4;
$migration
$migration
SELECT 'actor',id,link,event_type,event_param1,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 ORDER BY id;
SELECT 'missing-link',COUNT(*) FROM smart_scripts a LEFT JOIN smart_scripts b ON b.entryorguid=a.entryorguid AND b.source_type=a.source_type AND b.id=a.link AND b.event_type=61 WHERE a.entryorguid=101344 AND a.source_type=0 AND a.link<>0 AND b.id IS NULL;
SELECT 'unrelated',COUNT(*) FROM smart_scripts a LEFT JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.entryorguid<>101344 AND b.id IS NULL;
DELETE FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id BETWEEN 1 AND 4;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,action_type,comment) VALUES (101344,0,3,61,1,'custom');
$migration
SELECT 'custom-slot',COUNT(*) FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id=3 AND action_type=1;
SELECT 'no-partial-bootstrap',COUNT(*) FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id IN (2,4);
DELETE FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id BETWEEN 1 AND 4;
UPDATE creature_template SET ScriptName='custom' WHERE entry=101344;
$migration
SELECT 'custom-script',COUNT(*) FROM smart_scripts WHERE entryorguid=101344 AND source_type=0 AND id BETWEEN 1 AND 4;
"@
$config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$config[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Priest messenger fixture failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("actor`t0`t1`t19`t40705`t1`t0`t7","actor`t1`t0`t61`t0`t41`t5000`t1","actor`t2`t3`t54`t0`t29`t0`t7","actor`t3`t4`t61`t0`t207`t0`t7","actor`t4`t0`t61`t0`t41`t30000`t1","missing-link`t0","unrelated`t0","custom-slot`t1","no-partial-bootstrap`t0","custom-script`t0")){if($rows -notcontains $expected){throw "Missing priest messenger result $expected"}}
# Walk both real action graphs: summoned invoker remains the follow/personal
# target; acceptance has only the existing TALK followed by delayed cleanup.
$events=@{};foreach($line in $rows){$f=$line.Split("`t");if($f[0] -eq 'actor'){$events[[int]$f[1]]=$f}}
foreach($case in @(@(0,'1,41'),@(2,'29,207,41'))){$id=[int]$case[0];$actions=@();do{$row=$events[$id];$actions+=$row[5];$id=[int]$row[2]}while($id);if(($actions -join ',') -ne $case[1]){throw 'Wrong messenger continuation'}}
'PASS: repeatable native acceptance cleanup and personal follow/timeout graphs; custom AI/event slots preserved. Actual Priest arrival, dialogue, movement and acceptance remain client checks.'

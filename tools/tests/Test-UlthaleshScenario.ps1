param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_15_27_ulthalesh_finale.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ulthalesh"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content $MigrationFile -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN (100344,102200,246021,101282) AND source_type IN(0,1);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry IN(245794,245789);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
UPDATE gameobject_template SET Data1=0 WHERE entry=245794;
CREATE TEMPORARY TABLE instance_template LIKE instance_template;
"@
# LIKE must resolve the native instance table before it is shadowed.
$sql=$sql.Replace('CREATE TEMPORARY TABLE instance_template LIKE instance_template;',"CREATE TEMPORARY TABLE saved_instance LIKE instance_template;`nCREATE TEMPORARY TABLE instance_template LIKE saved_instance;")
$select=@"
SELECT 'smart',JSON_OBJECT('entry',entryorguid,'source',source_type,'id',id,'link',link,'event',event_type,'flags',event_flags,'e1',event_param1,'e2',event_param2,'action',action_type,'a1',action_param1,'a2',action_param2,'target',target_type,'t1',target_param1) FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'compass',g.entry,g.type,g.Data1,l.Item,l.Chance,o.QuestID FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 JOIN quest_objectives o ON o.ObjectID=l.Item AND o.Type=1 WHERE g.entry=245794 AND o.QuestID=40606;
SELECT 'go_preserved',COUNT(*) FROM gameobject_template g JOIN saved_go s ON s.entry=g.entry WHERE g.Data0=s.Data0 AND g.flags=s.flags AND g.Data8=s.Data8 AND g.Data30=s.Data30 AND g.ScriptName=s.ScriptName;
SELECT 'instance',map,script FROM instance_template WHERE map=1533;
"@
$sql+="`n"+$migration+"`nSELECT 'PASS1';`n"+$select+"`n"+$migration+"`nSELECT 'PASS2';`n"+$select+@"

UPDATE instance_template SET script='unrelated_custom_instance' WHERE map=1533;
$migration
SELECT 'custom_instance',COUNT(*) FROM instance_template WHERE map=1533 AND script='unrelated_custom_instance';
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ulthalesh SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
if($rows -notcontains "custom_instance`t1"){throw 'Custom instance script overwritten'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$rows)
& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe -X utf8 "$PSScriptRoot/ulthalesh_scenario_test.py" "$out/rows.tsv" "$repo"
if($LASTEXITCODE){throw 'Ulthalesh native-data replay failed'}

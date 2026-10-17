param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-journals"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_04_stormheim_kjell_access.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{gameobject='id IN(244708,244717,244718,244719,244720,251571)';gameobject_template='entry IN(244708,244717,244718,244719,244720,251571)';quest_poi_points='QuestID=39984'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE gameobject SET position_x=3432.79,position_y=1564.77,position_z=9.06416 WHERE guid=114999; UPDATE quest_poi_points SET X=3433,Y=1565 WHERE QuestID=39984 AND Idx1=3 AND Idx2=0;`n$migration`n$migration`n"
 $sql+="UPDATE saved_gameobject SET position_x=3438.79,position_y=1560.27,position_z=7.01219 WHERE guid=114999; UPDATE saved_quest_poi_points SET X=3439,Y=1560 WHERE QuestID=39984 AND Idx1=3 AND Idx2=0;`n"
 foreach($table in $tables.Keys){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@'
SELECT 'placement',COUNT(*) FROM gameobject WHERE guid=114999 AND id=244719 AND map=1220 AND PhaseId='' AND ABS(position_x-3438.79)<0.001 AND ABS(position_y-1560.27)<0.001 AND ABS(position_z-7.01219)<0.001;
SELECT 'poi',X,Y FROM quest_poi_points WHERE QuestID=39984 AND Idx1=3 AND Idx2=0;
'@
if(!$Live){
 $sql+="UPDATE gameobject SET position_x=123.5 WHERE guid=114999; UPDATE quest_poi_points SET X=3433,Y=1565 WHERE QuestID=39984 AND Idx1=3 AND Idx2=0;`n$migration`nSELECT 'custom-position',position_x FROM gameobject WHERE guid=114999; SELECT 'unmoved-poi',X,Y FROM quest_poi_points WHERE QuestID=39984 AND Idx1=3 AND Idx2=0; UPDATE gameobject SET position_x=3432.79,position_y=1564.77,position_z=9.06416 WHERE guid=114999; UPDATE gameobject_template SET ScriptName='custom' WHERE entry=244719;`n$migration`nSELECT 'custom-script',position_z FROM gameobject WHERE guid=114999; UPDATE gameobject_template SET ScriptName='' WHERE entry=244719; UPDATE gameobject SET map=1 WHERE guid=114999;`n$migration`nSELECT 'wrong-map',position_z FROM gameobject WHERE guid=114999; UPDATE gameobject SET map=1220 WHERE guid=114999; UPDATE gameobject_template SET displayId=1 WHERE entry=244719;`n$migration`nSELECT 'wrong-model',position_z FROM gameobject WHERE guid=114999; UPDATE gameobject_template SET displayId=15405 WHERE entry=244719; UPDATE quest_poi_points SET X=123,Y=456 WHERE QuestID=39984 AND Idx1=3 AND Idx2=0;`n$migration`nSELECT 'custom-poi',X,Y FROM quest_poi_points WHERE QuestID=39984 AND Idx1=3 AND Idx2=0;"
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Kjell migration check failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/kjell-data$(if($Live){'-live'}).tsv"
foreach($row in @("placement`t1","poi`t3439`t1560")){if($rows -notcontains $row){throw "Kjell repair contract changed: $row"}}
if(!$Live){
 foreach($table in $tables.Keys){$a="actual-$table`t";$e="expected-$table`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object);$expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object);if($actual.Count -ne $expected.Count -or(Compare-Object $actual $expected)){throw "Kjell full-row preservation failed: $table"}}
 foreach($row in @("custom-position`t123.5","unmoved-poi`t3433`t1565","custom-script`t9.06416","wrong-map`t9.06416","wrong-model`t9.06416","custom-poi`t123`t456")){if($rows -notcontains $row){throw "Kjell migration guard changed: $row"}}
}
if($Live){'PASS: live Kjell spawn and objective POI match the repaired placement.'}
else{'PASS: scoped Kjell spawn/POI repair twice, complete other fields/objects/templates preserved, custom positions/markers/scripts, wrong-map and wrong-model guards.'}

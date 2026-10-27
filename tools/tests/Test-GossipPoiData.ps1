$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/gossip-poi-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $sql=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'guard',entry,gossip_menu_id,ScriptName FROM creature_template WHERE entry=3296;
SELECT 'option',MenuID,OptionID,OptionNpc,ActionMenuID,ActionPoiID FROM gossip_menu_option WHERE MenuID=1951 AND OptionID=10;
SELECT 'poi',entry,ROUND(x,2),ROUND(y,2),icon,flags,data,icon_name FROM points_of_interest WHERE entry=177;
SELECT 'links',COUNT(*) FROM gossip_menu_option g JOIN points_of_interest p ON p.entry=g.ActionPoiID WHERE g.OptionNpc=0;
SELECT 'nearby',COUNT(*) FROM creature WHERE id=3310 AND map=1 AND ABS(position_x-1800.22)<1 AND ABS(position_y+4369.74)<1;
'@
 $prior=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$db[3]
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names 2> "$out/sql-stderr.txt")
  if($LASTEXITCODE -or (Get-Item "$out/sql-stderr.txt").Length){throw 'Read-only gossip POI query failed'}
 }finally{$env:MYSQL_PWD=$prior}
 foreach($row in @("guard`t3296`t1951`tguard_generic","option`t1951`t10`t0`t1902`t177","poi`t177`t1800.22`t-4369.74`t7`t99`t0`tThe Sky Tower","nearby`t2")){
  if($rows -notcontains $row){throw "Orgrimmar native POI contract differs: $row"}
 }
 $links=@($rows|Where-Object {$_ -like "links`t*"})
 if($links.Count -ne 1 -or [int]$links[0].Split("`t")[1] -le 0){throw 'No valid database POI links'}
 "PASS readonly guard3296/menu1951/10/POI177 and both native Doras spawns; $($links[0]); no SQL or character writes"
}

param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-batrider"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_17_stormheim_batrider.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_template','gossip_menu_option')){
  $filter=if($table -eq 'creature_template'){'entry IN(97250,93624)'}else{'MenuID IN(18762,18657)'}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template SET ScriptName='' WHERE entry=97250; UPDATE gossip_menu_option SET OptionText='Can you throw me at a stronghold?' WHERE MenuID=18762 AND OptionID=0;`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=97250 AND ScriptName='npc_stormheim_foothold_batrider' AND AIName='' AND gossip_menu_id=18762 AND VehicleId=4387 AND npcflag=3 AND faction=83;
SELECT 'option',COUNT(*) FROM gossip_menu_option WHERE MenuID=18762 AND OptionID=0 AND OptionNpc=0 AND OptionBroadcastTextID=98963 AND OptionText='Can you give me a lift back to the Foothold?' AND ActionMenuID=0 AND ActionPoiID=0 AND BoxMoney=0;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=97250 AND quest=38317;
SELECT 'source',COUNT(*) FROM creature WHERE guid=340420 AND id=97250 AND map=1220 AND phaseMask=1 AND PhaseId='' AND modelid=0 AND npcflag=0 AND MovementType=0;
SELECT 'duplicate',COUNT(*) FROM creature WHERE guid=341178;
SELECT 'native-models',COUNT(*) FROM creature_template_wdb WHERE entry=97250 AND Displayid1=62917 AND Displayid2=62918;
SELECT 'extra-menu-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18762;
'@
if(!$Live){
 $sql+=@"
SELECT 'unrelated-template',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry=93624 AND a.ScriptName=b.ScriptName;
UPDATE creature_template SET ScriptName='custom' WHERE entry=97250;
UPDATE gossip_menu_option SET OptionText='custom' WHERE MenuID=18762 AND OptionID=0;
$migration
SELECT 'custom-binding',COUNT(*) FROM creature_template WHERE entry=97250 AND ScriptName='custom';
SELECT 'custom-text',COUNT(*) FROM gossip_menu_option WHERE MenuID=18762 AND OptionID=0 AND OptionText='custom';
UPDATE creature_template SET ScriptName='',VehicleId=1 WHERE entry=97250;
$migration
SELECT 'custom-vehicle',COUNT(*) FROM creature_template WHERE entry=97250 AND ScriptName='' AND VehicleId=1;
"@
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Bat-Rider SQL failed'}}finally{$env:MYSQL_PWD=$old}
$rows | Set-Content "$out/$(if($Live){'live'}else{'migration'}).tsv"
$expected=@("binding`t1","option`t1","ender`t1","source`t1","duplicate`t0","native-models`t1","extra-menu-conditions`t0")
if(!$Live){$expected+=@("unrelated-template`t1","custom-binding`t1","custom-text`t1","custom-vehicle`t1")}
if(($rows -join '|') -ne ($expected -join '|')){throw "Bat-Rider SQL contract: $($rows -join '|')"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$text=@("SELECT COUNT(*) FROM broadcast_text WHERE ID=98963 AND Text='Can you give me a lift back to the Foothold?' AND Text1=Text AND ConditionID=0;" | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE -or $text.Count -ne 1 -or $text[0] -ne '1'){throw 'Hotfix98963 native menu text changed'}}finally{$env:MYSQL_PWD=$old}
if($Live){'PASS: live Bat-Rider binding, native fallback/broadcast/menu/vehicle/models, public38317 ender and sole source.'}else{'PASS: Bat-Rider migration twice, native menu/vehicle/models/ender retained; unrelated template, custom script/text/vehicle preserved.'}

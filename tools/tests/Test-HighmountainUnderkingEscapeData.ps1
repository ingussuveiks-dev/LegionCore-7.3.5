param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_02_highmountain_underking_escape.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_template','creature_text','spell_script_names')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template SET ScriptName='' WHERE entry=97442; DELETE FROM creature_text WHERE CreatureID=97442 AND GroupID=50;`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'actor',COUNT(*) FROM creature_template WHERE entry=97442 AND ScriptName='npc_highmountain_underking_escape';
SELECT 'line',COUNT(*) FROM creature_text WHERE CreatureID=97442 AND GroupID=50 AND BroadcastTextID=99475 AND Sound=54321 AND MinTimer=6006;
SELECT 'spell',COUNT(*) FROM spell_script_names WHERE spell_id=192468 AND ScriptName='spell_highmountain_underking_escape';
SELECT 'equipment',COUNT(*) FROM creature_equip_template WHERE CreatureID=97442 AND ItemID1=132179;
SELECT 'portal',COUNT(*) FROM gameobject WHERE guid=113594 AND id=243067 AND PhaseId='9180';
"@
$expected=@("actor`t1","line`t1","spell`t1","equipment`t1","portal`t1")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b ON a.entry=b.entry WHERE a.entry<>97442 AND a.ScriptName<>b.ScriptName;
SELECT 'other-lines',(SELECT COUNT(*) FROM saved_creature_text WHERE NOT(CreatureID=97442 AND GroupID=50))-(SELECT COUNT(*) FROM creature_text WHERE NOT(CreatureID=97442 AND GroupID=50));
UPDATE creature_template SET ScriptName='custom_escape' WHERE entry=97442;
UPDATE creature_text SET BroadcastTextID=123 WHERE CreatureID=97442 AND GroupID=50;
"@
 $sql+="$migration`nSELECT 'custom-ai',COUNT(*) FROM creature_template WHERE entry=97442 AND ScriptName='custom_escape'; SELECT 'custom-text',COUNT(*) FROM creature_text WHERE CreatureID=97442 AND GroupID=50 AND BroadcastTextID=123;"
 $expected+=@("other-ai`t0","other-lines`t0","custom-ai`t1","custom-text`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Underking escape SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Underking escape SQL mismatch: $($actual -join '|')"}
'PASS: escape actor, native dialogue, visual-only spell binding, idempotence and preservation of custom data.'

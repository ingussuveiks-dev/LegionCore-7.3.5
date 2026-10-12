param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_18_highmountain_worm_support.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('creature_template','spell_script_names') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'calls',COUNT(*) FROM spell_script_names WHERE spell_id IN(195144,195146,195149) AND ScriptName='spell_highmountain_call_tribe';
SELECT 'allies',COUNT(*) FROM creature_template WHERE entry IN(99215,99219,99221) AND AIName='' AND ScriptName='npc_highmountain_worm_ally';
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(99215,99219,99221) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'native-attacks',COUNT(*) FROM creature_template_spell WHERE (entry=99215 AND spell=194019) OR (entry=99219 AND spell=195155) OR (entry=99221 AND spell IN(12548,192412));
SELECT 'totem-aura',COUNT(*) FROM creature_template_addon WHERE entry=97508 AND auras='195025';
SELECT 'native-kills',COUNT(*) FROM quest_objectives WHERE QuestID=39776 AND Type=0 AND Amount=1 AND ObjectID IN(97013,99216,99217);
"@
$expected=@("calls`t3","allies`t3","scaling`t3","native-attacks`t4","totem-aura`t1","native-kills`t3")
if(!$Live){
 $sql+=@"
SELECT 'other-scripts',COUNT(*) FROM saved_spell_script_names a LEFT JOIN spell_script_names b ON a.spell_id=b.spell_id AND a.ScriptName=b.ScriptName WHERE b.spell_id IS NULL;
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry NOT IN(99215,99219,99221) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
UPDATE creature_template SET ScriptName='custom_guard' WHERE entry=99215;
"@
 $sql+="`n$migration`nSELECT 'custom-ai-preserved',COUNT(*) FROM creature_template WHERE entry=99215 AND ScriptName='custom_guard';"
 $expected+=@("other-scripts`t0","other-ai`t0","custom-ai-preserved`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Worm support SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Worm support SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: deployed three spell handlers, three ally AI bindings, native scaling/spells/totem aura and kill objectives.'}
else{'PASS: migration twice in temporary tables, bindings/native data, unrelated scripts and custom AI preservation.'}

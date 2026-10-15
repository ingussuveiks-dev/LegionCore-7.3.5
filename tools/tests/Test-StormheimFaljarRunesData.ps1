param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-final-judgment"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_41_stormheim_faljar_runestones.sql" -Raw
$sql=@"
SELECT 'source-runes',COUNT(*) FROM creature WHERE id=97221 AND map=1220 AND guid IN(291988,291989);
SELECT 'source-point',guid,position_x,position_y,position_z,orientation FROM creature WHERE guid IN(291988,291989) ORDER BY guid;
SELECT 'proxy-spawns',COUNT(*) FROM creature WHERE id=97222;
SELECT 'transform',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=83693 AND w.Displayid1=61276 AND t.VehicleId=0;
"@
if(!$Live){$sql+=@"
CREATE TEMPORARY TABLE saved_template LIKE creature_template;
INSERT INTO saved_template SELECT * FROM creature_template WHERE entry IN(93093,93428,97221,97222,97223,117199,83693);
CREATE TEMPORARY TABLE creature_template LIKE saved_template;
INSERT INTO creature_template SELECT * FROM saved_template;
UPDATE creature_template SET ScriptName='' WHERE entry=97222;
CREATE TEMPORARY TABLE saved_addon LIKE creature_template_addon;
INSERT INTO saved_addon SELECT * FROM creature_template_addon WHERE entry IN(93093,93428,97221,97223,117199);
CREATE TEMPORARY TABLE creature_template_addon LIKE saved_addon;
INSERT INTO creature_template_addon SELECT * FROM saved_addon;
UPDATE creature_template_addon SET auras='209164 209363 89319' WHERE entry=93093;
CREATE TEMPORARY TABLE saved_spells LIKE spell_script_names;
INSERT INTO saved_spells SELECT * FROM spell_script_names WHERE spell_id IN(208547,209177,3365);
CREATE TEMPORARY TABLE spell_script_names LIKE saved_spells;
INSERT INTO spell_script_names SELECT * FROM saved_spells;
INSERT IGNORE INTO spell_script_names VALUES(209177,'independent_adapter');
CREATE TEMPORARY TABLE saved_text LIKE creature_text;
INSERT INTO saved_text SELECT * FROM creature_text WHERE CreatureID IN(93093,93428);
CREATE TEMPORARY TABLE creature_text LIKE saved_text;
INSERT INTO creature_text SELECT * FROM saved_text;
$migration
$migration
UPDATE saved_template SET ScriptName='npc_stormheim_faljar_rune' WHERE entry=97222;
UPDATE saved_addon SET auras='209164 89319' WHERE entry=93093;
SELECT 'template-actual',t.* FROM creature_template t ORDER BY entry;
SELECT 'template-expected',t.* FROM saved_template t ORDER BY entry;
SELECT 'addon-actual',a.* FROM creature_template_addon a ORDER BY entry;
SELECT 'addon-expected',a.* FROM saved_addon a ORDER BY entry;
SELECT 'text-actual',t.* FROM creature_text t WHERE COALESCE(comment,'')!='The Final Judgment owned sequence' ORDER BY CreatureID,GroupID,ID,Text;
SELECT 'text-expected',t.* FROM saved_text t WHERE COALESCE(comment,'')!='The Final Judgment owned sequence' ORDER BY CreatureID,GroupID,ID,Text;
SELECT 'adapter',COUNT(*) FROM spell_script_names WHERE spell_id=209177 AND ScriptName='independent_adapter';
"@}
$sql+=@"
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=97222 AND ScriptName='npc_stormheim_faljar_rune' AND AIName='' AND faction=35 AND unit_flags=0 AND minlevel=1 AND maxlevel=1;
SELECT 'spell-bindings',COUNT(*) FROM spell_script_names WHERE spell_id IN(208547,209177) AND ScriptName='spell_stormheim_faljar_bulwark';
SELECT 'source-pose',COUNT(*) FROM creature_template_addon WHERE entry=93093 AND auras='209164 89319';
SELECT 'new-texts',COUNT(*) FROM creature_text WHERE comment='The Final Judgment owned sequence';
SELECT 'voice',CreatureID,GroupID,BroadcastTextID,Sound FROM creature_text WHERE comment='The Final Judgment owned sequence' ORDER BY CreatureID,GroupID;
"@
if(!$Live){$sql+=@"
UPDATE creature_template SET ScriptName='custom_rune' WHERE entry=97222;
$migration
SELECT 'custom',ScriptName FROM creature_template WHERE entry=97222;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}|Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Faljar rune SQL failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("source-runes`t2","proxy-spawns`t0","transform`t1","source-point`t291988`t3630.62`t759.943`t38.7806`t5.05479","source-point`t291989`t3655.53`t784.203`t39.1235`t3.80961","binding`t1","spell-bindings`t2","source-pose`t1","new-texts`t7","voice`t93093`t10`t114753`t69871","voice`t93093`t12`t98472`t53054","voice`t93093`t13`t98473`t53055","voice`t93428`t10`t98468`t53050","voice`t93428`t11`t98462`t53045","voice`t93428`t12`t98466`t53048","voice`t93428`t13`t98471`t53053")){if($rows -notcontains $line){throw "Missing: $line"}}
if(!$Live){
 foreach($tag in @('template','addon','text')){$a="$tag-actual`t";$b="$tag-expected`t";$actual=@($rows|Where-Object {$_ -like "$a*"}|ForEach-Object {$_.Substring($a.Length)});$expected=@($rows|Where-Object {$_ -like "$b*"}|ForEach-Object {$_.Substring($b.Length)});if(!$actual.Count -or (Compare-Object $actual $expected)){throw "Unexpected $tag mutation"}}
 foreach($line in @("custom`tcustom_rune","adapter`t1")){if($rows -notcontains $line){throw "Missing: $line"}}
}
$rows|Set-Content "$out/runes-data$(if($Live){'-live'}).tsv"
'PASS: exact native rune anchors, scoped attack proxy, transform reference, seven native text/voice bindings; repeat migration/full template/addon/old-text preservation, custom binding and independent spell adapter.'

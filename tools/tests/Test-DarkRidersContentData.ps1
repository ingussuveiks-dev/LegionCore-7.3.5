param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migrations=@('2026_10_20_10_dark_riders_reveal_handoff.sql','2026_10_20_11_dark_riders_scythe_jump.sql','2026_10_20_12_dark_riders_ambient_motion.sql') | ForEach-Object {Get-Content "$repo/sql/updates/world/$_" -Raw}
$migration=$migrations -join "`n"
$setup=@'
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN(101286,101282);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(101286,101282);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
CREATE TEMPORARY TABLE saved_dest LIKE spell_target_position;
CREATE TEMPORARY TABLE spell_target_position LIKE saved_dest;
CREATE TEMPORARY TABLE saved_ct LIKE creature_template;
INSERT INTO saved_ct SELECT * FROM creature_template WHERE entry IN(100835,100932,100845);
CREATE TEMPORARY TABLE creature_template LIKE saved_ct;
INSERT INTO creature_template SELECT * FROM saved_ct;
CREATE TEMPORARY TABLE saved_move LIKE creature_template_movement;
INSERT INTO saved_move SELECT * FROM creature_template_movement WHERE CreatureId IN(100835,100932,101979);
CREATE TEMPORARY TABLE creature_template_movement LIKE saved_move;
INSERT INTO creature_template_movement SELECT * FROM saved_move;
'@
$select=@'
SELECT 'REQUEST',id,event_type,event_param1,event_param2,action_type,action_param1,link FROM smart_scripts WHERE entryorguid=101286 AND source_type=0 AND id=92;
SELECT 'ROUTE',SourceGroup,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceEntry=101286 AND SourceGroup IN(14,93) ORDER BY SourceGroup,ElseGroup;
SELECT 'DEST',id,target_map,ROUND(target_position_x,1),ROUND(target_position_y,2),ROUND(target_position_z,4) FROM spell_target_position WHERE id=199727;
SELECT 'MOTION',CreatureId,Ground,Flight,Rooted FROM creature_template_movement WHERE CreatureId IN(100835,100932) ORDER BY CreatureId;
SELECT 'SCRIPT',entry,AIName,ScriptName,unit_flags FROM creature_template WHERE entry IN(100835,100932) ORDER BY entry;
SELECT 'OBJECTIVE',ObjectID,StorageIndex FROM quest_objectives WHERE QuestID=40623 ORDER BY StorageIndex;
SELECT 'OTHER_CT',COUNT(*) FROM creature_template a JOIN saved_ct b USING(entry) WHERE a.entry=100845 AND a.AIName=b.AIName AND a.ScriptName=b.ScriptName AND a.unit_flags=b.unit_flags;
SELECT 'OTHER_MOVE',COUNT(*) FROM creature_template_movement a JOIN saved_move b USING(CreatureId) WHERE a.CreatureId=101979 AND a.Ground<=>b.Ground AND a.Flight<=>b.Flight AND a.Rooted<=>b.Rooted;
SELECT 'NATIVE_ROUTES',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.action_type=b.action_type AND a.action_param1=b.action_param1 AND a.link=b.link;
'@
$sql=$setup+"`n"+$migration+"`nSELECT 'FIRST';`n"+$select+"`n"+$migration+"`nSELECT 'SECOND';`n"+$select+"`nSELECT 'END';`nUPDATE spell_target_position SET target_position_x=123 WHERE id=199727;`n"+$migration+"`nSELECT 'CUSTOM_DEST',target_position_x FROM spell_target_position WHERE id=199727;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Content data SQL failed'}}finally{$env:MYSQL_PWD=$old}
$first=[Array]::IndexOf($rows,'FIRST');$second=[Array]::IndexOf($rows,'SECOND');$end=[Array]::IndexOf($rows,'END')
if(($rows[($first+1)..($second-1)] -join "`n") -ne ($rows[($second+1)..($end-1)] -join "`n")){throw 'Content migrations are not idempotent'}
foreach($expected in @("REQUEST`t92`t52`t1`t101286`t34`t10085003`t0","ROUTE`t14`t1`t1026`t3","ROUTE`t14`t1`t1014`t3","ROUTE`t93`t1`t988`t3","DEST`t199727`t1533`t-10964.0`t-1891.98`t-48.3921","MOTION`t100835`t0`t1`t0","MOTION`t100932`t0`t1`t0","SCRIPT`t100835`t`tnpc_dark_riders_ambient`t33587968","SCRIPT`t100932`t`tnpc_dark_riders_ambient`t33555200","OBJECTIVE`t103473`t0","OBJECTIVE`t100813`t3","OTHER_CT`t1","OTHER_MOVE`t1","CUSTOM_DEST`t123")){
 if($rows -notcontains $expected){throw "Content data mismatch: $expected"}
}
'PASS: repeated real-schema migrations; Warlock-only reveal request, native DK/druid routes, grounded jump destination, gravity/motion bindings, quest lookup IDs, native flags and unrelated/custom destination preservation'

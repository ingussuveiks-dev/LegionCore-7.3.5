$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_04_valsharah_rituals.sql" -Raw
$tables=@('smart_scripts','creature_template','spell_script_names')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'early-scene-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=91109 AND source_type=0 AND action_type=33 AND action_param1=92742;
SELECT 'scene-chain',COUNT(*) FROM smart_scripts WHERE entryorguid=91109 AND source_type=0 AND id=5 AND link=6 AND action_type=72;
SELECT 'scene-cast',COUNT(*) FROM smart_scripts WHERE entryorguid=91109 AND source_type=0 AND id=8 AND action_type=85 AND action_param1=197680;
SELECT 'scene-hook',COUNT(*) FROM spell_script_names WHERE spell_id=197680 AND ScriptName='spell_valsharah_summon_ysera_ritual';
SELECT 'ride-kept',COUNT(*) FROM smart_scripts WHERE entryorguid=91109 AND source_type=0 AND id=1 AND action_type=15 AND action_param1=40122;
SELECT 'escort-accepts',COUNT(*) FROM smart_scripts WHERE entryorguid=103022 AND source_type=0 AND event_type=19 AND event_param1 IN(38675,41724) AND action_type=85 AND action_param1=305067;
SELECT 'no-old-completion',COUNT(*) FROM smart_scripts WHERE (entryorguid=103022 AND action_type=33 AND action_param1=103022) OR (entryorguid=104646 AND event_type=6 AND action_type=45) OR (entryorguid IN(10473900,10473902,10578639,10302200) AND source_type=9);
SELECT 'meet-kept',COUNT(*) FROM smart_scripts WHERE entryorguid=104739 AND source_type=0 AND id=0 AND action_type=33 AND action_param1=104645;
SELECT 'scripts',COUNT(*) FROM creature_template WHERE (entry=103022 AND ScriptName='npc_valsharah_path_tyrande') OR (entry=104739 AND ScriptName='npc_valsharah_vigil_tyrande') OR (entry IN(104643,104644,104646) AND ScriptName='npc_valsharah_vigil_enemy');
SELECT 'waypoints',COUNT(*) FROM waypoints WHERE entry=103022 AND pointid BETWEEN 1 AND 6;
SELECT 'prerequisites',COUNT(*),COUNT(DISTINCT ElseGroup) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=38377 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(38148,38322,38323);
SELECT 'dialogue',COUNT(DISTINCT GroupID) FROM creature_text WHERE CreatureID=104739 AND GroupID IN(0,1,2,6,7,8);
SELECT 'destination',COUNT(*) FROM spell_target_position WHERE id=197487 AND target_map=1220 AND ABS(target_position_x-2606.18)<0.01 AND ABS(target_position_y-6725.01)<0.01;
SELECT 'unrelated',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(103022,104739,104643,104644,104646) AND (a.ScriptName<>b.ScriptName OR a.AIName<>b.AIName);
UPDATE creature_template SET ScriptName='custom_tyrande',AIName='custom_ai' WHERE entry=103022;
$migration
SELECT 'custom',ScriptName,AIName FROM creature_template WHERE entry=103022;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$fixture;if($LASTEXITCODE){throw 'Ritual SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$expected=@("early-scene-credit`t0","scene-chain`t1","scene-cast`t1","scene-hook`t1","ride-kept`t1","escort-accepts`t2","no-old-completion`t0","meet-kept`t1","scripts`t5","waypoints`t6","prerequisites`t3`t3","dialogue`t6","destination`t1","unrelated`t0","custom`tcustom_tyrande`tcustom_ai")
if(($rows -join '|') -ne ($expected -join '|')){throw "Migration mismatch: $($rows -join '|')"}
'PASS: actual migration twice; scene linkage and native completion destination, 40122 ride, both escort accepts, meeting objective, all three alternative returns, existing path/dialogue, custom and unrelated scripts preserved.'

$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
function Invoke-Fixture([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match "^${database}DatabaseInfo\s*="}).Split('"')[1].Split(';')
 $old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Finale SQL fixture failed'};return $rows}finally{$env:MYSQL_PWD=$old}
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_03_valsharah_finale.sql" -Raw
$tables=@('quest_start_scripts','quest_template_addon','smart_scripts','creature_template','spell_scene','conditions')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'autofinish',COUNT(*) FROM quest_start_scripts WHERE id IN(38687,41763) AND command=7;
SELECT 'flags',COUNT(*) FROM quest_template_addon WHERE ID IN(38687,41763) AND SpecialFlags&2;
SELECT 'shortcuts',COUNT(*) FROM smart_scripts WHERE (entryorguid=10472800 AND source_type=9) OR (entryorguid=104799 AND source_type=0 AND action_type=33);
SELECT 'scripts',COUNT(*) FROM creature_template WHERE (entry=104728 AND ScriptName='npc_valsharah_search_tyrande') OR (entry=104799 AND ScriptName='npc_valsharah_temple_departure') OR (entry IN(111258,111260,111259,111203,111198,111204) AND ScriptName='npc_valsharah_malfurion_search') OR (entry=93065 AND ScriptName='npc_valsharah_ysera_finale');
SELECT 'scene',COUNT(*) FROM spell_scene WHERE MiscValue=1246 AND SceneScriptPackageID=1594 AND ScriptName='scene_valsharah_choice';
SELECT 'clicks',COUNT(*) FROM npc_spellclick_spells WHERE (npc_entry IN(111258,111203) AND spell_id=221373) OR (npc_entry IN(111260,111198) AND spell_id=221466) OR (npc_entry IN(111259,111204) AND spell_id=221375);
SELECT 'accept-dialogue',COUNT(*) FROM smart_scripts WHERE entryorguid=104728 AND source_type=0 AND event_type=19 AND event_param1 IN(38687,41763) AND action_type=1;
SELECT 'active-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=93065 AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'faction-prerequisites',COUNT(*),COUNT(DISTINCT ElseGroup) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=38743 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(38687,41763);
SELECT 'unrelated',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(104728,104799,111258,111260,111259,111203,111198,111204,93065) AND (a.ScriptName<>b.ScriptName OR a.AIName<>b.AIName);
UPDATE creature_template SET ScriptName='custom_ysera',AIName='custom_ai' WHERE entry=93065;
UPDATE spell_scene SET ScriptName='custom_choice' WHERE MiscValue=1246;
$migration
SELECT 'custom-npc',ScriptName,AIName FROM creature_template WHERE entry=93065;
SELECT 'custom-scene',ScriptName FROM spell_scene WHERE MiscValue=1246;
"@
$rows=Invoke-Fixture World $fixture
$expected=@("autofinish`t0","flags`t0","shortcuts`t0","scripts`t9","scene`t1","clicks`t6","accept-dialogue`t2","active-scaling`t1","faction-prerequisites`t2`t2","unrelated`t0","custom-npc`tcustom_ysera`tcustom_ai","custom-scene`tcustom_choice")
if(($rows -join '|') -ne ($expected -join '|')){throw "World migration mismatch: $($rows -join '|')"}
$tables=@('character_queststatus','character_queststatus_objectives','character_queststatus_rewarded')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_;"}) -join "`n"
$fixture+=@"
INSERT INTO character_queststatus(guid,account,quest,status) VALUES (1,1,38687,1),(2,2,41763,3),(3,3,38743,1),(4,4,38743,1),(5,5,12345,1);
INSERT INTO character_queststatus_rewarded VALUES (4,4,38743);
INSERT INTO character_queststatus_objectives VALUES (1,1,38687,0,1),(1,1,38687,5,1),(2,2,41763,2,1),(3,3,38743,0,1),(3,3,38743,1,1),(4,4,38743,0,1),(5,5,12345,0,1);
"@
$fixture+=Get-Content "$repo/sql/updates/characters/2026_10_10_01_valsharah_finale_progress.sql" -Raw
$fixture+="SELECT guid,status FROM character_queststatus ORDER BY guid; SELECT guid,quest,objective,data FROM character_queststatus_objectives ORDER BY guid,objective;"
$rows=Invoke-Fixture Character $fixture
$expected=@("1`t3","2`t3","3`t3","4`t1","5`t1","3`t38743`t1`t1","4`t38743`t0`t1","5`t12345`t0`t1")
if(($rows -join '|') -ne ($expected -join '|')){throw "Character migration mismatch: $($rows -join '|')"}
'PASS: actual world migration twice, native actor/click bindings and active scaling; character shortcut repair retains real arrival, rewarded and unrelated quests.'

param([switch]$MigrationOnly)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/grey-shoals"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Grey Shoals SQL check failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_07_grey_shoals_native_supplies.sql" -Raw
$migrationRows=@(Query @"
CREATE TEMPORARY TABLE saved_go LIKE gameobject;
INSERT INTO saved_go SELECT * FROM gameobject WHERE guid BETWEEN 147040012 AND 147040015 OR guid IN(113990,115036);
CREATE TEMPORARY TABLE gameobject LIKE saved_go;
INSERT INTO gameobject SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_template LIKE gameobject_template;
INSERT INTO saved_template SELECT * FROM gameobject_template WHERE entry IN(241741,244704,266170,245924);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_template;
INSERT INTO gameobject_template SELECT * FROM saved_template;
UPDATE gameobject SET id=IF(guid IN(147040012,147040014),241741,244704) WHERE guid BETWEEN 147040012 AND 147040015;
UPDATE gameobject_template SET ScriptName='go_grey_shoals_supplies' WHERE entry IN(241741,244704);
UPDATE gameobject_template SET Data3=1,ScriptName='' WHERE entry=266170;
$migration
$migration
SELECT 'native-spawns',COUNT(*) FROM gameobject WHERE guid BETWEEN 147040012 AND 147040015 AND id=266170;
SELECT 'native-template',COUNT(*) FROM gameobject_template WHERE entry=266170 AND type=10 AND displayId=35575 AND Data10=229972 AND Data3=1000 AND ScriptName='go_grey_shoals_supplies';
SELECT 'old-hooks',COUNT(*) FROM gameobject_template WHERE entry IN(241741,244704) AND ScriptName<>'';
SELECT 'unrelated-spawns',COUNT(*) FROM gameobject g JOIN saved_go s ON g.guid=s.guid WHERE g.guid IN(113990,115036) AND g.id=s.id AND g.position_x=s.position_x AND g.position_y=s.position_y AND g.position_z=s.position_z;
SELECT 'positions',COUNT(*) FROM gameobject g JOIN saved_go s ON g.guid=s.guid WHERE g.guid BETWEEN 147040012 AND 147040015 AND g.position_x=s.position_x AND g.position_y=s.position_y AND g.position_z=s.position_z AND g.spawntimesecs=s.spawntimesecs;
SELECT 'unrelated-template',COUNT(*) FROM gameobject_template g JOIN saved_template s ON g.entry=s.entry WHERE g.entry=245924 AND g.ScriptName=s.ScriptName AND g.Data3=s.Data3 AND g.Data10=s.Data10;
"@)
foreach($expected in @("native-spawns`t4","native-template`t1","old-hooks`t0","unrelated-spawns`t2","positions`t4","unrelated-template`t1")){
 if($migrationRows -notcontains $expected){throw "Native supply repeated migration failed: $expected"}
}
'PASS: repeated native rack migration in temporary shadow tables; original positions, foreign spawns and unrelated template preserved.'
$rotationMigration=Get-Content "$repo/sql/updates/world/2026_10_18_08_grey_shoals_rotation.sql" -Raw
$rotationRows=@(Query @"
CREATE TEMPORARY TABLE saved_ct LIKE creature_template;
INSERT INTO saved_ct SELECT * FROM creature_template WHERE entry IN(115025,115026,115031,115751,115739);
CREATE TEMPORARY TABLE creature_template LIKE saved_ct;
INSERT INTO creature_template SELECT * FROM saved_ct;
UPDATE creature_template SET ScriptName='' WHERE entry IN(115025,115026,115031,115751);
$rotationMigration
$rotationMigration
SELECT 'rotation-bindings',COUNT(*) FROM creature_template WHERE entry IN(115025,115026,115031,115751) AND AIName='SmartAI' AND ScriptName='npc_grey_shoals';
SELECT 'credit-preserved',COUNT(*) FROM creature_template c JOIN saved_ct s ON c.entry=s.entry WHERE c.entry=115739 AND c.AIName=s.AIName AND c.ScriptName=s.ScriptName;
SELECT 'flags-preserved',COUNT(*) FROM creature_template c JOIN saved_ct s ON c.entry=s.entry WHERE c.entry IN(115025,115026,115031,115751) AND c.unit_flags=s.unit_flags AND c.unit_flags2=s.unit_flags2 AND c.faction=s.faction AND c.lootid=s.lootid;
UPDATE creature_template SET ScriptName='external_sentinel' WHERE entry=115025;
UPDATE creature_template SET AIName='EventAI',ScriptName='' WHERE entry=115026;
$rotationMigration
SELECT 'foreign-hook',COUNT(*) FROM creature_template WHERE entry=115025 AND ScriptName='external_sentinel';
SELECT 'other-ai',COUNT(*) FROM creature_template WHERE entry=115026 AND AIName='EventAI' AND ScriptName='';
"@)
foreach($expected in @("rotation-bindings`t4","credit-preserved`t1","flags-preserved`t4","foreign-hook`t1","other-ai`t1")){
 if($rotationRows -notcontains $expected){throw "Rotation migration failed: $expected"}
}
'PASS: repeated rotation binding migration retains SmartAI, faction/flags/loot and unrelated or externally bound templates.'
if($MigrationOnly){return}
$objectives=@(Query 'SELECT ID,Type,StorageIndex,ObjectID,Amount,Flags,TaskStep FROM quest_objectives WHERE QuestID=44737 ORDER BY ID;')
$expected=@("287220`t15`t-1`t0`t1`t0`t0","287221`t0`t1`t115025`t17`t92`t6","287222`t0`t2`t115031`t20`t92`t5","287223`t0`t3`t115026`t50`t92`t2","287226`t0`t0`t115739`t20`t92`t5","287238`t0`t4`t115751`t4`t92`t25")
if(($objectives -join ',') -ne ($expected -join ',')){throw 'Server objective mapping/weights differ from the native behavioral fixture'}
$objectives | Set-Content "$out/objectives.tsv"
$spawns=@(Query 'SELECT id,COUNT(*) FROM creature WHERE id IN(115025,115026,115031,115751) AND map=1220 AND areaId=8297 GROUP BY id ORDER BY id;')
if(($spawns -join ',') -ne "115025`t7,115026`t5,115031`t4,115751`t4"){throw 'Encounter spawn inventory changed; re-audit geometry and retail placement'}
if(@(Query 'SELECT guid FROM creature WHERE id IN(115025,115026,115031,115751) AND (map<>1220 OR areaId<>8297 OR modelid<>0);').Count){throw 'Unexpected actor placement/model override'}
Query 'SELECT guid,id,position_x,position_y,position_z,phaseMask,PhaseId,spawntimesecs FROM creature WHERE id IN(115025,115026,115031,115751) ORDER BY guid;' | Set-Content "$out/spawns.tsv"
$sai=@(Query "SELECT entryorguid,id FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0 AND (Difficulties<>'' OR (event_flags & 30)<>0 OR event_param1>event_param2 OR event_param3>event_param4);")
if($sai.Count){throw 'Combat timers/difficulty admission changed'}
Query 'SELECT entryorguid,id,action_param1,target_type FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0 ORDER BY entryorguid,id;' | Set-Content "$out/smart-spells.tsv"
$supplies=@(Query 'SELECT guid,id FROM gameobject WHERE guid BETWEEN 147040012 AND 147040015 AND map=1220 AND areaId=8297 ORDER BY guid;')
if(($supplies -join ',') -ne "147040012`t266170,147040013`t266170,147040014`t266170,147040015`t266170"){throw 'Native supply template binding changed'}
$supplies | Set-Content "$out/supplies.tsv"
Query 'SELECT entry,name,displayId,Data10,VerifiedBuild FROM gameobject_template WHERE entry IN(241741,244704,266170);' | Set-Content "$out/supply-evidence.tsv"
if(@(Query "SELECT entry FROM creature_template WHERE entry IN(115025,115026,115031,115751) AND AIName<>'SmartAI';").Count){throw 'Combat AI binding changed'}
if(@(Query "SELECT entry FROM creature_template WHERE entry IN(115025,115026,115031,115751) AND ScriptName<>'npc_grey_shoals';").Count){throw 'Missing live rotation AI script binding'}
'PASS: live objective weights, 20 actor spawns, no model overrides, combat timer/difficulty data, four native racks at reconstructed positions. Spawn authenticity/rotation and full native casts are separate audit findings.'

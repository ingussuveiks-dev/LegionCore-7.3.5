$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_09_16_azsuna_rescue.sql" -Raw
$tables=@('quest_template','quest_start_scripts','quest_template_addon','creature_template','spell_script_names','spell_scene','smart_scripts')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'start',StartScript FROM quest_template WHERE ID=37530;
SELECT 'bypass',COUNT(*) FROM quest_start_scripts WHERE id=37530 AND command=7 AND datalong=37530;
SELECT 'event',SpecialFlags & 2 FROM quest_template_addon WHERE ID=37530;
SELECT 'actors',COUNT(*) FROM creature_template WHERE ScriptName LIKE 'npc_azsuna_rescue_%';
SELECT 'buttons',spell1,spell2,spell3,VehicleId FROM creature_template WHERE entry=89089;
SELECT 'immune',SUM(unit_flags & 770) FROM creature_template WHERE entry IN(89116,89117);
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=179215 AND ScriptName='spell_azsuna_rescue_meteor';
SELECT 'scene',SceneScriptPackageID,ScriptName FROM spell_scene WHERE MiscValue=1148;
SELECT 'duplicate-reward',COUNT(*) FROM smart_scripts WHERE entryorguid=91403 AND source_type=0 AND event_type=20 AND event_param1=37470 AND action_param1=181404;
SELECT 'real-reward',RewardSpell FROM quest_template WHERE ID=37470;
SELECT 'unchanged-spawns',COUNT(*) FROM creature WHERE id=88855 AND map=1220;
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID IN(37530,37470);
SELECT 'unrelated-templates',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(89009,89089,89116,89117,91402) AND (a.ScriptName<>b.ScriptName OR a.unit_flags<>b.unit_flags OR a.spell1<>b.spell1);
UPDATE creature_template SET ScriptName='custom_farondis' WHERE entry=89089;
$migration
SELECT 'custom',ScriptName FROM creature_template WHERE entry=89089;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
 if($LASTEXITCODE){throw 'Rescue SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$expected=@("start`t0","bypass`t0","event`t0","actors`t5","buttons`t178784`t179215`t225948`t3972","immune`t0","binding`t1","scene`t1520`tscene_azsuna_rescue","duplicate-reward`t0","real-reward`t181404","unchanged-spawns`t1","objectives`t8","unrelated-templates`t0","custom`tcustom_farondis")
if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
'PASS: repeatable real migration in temporary tables; all bindings, native bar/scene, removal of completion bypass, original final quest kill and reward, unrelated templates and custom scripts preserved.'

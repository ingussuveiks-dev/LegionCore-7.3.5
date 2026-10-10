$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_02_valsharah_tyrande_handoff.sql" -Raw
$tables=@('spell_script_names','spell_scene','smart_scripts')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
DELETE FROM spell_script_names WHERE spell_id=206723 AND ScriptName='spell_valsharah_corruption_handoff';
UPDATE spell_scene SET ScriptName=NULL WHERE MiscValue=1350;
DELETE FROM smart_scripts WHERE entryorguid=102938 AND source_type=0 AND id=1;
$migration
$migration
SELECT 'spell',COUNT(*) FROM spell_script_names WHERE spell_id=206723 AND ScriptName='spell_valsharah_corruption_handoff';
SELECT 'scene',COUNT(*) FROM spell_scene WHERE MiscValue=1350 AND SceneScriptPackageID=1676 AND ScriptName='scene_valsharah_tyrande_handoff';
SELECT 'horde',COUNT(*) FROM smart_scripts WHERE entryorguid=102938 AND source_type=0 AND id=1 AND event_type=19 AND event_param1=41054 AND action_type=85 AND action_param1=203477 AND target_type=7;
SELECT 'alliance',COUNT(*) FROM smart_scripts WHERE entryorguid=102938 AND source_type=0 AND id=0 AND event_type=19 AND event_param1=41056 AND action_type=85 AND action_param1=203477 AND target_type=7;
SELECT 'unrelated',COUNT(*) FROM saved_spell_script_names s LEFT JOIN spell_script_names n ON n.spell_id=s.spell_id AND n.ScriptName=s.ScriptName WHERE s.spell_id<>206723 AND n.spell_id IS NULL;
UPDATE spell_scene SET ScriptName='custom_scene' WHERE MiscValue=1350;
UPDATE smart_scripts SET action_param1=12345 WHERE entryorguid=102938 AND source_type=0 AND id=1;
$migration
SELECT 'custom-scene',ScriptName FROM spell_scene WHERE MiscValue=1350;
SELECT 'custom-action',action_param1 FROM smart_scripts WHERE entryorguid=102938 AND source_type=0 AND id=1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$fixture;if($LASTEXITCODE){throw 'Valsharah handoff SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$expected=@("spell`t1","scene`t1","horde`t1","alliance`t1","unrelated`t0","custom-scene`tcustom_scene","custom-action`t12345")
if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
'PASS: actual handoff migration twice on temporary copies; native scene/spell bindings, both faction companion actions, unrelated and custom bindings preserved.'

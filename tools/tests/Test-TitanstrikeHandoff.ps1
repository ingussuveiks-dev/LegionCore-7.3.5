param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-handoff"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_23_titanstrike_aftermath_handoff.sql" -Raw
$original="INSERT INTO saved_smart(entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,action_param2,target_type,target_param1,comment) SELECT 106730,9,6,0,100,1,205,92,50997,21,100,'Pinned pre23 handoff' WHERE NOT EXISTS(SELECT 1 FROM saved_smart WHERE entryorguid=106730 AND source_type=9 AND id=6);"
$reset=@"
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET action_type=81 WHERE entryorguid=103154 AND source_type=0 AND id=3;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actors;
DELETE FROM spell_script_names;
DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM saved_conversation;
DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM saved_lines;
DELETE FROM spell_linked_spell; INSERT INTO spell_linked_spell SELECT * FROM saved_links;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(106730,103154);
$original
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_actors LIKE creature_template;
INSERT INTO saved_actors SELECT * FROM creature_template WHERE entry IN(106730,103154);
CREATE TEMPORARY TABLE creature_template LIKE saved_actors;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
CREATE TEMPORARY TABLE saved_conversation LIKE conversation_actor;
INSERT INTO saved_conversation SELECT * FROM conversation_actor WHERE entry=2167;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_conversation;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data WHERE entry=2167;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
CREATE TEMPORARY TABLE saved_links LIKE spell_linked_spell;
INSERT INTO saved_links SELECT * FROM spell_linked_spell WHERE spell_trigger=-211934;
CREATE TEMPORARY TABLE spell_linked_spell LIKE saved_links;
$reset
SELECT 'before-flag',action_type FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=3;
SELECT 'before-credit',action_type FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=6;
$migration
SELECT 'FIRST',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
$migration
SELECT 'SECOND',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'after-flag',action_type FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=3;
SELECT 'after-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=6;
SELECT 'binding',spell_id,ScriptName FROM spell_script_names;
SELECT 'native-pose',COUNT(*) FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=0 AND action_type=128 AND action_param1=8631;
SELECT 'native-aura',COUNT(*) FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=1 AND action_type=11 AND action_param1=211970 AND link=3;
SELECT 'native-actions',COUNT(*) FROM smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id) WHERE NOT(current.entryorguid=103154 AND current.source_type=0 AND current.id=3) AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.link=old.link AND current.event_type=old.event_type AND current.event_flags=old.event_flags;
SELECT 'expected-native-actions',COUNT(*)-2 FROM saved_smart;
"@
$variants=[ordered]@{
 'custom-hati'="UPDATE creature_template SET ScriptName='custom_hati' WHERE entry=103154;"
 'custom-prustaga'="UPDATE creature_template SET ScriptName='custom_prustaga' WHERE entry=106730;"
 'custom-spell'="INSERT INTO spell_script_names VALUES(211960,'custom_aftermath');"
 'mixed-state'='UPDATE smart_scripts SET action_type=83 WHERE entryorguid=103154 AND source_type=0 AND id=3;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=123 WHERE entryorguid=106730 AND source_type=9 AND id=6;'
 'custom-credit-target'='UPDATE smart_scripts SET target_param1=40 WHERE entryorguid=106730 AND source_type=9 AND id=6;'
 'custom-pose'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=103154 AND source_type=0 AND id=0;'
 'custom-aura'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=103154 AND source_type=0 AND id=1;'
 'custom-screen'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=106730 AND source_type=9 AND id=4;'
 'custom-data'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=106730 AND source_type=9 AND id=5;'
 'custom-death'='UPDATE smart_scripts SET action_type=1 WHERE entryorguid=106730 AND source_type=9 AND id=7;'
 'extra-action'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(106730,9,20,0,37,1,'Custom');"
 'short-conversation'='UPDATE conversation_actor SET duration=9400 WHERE entry=2167 AND id=0;'
 'custom-actor'='UPDATE conversation_actor SET creatureId=1 WHERE entry=2167 AND id=0;'
 'custom-line'='UPDATE conversation_data SET textId=1 WHERE entry=2167 AND idx=4;'
 'missing-line'='DELETE FROM conversation_data WHERE entry=2167 AND idx=4;'
 'custom-flag-chance'='UPDATE smart_scripts SET event_chance=50 WHERE entryorguid=103154 AND source_type=0 AND id=3;'
 'custom-credit-timer'='UPDATE smart_scripts SET event_param1=1000 WHERE entryorguid=106730 AND source_type=9 AND id=6;'
 'missing-remove-link'='DELETE FROM spell_linked_spell WHERE spell_trigger=-211934;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=6;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Handoff TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("before-flag`t81","before-credit`t205","after-flag`t83","after-credit`t0","binding`t211960`tspell_titanstrike_aftermath","native-pose`t1","native-aura`t1")){if($rows -notcontains $expected){throw "Handoff mismatch: $expected"}}
foreach($name in $variants.Keys){if($rows -notcontains "$name`t1"){throw "Custom handoff overwritten: $name"}}
$count=($rows | Where-Object {$_ -like "native-actions`t*"}).Split("`t")[1]
$expectedCount=($rows | Where-Object {$_ -like "expected-native-actions`t*"}).Split("`t")[1]
if($count -ne $expectedCount){throw 'Other native actions changed'}
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Handoff migration not idempotent'}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Handoff.inc",(Section $source 'namespace TitanstrikeAftermath' 'class spell_titanstrike_aftermath'))
$source=Get-Content "$repo/src/server/game/Entities/Object/QuestObjectScope.h" -Raw
[IO.File]::WriteAllText("$out/Scope.inc",(Section $source 'struct QuestObjectScope' '#endif'))
$source=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
[IO.File]::WriteAllText("$out/ObjectScope.inc",(Section $source 'void WorldObject::BindQuestObjectScope(' 'GameObject* WorldObject::SummonGameObject('))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/QuestDelay.inc",(Section $source 'void Player::AddQuestDelayedEvent(' 'void Player::BindQuestObject('))
[IO.File]::WriteAllText("$out/QuestBind.inc",(Section $source 'void Player::BindQuestObject(' 'void Player::CancelQuestObjects('))
$source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifetime.inc",(Section $source '    uint32 duration = 0;' '    if (!GetMap()->AddToMap(this))'))
[IO.File]::WriteAllText("$out/Playback.inc",(Section $source 'bool Conversation::CanNeverSee2(' 'bool Conversation::LoadFromDB('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/FlagAction.inc",(Section $source '        case SMART_ACTION_ADD_NPC_FLAG:' '        case SMART_ACTION_CROSS_CAST:'))
[IO.File]::WriteAllText("$out/CreditAction.inc",(Section $source '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TitanstrikeHandoffTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Hati handoff regression failed'}
if($VerifyReleaseBinary){$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_handoff_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical handoff PE verification failed'}}finally{$env:PYTHONPATH=$previous}}
"PASS: guarded handoff23 changes only early flag/50997 ownership; native pose/overcharge/list/death preserved, idempotent and$($variants.Count) custom variants retained"

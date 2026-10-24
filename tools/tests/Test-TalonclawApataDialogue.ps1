param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-apata-dialogue"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_28_talonclaw_apata_dialogue.sql" -Raw
$reset=@'
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET action_type=11 WHERE id=18;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM creature_text; INSERT INTO creature_text SELECT * FROM saved_text;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM waypoints; INSERT INTO waypoints SELECT * FROM saved_waypoints;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100235;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100235 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_text LIKE creature_text;
INSERT INTO saved_text SELECT * FROM creature_text WHERE CreatureID=100235;
CREATE TEMPORARY TABLE creature_text LIKE saved_text;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100235 AND SourceId=0;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_waypoints LIKE waypoints;
INSERT INTO saved_waypoints SELECT * FROM waypoints WHERE entry IN(100235,10023500,10023501);
CREATE TEMPORARY TABLE waypoints LIKE saved_waypoints;
$reset
SELECT 'BEFORE',smart_scripts.* FROM smart_scripts ORDER BY id,link;
$migration
SELECT 'FIRST',smart_scripts.* FROM smart_scripts ORDER BY id,link;
$migration
SELECT 'SECOND',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'event',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2 FROM smart_scripts WHERE id BETWEEN 15 AND 19 ORDER BY id;
SELECT 'unchanged-text',COUNT(*) FROM creature_text current JOIN saved_text old USING(CreatureID,GroupID,ID,Text) WHERE current.Type=old.Type AND current.Sound=old.Sound AND current.BroadcastTextID=old.BroadcastTextID AND current.Duration=old.Duration;
SELECT 'unchanged-conditions',COUNT(*) FROM conditions current JOIN saved_conditions old USING(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3);
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_apata';"
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-action'='UPDATE smart_scripts SET action_type=85 WHERE id=18;'
 'custom-group'='UPDATE smart_scripts SET action_param1=15 WHERE id=18;'
 'custom-duration'='UPDATE smart_scripts SET action_param2=9000 WHERE id=18;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE id=18;'
 'custom-link'='UPDATE smart_scripts SET link=19 WHERE id=17;'
 'custom-next'='UPDATE smart_scripts SET event_param1=12 WHERE id=19;'
 'custom-path'='UPDATE smart_scripts SET action_param2=1 WHERE id=17;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=18;'
 'custom-chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=18;'
 'custom-start'='UPDATE smart_scripts SET action_param2=1000 WHERE id=15;'
 'missing-text'='DELETE FROM creature_text WHERE GroupID=13;'
 'missing-waypoint'='DELETE FROM waypoints WHERE entry=10023501 AND pointid=7;'
 'custom-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,19,100235,9,1);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY id,link;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY id,link;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Apata temporary SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
$first=Snapshot 'FIRST';$second=Snapshot 'SECOND'
if($first.Count -ne 23 -or (Compare-Object $first $second)){throw 'Apata migration not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM smart_scripts;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'BEFORE'|ForEach-Object {$fields=$_.Split("`t");if($fields[$columns.IndexOf('id')] -eq '18'){$fields[$columns.IndexOf('action_type')]='1'};$fields -join "`t"})
if(Compare-Object $first $expected){throw 'Apata migration changed more than the one action opcode'}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom Apata overwritten: $name"}}
foreach($expected in @("unchanged-text`t16","unchanged-conditions`t3")){if($rows -notcontains $expected){throw "Apata preservation mismatch: $expected"}}
$events=@($rows|Where-Object {$_ -like "event`t*"}|ForEach-Object {$fields=$_.Substring(6).Split("`t");if($Baseline -and $fields[0] -eq '18'){$fields[6]='11'};$fields -join "`t"})
[IO.File]::WriteAllLines("$out/events.tsv",[string[]]$events)
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first) -replace '\be\b','nativeEvent'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$sections=@{
 'RepeatGuard'=@('    if ((e.event.event_phase_mask && !IsInPhase(','    ConditionList conds =')
 'TextOver'=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
 'DataSet'=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')
 'Die'=@('        case SMART_ACTION_DIE:','        case SMART_ACTION_SET_IN_COMBAT_WITH_ZONE:')
 'Talk'=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
 'Cast'=@('        case SMART_ACTION_CAST:','        case SMART_ACTION_CAST_CUSTOM:')
 'Waypoint'=@('        case SMART_ACTION_WP_START:','        case SMART_ACTION_WP_PAUSE:')
 'FindLink'=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
}
foreach($name in $sections.Keys){[IO.File]::WriteAllText("$out/$name.inc",(Section $source $sections[$name][0] $sections[$name][1]))}
$follow=Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('
[IO.File]::WriteAllText("$out/FollowLink.inc",$follow.Substring(0,$follow.LastIndexOf('}')))
$timer=Section $source '    if (mUseTextTimer && me)' 'void SmartScript::FillScript('
[IO.File]::WriteAllText("$out/TextTimer.inc",$timer.Substring(0,$timer.LastIndexOf('}')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawApataDialogueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/events.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Apata dialogue regression failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawApataCatalog.cpp -ProbeName TalonclawApataCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("SPELL`t13`tSwim Speed (TEST)","EFFECT`t13`t0`t6`t58`t0`t0`t1`t0","EFFECT`t197355`t0`t6`t430`t1143`t0`t18`t8")){if($native -notcontains $expected){throw "Apata native record changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_28_talonclaw_apata_dialogue.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_28_talonclaw_apata_dialogue.sql'; SELECT 'talk',action_type,action_param1,action_param2 FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND id=18 AND link=0;"
 foreach($expected in @("release`t$hash`tRELEASED","talk`t1`t13`t8000")){if($live -notcontains $expected){throw "Apata deployment mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_apata_dialogue_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Apata native Release consumers missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: Apata28 changes only CAST13 to TALK13, idempotent and$($variants.Count) custom cases preserved; native11/12/path13/14 and26972 Swim Speed identity verified, client speech/voice/route pending"

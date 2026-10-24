param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-explosion-warning"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_24_titanstrike_explosion_warning.sql" -Raw
$reset=@'
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET event_param1=5000,event_param2=5000 WHERE id=2;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM saved_actors;
DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM saved_lines;
DELETE FROM spell_script_names; INSERT INTO spell_script_names SELECT * FROM saved_bindings;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=106730 AND source_type=9;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=106730;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
INSERT INTO saved_actors SELECT * FROM conversation_actor WHERE entry=2147;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data WHERE entry=2147;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
INSERT INTO saved_bindings SELECT * FROM spell_script_names WHERE spell_id IN(211753,211960);
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
$reset
SELECT 'BEFORE',smart_scripts.* FROM smart_scripts ORDER BY id;
$migration
SELECT 'FIRST',smart_scripts.* FROM smart_scripts ORDER BY id;
$migration
SELECT 'SECOND',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'timer',event_param1,event_param2 FROM smart_scripts WHERE id=2;
SELECT 'native',COUNT(*) FROM smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id)
WHERE current.link=old.link AND current.event_type=old.event_type AND current.event_flags=old.event_flags
AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2
AND current.target_type=old.target_type AND current.target_param1=old.target_param1
AND (current.id=2 OR (current.event_param1=old.event_param1 AND current.event_param2=old.event_param2));
SELECT 'EVENT',id,event_param1,event_param2,action_type,IF(action_type=205,action_param2,action_param1) FROM smart_scripts ORDER BY id;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE creature_template SET ScriptName='custom_prustaga';"
 'custom-timer'='UPDATE smart_scripts SET event_param1=6000,event_param2=6000 WHERE id=2;'
 'ranged-timer'='UPDATE smart_scripts SET event_param2=6000 WHERE id=2;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE id=2;'
 'custom-cast'='UPDATE smart_scripts SET action_param2=2 WHERE id=2;'
 'custom-chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=2;'
 'custom-screen'='UPDATE smart_scripts SET event_param1=4000 WHERE id=4;'
 'custom-credit'='UPDATE smart_scripts SET action_param2=1 WHERE id=1;'
 'custom-aura'='UPDATE smart_scripts SET action_param1=1 WHERE id=3;'
 'custom-death'='UPDATE smart_scripts SET action_type=1 WHERE id=7;'
 'custom-conversation'='UPDATE conversation_actor SET duration=20000 WHERE id=0;'
 'custom-line'='UPDATE conversation_data SET textId=16000 WHERE idx=2;'
 'missing-line'='DELETE FROM conversation_data WHERE idx=2;'
 'custom-spell'="INSERT INTO spell_script_names VALUES(211753,'custom_finale');"
 'missing-handoff'='DELETE FROM spell_script_names WHERE spell_id=211960;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',event_param1 FROM smart_scripts WHERE id=2;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($query | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Warning regression SQL failed'};return $rows}finally{$env:MYSQL_PWD=$previous}
}
$rows=Query $sql
foreach($expected in @("timer`t15700`t15700","native`t7")){if($rows -notcontains $expected){throw "Warning migration mismatch: $expected"}}
foreach($name in $variants.Keys){$expected=if($name -eq 'custom-timer'){6000}else{5000};if($rows -notcontains "$name`t$expected"){throw "Custom warning definition overwritten: $name"}}
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Warning migration is not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM smart_scripts;' | ForEach-Object {$_.Split("`t")[0]})
$expectedRows=@($rows | Where-Object {$_ -like "BEFORE`t*"} | ForEach-Object {
 $fields=$_.Substring(7).Split("`t")
 if($fields[$columns.IndexOf('id')] -eq '2'){
  $fields[$columns.IndexOf('event_param1')]='15700'
  $fields[$columns.IndexOf('event_param2')]='15700'
 }
 $fields -join "`t"
})
if(Compare-Object $first $expectedRows){throw 'Warning migration changed fields beyond the two delay bounds'}
[IO.File]::WriteAllLines("$out/events.tsv",[string[]]@($rows | Where-Object {$_ -like "EVENT`t*"} | ForEach-Object {$_.Substring(6)}))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$start=$source.IndexOf('void SmartScript::UpdateTimer(');$end=$source.IndexOf('bool SmartScript::CheckTimer(',$start)
if($start -lt 0 -or $end -le $start){throw 'Native timer body missing'}
[IO.File]::WriteAllText("$out/Timer.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TitanstrikeExplosionWarningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/events.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native explosion-warning timer regression failed'}
& "$PSScriptRoot/Test-TitanstrikeFinaleConversation.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary -ClientPath $ClientPath *> "$out/native-conversation.log"
if($ClientPath){
 $audio=Get-Content "$repo/build-extractors/runtime-test/titanstrike-finale-conversation/audio-summary.json" -Raw | ConvertFrom-Json
 if($audio.RequiredMs -ne 20076 -or [Math]::Ceiling($audio.Payloads[2].DurationSeconds*1000)+15700 -ne 19076){throw 'Actual warning voice budget changed'}
}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_24_titanstrike_explosion_warning.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_24_titanstrike_explosion_warning.sql'; SELECT 'timer',event_param1,event_param2 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=2;"
 foreach($expected in @("release`t$hash`tRELEASED","timer`t15700`t15700")){if($live -notcontains $expected){throw "Warning not deployed: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_orestes_farewell_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native timer dispatcher verification failed'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: warning24 derives the one changed camera/overload delay from native4557 start15700; idempotent and$($variants.Count) custom variants retained; actual visibility/audio and failed conversation creation remain separate checks"

param([switch]$Baseline,[switch]$BaselineVoice,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-apata-death-warning"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_34_talonclaw_apata_death_warning.sql" -Raw
$reset=@'
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET action_param1=21 WHERE id=21;
DELETE FROM creature_text; INSERT INTO creature_text SELECT * FROM saved_text;
UPDATE creature_text SET BroadcastTextID=0 WHERE GroupID=20;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM creature_template_wdb; INSERT INTO creature_template_wdb SELECT * FROM saved_wdb;
DELETE FROM creature; INSERT INTO creature SELECT * FROM saved_spawns;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM spell_scene; INSERT INTO spell_scene SELECT * FROM saved_scene;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100235 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_text LIKE creature_text;
INSERT INTO saved_text SELECT * FROM creature_text WHERE CreatureID=100235;
CREATE TEMPORARY TABLE creature_text LIKE saved_text;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100235;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_wdb LIKE creature_template_wdb;
INSERT INTO saved_wdb SELECT * FROM creature_template_wdb WHERE Entry=100235;
CREATE TEMPORARY TABLE creature_template_wdb LIKE saved_wdb;
CREATE TEMPORARY TABLE saved_spawns LIKE creature;
INSERT INTO saved_spawns SELECT * FROM creature WHERE id=100235;
CREATE TEMPORARY TABLE creature LIKE saved_spawns;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=100235;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;
INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue=1145;
CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;
$reset
SELECT 'SMARTBEFORE',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'TEXTBEFORE',creature_text.* FROM creature_text ORDER BY GroupID,ID;
$migration
SELECT 'SMARTFIRST',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'TEXTFIRST',creature_text.* FROM creature_text ORDER BY GroupID,ID;
$migration
SELECT 'SMARTSECOND',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'TEXTSECOND',creature_text.* FROM creature_text ORDER BY GroupID,ID;
SELECT 'event',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2 FROM smart_scripts WHERE id IN(21,22) ORDER BY id;
SELECT 'voice',GroupID,Sound,BroadcastTextID,Type,0 FROM creature_text WHERE GroupID=20;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-script'="UPDATE creature_template SET ScriptName='custom_apata';"
 'custom-display'='UPDATE creature_template_wdb SET DisplayID1=1;'
 'extra-display'='UPDATE creature_template_wdb SET DisplayID2=1;'
 'spawn-override'='INSERT INTO creature(guid,id,map,modelid) VALUES(9999999,100235,1511,1);'
 'custom-text'="UPDATE creature_text SET Text='custom warning' WHERE GroupID=20;"
 'custom-sound'='UPDATE creature_text SET Sound=1 WHERE GroupID=20;'
 'custom-broadcast'='UPDATE creature_text SET BroadcastTextID=1 WHERE GroupID=20;'
 'custom-type'='UPDATE creature_text SET Type=12 WHERE GroupID=20;'
 'custom-emote'='UPDATE creature_text SET Emote=1 WHERE GroupID=20;'
 'custom-group'='UPDATE smart_scripts SET action_param1=18 WHERE id=21;'
 'custom-hold'='UPDATE smart_scripts SET action_param2=5000 WHERE id=21;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=21;'
 'custom-input'='UPDATE smart_scripts SET event_param2=5 WHERE id=21;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=21;'
 'custom-death'='UPDATE smart_scripts SET action_type=1 WHERE id=22;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=21;'
 'missing-line'='DELETE FROM creature_text WHERE GroupID=20;'
 'custom-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,22,100235,9,1);'
 'custom-package'='UPDATE spell_scene SET SceneScriptPackageID=1513;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-smart-before',smart_scripts.* FROM smart_scripts ORDER BY id,link;`nSELECT '$name-text-before',creature_text.* FROM creature_text ORDER BY GroupID,ID;`n$migration`nSELECT '$name-smart-after',smart_scripts.* FROM smart_scripts ORDER BY id,link;`nSELECT '$name-text-after',creature_text.* FROM creature_text ORDER BY GroupID,ID;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Warning TEMP SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
foreach($prefix in @('SMART','TEXT')){
 $columns=@(Query ('SHOW COLUMNS FROM '+$(if($prefix -eq 'SMART'){'smart_scripts'}else{'creature_text'})+';')|ForEach-Object {$_.Split("`t")[0]})
 $expected=@(Snapshot "${prefix}BEFORE"|ForEach-Object {$fields=$_.Split("`t");if($prefix -eq 'SMART'){if($fields[$columns.IndexOf('id')] -eq '21'){$fields[$columns.IndexOf('action_param1')]='20'}}elseif($fields[$columns.IndexOf('GroupID')] -eq '20'){$fields[$columns.IndexOf('BroadcastTextID')]='102115'};$fields -join "`t"})
 if(Compare-Object $expected (Snapshot "${prefix}FIRST")){throw "Warning changed unexpected $prefix fields"}
 if(Compare-Object (Snapshot "${prefix}FIRST") (Snapshot "${prefix}SECOND")){throw 'Warning migration not idempotent'}
 foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-$($prefix.ToLower())-before") (Snapshot "$name-$($prefix.ToLower())-after")){throw "Custom warning overwritten: $name/$prefix"}}
}
& "$PSScriptRoot/Test-TalonclawApataDialogue.ps1" *> "$out/related-dialogue.log"
if($LASTEXITCODE){throw 'Related native dialogue failed'}
$events=@(Snapshot 'event'|ForEach-Object {$fields=$_.Split("`t");if($Baseline -and $fields[0] -eq '21'){$fields[7]='21'};$fields -join "`t"})
[IO.File]::WriteAllLines("$out/events.tsv",[string[]]$events)
& "$repo/build-extractors/runtime-test/talonclaw-apata-dialogue/test.exe" "$out/events.tsv" death
if($LASTEXITCODE){throw 'Native warning/death replay failed'}
& "$PSScriptRoot/Test-TalonclawApataAdditionalVoice.ps1" -ClientPath $ClientPath *> "$out/related-voice.log"
if($LASTEXITCODE){throw 'Related native voice failed'}
$oldOut="$repo/build-extractors/runtime-test/talonclaw-apata-additional-voice"
$build=(Get-Content "$oldOut/build-native.cmd" -Raw).Replace($oldOut,$out).Replace('/DAPATA_ADDITIONAL_VOICE','/DAPATA_DEATH_WARNING')
Copy-Item -LiteralPath "$oldOut/SoundSelection.inc","$oldOut/CreatureTextEntry.inc" -Destination $out
[IO.File]::WriteAllText("$out/build-native.cmd",$build)
& "$out/build-native.cmd";if($LASTEXITCODE){throw 'Native warning voice build failed'}
$voice=@(Snapshot 'voice'|ForEach-Object {$fields=$_.Split("`t");if($BaselineVoice){$fields[2]='0'};$fields -join "`t"})
[IO.File]::WriteAllLines("$out/voice.tsv",[string[]]$voice)
$previous=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$previous";$native=@(& "$out/voice.exe" "$repo/build-extractors/bin/Release/dbc/enUS/" "$out/voice.tsv" "$repo/build-extractors/bin/Release/worldserver.conf");$native;[IO.File]::WriteAllLines("$out/native.tsv",[string[]]$native);if($LASTEXITCODE){throw 'Native warning voice selection failed'}}finally{$env:PATH=$previous}
if($ClientPath){
 $client=(Resolve-Path -LiteralPath $ClientPath).Path
 if(!(Get-Content "$client/.build.info"|Select-String '7\.3\.5\.26972')){throw 'Expected matching26972 client'}
 function IndexHashes{foreach($file in Get-ChildItem "$client/Data/data" -Filter '*.idx'|Sort-Object Name){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}}
 $before=@(IndexHashes);if(!$before.Count){throw 'Missing CASC indexes'}
 $asset="$out/1386266.audio"
 $files=@($native|Where-Object {$_ -like "FILE`t20`t62699`t*"});if($files.Count -ne 1){throw 'Missing native warning sound file'}
 $identity=[uint32]$files[0].Split("`t")[3];if($identity -ne 1386266){throw 'Native warning file changed'}
 try{& "$oldOut/CascAudioProbe.exe" $client ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw 'Native warning export failed'};& ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Warning full audio decode failed'};& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset|Set-Content "$out/audio-metadata.json";if($LASTEXITCODE){throw 'Warning audio metadata failed'}}finally{if(Compare-Object $before @(IndexHashes)){throw 'CASC indexes changed'}}
 Get-FileHash $asset;"PASS: full warning audio decode; $($before.Count) client indexes unchanged; immediate death/scene overlap still needs client observation"
}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_34_talonclaw_apata_death_warning.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_34_talonclaw_apata_death_warning.sql'; SELECT 'warning',action_param1 FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND id=21; SELECT 'voice',BroadcastTextID FROM creature_text WHERE CreatureID=100235 AND GroupID=20;"
 foreach($expected in @("release`t$hash`tRELEASED","warning`t20","voice`t102115")){if($live -notcontains $expected){throw "Live warning mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_apata_death_warning_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed speech/voice/death consumers missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: warning34 changes only world death TALK group and original warning broadcast, retains native cinematic farewell and20 custom variants; actual client timing pending"

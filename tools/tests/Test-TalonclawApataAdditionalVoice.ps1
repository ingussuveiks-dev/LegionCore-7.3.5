param([switch]$Baseline,[ValidateSet(0,2,21)][int]$BaselineGroup=0,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-apata-additional-voice"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_31_talonclaw_apata_additional_voice.sql" -Raw
$reset=@'
DELETE FROM creature_text; INSERT INTO creature_text SELECT * FROM saved_text;
UPDATE creature_text SET BroadcastTextID=CASE GroupID WHEN 2 THEN 65559 ELSE 0 END WHERE GroupID IN(2,21);
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM creature_template_wdb; INSERT INTO creature_template_wdb SELECT * FROM saved_wdb;
DELETE FROM creature_model_info; INSERT INTO creature_model_info SELECT * FROM saved_model;
DELETE FROM creature; INSERT INTO creature SELECT * FROM saved_spawns;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET action_param1=21 WHERE entryorguid=100235 AND source_type=0 AND id=21 AND action_type=1 AND action_param1=20 AND link=22;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_text LIKE creature_text;
INSERT INTO saved_text SELECT * FROM creature_text WHERE CreatureID=100235;
CREATE TEMPORARY TABLE creature_text LIKE saved_text;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100235;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_wdb LIKE creature_template_wdb;
INSERT INTO saved_wdb SELECT * FROM creature_template_wdb WHERE Entry=100235;
CREATE TEMPORARY TABLE creature_template_wdb LIKE saved_wdb;
CREATE TEMPORARY TABLE saved_model LIKE creature_model_info;
INSERT INTO saved_model SELECT * FROM creature_model_info WHERE DisplayID=66135;
CREATE TEMPORARY TABLE creature_model_info LIKE saved_model;
CREATE TEMPORARY TABLE saved_spawns LIKE creature;
INSERT INTO saved_spawns SELECT * FROM creature WHERE id=100235;
CREATE TEMPORARY TABLE creature LIKE saved_spawns;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100235 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
$reset
SELECT 'BEFORE',creature_text.* FROM creature_text ORDER BY GroupID,ID,Text;
SELECT 'SMARTBEFORE',smart_scripts.* FROM smart_scripts ORDER BY id,link;
$migration
SELECT 'FIRST',creature_text.* FROM creature_text ORDER BY GroupID,ID,Text;
$migration
SELECT 'SECOND',creature_text.* FROM creature_text ORDER BY GroupID,ID,Text;
SELECT 'SMARTAFTER',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'row',speech.GroupID,speech.Sound,speech.BroadcastTextID,speech.Type,
    (SELECT action_param2 FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND action_type=1 AND action_param1=speech.GroupID AND id IN(8,21))
FROM creature_text speech WHERE speech.GroupID IN(2,21) ORDER BY speech.GroupID;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-script'="UPDATE creature_template SET ScriptName='custom_apata';"
 'custom-display'='UPDATE creature_template_wdb SET DisplayID1=1;'
 'extra-display'='UPDATE creature_template_wdb SET DisplayID2=1;'
 'alternate-model'='UPDATE creature_model_info SET DisplayID_Other_Gender=1;'
 'spawn-override'='INSERT INTO creature(guid,id,map,modelid) VALUES(9999999,100235,1511,1);'
 'custom-broadcast'='UPDATE creature_text SET BroadcastTextID=1 WHERE GroupID=21;'
 'custom-sound'='UPDATE creature_text SET Sound=1 WHERE GroupID=2;'
 'custom-text'="UPDATE creature_text SET Text='custom farewell' WHERE GroupID=21;"
 'custom-type'='UPDATE creature_text SET Type=14 WHERE GroupID=21;'
 'custom-emote'='UPDATE creature_text SET Emote=1 WHERE GroupID=21;'
 'custom-language'='UPDATE creature_text SET Language=1 WHERE GroupID=2;'
 'missing-line'='DELETE FROM creature_text WHERE GroupID=21;'
 'first-hold'='UPDATE smart_scripts SET action_param2=3000 WHERE id=8;'
 'farewell-hold'='UPDATE smart_scripts SET action_param2=5000 WHERE id=21;'
 'farewell-link'='UPDATE smart_scripts SET link=0 WHERE id=21;'
 'farewell-input'='UPDATE smart_scripts SET event_param2=5 WHERE id=21;'
 'custom-death'='UPDATE smart_scripts SET action_type=1 WHERE id=22;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=21;'
 'repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=21;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',creature_text.* FROM creature_text ORDER BY GroupID,ID,Text;`n$migration`nSELECT '$name-after',creature_text.* FROM creature_text ORDER BY GroupID,ID,Text;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Additional Apata TEMP SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
$first=Snapshot 'FIRST';$second=Snapshot 'SECOND'
if($first.Count -ne 16 -or (Compare-Object $first $second)){throw 'Additional voice migration not idempotent'}
if(Compare-Object (Snapshot 'SMARTBEFORE') (Snapshot 'SMARTAFTER')){throw 'Additional voice migration changed SmartAI'}
$columns=@(Query 'SHOW COLUMNS FROM creature_text;'|ForEach-Object {$_.Split("`t")[0]})
$identities=@{2=102002;21=102116}
$expected=@(Snapshot 'BEFORE'|ForEach-Object {$fields=$_.Split("`t");$group=[int]$fields[$columns.IndexOf('GroupID')];if($identities.ContainsKey($group)){$fields[$columns.IndexOf('BroadcastTextID')]=[string]$identities[$group]};$fields -join "`t"})
if(Compare-Object $first $expected){throw 'Additional voice changed other fields'}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom additional voice overwritten: $name"}}
$data=@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$fields=$_.Substring(4).Split("`t");if($Baseline -and (!$BaselineGroup -or [int]$fields[0] -eq $BaselineGroup)){$fields[2]=if($fields[0] -eq '2'){'65559'}else{'0'}};$fields -join "`t"})
[IO.File]::WriteAllLines("$out/data.tsv",[string[]]$data)
$source=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
$start=$source.IndexOf('    uint32 fSound = [bct, sound, source, text]() -> uint32');$end=$source.IndexOf('    if (fSound)',$start)
if($start -lt 0 -or $end -le $start){throw 'Native sound selector missing'}
[IO.File]::WriteAllText("$out/SoundSelection.inc",$source.Substring($start,$end-$start))
$header=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.h" -Raw
$definition=[regex]::Match($header,'(?s)struct CreatureTextEntry\s*\{.*?\n\};');if(!$definition.Success){throw 'Native text definition missing'}
[IO.File]::WriteAllText("$out/CreatureTextEntry.inc",$definition.Value)
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$release=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
$includes=$release.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)
$libs=$release.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB','/DAPATA_ADDITIONAL_VOICE')
$compilerArgs+=($includes|ForEach-Object {'/I"'+$_+'"'});$compilerArgs+='"'+$repo+'/tools/tests/TalonclawApataVoice.cpp"';$compilerArgs+='/Fe"'+$out+'/voice.exe"';$compilerArgs+='/Fo"'+$out+'/voice.obj"';$compilerArgs+='/link';$compilerArgs+=($libs|ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build-native.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build-native.cmd";if($LASTEXITCODE){throw 'Additional voice native build failed'}
$previous=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$previous";$native=@(& "$out/voice.exe" "$repo/build-extractors/bin/Release/dbc/enUS/" "$out/data.tsv" "$repo/build-extractors/bin/Release/worldserver.conf");[IO.File]::WriteAllLines("$out/native.tsv",[string[]]$native);$native;if($LASTEXITCODE){throw 'Additional native voice selection failed'}}finally{$env:PATH=$previous}
if($ClientPath){
 $client=(Resolve-Path -LiteralPath $ClientPath).Path
 if(!(Get-Content "$client/.build.info"|Select-String '7\.3\.5\.26972')){throw 'Expected matching26972 client'}
 foreach($name in @('CascOpenStorage','CascReadFile')){$source=Get-Content "$repo/dep/CascLib/src/$name.cpp" -Raw;$boundary='STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE';if(!$source.Contains($boundary)){throw 'Read-only CASC boundary missing'};[IO.File]::WriteAllText("$out/${name}Audit.inc",$source.Replace($boundary,'STREAM_FLAG_READ_ONLY | STREAM_FLAG_WRITE_SHARE | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'))}
 [IO.File]::WriteAllText("$out/build-audio.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /MD /D__SYS_ZLIB /I`"$repo/dep/zlib`" /I`"$repo/dep/CascLib/src`" /I`"$out`" `"$repo/tools/research/CascAudioProbe.cpp`" /Fe`"$out/CascAudioProbe.exe`" /Fo`"$out/CascAudioProbe.obj`" /link `"$repo/build-extractors/dep/CascLib/Release/casc.lib`" `"$repo/build-extractors/dep/zlib/Release/zlib.lib`"`nexit /b %errorlevel%`n")
 & "$out/build-audio.cmd";if($LASTEXITCODE){throw 'Additional voice export build failed'}
 function IndexHashes{foreach($file in Get-ChildItem "$client/Data/data" -Filter '*.idx'|Sort-Object Name){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}}
 $before=@(IndexHashes);if(!$before.Count){throw 'Missing client indexes'};$payloads=@()
 try{foreach($line in $native|Where-Object {$_ -like "FILE`t*"}){$fields=$line.Split("`t");$group=[uint32]$fields[1];$identity=[uint32]$fields[3];$hold=[uint32]$fields[6];$asset="$out/$identity.audio";& "$out/CascAudioProbe.exe" $client ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw 'Additional voice export failed'};$probe=(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset)-join "`n";if($LASTEXITCODE){throw 'Additional voice metadata failed'};$metadata=$probe|ConvertFrom-Json;if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or $metadata.streams[0].channels -ne 1){throw 'Unexpected additional voice stream'};$seconds=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture);$milliseconds=[Math]::Ceiling($seconds*1000);if($milliseconds -le 0 -or ($hold -and $milliseconds -gt $hold)){throw 'Additional voice exceeds nonzero native hold'};& ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Additional voice full decode failed'};$payloads+=[pscustomobject]@{Group=$group;SoundKit=[uint32]$fields[2];FileDataID=$identity;DurationSeconds=$seconds;ExistingHoldMs=$hold;SHA256=(Get-FileHash $asset).Hash}}}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during additional voice review'}}
 if($payloads.Count -ne 2){throw 'Missing additional voice decode'}
 [ordered]@{Build='7.3.5.26972';StableIndexCount=$before.Count;Payloads=$payloads;ZeroHoldIsNotPlaybackProof=$true;ClientPlaybackVerified=$false}|ConvertTo-Json -Depth 6|Set-Content "$out/audio-summary.json"
 "PASS: two full native voice decodes; $($before.Count) client indexes unchanged. Farewell zero hold/instant death timing remains a client/lifecycle check."
}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_31_talonclaw_apata_additional_voice.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_31_talonclaw_apata_additional_voice.sql'; SELECT 'voice',GroupID,BroadcastTextID FROM creature_text WHERE CreatureID=100235 AND GroupID IN(2,21) ORDER BY GroupID;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Additional voice migration not released'}
 foreach($group in @(2,21)){if($live -notcontains "voice`t$group`t$($identities[$group])"){throw 'Live additional voice missing'}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_apata_voice_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native text/voice consumers missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: additional voice31 changes only two broadcast links; idempotent,$($variants.Count) custom variants preserved. Native selection/localized text verified, not complete scenario/death playback."

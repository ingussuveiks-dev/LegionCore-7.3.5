param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-finale-conversation"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_21_titanstrike_finale_conversation.sql" -Raw
$reset=@"
DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM saved_actors;
UPDATE conversation_actor SET duration=3550 WHERE entry=2147 AND id=0;
DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM saved_lines;
DELETE FROM conversation_creature; INSERT INTO conversation_creature SELECT * FROM saved_creatures;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
INSERT INTO saved_actors SELECT * FROM conversation_actor;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
CREATE TEMPORARY TABLE saved_creatures LIKE conversation_creature;
INSERT INTO saved_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE conversation_creature LIKE saved_creatures;
$reset
$migration
SELECT 'FIRST',conversation_actor.* FROM conversation_actor ORDER BY entry,id;
$migration
SELECT 'SECOND',conversation_actor.* FROM conversation_actor ORDER BY entry,id;
SELECT 'sum',SUM(duration)+5000 FROM conversation_actor WHERE entry=2147;
SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry=2147 ORDER BY id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=2147 ORDER BY idx;
SELECT 'unrelated-actors',COUNT(*) FROM conversation_actor current JOIN saved_actors old USING(entry,id) WHERE NOT(current.entry=2147 AND current.id=0) AND (current.actorId<>old.actorId OR current.creatureId<>old.creatureId OR current.displayId<>old.displayId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.unk3<>old.unk3 OR current.duration<>old.duration);
SELECT 'changed-lines',COUNT(*) FROM conversation_data current JOIN saved_lines old USING(entry,id) WHERE current.idx<>old.idx OR current.textId<>old.textId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2;
SELECT 'changed-creatures',COUNT(*) FROM conversation_creature current JOIN saved_creatures old USING(entry,id) WHERE current.creatureId<>old.creatureId OR current.creatureGuid<>old.creatureGuid OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.duration<>old.duration;
"@
$variants=[ordered]@{
 'custom-duration'='UPDATE conversation_actor SET duration=3551 WHERE entry=2147 AND id=0;'
 'custom-other-duration'='UPDATE conversation_actor SET duration=3600 WHERE entry=2147 AND id=1;'
 'custom-actor'='UPDATE conversation_actor SET actorId=1 WHERE entry=2147 AND id=1;'
 'custom-display'='UPDATE conversation_actor SET displayId=1 WHERE entry=2147 AND id=2;'
 'custom-flags'='UPDATE conversation_actor SET unk2=0 WHERE entry=2147 AND id=0;'
 'custom-line-time'='UPDATE conversation_data SET textId=15701 WHERE entry=2147 AND id=4557;'
 'custom-line-flags'='UPDATE conversation_data SET unk1=0 WHERE entry=2147 AND id=4556;'
 'missing-actor'='DELETE FROM conversation_actor WHERE entry=2147 AND id=2;'
 'missing-line'='DELETE FROM conversation_data WHERE entry=2147 AND id=4557;'
 'extra-actor'='INSERT INTO conversation_actor(entry,id) VALUES(2147,3);'
 'extra-line'='INSERT INTO conversation_data(entry,id,idx) VALUES(2147,999999,3);'
 'custom-creature'='INSERT INTO conversation_creature(entry,id,creatureId) VALUES(2147,0,106714);'
}
foreach($name in $variants.Keys){
 $sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',duration FROM conversation_actor WHERE entry=2147 AND id=0;`n"
}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Finale TEMP migration failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("sum`t27100","unrelated-actors`t0","changed-lines`t0","changed-creatures`t0")){
 if($rows -notcontains $expected){throw "Finale migration mismatch: $expected"}
}
foreach($name in $variants.Keys){$expected=if($name -eq 'custom-duration'){3551}else{3550};if($rows -notcontains "$name`t$expected"){throw "Custom definition overwritten: $name"}}
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Finale migration not idempotent'}
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@($rows | Where-Object {$_ -like "actor`t*" -or $_ -like "line`t*"}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
[IO.File]::WriteAllText("$out/ConversationStructs.inc",(Section $source 'struct ConversationActor' 'typedef std::unordered_map'))
$source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
[IO.File]::WriteAllText("$out/ConversationData.inc",$source.Substring($source.IndexOf('bool Conversation::GenerateDynamicFieldData(')))
[IO.File]::WriteAllText("$out/Lifetime.inc",(Section $source '    uint32 duration = 0;' '    if (!GetMap()->AddToMap(this))'))
[IO.File]::WriteAllText("$out/Update.inc",(Section $source 'void Conversation::Update(uint32 p_time)' 'void Conversation::Remove()'))
[IO.File]::WriteAllText("$out/Playback.inc",(Section $source 'bool Conversation::CanNeverSee2(' 'bool Conversation::LoadFromDB('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/TitanstrikeFinaleConversationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native finale conversation regression failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeFinaleCatalog.cpp -ProbeName TitanstrikeFinaleCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("LINE`t4555`t110338`t2000`t4556`t0`t0","LINE`t4556`t110339`t2000`t4557`t0`t0","LINE`t4557`t110340`t1000`t0`t0`t0","AUDIO_FILE`t62613`t1386063`t1`t1","AUDIO_FILE`t62614`t1386064`t1`t1","AUDIO_FILE`t62615`t1386065`t1`t1","CRITERION`t30125`t92`t50997`t0`t0`t0`t0`t0")){
 if($native -notcontains $expected){throw "Native finale mapping changed: $expected"}
}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_finale_conversation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical conversation lifetime PE verification failed'}}finally{$env:PYTHONPATH=$previous}
}
if($ClientPath){
 $client=(Resolve-Path -LiteralPath $ClientPath).Path
 if(!(Get-Content "$client/.build.info" | Select-String '7\.3\.5\.26972')){throw 'Expected matching26972 client'}
 foreach($name in @('CascOpenStorage','CascReadFile')){
  $source=Get-Content "$repo/dep/CascLib/src/$name.cpp" -Raw
  $boundary='STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'
  if(!$source.Contains($boundary)){throw "Missing read-only CASC boundary $name"}
  [IO.File]::WriteAllText("$out/${name}Audit.inc",$source.Replace($boundary,'STREAM_FLAG_READ_ONLY | STREAM_FLAG_WRITE_SHARE | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'))
 }
 [IO.File]::WriteAllText("$out/build-audio.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /MD /D__SYS_ZLIB /I`"$repo/dep/zlib`" /I`"$repo/dep/CascLib/src`" /I`"$out`" `"$repo/tools/research/CascAudioProbe.cpp`" /Fe`"$out/CascAudioProbe.exe`" /Fo`"$out/CascAudioProbe.obj`" /link `"$repo/build-extractors/dep/CascLib/Release/casc.lib`" `"$repo/build-extractors/dep/zlib/Release/zlib.lib`"`nexit /b %errorlevel%`n")
 & "$out/build-audio.cmd";if($LASTEXITCODE){throw 'Read-only CASC audio probe build failed'}
 function IndexHashes{
  $indexes=@(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx' | Sort-Object Name)
  if(!$indexes.Count){throw 'Missing client indexes'}
  foreach($file in $indexes){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}
 }
 $before=@(IndexHashes);$payloads=@();$ends=@();$sequential=0
 $identities=@(1386063,1386064,1386065);$starts=@(0,7800,15700);$additional=@(2000,2000,1000)
 try{for($index=0;$index -lt 3;++$index){
  $identity=$identities[$index];$asset="$out/$identity.audio"
  & "$out/CascAudioProbe.exe" $client ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw "CASC audio export failed: $identity"}
  $probe=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset);if($LASTEXITCODE){throw 'Audio metadata failed'}
  $metadata=($probe -join "`n") | ConvertFrom-Json
  if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or $metadata.streams[0].channels -ne 1){throw 'Unexpected voice stream'}
  $seconds=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if($seconds -le 0 -or $seconds -gt 20){throw 'Unexpected voice duration'}
  & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full native audio decode failed'}
  $voice=[Math]::Ceiling($seconds*1000);$ends+=$starts[$index]+$voice+$additional[$index];$sequential+=$voice+$additional[$index]
  $payloads+=[pscustomobject]@{FileDataID=$identity;DurationSeconds=$seconds;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash}
 }}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during audit'}}
 $required=[Math]::Max(($ends | Measure-Object -Maximum).Maximum,$sequential)
 [ordered]@{Build='7.3.5.26972';Conversation=2147;BaselineLifetimeMs=15650;RepairedLifetimeMs=27100;AbsoluteLineEndMs=$ends;SequentialLineEndMs=$sequential;RequiredMs=$required;NativePaddingMs=5000;StableIndexCount=$before.Count;Payloads=$payloads;ClientPlaybackVerified=$false} | ConvertTo-Json -Depth 6 | Set-Content "$out/audio-summary.json"
 if($required -ne 20076 -or $required+5000 -gt 27100){throw 'Native voice/gap budget changed; re-audit'}
 "PASS: three full native voice decodes, conservative ${required}ms line end +5000ms padding within27100ms; $($before.Count) CASC indexes unchanged; no playback claim"
}
'PASS: guarded2147 lifetime-only migration; twice idempotent,12 custom/missing variants preserved, native actors/lines unchanged'

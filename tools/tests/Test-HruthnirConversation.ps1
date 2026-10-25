param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'A baseline cannot verify installed data'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-conversation"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_02_hruthnir_conversation_budget.sql" -Raw
$reset=@'
DELETE FROM conversation_creature; INSERT INTO conversation_creature SELECT * FROM saved_creatures;
UPDATE conversation_creature SET duration=0 WHERE entry=497 AND id=0;
DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM saved_lines;
DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM saved_actors;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_creatures LIKE conversation_creature;
INSERT INTO saved_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE conversation_creature LIKE saved_creatures;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
INSERT INTO saved_actors SELECT * FROM conversation_actor;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
$reset
$migration
SELECT 'FIRST',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
$migration
SELECT 'SECOND',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
SELECT 'sum',SUM(duration)+5000 FROM conversation_creature WHERE entry=497;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=497 ORDER BY id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=497 ORDER BY idx;
SELECT 'unrelated-creatures',COUNT(*) FROM conversation_creature current JOIN saved_creatures old USING(entry,id) WHERE current.creatureId<>old.creatureId OR current.creatureGuid<>old.creatureGuid OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR (NOT(current.entry=497 AND current.id=0) AND current.duration<>old.duration);
SELECT 'changed-lines',COUNT(*) FROM conversation_data current JOIN saved_lines old USING(entry,id) WHERE current.idx<>old.idx OR current.textId<>old.textId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2;
SELECT 'changed-actors',COUNT(*) FROM conversation_actor current JOIN saved_actors old USING(entry,id) WHERE current.actorId<>old.actorId OR current.creatureId<>old.creatureId OR current.displayId<>old.displayId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.unk3<>old.unk3 OR current.duration<>old.duration;
SELECT 'row-counts',(SELECT COUNT(*) FROM conversation_creature)=(SELECT COUNT(*) FROM saved_creatures),(SELECT COUNT(*) FROM conversation_data)=(SELECT COUNT(*) FROM saved_lines),(SELECT COUNT(*) FROM conversation_actor)=(SELECT COUNT(*) FROM saved_actors);
"@
$variants=[ordered]@{
 'custom-duration'='UPDATE conversation_creature SET duration=68001 WHERE entry=497 AND id=0;'
 'custom-other-duration'='UPDATE conversation_creature SET duration=1 WHERE entry=497 AND id=1;'
 'custom-player-duration'='UPDATE conversation_creature SET duration=1 WHERE entry=497 AND id=2;'
 'custom-actor'='UPDATE conversation_creature SET creatureId=96469 WHERE entry=497 AND id=0;'
 'custom-guid'='UPDATE conversation_creature SET creatureGuid=1 WHERE entry=497 AND id=1;'
 'custom-player-guid'='UPDATE conversation_creature SET creatureGuid=1 WHERE entry=497 AND id=2;'
 'custom-flags'='UPDATE conversation_creature SET unk1=0 WHERE entry=497 AND id=0;'
 'custom-other-flags'='UPDATE conversation_creature SET unk2=0 WHERE entry=497 AND id=2;'
 'custom-line-time'='UPDATE conversation_data SET textId=40001 WHERE entry=497 AND id=1332;'
 'custom-line-index'='UPDATE conversation_data SET idx=10 WHERE entry=497 AND id=1334;'
 'custom-line-flags'='UPDATE conversation_data SET unk2=0 WHERE entry=497 AND id=1334;'
 'missing-creature'='DELETE FROM conversation_creature WHERE entry=497 AND id=1;'
 'missing-line'='DELETE FROM conversation_data WHERE entry=497 AND id=1334;'
 'extra-creature'='INSERT INTO conversation_creature(entry,id) VALUES(497,3);'
 'extra-line'='INSERT INTO conversation_data(entry,id,idx) VALUES(497,999999,10);'
 'model-definition'='INSERT INTO conversation_actor(entry,id) VALUES(497,0);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',duration FROM conversation_creature WHERE entry=497 AND id=0;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Hruthnir TEMP migration failed'}
 if($VerifyReleaseBinary){
  $live=@('SELECT id,duration FROM conversation_creature WHERE entry=497 ORDER BY id;' | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE -or (Compare-Object @("0`t68000","1`t0","2`t0") $live)){throw 'Canonical497 duration not deployed'}
 }
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("sum`t73000","unrelated-creatures`t0","changed-lines`t0","changed-actors`t0","row-counts`t1`t1`t1")){if($rows -notcontains $expected){throw "Migration mismatch: $expected"}}
foreach($name in $variants.Keys){$expected=if($name -eq 'custom-duration'){68001}else{0};if($rows -notcontains "$name`t$expected"){throw "Custom definition overwritten: $name"}}
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Hruthnir migration not idempotent'}
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@($rows | Where-Object {$_ -like "creature`t*" -or $_ -like "line`t*"}))
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
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/HruthnirConversationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Hruthnir conversation regression failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCommunionCatalog.cpp -ProbeName WarswordsCommunionCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
$voices=@(
 @(1324,98326,1325,0,500,1370060,60405,0),@(1325,98336,1331,7000,500,1370062,60407,0),
 @(1331,98799,1326,10000,500,1370063,60408,1),@(1326,98329,1327,12000,500,1370064,60409,0),
 @(1327,98327,1328,19000,0,1370065,60410,0),@(1328,98330,1329,22000,500,1370066,60411,0),
 @(1329,98331,1330,30000,500,1370067,60412,0),@(1330,98332,1332,32000,500,1370068,60413,0),
 @(1332,98800,1334,40000,500,1370069,60414,1)
)
foreach($voice in $voices){
 foreach($expected in @("LINE`t$($voice[0])`t$($voice[1])`t0`t$($voice[4])`t$($voice[2])`t0`t$($voice[7])`t60`t60","SOUND`t$($voice[1])`t$($voice[6])`t$($voice[5])")){if($native -notcontains $expected){throw "Native497 voice mapping differs: $expected"}}
}
foreach($expected in @("LINE`t1334`t0`t0`t0`t0`t0`t0`t60`t60","NONVERBAL`t1334")){if($native -notcontains $expected){throw 'Native blank terminal1334 differs'}}
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
 $before=@(IndexHashes);$payloads=@();$timings=@();$absolute=41000;$sequential=0
 try{foreach($voice in $voices){
  $identity=$voice[5];$asset="$out/$identity.audio"
  & "$out/CascAudioProbe.exe" $client ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw "CASC audio export failed: $identity"}
  $probe=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset);if($LASTEXITCODE){throw 'Audio metadata failed'}
  $metadata=($probe -join "`n") | ConvertFrom-Json
  if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or $metadata.streams[0].channels -ne 1 -or $metadata.streams[0].sample_rate -ne '22050'){throw 'Unexpected voice stream'}
  $seconds=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if($seconds -le 0 -or $seconds -gt 20){throw 'Unexpected voice duration'}
  & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full native voice decode failed'}
  $voiceMs=[Math]::Ceiling($seconds*1000);$end=$voice[3]+$voiceMs+$voice[4];$absolute=[Math]::Max($absolute,$end);$sequential+=$voiceMs+$voice[4]
  $timings+=[pscustomobject]@{Line=$voice[0];StartMs=$voice[3];VoiceMs=$voiceMs;AdditionalMs=$voice[4];AbsoluteEndMs=$end;SequentialEndMs=$sequential}
  $payloads+=[pscustomobject]@{FileDataID=$identity;DurationSeconds=$seconds;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash}
 }}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during audit'}}
 $required=[Math]::Max($absolute,$sequential)
 [ordered]@{Build='7.3.5.26972';Conversation=497;BaselineLifetimeMs=35000;RepairedLifetimeMs=73000;AbsoluteEndMs=$absolute;SequentialEndMs=$sequential;RequiredEndMs=$required;NativePaddingMs=5000;StableIndexCount=$before.Count;Lines=$timings;Payloads=$payloads;ClientPlaybackVerified=$false} | ConvertTo-Json -Depth 7 | Set-Content "$out/audio-summary.json"
 if($absolute -ne 46672 -or $sequential -ne 67992 -or $required+5000 -gt 73000){throw 'Native497 voice/gap budget changed; re-audit'}
 "PASS: nine full native497 voice decodes,46672ms absolute/67992ms conservative sequential +5000ms within73000ms; $($before.Count) CASC indexes unchanged, not client playback"
}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-HruthnirGossip.ps1" -VerifyReleaseBinary *> "$out/gossip.log"
 if(!$? -or $LASTEXITCODE){throw "Hruthnir cue regressions failed: $out/gossip.log"}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hruthnir_conversation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical497 lifetime producer/consumer verification failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: exact497 duration-only TEMP migration, twice idempotent;16 custom/missing variants preserved and unrelated definitions unchanged'

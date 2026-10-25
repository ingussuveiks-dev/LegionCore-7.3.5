param([ValidateSet(0,498,504)][int]$BaselineConversation=0,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
if($BaselineConversation -and $VerifyReleaseBinary){throw 'A baseline cannot verify installed data'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-HruthnirConversation.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary -ClientPath $ClientPath
if(!$? -or $LASTEXITCODE){throw 'Shared native conversation preparation failed'}
$common="$repo/build-extractors/runtime-test/hruthnir-conversation"
$out="$repo/build-extractors/runtime-test/magnar-conversations"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_03_magnar_conversation_budgets.sql" -Raw
$reset=@'
DELETE FROM conversation_creature; INSERT INTO conversation_creature SELECT * FROM saved_creatures;
UPDATE conversation_creature SET duration=IF(entry=498,4300,7550) WHERE entry IN(498,504) AND id=0;
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
SELECT 'sum',entry,SUM(duration)+5000 FROM conversation_creature WHERE entry IN(498,504) GROUP BY entry;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry IN(498,504) ORDER BY entry,id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(498,504) ORDER BY entry,idx;
SELECT 'unrelated-creatures',COUNT(*) FROM conversation_creature current JOIN saved_creatures old USING(entry,id) WHERE current.creatureId<>old.creatureId OR current.creatureGuid<>old.creatureGuid OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR (NOT(current.entry IN(498,504) AND current.id=0) AND current.duration<>old.duration);
SELECT 'changed-lines',COUNT(*) FROM conversation_data current JOIN saved_lines old USING(entry,id) WHERE current.idx<>old.idx OR current.textId<>old.textId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2;
SELECT 'changed-actors',COUNT(*) FROM conversation_actor current JOIN saved_actors old USING(entry,id) WHERE current.actorId<>old.actorId OR current.creatureId<>old.creatureId OR current.displayId<>old.displayId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.unk3<>old.unk3 OR current.duration<>old.duration;
SELECT 'row-counts',(SELECT COUNT(*) FROM conversation_creature)=(SELECT COUNT(*) FROM saved_creatures),(SELECT COUNT(*) FROM conversation_data)=(SELECT COUNT(*) FROM saved_lines),(SELECT COUNT(*) FROM conversation_actor)=(SELECT COUNT(*) FROM saved_actors);
"@
$variants=[ordered]@{
 'custom-duration'='UPDATE conversation_creature SET duration=1 WHERE entry={0} AND id=0;'
 'custom-other-duration'='UPDATE conversation_creature SET duration=1 WHERE entry={0} AND id=1;'
 'custom-actor'='UPDATE conversation_creature SET creatureId=96469 WHERE entry={0} AND id=0;'
 'custom-guid'='UPDATE conversation_creature SET creatureGuid=1 WHERE entry={0} AND id=1;'
 'custom-flags'='UPDATE conversation_creature SET unk1=0 WHERE entry={0} AND id=0;'
 'custom-other-flags'='UPDATE conversation_creature SET unk2=0 WHERE entry={0} AND id=1;'
 'custom-line-time'='UPDATE conversation_data SET textId=1 WHERE entry={0} AND idx=2;'
 'custom-line-index'='UPDATE conversation_data SET idx=10 WHERE entry={0} AND idx=2;'
 'custom-line-flags'='UPDATE conversation_data SET unk1=1 WHERE entry={0} AND idx=0;'
 'missing-creature'='DELETE FROM conversation_creature WHERE entry={0} AND id=1;'
 'missing-line'='DELETE FROM conversation_data WHERE entry={0} AND idx=2;'
 'extra-creature'='INSERT INTO conversation_creature(entry,id) VALUES({0},3);'
 'extra-line'='INSERT INTO conversation_data(entry,id,idx) VALUES({0},999999,10);'
 'model-definition'='INSERT INTO conversation_actor(entry,id) VALUES({0},0);'
}
foreach($identity in @(498,504)){foreach($name in $variants.Keys){
 $other=if($identity -eq 498){504}else{498}
 $sql+="$reset`n$($variants[$name] -f $identity)`n$migration`nSELECT '$identity-$name',duration FROM conversation_creature WHERE entry=$identity AND id=0; SELECT '$identity-$name-other',duration FROM conversation_creature WHERE entry=$other AND id=0;`n"
}}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Magnar TEMP migration failed'}
 if($VerifyReleaseBinary){
  $live=@('SELECT entry,id,duration FROM conversation_creature WHERE entry IN(498,504) ORDER BY entry,id;' | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE -or (Compare-Object @("498`t0`t26000","498`t1`t4300","504`t0`t42000","504`t1`t7550") $live)){throw 'Canonical498/504 duration not deployed'}
 }
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("sum`t498`t35300","sum`t504`t54550","unrelated-creatures`t0","changed-lines`t0","changed-actors`t0","row-counts`t1`t1`t1")){if($rows -notcontains $expected){throw "Migration mismatch: $expected"}}
foreach($identity in @(498,504)){foreach($name in $variants.Keys){
 $expected=if($name -eq 'custom-duration'){1}elseif($identity -eq 498){4300}else{7550}
 $other=if($identity -eq 498){42000}else{26000}
 if($rows -notcontains "$identity-$name`t$expected" -or $rows -notcontains "$identity-$name-other`t$other"){throw "Custom/independent guard failed: $identity-$name"}
}}
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Magnar migration not idempotent'}
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@($rows | Where-Object {$_ -like "creature`t*" -or $_ -like "line`t*"}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($BaselineConversation){"/DBASELINE=$BaselineConversation"}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$common`" `"$PSScriptRoot/MagnarConversationsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Magnar conversation regression failed'}
$native=Get-Content "$common/native.tsv"
$voices=@(
 @(498,1335,98333,1336,0,500,1370070,60415,1),@(498,1336,98337,1337,2350,500,1370071,60416,0),@(498,1337,98334,0,7300,500,1370072,60417,1),
 @(504,1341,98797,1342,0,1000,1370073,60418,1),@(504,1342,98795,1343,3950,1000,1370074,60419,0),@(504,1343,98798,1344,12900,1000,1370075,60420,1),@(504,1344,98338,0,21350,1000,1370076,60421,0)
)
foreach($voice in $voices){foreach($expected in @("LINE`t$($voice[1])`t$($voice[2])`t0`t$($voice[5])`t$($voice[3])`t0`t$($voice[8])`t60`t60","SOUND`t$($voice[2])`t$($voice[7])`t$($voice[6])")){if($native -notcontains $expected){throw "Native Magnar voice mapping differs: $expected"}}}
if($ClientPath){
 $client=(Resolve-Path -LiteralPath $ClientPath).Path
 function IndexHashes{
  $indexes=@(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx' | Sort-Object Name)
  if(!$indexes.Count){throw 'Missing client indexes'}
  foreach($file in $indexes){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}
 }
 $before=@(IndexHashes);$payloads=@();$budgets=@()
 try{foreach($identity in @(498,504)){
  $absolute=0;$sequential=0;$timings=@()
  foreach($voice in @($voices|Where-Object {$_[0] -eq $identity})){
   $file=$voice[6];$asset="$out/$file.audio"
   & "$common/CascAudioProbe.exe" $client ('File{0:X8}' -f $file) $asset;if($LASTEXITCODE){throw "CASC voice export failed: $file"}
   $probe=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset);if($LASTEXITCODE){throw 'Voice metadata failed'}
   $metadata=($probe -join "`n") | ConvertFrom-Json
   if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or $metadata.streams[0].channels -ne 1 -or $metadata.streams[0].sample_rate -ne '22050'){throw 'Unexpected voice stream'}
   $seconds=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture)
   if($seconds -le 0 -or $seconds -gt 20){throw 'Unexpected voice duration'}
   & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full native voice decode failed'}
   $voiceMs=[Math]::Ceiling($seconds*1000);$end=$voice[4]+$voiceMs+$voice[5];$absolute=[Math]::Max($absolute,$end);$sequential+=$voiceMs+$voice[5]
   $timings+=[pscustomobject]@{Line=$voice[1];StartMs=$voice[4];VoiceMs=$voiceMs;AdditionalMs=$voice[5];AbsoluteEndMs=$end;SequentialEndMs=$sequential}
   $payloads+=[pscustomobject]@{FileDataID=$file;DurationSeconds=$seconds;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash}
  }
  $required=[Math]::Max($absolute,$sequential);$deadline=if($identity -eq 498){35300}else{54550};$expected=if($identity -eq 498){29533}else{48286};$expectedAbsolute=if($identity -eq 498){16903}else{35572}
  if($absolute -ne $expectedAbsolute -or $sequential -ne $expected -or $required+5000 -gt $deadline){throw 'Native Magnar budget changed; re-audit'}
  $budgets+=[pscustomobject]@{Conversation=$identity;AbsoluteEndMs=$absolute;SequentialEndMs=$sequential;RequiredEndMs=$required;RepairedDeadlineMs=$deadline;Lines=$timings}
 }}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during audit'}}
 [ordered]@{Build='7.3.5.26972';NativePaddingMs=5000;StableIndexCount=$before.Count;Budgets=$budgets;Payloads=$payloads;ClientPlaybackVerified=$false;ActorDespawnRepaired=$false} | ConvertTo-Json -Depth 7 | Set-Content "$out/audio-summary.json"
 "PASS: seven full native498/504 voice decodes;29533/48286ms conservative voice/gap ends +5000ms retained; $($before.Count) CASC indexes unchanged, not client playback or actor-lifetime repair"
}
'PASS: independent exact498/504 duration-only TEMP migration, twice idempotent;28 custom/missing variants preserved while the other definition repairs normally; unrelated497/other definitions unchanged'

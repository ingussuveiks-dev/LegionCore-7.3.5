param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-TitanstrikeFinaleConversation.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary -ClientPath $ClientPath
$common="$repo/build-extractors/runtime-test/titanstrike-finale-conversation"
$out="$repo/build-extractors/runtime-test/titanstrike-aftermath"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_22_titanstrike_aftermath_conversations.sql" -Raw
$reset=@"
DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM saved_actors;
UPDATE conversation_actor SET duration=IF(entry=2167,9400,3700) WHERE entry IN(2167,2173) AND id=0;
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
SELECT 'sum',entry,SUM(duration)+5000 FROM conversation_actor WHERE entry IN(2167,2173) GROUP BY entry;
SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry IN(2167,2173) ORDER BY entry,id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(2167,2173) ORDER BY entry,idx;
SELECT 'unrelated-actors',COUNT(*) FROM conversation_actor current JOIN saved_actors old USING(entry,id) WHERE NOT(current.entry IN(2167,2173) AND current.id=0) AND (current.actorId<>old.actorId OR current.creatureId<>old.creatureId OR current.displayId<>old.displayId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.unk3<>old.unk3 OR current.duration<>old.duration);
SELECT 'changed-lines',COUNT(*) FROM conversation_data current JOIN saved_lines old USING(entry,id) WHERE current.idx<>old.idx OR current.textId<>old.textId OR current.unk1<>old.unk1 OR current.unk2<>old.unk2;
SELECT 'changed-creatures',COUNT(*) FROM conversation_creature current JOIN saved_creatures old USING(entry,id) WHERE current.creatureId<>old.creatureId OR current.creatureGuid<>old.creatureGuid OR current.unk1<>old.unk1 OR current.unk2<>old.unk2 OR current.duration<>old.duration;
SELECT 'remove-link',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=-211934 AND spell_effect=211960 AND type=0 AND caster=0 AND target=0 AND actiontype=0 AND removeMask=0 AND effectMask=0;
"@
$variants=[ordered]@{
 'custom-duration'='UPDATE conversation_actor SET duration=9000 WHERE entry={0} AND id=0;'
 'custom-other-duration'='UPDATE conversation_actor SET duration=9000 WHERE entry={0} AND id=1;'
 'custom-actor'='UPDATE conversation_actor SET actorId=1 WHERE entry={0} AND id=1;'
 'custom-display'='UPDATE conversation_actor SET displayId=1 WHERE entry={0} AND id=0;'
 'custom-flags'='UPDATE conversation_actor SET unk2=0 WHERE entry={0} AND id=0;'
 'custom-line-time'='UPDATE conversation_data SET textId=1 WHERE entry={0} AND idx=4;'
 'custom-line-flags'='UPDATE conversation_data SET unk1=0 WHERE entry={0} AND idx=1;'
 'missing-actor'='DELETE FROM conversation_actor WHERE entry={0} AND id=1;'
 'missing-line'='DELETE FROM conversation_data WHERE entry={0} AND idx=4;'
 'extra-actor'='INSERT INTO conversation_actor(entry,id) VALUES({0},10);'
 'extra-line'='INSERT INTO conversation_data(entry,id,idx) VALUES({0},999999,10);'
 'custom-creature'='INSERT INTO conversation_creature(entry,id,creatureId) VALUES({0},0,106714);'
}
foreach($identity in @(2167,2173)){foreach($name in $variants.Keys){
 $other=if($identity -eq 2167){2173}else{2167}
 $sql+="$reset`n$($variants[$name] -f $identity)`n$migration`nSELECT '$identity-$name',duration FROM conversation_actor WHERE entry=$identity AND id=0; SELECT '$identity-$name-other',duration FROM conversation_actor WHERE entry=$other AND id=0;`n"
}}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Aftermath TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("sum`t2167`t73800","sum`t2173`t63700","unrelated-actors`t0","changed-lines`t0","changed-creatures`t0","remove-link`t1")){if($rows -notcontains $expected){throw "Aftermath mismatch: $expected"}}
foreach($identity in @(2167,2173)){foreach($name in $variants.Keys){
 $expected=if($name -eq 'custom-duration'){9000}elseif($identity -eq 2167){9400}else{3700}
 $other=if($identity -eq 2167){55000}else{50000}
 if($rows -notcontains "$identity-$name`t$expected" -or $rows -notcontains "$identity-$name-other`t$other"){throw "Custom/independent conversation guard failed: $identity-$name"}
}}
$first=@($rows | Where-Object {$_ -like "FIRST`t*"} | ForEach-Object {$_.Substring(6)})
$second=@($rows | Where-Object {$_ -like "SECOND`t*"} | ForEach-Object {$_.Substring(7)})
if(Compare-Object $first $second){throw 'Aftermath migration not idempotent'}
$definitions=@($rows | Where-Object {$_ -like "actor`t*" -or $_ -like "line`t*"})
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]$definitions)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$common`" `"$PSScriptRoot/TitanstrikeAftermathTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/conversation.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native aftermath conversation regression failed'}
$native=Get-Content "$common/native.tsv"
foreach($expected in @("EFFECT`t211960`t0`t219`t0`t2167`t0`t18`t0","EFFECT`t212050`t0`t219`t0`t2173`t0`t18`t0","LINE`t4826`t110970`t2000`t0`t0`t0","LINE`t4621`t111005`t1000`t0`t0`t0")){if($native -notcontains $expected){throw "Native aftermath creator/end changed: $expected"}}
if($ClientPath){
 $client=(Resolve-Path -LiteralPath $ClientPath).Path
 function IndexHashes{
  $indexes=@(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx' | Sort-Object Name);if(!$indexes.Count){throw 'Missing client indexes'}
  foreach($file in $indexes){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}
 }
 $lines=@{};$texts=@{};$files=@{}
 foreach($row in $native){$fields=$row.Split("`t");switch($fields[0]){
  'LINE'{$lines[[uint32]$fields[1]]=@{Broadcast=[uint32]$fields[2];Additional=[uint32]$fields[3]}}
  'TEXT'{$texts[[uint32]$fields[1]]=@([uint32]$fields[2],[uint32]$fields[3]) | Where-Object {$_}}
  'AUDIO_FILE'{$kit=[uint32]$fields[1];if(!$files.ContainsKey($kit)){$files[$kit]=@()};$files[$kit]+=[uint32]$fields[2]}
 }}
 $identities=@(1386075,1386076,1386077,1386078,1386080,1386081,1386082,1386083,1386084,1386085)
 $before=@(IndexHashes);$payloads=@{}
 try{foreach($identity in $identities){
  $asset="$out/$identity.audio";& "$common/CascAudioProbe.exe" $client ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw 'Native aftermath CASC export failed'}
  $probe=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset);if($LASTEXITCODE){throw 'Audio metadata failed'}
  $metadata=($probe -join "`n") | ConvertFrom-Json
  if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or $metadata.streams[0].channels -ne 1){throw 'Unexpected aftermath voice stream'}
  $seconds=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture);if($seconds -le 0 -or $seconds -gt 20){throw 'Unexpected voice duration'}
  & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full aftermath voice decode failed'}
  $payloads[[uint32]$identity]=[pscustomobject]@{FileDataID=$identity;VoiceMs=[Math]::Ceiling($seconds*1000);DurationSeconds=$seconds;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash}
 }}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during aftermath review'}}
 $budgets=@();$mappedFiles=@()
 foreach($identity in @(2167,2173)){
  $sequential=0;$absolute=0;$lifetime=5000;$timings=@()
  foreach($row in $definitions){$fields=$row.Split("`t");if([uint32]$fields[1] -ne $identity){continue};if($fields[0] -eq 'actor'){$lifetime+=[uint32]$fields[9];continue}
   $line=$lines[[uint32]$fields[2]];if(!$line){throw 'Missing native line'};$voice=0
   foreach($kit in $texts[$line.Broadcast]){foreach($file in $files[$kit]){if(!$payloads.ContainsKey($file)){throw 'Unexpected native voice mapping'};$mappedFiles+=$file;$voice=[Math]::Max($voice,$payloads[$file].VoiceMs)}}
   if(!$voice){throw 'Missing aftermath voice'}
   $end=[uint32]$fields[4]+$voice+$line.Additional;$absolute=[Math]::Max($absolute,$end);$sequential+=$voice+$line.Additional
   $timings+=[pscustomobject]@{Line=[uint32]$fields[2];Start=[uint32]$fields[4];VoiceMs=$voice;AdditionalMs=$line.Additional;AbsoluteEndMs=$end;SequentialEndMs=$sequential}
  }
  $required=[Math]::Max($absolute,$sequential);$expected=if($identity -eq 2167){67398}else{56364}
  if($required -ne $expected -or $lifetime -lt $required+5000){throw 'Aftermath native voice/padding budget failed'}
  $budgets+=[pscustomobject]@{Conversation=$identity;LifetimeMs=$lifetime;AbsoluteEndMs=$absolute;SequentialEndMs=$sequential;RequiredEndMs=$required;Lines=$timings}
 }
 if(@($mappedFiles | Sort-Object -Unique).Count -ne 10){throw 'Incomplete ten-voice mapping'}
 [ordered]@{Build='7.3.5.26972';NativePaddingMs=5000;StableIndexCount=$before.Count;Budgets=$budgets;Payloads=@($payloads.Values);ClientPlaybackVerified=$false} | ConvertTo-Json -Depth 7 | Set-Content "$out/audio-summary.json"
 'PASS: ten complete native aftermath voices decoded; absolute/sequential ends67398/56364 plus native5s padding retained;32-index stability checked; no client playback claim'
}
'PASS: guarded2167/2173 lifetime-only migration, twice idempotent;24 custom/missing variants preserve each definition and independently repair the other; no line/creature/unrelated actor change'

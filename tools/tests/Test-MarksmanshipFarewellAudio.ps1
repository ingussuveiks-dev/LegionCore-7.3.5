param([Parameter(Mandatory)][string]$ClientPath,[string]$ConversationPath='',[ValidateSet(1072,1033,1556)][uint32]$ConversationId=1072)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-farewell-audio"
if($ConversationId -ne 1072){$out="$repo/build-extractors/runtime-test/marksmanship-conversation-audio-$ConversationId"}
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipFarewell.ps1"
if(!$ConversationPath){
 $ConversationPath="$out/live-conversation.tsv"
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previousPassword=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $query="SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=$ConversationId ORDER BY id; SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry=$ConversationId ORDER BY id; SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=$ConversationId ORDER BY idx;"
  $rows=@($query | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Read-only live conversation data query failed'}
  [IO.File]::WriteAllLines($ConversationPath,[string[]]$rows)
 }finally{$env:MYSQL_PWD=$previousPassword}
}
$old=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MarksmanshipDialogueCatalog.cpp -ProbeName MarksmanshipDialogueCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
$native=Get-Content "$out/native.tsv"
$lines=@{};$broadcasts=@{};$files=@{}
foreach($row in $native){$fields=$row.Split("`t");switch($fields[0]){
 'CONVERSATION_LINE'{$lines[[uint32]$fields[1]]=@{Broadcast=[uint32]$fields[2];Additional=[uint32]$fields[5]}}
 'BROADCAST'{$broadcasts[[uint32]$fields[1]]=@([uint32]$fields[2],[uint32]$fields[3]) | Where-Object {$_}}
 'AUDIO_FILE'{$kit=[uint32]$fields[1];if(!$files.ContainsKey($kit)){$files[$kit]=@()};$files[$kit]+=[uint32]$fields[2]}
}}
$definitions=Get-Content $ConversationPath | ForEach-Object {$fields=$_.Split("`t");if($fields[0] -eq 'line'){[pscustomobject]@{Line=[uint32]$fields[2];Index=[uint32]$fields[3];Start=[uint32]$fields[4]}}} | Sort-Object Index
$expectedLines=@{1072='2625,2630,2658,2659,3427,2631';1033='2539,2541,2543';1556='3422'}
$expectedIndexes=@{1072='0,1,2,3,4,5';1033='0,1,2';1556='0'}
if(($definitions.Line -join ',') -ne $expectedLines[[int]$ConversationId] -or ($definitions.Index -join ',') -ne $expectedIndexes[[int]$ConversationId]){throw 'Native conversation line order changed; re-audit'}
if($ConversationId -eq 1072 -and ($lines[[uint32]3427].Broadcast -ne 0 -or $lines[[uint32]3427].Additional -ne 5000)){throw 'Native silent player marker changed; re-audit'}
$identities=@($definitions | ForEach-Object {$entry=$lines[$_.Line];if($entry.Broadcast){foreach($kit in $broadcasts[$entry.Broadcast]){$files[$kit]}}} | Sort-Object -Unique)
$expectedFileCount=@{1072=5;1033=3;1556=1}[[int]$ConversationId]
if($identities.Count -ne $expectedFileCount){throw 'Unexpected native conversation voice-file count'}
@{referenced_file_ids=$identities}|ConvertTo-Json|Set-Content "$out/files.json"
& "$repo/tools/research/Test-CascVisualFiles.ps1" -ClientPath $ClientPath -ReportPaths "$out/files.json" -OutputDirectory $out
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$command="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /MD /D__SYS_ZLIB /I`"$repo/dep/zlib`" /I`"$repo/dep/CascLib/src`" /I`"$out`" `"$repo/tools/research/CascAudioProbe.cpp`" /Fe`"$out/CascAudioProbe.exe`" /Fo`"$out/CascAudioProbe.obj`" /link `"$repo/build-extractors/dep/CascLib/Release/casc.lib`" `"$repo/build-extractors/dep/zlib/Release/zlib.lib`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/build-audio.cmd",$command);& "$out/build-audio.cmd";if($LASTEXITCODE){throw 'Read-only audio probe build failed'}
function IndexHashes{
 $indexes=@(Get-ChildItem -LiteralPath "$ClientPath/Data/data" -Filter '*.idx' | Sort-Object Name);if(!$indexes.Count){throw 'No client indexes'}
 foreach($file in $indexes){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}
}
$before=@(IndexHashes);$payloads=@{}
try{foreach($identity in $identities){
 $asset="$out/$identity.audio";& "$out/CascAudioProbe.exe" $ClientPath ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw "Audio read failed: $identity"}
 $metadata=@(& ffprobe -v error -show_entries 'format=duration,format_name:stream=codec_name,sample_rate,channels' -of json $asset);if($LASTEXITCODE){throw "Audio probe failed: $identity"}
 $parsed=($metadata -join "`n")|ConvertFrom-Json
 if(@($parsed.streams).Count -ne 1 -or $parsed.streams[0].codec_name -ne 'vorbis' -or $parsed.streams[0].channels -ne 1){throw 'Unexpected native voice stream'}
 $duration=[double]::Parse($parsed.format.duration,[Globalization.CultureInfo]::InvariantCulture);if($duration -le 0 -or $duration -gt 20){throw 'Unexpected voice length'}
 & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw "Full audio decode failed: $identity"}
 $payloads[$identity]=[pscustomobject]@{FileDataID=$identity;DurationSeconds=$duration;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash}
}}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during audio review'}}
$sequential=0
$timings=@(foreach($definition in $definitions){$entry=$lines[$definition.Line];$voice=0
 if($entry.Broadcast){foreach($kit in $broadcasts[$entry.Broadcast]){foreach($identity in $files[$kit]){$voice=[Math]::Max($voice,[Math]::Ceiling(1000*$payloads[$identity].DurationSeconds))}}}
 $sequentialStart=$sequential;$sequential+=$voice+$entry.Additional
 [pscustomobject]@{Line=$definition.Line;StartMs=$definition.Start;VoiceMs=$voice;AdditionalMs=$entry.Additional;EndMs=$definition.Start+$voice+$entry.Additional;SequentialStartMs=$sequentialStart;SequentialEndMs=$sequential}
})
$actorDuration=0;$creatureDuration=0;$actorCount=0;$creatureCount=0
foreach($row in Get-Content $ConversationPath){$fields=$row.Split("`t");if([uint32]$fields[1] -ne $ConversationId){throw 'Mixed conversation fixture'};switch($fields[0]){
 'creature'{$creatureDuration += [uint32]$fields[7];++$creatureCount}
 'actor'{$actorDuration += [uint32]$fields[9];++$actorCount}
 'line'{}
 default{throw 'Unexpected conversation fixture row'}
}}
if(($ConversationId -eq 1072 -and ($actorCount -ne 0 -or $creatureCount -ne 3)) -or ($ConversationId -eq 1033 -and ($actorCount -ne 2 -or $creatureCount -ne 0)) -or ($ConversationId -eq 1556 -and ($actorCount -ne 1 -or $creatureCount -ne 0))){throw 'Native actor/creature branch changed; re-audit'}
$duration=if($actorCount){$actorDuration}else{$creatureDuration}
if(!$duration){$duration=30000}
$duration+=5000
$absoluteRequired=($timings|Measure-Object EndMs -Maximum).Maximum
$required=[Math]::Max($absoluteRequired,$sequential)
[ordered]@{Build='7.3.5.26972';ConversationId=$ConversationId;LifetimeMs=$duration;AbsoluteRequiredLineEndMs=$absoluteRequired;SequentialRequiredLineEndMs=$sequential;RequiredLineEndMs=$required;NativePaddingMs=5000;BudgetSatisfied=($duration -ge $required+5000);StableIndexCount=$before.Count;Lines=$timings;Payloads=@($payloads.Values);ClientPlaybackVerified=$false}|ConvertTo-Json -Depth 8|Set-Content "$out/summary.json"
if($duration -lt $required+5000){throw "Conversation$ConversationId budget failed: lifetime${duration}ms < conservative voice/gap${required}ms + native5000ms; absolute-offset-only check is insufficient"}
if($ConversationId -eq 1072 -and $required -le 12950){throw 'Baseline truncation was not reproduced'}
$timings|Format-Table
"PASS: conversation$ConversationId, $expectedFileCount native voice payloads fully decoded, conservative absolute/sequential line end ${required}ms +5000ms retained by ${duration}ms lifetime; $($before.Count) indexes unchanged. No client playback claim."

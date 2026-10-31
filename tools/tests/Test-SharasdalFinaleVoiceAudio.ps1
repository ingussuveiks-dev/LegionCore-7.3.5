param([string]$AudioDirectory='',[string]$DataDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$AudioDirectory){$AudioDirectory="$repo/build-extractors/runtime-test/sharasdal-finale-audio2"}
if(!$DataDirectory){$DataDirectory="$repo/build-extractors/runtime-test/sharasdal-finale-voice-data"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-finale-voice-audio"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$fixture=Get-Content "$PSScriptRoot/SharasdalFinaleVoiceAudio.json" -Raw|ConvertFrom-Json
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalFinaleAudioCatalog.cpp -ProbeName SharasdalFinaleAudioCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native finale audio catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Encoding UTF8
$expected=@();foreach($row in $fixture.Files){$expected+="AUDIO_FILE`t$($row[0])`t$($row[1])"};if(Compare-Object $expected @($native|Where-Object {$_ -like "AUDIO_FILE`t*"})){throw 'Native sound/file mapping differs'}
$expected=@();foreach($row in $fixture.Lines){$expected+="LINE`t$($row -join "`t")"};if(Compare-Object $expected @($native|Where-Object {$_ -like "LINE`t*"})){throw 'Native line/BCT/additional/next chain differs'}
foreach($row in $fixture.Texts){$text=@($native|Where-Object {$_ -like "TEXT`t$($row[0])`t*"});if($text.Count -ne 1){throw 'Native voice broadcast ambiguous'};$f=$text[0].Split("`t");if($f.Count -ne 6 -or [uint32]$f[4] -ne $row[1] -or [uint32]$f[5] -ne $row[2]){throw 'Native male/female voice selection differs'}}
$voices=@{};$decoded=@()
foreach($payload in $fixture.Payloads){
 $file=Join-Path $AudioDirectory "$($payload.FileDataID).audio"
 if((Get-Item $file).Length -ne $payload.Bytes -or (Get-FileHash $file).Hash -cne $payload.SHA256){throw "Native finale voice payload differs: $($payload.FileDataID)"}
 $raw=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,channels' -of json $file);if($LASTEXITCODE){throw 'Finale native voice metadata failed'};$meta=($raw -join "`n")|ConvertFrom-Json
 if(@($meta.streams).Count -ne 1 -or $meta.streams[0].codec_name -ne 'vorbis' -or $meta.streams[0].channels -notin @(1,2)){throw 'Unexpected native finale audio format'}
 $seconds=[double]::Parse($meta.format.duration,[Globalization.CultureInfo]::InvariantCulture);$ms=[long][Math]::Ceiling(1000*$seconds)
 if($seconds -le 0 -or $seconds -gt 60 -or $ms -ne $payload.VoiceMs){throw 'Native finale voice duration differs'}
 & ffmpeg -nostdin -v error -i $file -f null -;if($LASTEXITCODE){throw 'Full native finale voice decode failed'}
 $voices[[uint32]$payload.FileDataID]=$ms;$decoded+=$payload
}
$old=Get-Content "$DataDirectory/baseline-conversation.tsv";$new=Get-Content "$DataDirectory/conversation.tsv";$summaries=@()
foreach($context in $fixture.Conversations){
 $entry=$context.Conversation;$oldLines=@($old|Where-Object {$_ -like "line`t$entry`t*"});$newLines=@($new|Where-Object {$_ -like "line`t$entry`t*"})
 if($oldLines.Count -ne $context.Lines.Count -or (Compare-Object $oldLines $newLines)){throw 'Finale native line payload/offset mutation'}
 $sequential=[long]0;$timings=@(foreach($row in $oldLines){
  $f=$row.Split("`t");if($f.Count -ne 7){throw 'Native line fixture layout changed'}
  $line=@($fixture.Lines|Where-Object {$_[0] -eq [uint32]$f[2]});if($line.Count -ne 1){throw 'Native audio line ambiguous'};$l=$line[0]
  $text=@($fixture.Texts|Where-Object {$_[0] -eq $l[1]});if($text.Count -ne 1){throw 'Native audio broadcast ambiguous'};$voice=[long]0
  foreach($kit in @($text[0][1],$text[0][2])|Where-Object {$_}){foreach($track in @($fixture.Files|Where-Object {$_[0] -eq $kit})){$voice=[Math]::Max($voice,$voices[[uint32]$track[1]])}}
  if(!$voice){throw 'Missing actual hashed native voice'};$additional=[long]$l[2];$sequential+=$voice+$additional
  [pscustomobject]@{Line=[uint32]$f[2];OffsetMs=[uint32]$f[3];VoiceMs=$voice;AdditionalMs=$additional;AbsoluteEndMs=[long]$f[3]+$voice+$additional;SequentialEndMs=$sequential}
 })
 $absolute=[long]($timings|Measure-Object AbsoluteEndMs -Maximum).Maximum;$required=[long][Math]::Max($absolute,$sequential)
 $oldSum=[long]0;$newSum=[long]0
 foreach($pair in @(@($old,'old'),@($new,'new'))){$actors=@($pair[0]|Where-Object {$_ -like "creature`t$entry`t*"});if($actors.Count -ne 3){throw 'Missing native three-actor payload'};foreach($actor in $actors){$f=$actor.Split("`t");if($f.Count -ne 8){throw 'Native creature fixture schema changed'};if($pair[1] -eq 'old'){$oldSum+=[long]$f[7]}else{$newSum+=[long]$f[7]}}}
 if($oldSum -ne $context.OriginalActorSumMs -or $oldSum+5000 -ge $required -or $required -ne $context.RequiredActorSumMs -or $absolute -ne $context.AbsoluteLineEndMs -or $sequential -ne $context.SequentialLineEndMs -or $newSum -ne $required){throw 'Finale original counterfactual/repaired voice budget differs'}
 $summaries+=[pscustomobject]@{Conversation=$entry;OriginalLifetimeMs=$oldSum+5000;RepairedLifetimeMs=$newSum+5000;RequiredLineEndMs=$required;Lines=$timings}
}
[ordered]@{Build=$fixture.Build;Conversations=$summaries;Payloads=$decoded;ClientPlaybackVerified=$false;Limitation='Native hashed full Vorbis decode and conservative absolute/sequential lifetime; no listening, actual client scheduling, overlap-free delivery, caller/defeat/pickup or full1066 claim.'}|ConvertTo-Json -Depth 10|Set-Content "$out/summary.json"
'PASS ten hashed matching26972 Vorbis payloads fully decoded, fresh native BCT/kit/file/gender/9-line chain, original offsets retained. COUNTERFACTUAL1906 13433ms /1923 18800ms truncate; repaired28788/28727ms cover required23788/23727ms plus native5000. Client playback/caller/full chain remain separate.'

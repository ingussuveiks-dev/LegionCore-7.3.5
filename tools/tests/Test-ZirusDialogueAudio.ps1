param([string]$AudioDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$AudioDirectory){$AudioDirectory="$repo/build-extractors/runtime-test/zirus-dialogue-audio"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/zirus-dialogue-audio-check"}
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $fixture=Get-Content "$PSScriptRoot/ZirusDialogueAudio.json" -Raw|ConvertFrom-Json
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ZirusDialogueCatalog.cpp -ProbeName ZirusDialogueCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native opening voice catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv";$payloads=@();$voiceRows=@();$budgets=@()
 foreach($context in $fixture.Conversations){$end=0;foreach($line in $context.Lines){
  foreach($expected in @("LINE`t$($line.Line)`t$($line.Broadcast)`t$($line.AdditionalMs)`t$($line.Next)","AUDIO_FILE`t$($line.SoundKit)`t$($line.FileDataID)")){if($native -notcontains $expected){throw 'Native line/BCT/kit/file identity changed'}}
  $broadcast=@($native|Where-Object {$_ -like "TEXT`t$($line.Broadcast)`t*"});if($broadcast.Count -ne 1){throw 'Native broadcast ambiguous'};$sounds=$broadcast[0].Split("`t")[-2..-1]|ForEach-Object {[uint32]$_}|Where-Object {$_};if(@($sounds).Count -ne 1 -or $sounds[0] -ne $line.SoundKit){throw 'Native male/female sound identity changed'}
  if($line.StartMs -ne $end){throw 'Cumulative source timing differs'};$end+=[long]$line.VoiceMs+[long]$line.AdditionalMs
  $voiceRows+="$($line.Line)`t$($line.VoiceMs)`t$($line.AdditionalMs)"
 };if($context.RequiredActorSumMs -ne $end -or $context.LifetimeMs -ne $end+5000){throw 'Derived native budget differs'};$budgets+=@{Conversation=$context.Conversation;RequiredActorSumMs=$end;PublishedLifetimeMs=$end+5000}}
 foreach($payload in $fixture.Payloads){
  $asset=Join-Path $AudioDirectory "$($payload.FileDataID).audio"
  if(!(Test-Path -LiteralPath $asset)){throw "Missing exported matching26972 audio $($payload.FileDataID); supply AudioDirectory containing the ten local CASC exports"}
  if((Get-Item $asset).Length -ne $payload.Bytes -or (Get-FileHash $asset).Hash -cne $payload.SHA256){throw 'Native hashed opening voice differs'}
  $raw=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,channels' -of json $asset);if($LASTEXITCODE){throw 'opening voice metadata failed'};$meta=($raw -join "`n")|ConvertFrom-Json
  if(@($meta.streams).Count -ne 1 -or $meta.streams[0].codec_name -ne 'vorbis' -or $meta.streams[0].channels -notin @(1,2)){throw 'Native voice format differs'}
  $seconds=[double]::Parse($meta.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if([long][Math]::Ceiling($seconds*1000) -ne $payload.VoiceMs){throw 'Native full voice duration differs'}
  & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full native opening voice decode failed'};$payloads+=$payload
 }
 [IO.File]::WriteAllLines("$out/voices.tsv",[string[]]$voiceRows)
 [ordered]@{Build=$fixture.Build;Budgets=$budgets;Payloads=$payloads;ClientPlaybackVerified=$false;Limitation='Three full hashed decodes;1890 typed mapping/native gaps verified. No listening, client/cross-sequence/full1078 proof.'}|ConvertTo-Json -Depth 10|Set-Content "$out/summary.json"
 'PASS three full hashed native voice decodes;1890 content15157/published20157 with native6000/1000 gaps and5000 padding. Client playback separate.'
}

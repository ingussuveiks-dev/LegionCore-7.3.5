param([string]$AudioDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$AudioDirectory){$AudioDirectory="$repo/.codex/chen-student-audio1-2026-11-02"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-student-audio/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"}
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $fixture=Get-Content "$PSScriptRoot/MonkStudentRepliesAudio.json" -Raw|ConvertFrom-Json
 $prior=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MonkStudentVoiceCatalog.cpp -ProbeName MonkStudentVoiceCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native student voice catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $native=Get-Content "$out/native.tsv"
 if($native -notcontains "COMPLETE`t7`t4"){throw 'Full native text/kit family missing'}
 foreach($id in $fixture.UnvoicedReplies){
  $rows=@($native|Where-Object {$_ -like "TEXT`t$id`t*"})
  if($rows.Count -ne 1 -or [string]::Join(',',@($rows[0].Split("`t")[-2..-1])) -cne '0,0'){throw 'Native student male/female voice is not absent'}
 }
 foreach($voice in $fixture.ChenVoices){
  foreach($row in @("SOUND_NAME`t$($voice.SoundKit)`t$($voice.Name)","AUDIO_FILE`t$($voice.SoundKit)`t$($voice.FileDataID)")){if($native -notcontains $row){throw 'Native Chen kit/name/file identity differs'}}
  $text=@($native|Where-Object {$_ -like "TEXT`t$($voice.Broadcast)`t*"})
  if($text.Count -ne 1 -or [string]::Join(',',@($text[0].Split("`t")[-2..-1])) -cne "$($voice.SoundKit),0"){throw 'Native Chen text owner differs'}
 }
 $owners=@($native|Where-Object {$_ -like "OVERRIDE_NATIVE_OWNER`t*"})
 if($owners.Count -ne 1 -or [uint32]$owners[0].Split("`t")[1] -ne $fixture.WrongOverrideOwner -or $fixture.WrongStudentOverride -ne 62044){throw 'Student override does not uniquely resolve to original Chen100187'}
 foreach($payload in $fixture.Payloads){
  $asset=Join-Path $AudioDirectory "$($payload.FileDataID).audio"
  if(!(Test-Path -LiteralPath $asset) -or (Get-Item $asset).Length -ne $payload.Bytes -or (Get-FileHash $asset).Hash -cne $payload.SHA256){throw 'Full hashed matching local voice payload missing/different'}
  $raw=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,channels' -of json $asset);if($LASTEXITCODE){throw 'Native voice metadata failed'};$meta=($raw -join "`n")|ConvertFrom-Json
  if(@($meta.streams).Count -ne 1 -or $meta.streams[0].codec_name -ne 'vorbis' -or $meta.streams[0].channels -notin @(1,2)){throw 'Native voice format differs'}
  $seconds=[double]::Parse($meta.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if([long][Math]::Ceiling(1000*$seconds) -ne $payload.VoiceMs){throw 'Full native voice duration differs'}
  & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full native voice decode failed'}
 }
 [ordered]@{Build=$fixture.Build;Replies=$fixture.UnvoicedReplies;WrongOverrideOwner=$fixture.WrongOverrideOwner;ChenVoices=$fixture.ChenVoices;Payloads=$fixture.Payloads;ClientPlaybackVerified=$false;Pending=$fixture.Pending}|ConvertTo-Json -Depth 10|Set-Content "$out/summary.json"
 'PASS three native unvoiced replies and four actual Chen kit/name/file mappings/full hashed Vorbis decodes;62044 uniquely belongs to Chen100187. Default row restoration does not invent a student voice; client/initial timing remain separate.'
}

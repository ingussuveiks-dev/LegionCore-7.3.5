param([Parameter(Mandatory)][string]$AudioDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/merlis-combat-voice-audio/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $fixture=Get-Content "$PSScriptRoot/MerlisCombatVoiceAudio.json" -Raw|ConvertFrom-Json
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MerlisCombatVoiceCatalog.cpp -ProbeName MerlisCombatVoiceCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native combat voice mapping failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($row in $fixture.Payloads){
  foreach($expected in @("LINE`t$($row.Line)`t$($row.Broadcast)`t1000`t0","AUDIO_FILE`t$($row.SoundKit)`t$($row.FileDataID)")){if($native -notcontains $expected){throw 'Native combat line/voice identity changed'}}
  $text=@($native|Where-Object {$_ -like "TEXT`t$($row.Broadcast)`t*"});if($text.Count -ne 1 -or ($text[0].Split("`t")[-2..-1] -join ',') -ne "$($row.SoundKit),0"){throw 'Native sound identity differs'}
  $asset=Join-Path $AudioDirectory "$($row.FileDataID).audio"
  if((Get-Item $asset).Length -ne $row.Bytes -or (Get-FileHash $asset).Hash -cne $row.SHA256){throw 'Hashed matching26972 combat voice differs'}
  $raw=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,channels' -of json $asset);if($LASTEXITCODE){throw 'Voice metadata failed'};$meta=($raw -join "`n")|ConvertFrom-Json
  if(@($meta.streams).Count -ne 1 -or $meta.streams[0].codec_name -ne 'vorbis' -or $meta.streams[0].channels -notin @(1,2)){throw 'Native voice format differs'}
  $seconds=[double]::Parse($meta.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if([long][Math]::Ceiling(1000*$seconds) -ne $row.VoiceMs -or $row.PublishedLifetimeMs -lt $row.VoiceMs){throw 'Full voice no longer fits its native published lifetime'}
  & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Full combat voice decode failed'}
 }
 'PASS six fully decoded hashed native combat voices; fresh typed line/BroadcastText/SoundKit/FileData mapping, all full voice payloads fit published native lifetimes. No listening or actual client playback claim.'
}

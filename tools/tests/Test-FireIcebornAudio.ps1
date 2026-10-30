param(
 [string]$AudioDirectory,
 [string]$NativeCatalog,
 [string]$DataDirectory
)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$AudioDirectory){$AudioDirectory="$repo/build-extractors/runtime-test/fire-iceborn-audio"}
if(!$NativeCatalog){$NativeCatalog="$repo/build-extractors/runtime-test/fire-iceborn-story/native.tsv"}
if(!$DataDirectory){$DataDirectory="$repo/build-extractors/runtime-test/fire-iceborn-story-data"}
$out="$repo/build-extractors/runtime-test/fire-iceborn-audio-check"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 # Local payloads were read from the matching26972 client using read-only
 # handles. No media is distributed, and this check does not touch the client.
 $payloads=@(
  @(1267583,91570,'5C1D387D35441FCA5E610D67352067365F57DD7CE4608507611D8D3EC604A8BB'),
  @(1267585,79601,'5E6D9D38E5C0D7E08FF11195B607B10181771D929B7580AC608D04D9D2A8DAC1'),
  @(1267587,59342,'A66232B8F7572F9CA8E160A4DA655FB02AA17AF5F6CEA60781C55873984D6324'),
  @(1267589,74120,'A6C55C3D9522D0F96F37B1A576B8F6A45A4C45522EC4F20D54CB6FFFF4DF2FBA')
 )
 $native=Get-Content $NativeCatalog -Encoding UTF8;$voices=@{};$decoded=@()
 foreach($p in $payloads){
  $file=Join-Path $AudioDirectory "$($p[0]).audio"
  if((Get-Item $file).Length -ne $p[1] -or (Get-FileHash $file).Hash -cne $p[2]){throw "Native voice payload differs: $($p[0])"}
  $raw=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,channels' -of json $file);if($LASTEXITCODE){throw 'Native voice ffprobe failed'}
  $meta=($raw -join "`n")|ConvertFrom-Json
  if(@($meta.streams).Count -ne 1 -or $meta.streams[0].codec_name -ne 'vorbis' -or $meta.streams[0].channels -notin @(1,2)){throw 'Unexpected native voice format'}
  $seconds=[double]::Parse($meta.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if($seconds -le 0 -or $seconds -gt 60){throw 'Invalid native voice duration'}
  & ffmpeg -nostdin -v error -i $file -f null -;if($LASTEXITCODE){throw 'Full native voice decode failed'}
  $voices[[uint32]$p[0]]=[long][Math]::Ceiling(1000*$seconds)
  $decoded+=[pscustomobject]@{FileDataID=$p[0];Bytes=$p[1];SHA256=$p[2];VoiceMs=$voices[[uint32]$p[0]]}
 }
 $old=@(Get-Content "$DataDirectory/baseline-conversation.tsv");$new=@(Get-Content "$DataDirectory/conversation.tsv")
 $oldActor=@($old|Where-Object {$_ -like "actor`t*"});$newActor=@($new|Where-Object {$_ -like "actor`t*"})
 if($oldActor.Count -ne 1 -or $newActor.Count -ne 1){throw 'Missing full actor fixtures'}
 # Fixture row layout follows conversation_actor: see the data test.
 $oldFields=$oldActor[0].Split("`t");$newFields=$newActor[0].Split("`t")
 if($oldFields.Count -ne 10 -or $newFields.Count -ne 10){throw 'Actor fixture schema differs'}
 $durationIndex=9
 $oldDuration=[long]$oldFields[$durationIndex];$newDuration=[long]$newFields[$durationIndex]
 $oldFields[$durationIndex]=$newFields[$durationIndex]
 if(($oldFields -join "`t") -cne $newActor[0]){throw 'Audio fix changed more than the one actor duration'}
 $oldLines=@($old|Where-Object {$_ -like "line`t*"});$newLines=@($new|Where-Object {$_ -like "line`t*"})
 if($oldLines.Count -ne 4 -or (Compare-Object $oldLines $newLines)){throw 'Original four line definitions changed'}
 $expected=@(@(1870,101308,1871,53955,1267583,0),@(1871,101513,1872,53956,1267585,6250),@(1872,101537,1873,53957,1267587,12350),@(1873,101514,0,53958,1267589,17750))
 $timings=@();$sequential=0
 for($i=0;$i -lt 4;++$i){
  $e=$expected[$i];$line=@($native|Where-Object {$_ -like "CONVERSATION_LINE`t$($e[0])`t*"})
  if($line.Count -ne 1){throw 'Native line missing/ambiguous'};$f=$line[0].Split("`t")
  if([uint32]$f[2] -ne $e[1] -or [uint32]$f[3] -ne $e[2] -or $f[4] -ne '500' -or $f[6] -ne '0' -or [uint32]$f[7] -ne $e[3] -or !$f[8]){throw 'Native Lyandra line/voice chain differs'}
  if(@($native|Where-Object {$_ -ceq "AUDIO_FILE`t$($e[3])`t$($e[4])"}).Count -ne 1){throw 'Native sound kit/file mapping differs'}
  $fixture=$oldLines[$i].Split("`t")
  if([uint32]$fixture[2] -ne $e[0] -or [uint32]$fixture[4] -ne $e[5]){throw 'Original line identity/offset differs'}
  $voice=$voices[[uint32]$e[4]];$additional=[long]$f[4];$sequential+=$voice+$additional
  $timings+=[pscustomobject]@{Line=$e[0];OffsetMs=$e[5];VoiceMs=$voice;AdditionalMs=$additional;AbsoluteEndMs=$e[5]+$voice+$additional;SequentialEndMs=$sequential}
 }
 $absolute=[long]($timings|Measure-Object AbsoluteEndMs -Maximum).Maximum
 $required=[long][Math]::Max($absolute,$sequential);$padding=5000
 if($oldDuration -ne 6000 -or $oldDuration+$padding -ge $required+$padding){throw 'Truncating baseline not reproduced'}
 if($newDuration -ne $required -or $newDuration+$padding -lt $required+$padding -or $required -ne 48827 -or $absolute -ne 29564){throw 'Repaired native voice budget insufficient/changed'}
 [ordered]@{Build='7.3.5.26972';Conversation=786;OriginalLifetimeMs=$oldDuration+$padding;AbsoluteLineEndMs=$absolute;SequentialLineEndMs=$sequential;RepairedLifetimeMs=$newDuration+$padding;Payloads=$decoded;Lines=$timings;ClientPlaybackVerified=$false;SchedulingLimitation='Original offsets retained; conservative lifetime covers absolute offsets and sequential voice scheduling, without claiming exact client timing or absence of overlap.'}|ConvertTo-Json -Depth 8|Set-Content "$out/summary.json"
 'COUNTERFACTUAL original786 lifetime11000ms truncates native voices; repaired48827+5000ms covers conservative budget. PASS four actual hashed Vorbis payloads fully decoded, native26972 line/BCT/kit/file chain and unchanged SQL offsets. Client playback remains separate.'
}

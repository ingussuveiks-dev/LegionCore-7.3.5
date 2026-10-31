param([string]$AudioDirectory='',[string]$DataDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$AudioDirectory){$AudioDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-upper-audio"};if(!$DataDirectory){$DataDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-voice-data"};if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-voice-audio"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$fixture=Get-Content "$PSScriptRoot/SharasdalUpperVoiceAudio.json" -Raw|ConvertFrom-Json
$prior=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalUpperIntroCatalog.cpp -ProbeName SharasdalUpperIntroCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native upper voice catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$native=Get-Content "$out/native.tsv" -Encoding UTF8
if($native[-1] -cne "EOF`tupper-intro-audio-native"){throw 'Incomplete native upper audio catalog'}
foreach($pair in @(@('LINE',$fixture.Lines),@('AUDIO_FILE',$fixture.Files))){$expected=@($pair[1]|ForEach-Object {"$($pair[0])`t$($_ -join "`t")"});if(Compare-Object $expected @($native|Where-Object {$_ -like "$($pair[0])`t*"})){throw 'Native line/audio identity changed'}}
foreach($row in $fixture.Texts){$text=@($native|Where-Object {$_ -like "TEXT`t$($row[0])`t*"});if($text.Count -ne 1){throw 'Native broadcast ambiguous'};$f=$text[0].Split("`t");if($f.Count -ne 6 -or [uint32]$f[4] -ne $row[1] -or [uint32]$f[5] -ne $row[2]){throw 'Native voice gender selection differs'}}
$voices=@{}
foreach($payload in $fixture.Payloads){
 $file=Join-Path $AudioDirectory "$($payload.FileDataID).audio"
 if((Get-Item $file).Length -ne $payload.Bytes -or (Get-FileHash $file).Hash -cne $payload.SHA256){throw 'Native upper voice bytes differ'}
 $raw=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,channels' -of json $file);if($LASTEXITCODE){throw 'Native voice metadata failed'};$meta=($raw -join "`n")|ConvertFrom-Json
 if(@($meta.streams).Count -ne 1 -or $meta.streams[0].codec_name -ne 'vorbis' -or $meta.streams[0].channels -notin @(1,2)){throw 'Unexpected native voice stream'}
 $seconds=[double]::Parse($meta.format.duration,[Globalization.CultureInfo]::InvariantCulture);$ms=[long][Math]::Ceiling(1000*$seconds)
 if($seconds -le 0 -or $seconds -gt 60 -or $ms -ne $payload.VoiceMs){throw 'Native voice duration differs'}
 & ffmpeg -nostdin -v error -i $file -f null -;if($LASTEXITCODE){throw 'Full native voice decode failed'}
 $voices[[uint32]$payload.FileDataID]=$ms
}
$old=Get-Content "$DataDirectory/baseline-conversation.tsv";$new=Get-Content "$DataDirectory/conversation.tsv"
$oldLines=@($old|Where-Object {$_ -like "line`t1868`t*"});$lines=@($new|Where-Object {$_ -like "line`t1868`t*"})
if($oldLines.Count -ne 2 -or $lines.Count -ne 5 -or (Compare-Object $oldLines @($lines|Select-Object -First 2))){throw 'Original two native line offsets/payloads changed'}
$sequential=[long]0;$previousEnd=[long]0;$timings=@();$next=3961
for($index=0;$index -lt $lines.Count;++$index){
 $f=$lines[$index].Split("`t");$id=[uint32]$f[2];$offset=[long]$f[3]
 if($f.Count -ne 7 -or $id -ne $next -or $offset -ne $fixture.Offsets[$index] -or [int]$f[6] -ne $index){throw 'Native Next chain/offset/index differs'}
 $line=@($fixture.Lines|Where-Object {$_[0] -eq $id});if($line.Count -ne 1){throw 'Native line ambiguous'};$l=$line[0];$next=[int]$l[3]
 $text=@($fixture.Texts|Where-Object {$_[0] -eq $l[1]});if($text.Count -ne 1){throw 'Native text ambiguous'};$voice=[long]0
 foreach($kit in @($text[0][1],$text[0][2])|Where-Object {$_}){foreach($track in @($fixture.Files|Where-Object {$_[0] -eq $kit})){$voice=[Math]::Max($voice,$voices[[uint32]$track[1]])}}
 if(!$voice -or ($index -gt 1 -and $offset -ne $previousEnd) -or $offset -lt $previousEnd){throw 'Added voice overlap or unsupported offset'}
 $sequential+=$voice+[long]$l[2];$previousEnd=$offset+$voice+[long]$l[2]
 $timings+=[pscustomobject]@{Line=$id;OffsetMs=$offset;VoiceMs=$voice;AdditionalMs=[long]$l[2];AbsoluteEndMs=$previousEnd;SequentialEndMs=$sequential}
}
if($next -ne 0){throw 'Native line chain not fully traversed'}
$absolute=[long]($timings|Measure-Object AbsoluteEndMs -Maximum).Maximum;$required=[Math]::Max($absolute,$sequential);$sums=@()
foreach($rows in @(@($old),@($new))){$actors=@($rows|Where-Object {$_ -like "creature`t1868`t*"});if($actors.Count -ne 2){throw 'Original two actors missing'};$sum=[long]0;foreach($row in $actors){$f=$row.Split("`t");if($f.Count -ne 8){throw 'Actor schema differs'};$sum+=[long]$f[7]};$sums+=$sum}
if($required -ne $fixture.RequiredActorSumMs -or $absolute -ne $fixture.AbsoluteLineEndMs -or $sequential -ne $fixture.SequentialLineEndMs -or $sums[0] -ne $fixture.OriginalActorSumMs -or $sums[1] -ne $required -or $sums[0]+5000 -ge $required){throw 'Original/fixed native voice budget differs'}
[ordered]@{Build=$fixture.Build;Timings=$timings;OriginalLineCount=2;RepairedLineCount=5;OriginalLifetimeMs=$sums[0]+5000;RepairedLifetimeMs=$sums[1]+5000;Payloads=$fixture.Payloads;ClientPlaybackVerified=$false;Limitation=$fixture.Limitation}|ConvertTo-Json -Depth 10|Set-Content "$out/summary.json"
'PASS five matching hashed native Vorbis payloads fully decoded; complete3961->3962->8701->8702->8703->0 chain with native BCT/kit/file/gender/AdditionalDuration. Original0/6400 retained, later offsets derived without overlap. COUNTERFACTUAL two lines/20600ms; repaired five lines/47816ms cover absolute42816/sequential39864 plus native5000. Caller/NPC world expiry/client playback remain separate.'

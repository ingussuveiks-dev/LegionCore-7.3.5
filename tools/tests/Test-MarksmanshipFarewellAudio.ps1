param([Parameter(Mandatory)][string]$ClientPath)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-farewell-audio"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipFarewell.ps1"
$old=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MarksmanshipDialogueCatalog.cpp -ProbeName MarksmanshipDialogueCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
$native=Get-Content "$out/native.tsv"
$lines=@{};$broadcasts=@{};$files=@{}
foreach($row in $native){$fields=$row.Split("`t");switch($fields[0]){
 'CONVERSATION_LINE'{$lines[[uint32]$fields[1]]=@{Broadcast=[uint32]$fields[2];Additional=[uint32]$fields[5]}}
 'BROADCAST'{$broadcasts[[uint32]$fields[1]]=@([uint32]$fields[2],[uint32]$fields[3]) | Where-Object {$_}}
 'AUDIO_FILE'{$kit=[uint32]$fields[1];if(!$files.ContainsKey($kit)){$files[$kit]=@()};$files[$kit]+=[uint32]$fields[2]}
}}
$definitions=Get-Content "$repo/build-extractors/runtime-test/marksmanship-farewell/conversation.tsv" | ForEach-Object {$fields=$_.Split("`t");if($fields[0] -eq 'line'){[pscustomobject]@{Line=[uint32]$fields[2];Start=[uint32]$fields[4]}}}
if(@($definitions).Count -ne 6 -or $lines[[uint32]3427].Broadcast -ne 0 -or $lines[[uint32]3427].Additional -ne 5000){throw 'Native6-line/silent player marker data changed; re-audit'}
$identities=@($definitions | ForEach-Object {$entry=$lines[$_.Line];if($entry.Broadcast){foreach($kit in $broadcasts[$entry.Broadcast]){$files[$kit]}}} | Sort-Object -Unique)
if($identities.Count -ne 5){throw 'Expected5 native farewell voice files'}
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
$timings=@(foreach($definition in $definitions){$entry=$lines[$definition.Line];$voice=0
 if($entry.Broadcast){foreach($kit in $broadcasts[$entry.Broadcast]){foreach($identity in $files[$kit]){$voice=[Math]::Max($voice,[Math]::Ceiling(1000*$payloads[$identity].DurationSeconds))}}}
 [pscustomobject]@{Line=$definition.Line;StartMs=$definition.Start;VoiceMs=$voice;AdditionalMs=$entry.Additional;EndMs=$definition.Start+$voice+$entry.Additional}
})
$duration=5000
foreach($row in Get-Content "$repo/build-extractors/runtime-test/marksmanship-farewell/conversation.tsv"){$fields=$row.Split("`t");if($fields[0] -eq 'creature'){$duration += [uint32]$fields[7]}}
$required=($timings|Measure-Object EndMs -Maximum).Maximum
if($duration -lt $required+5000){throw 'Native farewell lifetime truncates voice/additional duration or removes native5s padding'}
if($required -le 12950){throw 'Baseline truncation was not reproduced'}
[ordered]@{Build='7.3.5.26972';LifetimeMs=$duration;RequiredLineEndMs=$required;NativePaddingMs=5000;StableIndexCount=$before.Count;Lines=$timings;Payloads=@($payloads.Values);ClientPlaybackVerified=$false}|ConvertTo-Json -Depth 8|Set-Content "$out/summary.json"
$timings|Format-Table
"PASS:5 native voice payloads fully decoded, maximum line end ${required}ms +5000ms retained by ${duration}ms lifetime; $($before.Count) indexes unchanged. No client playback claim."

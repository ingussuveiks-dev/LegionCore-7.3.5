param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ClientPath='')
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Voice baseline cannot certify installed code'}
if(!$ClientPath){throw 'Pass the matching26972 ClientPath for complete local voice validation'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-offer-voices"
New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile HruthnirOfferVoiceCatalog.cpp -ProbeName HruthnirOfferVoiceCatalog -OutputFile "$out/native.tsv"
 if($LASTEXITCODE){throw 'Native offer voice catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
$expected=@("VOICE`t98692`t60402`t1370057","VOICE`t98324`t60403`t1370058","VOICE`t98325`t60404`t1370059")
if(Compare-Object $expected $native){throw 'Native offer voice mapping changed; re-audit'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@('SELECT GroupID,ID,BroadcastTextID,Sound,Duration FROM creature_text WHERE CreatureID=96468 AND GroupID IN(14,15,16) ORDER BY GroupID,ID;'|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only offer text query failed'}
}finally{$env:MYSQL_PWD=$previous}
if(Compare-Object @("14`t0`t98692`t60402`t0","15`t0`t98324`t60403`t0","16`t0`t98325`t60404`t0") $rows){throw 'Custom offer texts require a separate voice budget audit'}
$client=(Resolve-Path -LiteralPath $ClientPath).Path
if(!(Get-Content "$client/.build.info"|Select-String '7\.3\.5\.26972')){throw 'Expected matching26972 client'}
foreach($name in @('CascOpenStorage','CascReadFile')){
 $source=Get-Content "$repo/dep/CascLib/src/$name.cpp" -Raw
 $boundary='STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'
 if(!$source.Contains($boundary)){throw "Missing read-only CASC boundary $name"}
 [IO.File]::WriteAllText("$out/${name}Audit.inc",$source.Replace($boundary,'STREAM_FLAG_READ_ONLY | STREAM_FLAG_WRITE_SHARE | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'))
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/build-audio.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /MD /D__SYS_ZLIB /I`"$repo/dep/zlib`" /I`"$repo/dep/CascLib/src`" /I`"$out`" `"$repo/tools/research/CascAudioProbe.cpp`" /Fe`"$out/CascAudioProbe.exe`" /Fo`"$out/CascAudioProbe.obj`" /link `"$repo/build-extractors/dep/CascLib/Release/casc.lib`" `"$repo/build-extractors/dep/zlib/Release/zlib.lib`"`nexit /b %errorlevel%`n")
& "$out/build-audio.cmd";if($LASTEXITCODE){throw 'Read-only audio probe build failed'}
function IndexHashes{
 $indexes=@(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx'|Sort-Object Name)
 if(!$indexes.Count){throw 'Missing client indexes'}
 foreach($file in $indexes){$stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$sha=[Security.Cryptography.SHA256]::Create();try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}}
}
$before=@(IndexHashes);$payloads=@();$voiceBoundary=0
try{foreach($identity in @(1370057,1370058,1370059)){
 $asset="$out/$identity.audio"
 & "$out/CascAudioProbe.exe" $client ('File{0:X8}' -f $identity) $asset;if($LASTEXITCODE){throw "CASC export failed $identity"}
 $probe=@(& ffprobe -v error -show_entries 'format=duration:stream=codec_name,sample_rate,channels' -of json $asset);if($LASTEXITCODE){throw 'Voice metadata failed'}
 $metadata=($probe -join "`n")|ConvertFrom-Json
 if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or $metadata.streams[0].channels -ne 1 -or $metadata.streams[0].sample_rate -ne '22050'){throw 'Unexpected native offer voice stream'}
 $seconds=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture)
 if($seconds -le 0 -or $seconds -gt 30){throw 'Unexpected native offer voice duration'}
 & ffmpeg -nostdin -v error -i $asset -f null -;if($LASTEXITCODE){throw 'Complete voice decode failed'}
 $duration=[uint32][Math]::Ceiling($seconds*1000)
 if($identity -eq 1370058){$voiceBoundary=$duration}
 $payloads+=[pscustomobject]@{FileDataID=$identity;DurationSeconds=$seconds;DurationMs=$duration;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash}
}}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client indexes changed during voice audit'}}
if($voiceBoundary -ne 12629){throw 'Native60403 duration changed; re-audit'}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "c7239252:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned voice source unavailable'}}
$matches=[regex]::Matches($source,'AddDelayedEvent\((\d+), \[offerToken, companion, expectedAI, instanceId\]')
if($matches.Count -ne 1){throw 'Expected one scoped point6 offer queue'}
$delay=[uint32]$matches[0].Groups[1].Value
if($Baseline){if($delay -ne 6500 -or $delay -ge $voiceBoundary){throw 'Pinned early offer no longer reproduces'};& "$PSScriptRoot/Test-HruthnirGossip.ps1" -VoiceBaseline}
else{if($delay -ne 13000 -or $delay -lt $voiceBoundary){throw 'Readiness speech/gossip precedes the native voice end'};& "$PSScriptRoot/Test-HruthnirGossip.ps1"}
if($LASTEXITCODE){throw 'Native offer boundary regression failed'}
[ordered]@{Build='7.3.5.26972';Baseline=[bool]$Baseline;DelayMs=$delay;NativeVoiceEndMs=$voiceBoundary;StableIndexCount=$before.Count;Payloads=$payloads;ClientPlaybackVerified=$false}|ConvertTo-Json -Depth 6|Set-Content "$out/audio-summary.json"
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hruthnir_offer_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed offer wait or lifetime guards missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: three complete native offer voice decodes,60403 ends at12629ms; scoped wait=$delay ms, $($before.Count) CASC indexes unchanged. Baseline=$Baseline; actual client playback/mixing remains unverified."

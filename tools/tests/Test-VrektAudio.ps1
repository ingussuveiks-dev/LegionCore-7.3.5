param([Parameter(Mandatory)][string]$ClientPath)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-audio"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-VrektOutro.ps1"
if($LASTEXITCODE){throw 'Native sound/scene regression failed'}
$client=(Resolve-Path -LiteralPath $ClientPath).Path
if(!(Get-Content "$client/.build.info" | Select-String '7\.3\.5\.26972')){throw 'Expected native26972 client storage'}
$native=Get-Content "$repo/build-extractors/runtime-test/vrekt-outro/native-lines.txt"
$files=@($native | ForEach-Object {if($_ -match '^OUTROFILE (\d+) kit=77643 file=(\d+) frequency=(\d+) volume=([\d.]+)$'){[pscustomobject]@{Row=[uint32]$Matches[1];File=[uint32]$Matches[2];Frequency=[uint32]$Matches[3];Volume=$Matches[4]}}})
if($files.Count -ne 1 -or $files[0].Row -ne 194118 -or $files[0].File -ne 1519112){throw 'Native outro sound-file mapping changed; re-audit'}
foreach($name in @('CascOpenStorage','CascReadFile')){
 $source=[IO.File]::ReadAllText("$repo/dep/CascLib/src/$name.cpp")
 $old='STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'
 if(!$source.Contains($old)){throw "Missing read-only CASC boundary $name"}
 [IO.File]::WriteAllText("$out/${name}Audit.inc",$source.Replace($old,'STREAM_FLAG_READ_ONLY | STREAM_FLAG_WRITE_SHARE | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'))
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /MD /D__SYS_ZLIB /I`"$repo/dep/zlib`" /I`"$repo/dep/CascLib/src`" /I`"$out`" `"$repo/tools/research/CascAudioProbe.cpp`" /Fe`"$out/CascAudioProbe.exe`" /Fo`"$out/CascAudioProbe.obj`" /link `"$repo/build-extractors/dep/CascLib/Release/casc.lib`" `"$repo/build-extractors/dep/zlib/Release/zlib.lib`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/build.cmd",$cmd);& "$out/build.cmd"
if($LASTEXITCODE){throw 'Read-only CASC audio probe build failed'}
function IndexHashes{
 $indexes=@(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx' | Sort-Object Name)
 if(!$indexes.Count){throw 'Missing client CASC indexes'}
 foreach($file in $indexes){
  $stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
  $sha=[Security.Cryptography.SHA256]::Create()
  try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}
 }
}
$before=@(IndexHashes);$asset="$out/1519112.audio"
& "$out/CascAudioProbe.exe" $client ('File{0:X8}' -f $files[0].File) $asset
if($LASTEXITCODE){throw 'Native CASC audio export failed'}
if(Compare-Object $before @(IndexHashes)){throw 'Client CASC indexes changed during audit'}
$probe=@(& ffprobe -v error -show_entries 'format=duration,format_name:stream=codec_name,sample_rate,channels' -of json $asset)
if($LASTEXITCODE){throw 'Audio format validation failed'}
$metadata=($probe -join "`n") | ConvertFrom-Json
$duration=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture)
if($duration -le 0 -or $duration -gt 11.117){throw 'Full audio exceeds the native11117ms conversation lifetime'}
& ffmpeg -nostdin -v error -i $asset -f null -
if($LASTEXITCODE){throw 'Full audio decode failed'}
$result=[ordered]@{Quest=44923;Conversation=3982;Line=9085;BroadcastText=123646;SoundKit=77643;SoundKitEntry=$files[0].Row;FileDataID=$files[0].File;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash;DurationSeconds=$duration;Audio=$metadata;StableIndexCount=$before.Count;ClientBuild='7.3.5.26972';ClientPlaybackVerified=$false}
$result | ConvertTo-Json -Depth 7 | Set-Content "$out/summary.json"
"PASS: native outro sound FileDataID1519112 fully read and decoded ($duration seconds); $($before.Count) CASC indexes unchanged. Audio was not played and no client UI was operated."

param([Parameter(Mandatory)][string]$ClientPath)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/rockaway-audio"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-VrektOutro.ps1" -QuestID 44746
if($LASTEXITCODE){throw 'Native Rockaway conversation regression failed'}
$client=(Resolve-Path -LiteralPath $ClientPath).Path
if(!(Get-Content "$client/.build.info" | Select-String '7\.3\.5\.26972')){throw 'Expected native26972 client storage'}
$source=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.cpp" -Raw
$a=$source.IndexOf('    uint32 fSound = [bct, sound, source, text]() -> uint32')
$b=$source.IndexOf('    if (fSound)',$a)
if($a -lt 0 -or $b -le $a){throw 'Native SendText sound selector boundary changed'}
[IO.File]::WriteAllText("$out/SoundSelection.inc",$source.Substring($a,$b-$a))
$header=Get-Content "$repo/src/server/game/Texts/CreatureTextMgr.h" -Raw
$definition=[regex]::Match($header,'(?s)struct CreatureTextEntry\s*\{.*?\n\};')
if(!$definition.Success){throw 'Missing native CreatureTextEntry definition'}
[IO.File]::WriteAllText("$out/CreatureTextEntry.inc",$definition.Value)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sql="SELECT c.CreatureID,c.GroupID,c.ID,c.Type,c.Sound,c.BroadcastTextID,c.Duration,c.MinTimer,c.MaxTimer,w.DisplayID1
FROM creature_text c JOIN creature_template_wdb w ON w.Entry=c.CreatureID
WHERE c.CreatureID IN(115133,115132) ORDER BY c.CreatureID,c.GroupID,c.ID;"
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only dialogue snapshot failed'}
 $rows | Set-Content "$out/data.tsv"
 $sql="SELECT COUNT(*) FROM creature_template_wdb w JOIN creature_model_info m ON m.DisplayID=w.DisplayID1 WHERE w.Entry IN(115132,115133) AND w.DisplayID2=0 AND w.DisplayID3=0 AND w.DisplayID4=0 AND m.DisplayID_Other_Gender=0;
SELECT COUNT(*) FROM creature WHERE id IN(115132,115133) AND modelid<>0;
SELECT duration FROM conversation_actor WHERE entry=3963 AND id=0;
SELECT COUNT(*) FROM creature_text WHERE CreatureID IN(115132,115133) AND Probability=100 AND SpellID=0 AND Emote=0;"
 $scope=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE -or $scope.Count -ne 4 -or ($scope[0..1] -join '|') -ne '2|0' -or [uint32]$scope[2] -eq 0 -or $scope[3] -ne '4'){throw 'Reinspect actor model/gender selection, spawn overrides, conversation hold or speech admission'}
 $outroHoldMs=[uint32]$scope[2]
}finally{$env:MYSQL_PWD=$previous}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$release.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)
$libs=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/RockawayDialogue.cpp"';$compilerArgs+='/Fe"'+$out+'/dialogue.exe"';$compilerArgs+='/Fo"'+$out+'/dialogue.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/build-dialogue.cmd",$cmd);& "$out/build-dialogue.cmd"
if($LASTEXITCODE){throw 'Native dialogue catalog build failed'}
$previousPath=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 $native=@(& "$out/dialogue.exe" "$repo/build-extractors/bin/Release/dbc/enUS/" "$out/data.tsv" "$repo/build-extractors/runtime-test/rockaway-outro/worldserver.conf")
 $native | Set-Content "$out/native-lines.txt";$native
 if($LASTEXITCODE){throw 'Native dialogue catalog failed'}
}finally{$env:PATH=$previousPath}
foreach($name in @('CascOpenStorage','CascReadFile')){
 $source=[IO.File]::ReadAllText("$repo/dep/CascLib/src/$name.cpp")
 $old='STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'
 if(!$source.Contains($old)){throw "Missing read-only CASC boundary $name"}
 [IO.File]::WriteAllText("$out/${name}Audit.inc",$source.Replace($old,'STREAM_FLAG_READ_ONLY | STREAM_FLAG_WRITE_SHARE | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'))
}
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /MD /D__SYS_ZLIB /I`"$repo/dep/zlib`" /I`"$repo/dep/CascLib/src`" /I`"$out`" `"$repo/tools/research/CascAudioProbe.cpp`" /Fe`"$out/CascAudioProbe.exe`" /Fo`"$out/CascAudioProbe.obj`" /link `"$repo/build-extractors/dep/CascLib/Release/casc.lib`" `"$repo/build-extractors/dep/zlib/Release/zlib.lib`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/build-casc.cmd",$cmd);& "$out/build-casc.cmd"
if($LASTEXITCODE){throw 'Read-only CASC probe build failed'}
function IndexHashes{
 $indexes=@(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx' | Sort-Object Name)
 if(!$indexes.Count){throw 'Missing client CASC indexes'}
 foreach($file in $indexes){
  $stream=[IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
  $sha=[Security.Cryptography.SHA256]::Create()
  try{$file.Name+':'+[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$sha.Dispose();$stream.Dispose()}
 }
}
$links=@($native | ForEach-Object {if($_ -match '^FILE (\S+) (\d+) (\d+) (\d+) (\d+) ([\d.]+)$'){
 [pscustomobject]@{Key=$Matches[1];SoundKit=[uint32]$Matches[2];Row=[uint32]$Matches[3];File=[uint32]$Matches[4];Frequency=[uint32]$Matches[5];Volume=$Matches[6]}
}})
if(@($links.Key | Sort-Object -Unique).Count -ne 5){throw 'Missing one of four speech groups/outro sound links'}
$payloads=@();$before=@(IndexHashes)
try{
 foreach($file in @($links.File | Sort-Object -Unique)){
  $asset="$out/$file.audio"
  & "$out/CascAudioProbe.exe" $client ('File{0:X8}' -f $file) $asset
  if($LASTEXITCODE){throw "CASC audio export failed: $file"}
  $probe=@(& ffprobe -v error -show_entries 'format=duration,format_name:stream=codec_name,sample_rate,channels' -of json $asset)
  if($LASTEXITCODE){throw "Audio probe failed: $file"}
  $metadata=($probe -join "`n") | ConvertFrom-Json
  if(@($metadata.streams).Count -ne 1 -or $metadata.streams[0].codec_name -ne 'vorbis' -or
     $metadata.streams[0].sample_rate -ne '22050' -or $metadata.streams[0].channels -ne 1){throw "Unexpected voice stream: $file"}
  $duration=[double]::Parse($metadata.format.duration,[Globalization.CultureInfo]::InvariantCulture)
  if($duration -le 0 -or $duration -gt 60){throw "Reinspect unexpected voice duration: $file"}
  & ffmpeg -nostdin -v error -i $asset -f null -
  if($LASTEXITCODE){throw "Full audio decode failed: $file"}
  $payloads+=[pscustomobject]@{FileDataID=$file;Bytes=(Get-Item $asset).Length;SHA256=(Get-FileHash $asset).Hash;DurationSeconds=$duration;Audio=$metadata}
 }
}finally{if(Compare-Object $before @(IndexHashes)){throw 'Client CASC indexes changed during audio audit'}}
$source=Get-Content "$repo/src/server/scripts/Legion/rockaway_coast.h" -Raw
$pause=[regex]::Match($source,'Talk\(4\);\s*finishTime = (\d+);')
if(!$pause.Success){throw 'Missing native final speech/credit pause'}
$finalPause=[uint32]$pause.Groups[1].Value
$timing=@($links | ForEach-Object {
 $link=$_;$audio=$payloads | Where-Object FileDataID -eq $link.File | Select-Object -First 1
 [pscustomobject]@{Key=$link.Key;SoundKit=$link.SoundKit;SoundKitEntry=$link.Row;FileDataID=$link.File;Frequency=$link.Frequency;Volume=$link.Volume;DurationSeconds=$audio.DurationSeconds}
})
$result=[ordered]@{Quest=44746;ClientBuild='7.3.5.26972';StableIndexCount=$before.Count;FinalPauseMs=$finalPause;OutroHoldMs=$outroHoldMs;Links=$timing;Payloads=$payloads;NativeLines=$native;ClientPlaybackVerified=$false}
$result | ConvertTo-Json -Depth 9 | Set-Content "$out/summary.json"
$timing | Format-Table Key,SoundKit,FileDataID,DurationSeconds
$last=@($timing | Where-Object Key -eq '115133:4')
if(@($last | Where-Object {1000*$_.DurationSeconds -gt $finalPause}).Count){throw 'Final voice exceeds the production credit pause; inspect summary.json'}
$outro=@($timing | Where-Object Key -eq 'outro')
if(@($outro | Where-Object {1000*$_.DurationSeconds -gt $outroHoldMs}).Count){throw 'Outro voice exceeds native actor hold; inspect summary.json'}
$hashes=@{}
foreach($path in @('src/server/game/Texts/CreatureTextMgr.cpp','src/server/game/Texts/CreatureTextMgr.h','src/server/scripts/Legion/rockaway_coast.h','tools/tests/RockawayDialogue.cpp','tools/tests/Test-RockawayAudio.ps1')){$hashes[$path]=(Get-FileHash "$repo/$path").Hash}
foreach($name in @('BroadcastText','CreatureDisplayInfo','ConversationLine','SoundKit','SoundKitEntry')){$hashes["$name.db2"]=(Get-FileHash "$repo/build-extractors/bin/Release/dbc/enUS/$name.db2").Hash}
$hashes['data.tsv']=(Get-FileHash "$out/data.tsv").Hash
$hashes['native-lines.txt']=(Get-FileHash "$out/native-lines.txt").Hash
$hashes | ConvertTo-Json | Set-Content "$out/inputs.json"
"PASS: all selected speech payloads fully decoded; final voice fits $finalPause ms, outro fits native$outroHoldMs ms actor hold; $($before.Count) CASC indexes unchanged. Client playback remains unverified."

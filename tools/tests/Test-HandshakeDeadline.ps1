param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/handshake-deadline"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/game/Server/WorldSocket.cpp")
$start=$source.IndexOf('bool WorldSocket::Update()')
$start=$source.IndexOf('{',$start)+1
$end=$source.IndexOf('    EncryptablePacket* queued;',$start)
if($start -le 0 -or $end -lt $start){throw 'Missing production handshake deadline'}
[IO.File]::WriteAllText("$output/HandshakeDeadline.inc",$source.Substring($start,$end-$start))
$start=$source.IndexOf('    if (error)', $source.IndexOf('void WorldSocket::InitializeHandler('))
$end=$source.IndexOf('    GetReadBuffer().WriteCompleted(', $start)
if($start -lt 0 -or $end -le $start){throw 'Missing actual initializer error handler'}
[IO.File]::WriteAllText("$output/InitializerError.inc",$source.Substring($start,$end-$start))
$start=$source.IndexOf('void WorldSocket::SetWorldSession(')
$start=$source.IndexOf('{',$start)+1
$end=$source.IndexOf('bool WorldSocket::ReadHeaderHandler()', $start)
if($start -le 0 -or $end -le $start){throw 'Missing actual session binding handler'}
$body=$source.Substring($start,$end-$start)
$body=$body.Substring(0,$body.LastIndexOf('}'))
[IO.File]::WriteAllText("$output/SetWorldSession.inc",$body)
foreach($case in @('AUTH_SESSION','AUTH_CONTINUED_SESSION')){
 $start=$source.IndexOf('            if (_authed)', $source.IndexOf("case CMSG_${case}:"))
 $end=$source.IndexOf('            std::shared_ptr<', $start)
 if($start -lt 0 -or $end -le $start){throw "Missing duplicate $case admission"}
 [IO.File]::WriteAllText("$output/Duplicate${case}.inc",$source.Substring($start,$end-$start))
}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('build-extractors/runtime-test/handshake-deadline','src/server/shared/Networking','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$args+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$repo+'/tools/tests/HandshakeDeadlineTest.cpp"'
$args+='/Fe"'+$output+'/HandshakeDeadlineTest.exe"'
$args+='/Fo"'+$output+'/HandshakeDeadlineTest.obj"'
$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-item.cmd",$cmd)
& "$output/build-item.cmd"
if($LASTEXITCODE){throw 'Handshake deadline test build failed'}

$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/HandshakeDeadlineTest.exe"
 if($LASTEXITCODE){throw 'Handshake deadline regression failed'}
} finally {$env:PATH=$previousPath}
if($VerifyReleaseBinary){
 if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
 $python=(Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_handshake_lifetime_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Actual Release lacks socket authentication lifetime repair'}
}

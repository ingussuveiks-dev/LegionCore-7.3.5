param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/instance-connect-retry")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract($source,$marker,$name) {
    $start=$source.IndexOf($marker)
    if($start -lt 0){throw "Missing production handler: $marker"}
    $end=$source.IndexOf('{',$start)+1;$depth=1
    while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
    if($depth){throw 'Unbalanced handler'}
    [IO.File]::WriteAllText("$output/$name",$source.Substring($start,$end-$start))
}
$session=[IO.File]::ReadAllText("$repo/src/server/game/Server/WorldSession.cpp")
Extract $session 'bool WorldSession::HasInstanceConnection()' 'HasInstanceConnection.inc'
Extract $session 'void WorldSession::RetryInstanceConnection(' 'RetryInstanceConnection.inc'
Extract $session 'void WorldSession::SendConnectToInstance(' 'SendConnectToInstance.inc'
$send=[IO.File]::ReadAllText("$output/SendConnectToInstance.inc")
$start=$send.IndexOf('uint64 const key = _instanceConnectKey.Raw;')
if($start -lt 0){throw 'Missing instance login deadline'}
[IO.File]::WriteAllText("$output/InstanceConnectDeadline.inc",$send.Substring($start,$send.LastIndexOf('}')-$start))
$socket=[IO.File]::ReadAllText("$repo/src/server/game/Server/WorldSocket.cpp")
Extract $socket 'void WorldSocket::HandleConnectToFailed(' 'ClientConnectFailed.inc'
$world=[IO.File]::ReadAllText("$repo/src/server/game/World/World.cpp")
Extract $world 'void World::ProcessLinkInstanceSocket(' 'LinkInstanceSocket.inc'
if(!$send.Contains('_instanceConnectSerial = uint32(serial);')){throw 'Attempt serial not tracked'}
if(!$session.Contains('m_Functions.Update(diff);')){throw 'Session deadline queue not updated'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/InstanceConnectRetryTest.cpp" /Fe"$output/InstanceConnectRetryTest.exe" /Fo"$output/InstanceConnectRetryTest.obj"
if errorlevel 1 exit /b 1
"$output/InstanceConnectRetryTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Instance connection retry regression failed'}

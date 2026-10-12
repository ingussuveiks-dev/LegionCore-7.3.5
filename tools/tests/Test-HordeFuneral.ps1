param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/horde-funeral")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Extract([string]$file,[string]$token,[string]$name){
 $source=[IO.File]::ReadAllText("$repo/$file")
 $begin=$source.IndexOf($token);if($begin -lt 0){throw "Missing $token"}
 $open=$source.IndexOf('{',$begin);$close=$open+1;$depth=1
 while($depth -gt 0 -and $close -lt $source.Length){if($source[$close] -eq '{'){++$depth};if($source[$close] -eq '}'){--$depth};++$close}
 if($depth){throw 'Unbalanced production code'}
 [IO.File]::WriteAllText("$output/$name",$source.Substring($begin,$close-$begin))
}
Extract 'src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp' 'namespace HordeFuneral' 'Funeral.inc'
Extract 'src/server/game/Entities/Player/Player.cpp' 'void Player::SendMovieStart(' 'MovieStart.inc'
Extract 'src/server/game/Handlers/MiscHandler.cpp' 'void WorldSession::HandleCompleteMovie(' 'MovieComplete.inc'
$source=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp")
if(!$source.Contains('HordeFuneral::CanStart(who->ToPlayer())') -or !$source.Contains('HordeFuneral::StartMovie(who->ToPlayer());')){throw 'Funeral helper not wired to NPC'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HordeFuneralTest.cpp" /Fe"$output/HordeFuneralTest.exe" /Fo"$output/HordeFuneralTest.obj"
if errorlevel 1 exit /b 1
"$output/HordeFuneralTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Horde funeral regression failed'}

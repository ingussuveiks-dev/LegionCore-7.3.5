param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/movie-movement"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
function Extract([string]$text,[string]$signature){
 $start=$text.IndexOf($signature);if($start -lt 0){throw "Missing $signature"}
 $open=$text.IndexOf('{',$start);$depth=1;$end=$open+1
 while($depth -gt 0){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++}
 $text.Substring($start,$end-$start)
}
$methods=@('void Player::SendMovieStart(','void Player::EndMovieMovement(','void Player::UpdateMovieMovement(') | ForEach-Object { (Extract $source $_).Replace('Player::','') }
$header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
$methods+=(Extract $header 'void setWatchinMovie(')
[IO.File]::WriteAllText("$out/MovieMethods.inc",($methods -join "`n"))
$control=Extract $source 'void Player::SetClientControl('
$a=$control.IndexOf('    if (target == this');$b=$control.IndexOf('    // don''t allow possession',$a)
[IO.File]::WriteAllText("$out/MovieControl.inc",$control.Substring($a,$b-$a))
$movement=Get-Content "$repo/src/server/game/Handlers/MovementHandler.cpp" -Raw
foreach($signature in @('void WorldSession::HandleMovementOpcode(','void WorldSession::RelocateMover(')){
 $method=Extract $movement $signature
 $guard=$method.IndexOf('IsMovieMovementHeld()')
 $relocate=$method.IndexOf('UpdatePosition(')
 if($guard -lt 0 -or ($relocate -gt 0 -and $guard -gt $relocate)){throw 'Movie movement/ACK guard missing before relocation'}
}
foreach($signature in @('void Player::Update(','void Player::RemoveFromWorld(')){
 $body=Extract $source $signature
 if($body -notmatch '(UpdateMovieMovement|EndMovieMovement)\('){throw 'Movie lifecycle cleanup missing'}
}
$teleport=Extract $source 'bool Player::SafeTeleport('
if(([regex]::Matches($teleport,'EndMovieMovement\(\);')).Count -ne 2){throw 'Actual near/far transfer cleanup missing'}
if($teleport.IndexOf('EndMovieMovement();') -lt $teleport.IndexOf('if (IsHasDelayedTeleport())')){throw 'Delayed transfer releases movie too early'}
$completion=Extract (Get-Content "$repo/src/server/game/Handlers/MiscHandler.cpp" -Raw) 'void WorldSession::HandleCompleteMovie('
if($completion -notmatch 'setWatchinMovie\(false\)'){throw 'Native skip/completion no longer releases hold'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MovieMovementTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Movie movement native handler regression failed'}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$binary.Contains('Movie movement timeout for %s; releasing missing completion')){throw 'Canonical Release lacks movie movement implementation'}
 'PASS: canonical Release movie implementation marker'
}

param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/scene-lifecycle"
New-Item -ItemType Directory -Force $out | Out-Null
if($Baseline){$source=(& git -C $repo show 4449e4cc:src/server/game/Entities/Player/Player.cpp) -join "`n"}else{$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw}
$start=$source.IndexOf('void Player::SendSpellScene(');$end=$source.IndexOf('void Player::HandleArenaDeserter()',$start)
if($start -lt 0 -or $end -lt $start){throw 'Missing production scene lifecycle'}
[IO.File]::WriteAllText("$out/Lifecycle.inc",$source.Substring($start,$end-$start))
$source=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$source.Substring($source.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/SceneLifecycleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native scene lifecycle regression failed'}

param([string]$BaselineRevision)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/player-interrupt-cooldown"
if($BaselineRevision){$output+='-baseline'}
New-Item -ItemType Directory -Force $output | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:src/server/game/Entities/Player/Player.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline Player implementation'}}
$start=$source.IndexOf('void Player::ProhibitSpellSchool(');$end=$source.IndexOf('void Player::InitDataForForm(',$start)
if($start -lt 0 -or $end -le $start){throw 'Production interrupt cooldown boundaries changed'}
[IO.File]::WriteAllText("$output/PlayerInterruptCooldown.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /DNDEBUG /I`"$output`" `"$PSScriptRoot/PlayerInterruptCooldownTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Player interrupt cooldown regression failed'}

$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-rounds"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$header=Get-Content "$repo/src/server/scripts/Legion/stormheim_rounds.h" -Raw
[IO.File]::WriteAllText("$output/RoundsHeader.inc",[regex]::Replace($header,'(?m)^#include.*\r?\n',''))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_rounds.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Rounds.inc",$source.Substring(0,$source.IndexOf('class spell_stormheim_rounds_start')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('bool Player::CanCompleteQuest(');$b=$core.IndexOf('bool Player::CanCompleteRepeatableQuest(',$a)
[IO.File]::WriteAllText("$output/CanCompleteQuest.inc",$core.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/StormheimRoundsTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Stormheim rounds regression failed'}

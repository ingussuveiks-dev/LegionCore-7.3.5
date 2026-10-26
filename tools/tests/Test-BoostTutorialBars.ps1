param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-tutorial-bars"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$marker){$a=$source.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){if($source[$b] -eq '{'){$depth++};if($source[$b] -eq '}'){$depth--};$b++};$source.Substring($a,$b-$a)}
[IO.File]::WriteAllText("$out/Send.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::SendActionButtons('))
$script=Get-Content "$repo/src/server/scripts/Scenario/BoostExperience/boost_experience.cpp" -Raw
[IO.File]::WriteAllText("$out/Refresh.inc",(Block $script 'void RefreshTutorialActionBar()'))
if($script -notmatch 'void OnPlayerEnter\(Player\* player\) override\s*\{\s*_arrivalBarsPending = true;' -or $script -notmatch 'void Update\(uint32 diff\) override\s*\{\s*UpdateConstructLesson\(diff\);\s*RefreshTutorialActionBar\(\);'){throw 'Arrival refresh is not armed/called alongside the real construct lifecycle'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$flags=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $flags /I`"$out`" `"$PSScriptRoot/BoostTutorialBarsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Tutorial overlay regression failed'}

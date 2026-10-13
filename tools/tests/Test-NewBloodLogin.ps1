$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-login"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$source.IndexOf('    // The New Blood''s scenario selector is transient.')
$b=$source.IndexOf('    _LoadDailyQuestStatus(', $a)
if($a -lt 0 -or $b -lt $a){throw 'Login restoration missing'}
[IO.File]::WriteAllText("$output/Restore.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Maps/MapManager.cpp" -Raw
$a=$source.IndexOf('        bool brokenShoreIntro =')
$b=$source.IndexOf('    InstanceTemplate const* instance', $a)
$block=$source.Substring($a,$b-$a)
$block=$block.Substring(0,$block.LastIndexOf('    }'))
[IO.File]::WriteAllText("$output/Access.inc",$block)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/NewBloodLoginTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'New Blood login regression failed'}

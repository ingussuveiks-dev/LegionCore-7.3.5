$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-escape"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$s=Get-Content "$repo/src/server/scripts/Legion/new_blood.cpp" -Raw
$a=$s.IndexOf('class spell_new_blood_escape_meteor');$b=$s.IndexOf('void AddSC_new_blood()', $a)
if($a -lt 0 -or $b -lt $a){throw 'Meteor production script missing'}
[IO.File]::WriteAllText("$output/EscapeMeteor.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$s.IndexOf('    // handle emergency case - try to use other provided targets if no conditions provided')
$b=$s.IndexOf('    WorldObject* target = SearchNearbyTarget(', $a)
if($a -lt 0 -or $b -lt $a){throw 'Native target-46 production resolution missing'}
[IO.File]::WriteAllText("$output/NearbyDestination.inc",$s.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/NewBloodEscapeTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Escape destination regression failed'}

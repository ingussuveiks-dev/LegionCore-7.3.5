$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-riders-scythe"
New-Item -ItemType Directory -Force $out | Out-Null
$s=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$s.IndexOf('    // handle emergency case - try to use other provided targets if no conditions provided');$b=$s.IndexOf('    WorldObject* target = SearchNearbyTarget(', $a)
[IO.File]::WriteAllText("$out/NearbyDestination.inc",$s.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersScytheJumpTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Scythe destination regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkRidersScytheCatalog.cpp -ProbeName DarkRidersScytheCatalog -OutputFile "$out/native.tsv"
if((Get-Content "$out/native.tsv") -notcontains 'effect 199727 0 213 1 46'){throw 'Scythe spell is no longer native caster jump/nearby destination'}
'PASS: native7.3.5 jump213/caster1/destination46 confirmed'

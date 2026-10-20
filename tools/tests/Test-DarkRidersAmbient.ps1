param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-riders-ambient"
New-Item -ItemType Directory -Force $out | Out-Null
$s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp" -Raw
$a=$s.IndexOf('namespace DarkRidersAmbient');$b=$s.IndexOf('class npc_dark_riders_ambient',$a)
[IO.File]::WriteAllText("$out/Ambient.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$a=$s.IndexOf('void Creature::UpdateMovementFlags()');$b=$s.IndexOf('void Creature::UpdateStat()',$a)
[IO.File]::WriteAllText("$out/Movement.inc",$s.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkRidersAmbientTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Ambient motion regression failed'}
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 foreach($symbol in @('?UpdateAI@MotionAI@DarkRidersAmbient@@','?MovementInform@MotionAI@DarkRidersAmbient@@')){if(!$map.Contains($symbol)){throw "Missing canonical motion handler: $symbol"}}
 'PASS: canonical motion handler symbols; deployed instructions recorded separately'
}

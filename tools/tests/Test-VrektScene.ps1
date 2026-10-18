$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-scene"
New-Item -ItemType Directory -Force $out | Out-Null
function Block($source,$start){
 $a=$source.IndexOf($start);if($a -lt 0){throw "Missing native boundary: $start"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 $source.Substring($a,$b-$a)
}
$motion=Get-Content "$repo/src/server/game/Movement/MotionMaster.cpp" -Raw
$methods=(Block $motion 'void MotionMaster::MoveTakeoff(')+"`n"+(Block $motion 'void MotionMaster::MoveSmoothFlyPath(uint32 pointID, Position const position, float flightSpeed')
$methods=$methods.Replace('float flightSpeed/* = 0.0f*/','float flightSpeed')
[IO.File]::WriteAllText("$out/SceneNativeMovement.inc",$methods)
$point=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/SceneNativeInform.inc",(Block $point 'void EffectMovementGenerator::Finalize(')+"`n"+(Block $point 'void EffectMovementGenerator::MovementInform('))
$types=Get-Content "$repo/src/server/game/Movement/MotionMaster.h" -Raw
$unit=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
[IO.File]::WriteAllText("$out/SceneEnums.inc",(Block $types 'enum MovementGeneratorType : uint8')+";`n"+(Block $types 'enum MovementSlot')+";`n"+(Block $unit 'enum class AnimTier : uint8')+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /I`"$out`" /I`"$repo/src/server/scripts/Legion`" `"$PSScriptRoot/VrektSceneTest.cpp`" /Fe`"$out/VrektSceneTest.exe`" /Fo`"$out/VrektSceneTest.obj`"`nif errorlevel 1 exit /b 1`n`"$out/VrektSceneTest.exe`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd";if($LASTEXITCODE){throw 'Vrekt scene native regression failed'}

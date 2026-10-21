$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/pet-follow-movement"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$marker){$a=$source.IndexOf($marker);if($a -lt 0){throw "Missing production $marker"};$b=$source.IndexOf('{',$a);$end=$b+1;$depth=1;while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){$depth++};if($source[$end] -eq '}'){$depth--};$end++};if($depth){throw 'Unbalanced production handler'};$source.Substring($a,$end-$a)}
function Save([string]$name,[string]$body){[IO.File]::WriteAllText("$out/$name.inc",$body)}
$source=Get-Content "$repo/src/server/game/Movement/MovementGenerators/TargetedMovementGenerator.cpp" -Raw
foreach($pair in @(@('MoveType','static UnitMoveType GetFollowTargetMoveType('),@('Velocity','static Optional<float> GetVelocity('),@('Predict','static Position const PredictPosition('))){Save $pair[0] (Block $source $pair[1])}
Save 'Walking' ('template<>' + "`n" + (Block $source 'bool FollowMovementGenerator<Creature>::EnableWalking()'))
Save 'Update' ('template<>' + "`n" + (Block $source 'bool FollowMovementGenerator<Creature>::DoUpdate('))
Save 'PositionOkay' ('template<class T>' + "`n" + (Block $source 'bool FollowMovementGenerator<T>::PositionOkay('))
$spline=Get-Content "$repo/src/server/game/Movement/Spline/MoveSpline.cpp" -Raw
Save 'SelectSpeed' (Block $spline 'uint32 SelectSpeedType(')
Save 'Finalize' (Block $spline 'void MoveSpline::_Finalize(')
Save 'MovementFlags' ((Block (Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw) 'enum MovementFlags :')+';')
Save 'MoveTypes' ((Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum UnitMoveType')+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /D_USE_MATH_DEFINES /W4 /I`"$out`" `"$PSScriptRoot/PetFollowMovementTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native pet-follow movement regression failed'}

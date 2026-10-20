param([string]$BaselineRevision)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/visible-aura-removal"
if($BaselineRevision){$output+='-baseline'}
New-Item -ItemType Directory -Force $output | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:src/server/game/Entities/Unit/Unit.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline Unit implementation'}}
foreach($section in @(@('VisibleAuraRemoval','void Unit::RemoveVisibleAura(','float Unit::ApplyEffectModifiers('),@('VisibleAuraComparator','bool Unit::VisibleAuraSlotCompare::operator()','float Unit::CalculateDefaultCoefficient('))){
    $start=$source.IndexOf($section[1]);$end=$source.IndexOf($section[2],$start)
    if($start -lt 0 -or $end -le $start){throw 'Production visible aura removal boundaries changed'}
    [IO.File]::WriteAllText("$output/$($section[0]).inc",$source.Substring($start,$end-$start))
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /DNDEBUG /I`"$output`" /I`"C:/local/boost_1_85_0`" `"$PSScriptRoot/VisibleAuraRemovalTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Visible aura removal regression failed'}

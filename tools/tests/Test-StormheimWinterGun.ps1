$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-winter-gun"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_no_wings.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/NoWings.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_no_wings()')))
$core=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
foreach($segment in @(@('JumpDest','void Spell::EffectJumpDest(','void Spell::CalculateJumpSpeeds('),@('JumpSpeeds','void Spell::CalculateJumpSpeeds(','void Spell::EffectIncreaseCurrencyCap('))){
 $a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing core handler $($segment[0])"}
 [IO.File]::WriteAllText("$output/$($segment[0]).inc",$core.Substring($a,$b-$a))
}
$range=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$range.IndexOf('bool Unit::IsWithinCombatRange(');$b=$range.IndexOf('bool Unit::IsWithinMeleeRange(',$a)
[IO.File]::WriteAllText("$output/CombatRange.inc",$range.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimWinterGunTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Winter gun production regression failed'}

param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/ritual-ruination"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/ritual_ruination.cpp" -Raw
$a=$source.IndexOf('namespace RitualRuination')
$b=$source.IndexOf('class spell_ritual_fel_bat',$a)
[IO.File]::WriteAllText("$output/Ritual.inc",$source.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
$moving=[regex]::Match($unit,'bool isMoving\(\) const\s*\{[^}]+\}')
if(!$moving.Success){throw 'Native moving predicate missing'}
[IO.File]::WriteAllText("$output/IsMoving.inc",$moving.Value)
$summons=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
$a=$summons.IndexOf('void TempSummon::InitStats(')
$b=$summons.IndexOf('    if (owner)',$summons.IndexOf('    if (!m_Properties)',$a))
$prefix=$summons.Substring($a,$b-$a)
if(!$prefix.Contains('if (!m_Properties)')){throw 'Native direct summon early return changed'}
$registration=[regex]::Match($summons,'    if \(owner\)\s*owner->tempSummonList\[GetEntry\(\)\].push_back\(GetGUID\(\)\);')
if(!$registration.Success){throw 'Native summon registration missing'}
[IO.File]::WriteAllText("$output/DirectSummonRegistry.inc",$prefix+$registration.Value+"`n}`n")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/RitualRuinationTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`nif errorlevel 1 exit /b 1`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/DirectSummonRegistryTest.cpp`" /Fe`"$output/registry.exe`" /Fo`"$output/registry.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/registry.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Ritual Ruination native handler regression failed'}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$binary.Contains('Ritual Ruination: ended stale Fel Bat controller')){throw 'Canonical executable has stale Ritual Ruination code'}
 if(!$binary.Contains('Ritual Ruination: created quest-bound actor %u for player %u')){throw 'Canonical executable lacks personal-actor recovery repair'}
 'PASS: deployed Release contains the Ritual Ruination repair'
}

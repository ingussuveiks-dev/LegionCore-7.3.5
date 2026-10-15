param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ulthalesh"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp" -Raw
$a=$s.IndexOf('namespace DarkRidersArtifact');$b=$s.IndexOf('class instance_dark_riders_artifact',$a)
[IO.File]::WriteAllText("$out/ProtectFinale.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$s.IndexOf('void SmartAI::DamageTaken(');$b=$s.IndexOf('void SmartAI::HealReceived(',$a)
[IO.File]::WriteAllText("$out/SmartDamage.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$s.IndexOf('uint32 Unit::DealDamage(');$b=$s.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);',$a)
if($s.IndexOf('victim->GetAI()->DamageTaken(this, damage, damagetype);',$a) -gt $b){throw 'Native damage order changed'}
$end=$s.IndexOf('    if (!victim->IsControlledByPlayer()',$b)
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",$s.Substring($b,$end-$b))
if($s.IndexOf('    if (health <= damage)',$end) -lt $end){throw 'Native death check missing after hook'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/UlthaleshDamageTest.cpp`" /Fe`"$out/damage.exe`" /Fo`"$out/damage.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/damage.exe`"`r`n"
[IO.File]::WriteAllText("$out/damage-run.cmd",$cmd);& "$out/damage-run.cmd"
if($LASTEXITCODE){throw 'Ulthalesh native damage regression failed'}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 if(!$binary.Contains("Ulthalesh: protected Ariden's finale from a scaled killing blow")){throw 'Canonical Release lacks Ulthalesh hook'}
 'PASS: canonical executable contains Ulthalesh finale protection'
}

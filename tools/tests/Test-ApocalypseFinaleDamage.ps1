param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/build-extractors/runtime-test/apocalypse-finale-damage-$mode"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
New-Item -ItemType Directory -Force -Path $out | Out-Null
$s=if($Baseline){(& git -C $repo show 9731fc183:src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp) -join "`n"}else{Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_riders_artifact.cpp" -Raw}
$a=$s.IndexOf('namespace DarkRidersArtifact');$b=$s.IndexOf('namespace DarkRidersArrival',$a)
[IO.File]::WriteAllText("$out/ProtectFinale.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$s.IndexOf('void SmartAI::DamageTaken(');$b=$s.IndexOf('void SmartAI::HealReceived(',$a)
[IO.File]::WriteAllText("$out/SmartDamage.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$s.IndexOf('uint32 Unit::DealDamage(');$b=$s.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);',$a)
if($s.IndexOf('victim->GetAI()->DamageTaken(this, damage, damagetype);',$a) -gt $b){throw 'Native damage order changed'}
$hook=$s.IndexOf('    if (Creature* creature = victim->ToCreature())',$b)
if($b -lt $a -or $hook -lt $b){throw 'Native scaled damage/creature hook missing'}
# Extract the complete post-scaling hook block. Loot tagging now has its own
# Creature gate, so its old IsControlledByPlayer spelling is not a boundary.
$end=$s.IndexOf('{',$hook)+1;$depth=1
while($depth -and $end -lt $s.Length){if($s[$end] -eq '{'){++$depth};if($s[$end] -eq '}'){--$depth};++$end}
if($depth){throw 'Unbalanced native post-scaling damage hook'}
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",$s.Substring($b,$end-$b))
if($s.IndexOf('    if (health <= damage)',$end) -lt $end){throw 'Native death check missing after hook'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$define=if($Baseline){'/DAPOCALYPSE_BASELINE'}else{''}
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $define /I`"$out`" `"$PSScriptRoot/ApocalypseFinaleDamageTest.cpp`" /Fe`"$out/damage.exe`" /Fo`"$out/damage.obj`" /Fd`"$out/damage-compile.pdb`" /link /INCREMENTAL:NO`r`nif errorlevel 1 exit /b 1`r`n`"$out/damage.exe`"`r`n"
[IO.File]::WriteAllText("$out/damage-run.cmd",$cmd);& "$out/damage-run.cmd"
if($LASTEXITCODE){throw 'Apocalypse native damage regression failed'}
}

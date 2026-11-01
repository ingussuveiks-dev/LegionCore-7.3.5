param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release",[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/summon-controller-cleanup")
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify a repaired executable'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out) -Test {
 $relative='src/server/game/Entities/Unit/Unit.cpp'
 if($Baseline){$source=(& git -C $repo show "c8c57558d:$relative") -join "`n";if($LASTEXITCODE){throw 'Cannot read pinned pre-fix source'}}else{$source=[IO.File]::ReadAllText("$repo/$relative")}
 $start=$source.IndexOf('void Unit::RemoveAllMinionsByFilter(');$end=$source.IndexOf('void Unit::SetCharm(', $start)
 if($start -lt 0 -or $end -le $start){throw 'Actual summon cleanup method missing'}
 [IO.File]::WriteAllText("$out/SummonControllerCleanup.inc",$source.Substring($start,$end-$start))
 $aura=[IO.File]::ReadAllText("$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp")
 $start=$aura.IndexOf('void AuraEffect::HandleSummonController(');$end=$aura.IndexOf('void AuraEffect::HandleModNextSpell(', $start)
 if($start -lt 0 -or $end -le $start){throw 'Actual summon-controller aura method missing'}
 [IO.File]::WriteAllText("$out/SummonControllerAura.inc",$aura.Substring($start,$end-$start))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 if(!$vs){throw 'MSVC compiler missing'}
 $sanitize=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 $mode=if($Baseline){'baseline'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`nif errorlevel 1 exit /b 1`ncl /nologo /EHsc /std:c++17 /W4 $sanitize /I`"$out`" `"$PSScriptRoot/SummonControllerCleanupTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $mode`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/stdout.log"
 if($LASTEXITCODE){Get-Content "$out/stdout.log" -Tail 16;throw 'Summon-controller cleanup regression failed'}
 $lines=@(Get-Content "$out/stdout.log")
 if($Baseline){if(@($lines|Where-Object {$_ -eq 'EXPECTED_BROKEN native controller leaves an ordinary owned summon'}).Count -ne 1){throw 'Expected counterfactual was not reproduced'}}
 elseif(@($lines|Where-Object {$_ -like 'CASE * PASS'}).Count -ne 31 -or $lines[-1] -cne 'PASS summon controller cleanup31'){throw 'Incomplete cleanup regression matrix'}
 $lines|Select-Object -Last 2
 if($VerifyReleaseBinary){
  $prior=$env:PYTHONPATH
  try{
   $env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior"
   & "$env:USERPROFILE/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe" "$PSScriptRoot/verify_summon_controller_cleanup_release.py" $ReleaseDirectory
   if($LASTEXITCODE){throw 'Linked summon-controller cleanup verification failed'}
  }finally{$env:PYTHONPATH=$prior}
 }
}

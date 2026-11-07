param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned Hunter counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/survival-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test/survival-graveyard-native","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 if(!$Baseline){
  $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp" -Raw
  $a=$source.IndexOf('        WorldLocation* GetClosestGraveYard(');$b=$source.IndexOf('        void Initialize()',$a);if($a -lt 0 -or $b -le $a){throw 'Actual shared cemetery callback missing'}
  [IO.File]::WriteAllText("$out/FuryGraveyard.inc",$source.Substring($a,$b-$a))
  $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  $cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" `"$PSScriptRoot/SurvivalGraveyardTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
  [IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){throw "Hunter cemetery ASAN failed: $out/source.log"}
 }
 if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SurvivalGraveyardBaseline *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SurvivalGraveyard *> "$out/native.log"}
 if(!$?){throw "Native Hunter cemetery failed: $out/native.log"}
 $text=[IO.File]::ReadAllText("$out/native.log")
 if($Baseline){if($text -notmatch 'COUNTERFACTUAL pinned5d3d9312b Hunter native110/release104'){throw 'Unexpected Hunter baseline variant/count'};'COUNTERFACTUAL Hunter original instance/native110:104 outdoor transfers leave ghosts,26 choose Fury cemetery;not successful gameplay'}else{
  if($text -notmatch 'PASS native Survival graveyard110/release104' -or $text -match 'COUNTERFACTUAL pinned'){throw 'Unexpected native Hunter repaired variant/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_survival_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw "Selected Hunter cemetery Release failed: $out/release.log"}}finally{$env:PYTHONPATH=$old}}
  'PASS Survival cemetery:source58 ASAN/native110 with104 actual ghost release/near ACK/delayed revival,13 steps/native6->7 boundary,distinct5295/5296 and retained Fury;no progression/item grant. Full client/corpse/pet/harpoon traversal separate.'
 }
}

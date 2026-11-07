param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned early-capsule counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/warrior-login-recovery-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('warrior-login-recovery-native','scenario-resume','scepter-login','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-ScenarioResume.ps1" -AddressSanitizer -LoginBaseline:$Baseline *> "$out/source.log";if(!$?){throw "Actual/pinned early-loader source failed: $out/source.log"}
 if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -WarriorRecoveryBaseline *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -WarriorRecovery *> "$out/native.log"}
 if(!$?){throw "Native early recovery failed: $out/native.log"}
 $native=[IO.File]::ReadAllText("$out/native.log")
 if($Baseline){
  if($native -notmatch 'COUNTERFACTUAL pinned5ec346c8e early capsule/current native support132'){throw 'Unexpected native baseline variant/count'}
  'COUNTERFACTUAL pinned5ec346c8e early loader/source132 and native capsule132 with current native support;no historical whole binary or ordinary gameplay success claimed.'
 }else{
  if($native -notmatch 'PASS actual early capsule/native resume138' -or $native -match 'COUNTERFACTUAL pinned5ec346c8e'){throw 'Unexpected native recovery variant/count'}
  & "$PSScriptRoot/Test-ScepterLogin.ps1" *> "$out/scepter.log";if(!$?){throw 'Retained Scepter quest-load ordering failed'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warrior_login_recovery_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw "Selected early-loader Release failed: $out/release.log"}}finally{$env:PYTHONPATH=$old}}
  'PASS Warrior missing-checkpoint recovery:source132 ASAN/native138,all3 routes/Human+Orc,one accepted quest-row load,explicit fresh native entrance and no old-instance adoption,late active/rewarded admission,existing valid live/fresh/complete record preservation;wrong source/class/map/unlock/position/status refuse. Quest parser/saved transport/position-valid endpoints controlled;full character/client login remains separate.'
 }
}

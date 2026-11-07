param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned route-dependency counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/marksmanship-login-recovery-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('marksmanship-login-recovery-native','marksmanship-login-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-MarksmanshipLoginSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw "Marksmanship source recovery failed: $out/source.log"}
 if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanRecoveryBaseline *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanRecovery *> "$out/native.log"}
 if(!$?){throw "Native Marksmanship recovery failed: $out/native.log"}
 $native=[IO.File]::ReadAllText("$out/native.log");$source=[IO.File]::ReadAllText("$out/source.log")
 if($Baseline){
  if($native -notmatch 'COUNTERFACTUAL pinned387b8a385 route/early capsule62' -or $source -notmatch 'COUNTERFACTUAL pinned387b8a385 source Marksmanship68'){throw 'Unexpected Marksmanship counterfactual variant/count'}
  'COUNTERFACTUAL pinned387b8a385 route dependency and early/normal/Prepare/CanResume source68 ASAN;native early capsule62 with current native support. Empty checkpoint selection absent. Namespace isolation retains the pinned inline dependency;not historical whole-binary/client success.'
 }else{
  if($native -notmatch 'PASS native Marksmanship login64' -or $source -notmatch 'PASS extracted Marksmanship early login68 ASAN'){throw 'Unexpected Marksmanship recovery variant/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_marksmanship_login_recovery_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw "Selected Marksmanship login Release failed: $out/release.log"}}finally{$env:PYTHONPATH=$old}}
  'PASS Marksmanship empty-checkpoint login:source68 ASAN/native64,Human+Orc,actual201674/972 fresh entrance,one accepted quest load,existing live/fresh/complete and recorded claim preservation,source/status/foreign receipt/selected-map safeguards;zero-instance+boot seed refuses a persisted real103490 claim. No item/quest/return grant. Parser/transport/position/binding endpoints controlled;full saved-character/group/client/earned-return remain separate.'
 }
}

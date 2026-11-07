param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned source/caller counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/survival-login-recovery-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('survival-login-recovery-native','survival-login-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-SurvivalLoginSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw "Hunter early-loader source failed: $out/source.log"}
 if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -HunterRecoveryBaseline *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -HunterRecovery *> "$out/native.log"}
 if(!$?){throw "Native Hunter recovery failed: $out/native.log"}
 $native=[IO.File]::ReadAllText("$out/native.log");$source=[IO.File]::ReadAllText("$out/source.log")
 if($Baseline){
  if($native -notmatch 'COUNTERFACTUAL pinned0af584162 early/caller capsules116' -or $source -notmatch 'COUNTERFACTUAL pinned0af584162 source Hunter68'){throw 'Unexpected Hunter counterfactual variant/count'}
  'COUNTERFACTUAL pinned0af584162 early/caller source68 ASAN and native capsules116 with current native support;missing-entry selection and earned-owner exit absent. Not historical whole-binary/client success.'
 }else{
  if($native -notmatch 'PASS native Hunter login120' -or $source -notmatch 'PASS extracted Hunter early login68 ASAN'){throw 'Unexpected Hunter recovery variant/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_acquisition_login_recovery_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw "Selected Hunter Release failed: $out/release.log"}}finally{$env:PYTHONPATH=$old}}
  'PASS Hunter login recovery:source68 ASAN/native120,Human+Orc,native198950 fresh973 entrance,existing exact live receipt and selected-map binding safeguards;real owned loading/equipment/inventory-bag/bank-bag Talonclaw plus persisted flight/claim uses190598 exit without credit/item/resurrection. Valid-map,failed/foreign/pending/duplicate teleport guards. Quest parser,saved transport/position,selected-map,zone/teleport endpoints controlled;full character/group/client/arrival separate.'
 }
}

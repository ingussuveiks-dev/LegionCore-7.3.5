param([switch]$Baseline,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and ($VerifyLiveData -or $VerifyReleaseBinary)){throw 'Missing-data counterfactual cannot certify live data or Release'}
$out="$repo/build-extractors/runtime-test/survival-resume-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test/survival-resume-data","$repo/build-extractors/runtime-test/survival-entrance-native","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SurvivalEntranceBaseline *> "$out/native.log"}else{
  & "$PSScriptRoot/Test-SurvivalResumeData.ps1" -VerifyLiveData:$VerifyLiveData *> "$out/data.log";if(!$?){throw "Native entrance SQL regression failed: $out/data.log"}
  if($VerifyLiveData){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SurvivalEntranceLive *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SurvivalEntrance *> "$out/native.log"}
 }
 if(!$?){throw "Native Hunter entrance regression failed: $out/native.log"}
 $native=[IO.File]::ReadAllText("$out/native.log")
 if($Baseline){if($native -notmatch 'COUNTERFACTUAL native Hunter entrance30'){throw 'Unexpected missing-entrance baseline/count'};'COUNTERFACTUAL original missing1178/1384 data:24 native late records fail;ordinary/existing/Fury6 controls retained;not ordinary success'}else{
  if($native -notmatch 'PASS native Hunter entrance40' -or $native -match 'COUNTERFACTUAL native Hunter'){throw 'Unexpected corrected native entrance/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warrior_scenario_resume_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log";if($LASTEXITCODE){throw 'Actual linked native Recorder/LFG data branch failed'}}finally{$env:PYTHONPATH=$old}}
  'PASS native Survival entrance:data TEMP16 refusal/partial/idempotence/preservation,native40 actual recorder/prepare/live/fresh/two constructors and terrain;shared Fury retained. No movement/item/credit/character writes;empty old receipts and full client/login remain separate.'
 }
}

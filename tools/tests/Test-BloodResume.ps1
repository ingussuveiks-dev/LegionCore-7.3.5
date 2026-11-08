param([switch]$Baseline,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and ($VerifyLiveData -or $VerifyReleaseBinary)){throw 'Missing-entrance counterfactual cannot certify live Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/blood-resume-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('blood-entrance-native','blood-resume-data','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -BloodEntranceBaseline *> "$out/native.log";if(!$?){throw 'Blood absent-entrance counterfactual failed'}
  if((Get-Content "$out/native.log" -Raw) -notmatch 'COUNTERFACTUAL native Blood entrance28'){throw 'Unexpected Blood counterfactual variant/count'}
  'COUNTERFACTUAL Blood missing1059 entrance:20 lost late checkpoints and8 retained ordinary/existing/shared972/completed controls. TEMP absence only,not ordinary successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-BloodResumeData.ps1" -VerifyLiveData:$VerifyLiveData *> "$out/data.log";if(!$?){throw 'Blood entrance data failed'}
  if($VerifyLiveData){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -BloodEntranceLive *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -BloodEntrance *> "$out/native.log"};if(!$?){throw 'Blood native entrance failed'}
  if((Get-Content "$out/native.log" -Raw) -notmatch 'PASS native Blood entrance36'){throw 'Unexpected Blood native variant/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warrior_scenario_resume_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked native Recorder verification failed'}}finally{$env:PYTHONPATH=$old}}
  'PASS Blood1059/940 native entrance:full-schema TEMP18 refusals/idempotence/source+route+portal preservation,native36 both factions,20 late checkpoints,original poses,live/boot/numeric mismatch,destroyed Map/recycledID/fresh step0,real native entry floor,completed cleanup and Hunter972 control. Earlier quest/stage/pose and final48857 input controlled;full saved-character/group/client traversal remains separate.'
 }
}

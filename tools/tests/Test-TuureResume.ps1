param([switch]$Baseline,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and ($VerifyLiveData -or $VerifyReleaseBinary)){throw 'Legacy-coordinate counterfactual cannot certify live Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/tuure-resume-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('tuure-entrance-native','tuure-resume-data','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -TuureEntranceBaseline *> "$out/native.log";if(!$?){throw 'Tuure legacy-coordinate counterfactual failed'}
  if((Get-Content "$out/native.log" -Raw) -notmatch 'COUNTERFACTUAL native Tuure entrance22'){throw 'Unexpected Tuure counterfactual variant/count'}
  'COUNTERFACTUAL native Tuure legacy1243:12 wrong late receipts+2 wrong fresh restarts,8 original/existing/shared972/completed controls retained. TEMP original Dalaran coordinates,not ordinary successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-TuureResumeData.ps1" -VerifyLiveData:$VerifyLiveData *> "$out/data.log";if(!$?){throw 'Tuure entrance data failed'}
  if($VerifyLiveData){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -TuureEntranceLive *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -TuureEntrance *> "$out/native.log"};if(!$?){throw 'Tuure native entrance failed'}
  if((Get-Content "$out/native.log" -Raw) -notmatch 'PASS native Tuure entrance28'){throw 'Unexpected Tuure native variant/count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_warrior_scenario_resume_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Recorder failed'}}finally{$env:PYTHONPATH=$old}}
  'PASS Tuure1243/1085 native5480 entrance:full-schema TEMP22 refusals,four legacy cells/name retained,absent/idempotent/custom protection,native28 both factions,12 late receipts,ordinary/existing/live/boot/numeric mismatch,destroyed Map/recycledID/fresh step0,real floor/facing conversion,actual two-event ALL-root cleanup and shared972 control. Quest/stage/pose/final events controlled;full saved-character/group/client traversal separate.'
 }
}

param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned death-pose counterfactual cannot certify repaired Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/sharasdal-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test/sharasdal-graveyard-source","$repo/build-extractors/runtime-test/sharasdal-graveyard-native","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-SharasdalGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Sharasdal cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalGraveyard -SharasdalGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Sharasdal cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinnedd8cc8bb9e Sharasdal source178' -or $native -notmatch 'COUNTERFACTUAL pinnedd8cc8bb9e Sharasdal native72/release64'){throw 'Unexpected counterfactual count/variant'}
  'COUNTERFACTUAL pinned Sharasdal source178 ASAN/native72/release64:death pose,including below-map,returned as successful recovery;current native support,not historical whole binary.'
 }else{
  if($source -notmatch 'PASS Sharasdal graveyard206' -or $native -notmatch 'PASS Sharasdal native72/release64'){throw 'Unexpected repaired count/variant'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_sharasdal_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Sharasdal linked cemetery proof failed'}}finally{$env:PYTHONPATH=$old}}
  'PASS Sharasdal cemetery:source206 ASAN/native72 with64 real releases/ACKs,all6 stages+completed,earned/unearned stage3,50336/47131 and50314 AND both orders,5443/5473 native floor/facing,28 source refusals,missing/foreign scenario null,no progression/item/ascent-owner mutation. Earlier states/final criterion controlled;client/elevator/path/pet/group remain separate.'
 }
}

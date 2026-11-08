param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/blood-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('blood-graveyard-native','blood-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-BloodGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Tuure cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -BloodGraveyard -BloodGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Tuure cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinned9d0d0d1bb Blood source115' -or $native -notmatch 'COUNTERFACTUAL pinned9d0d0d1bb Blood native102/release90'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Blood source115 ASAN/native102/release90:pinned whole instance echoes unsafe corpse poses through real release/ACK with neither barrier checkpoint. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS extracted Blood graveyard156' -or $native -notmatch 'PASS Blood native102/release90'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_blood_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Tuure cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Blood cemetery:source156 ASAN/native102 with90 releases/ACKs,201678/5440/5441 real floors/facing,all11 stages+completed,both native barrier48612 at2->3 and48687 at6->7/early+duplicate denials,Human+Orc/active statuses,41 source refusals,retained Holy/Hunter/other/no-scenario;no callback progression/item/direct resurrection. Earlier state/final criterion endpoints controlled;client/barrier/pet/group/corpse traversal separate.'
 }
}

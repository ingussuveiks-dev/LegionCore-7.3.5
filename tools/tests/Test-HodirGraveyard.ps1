param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/hodir-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hodir-graveyard-native','hodir-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-HodirGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Hodir cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -HodirGraveyard -HodirGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Hodir cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinnedd2f2840b2 Hodir graveyard source79' -or $native -notmatch 'COUNTERFACTUAL pinnedd2f2840b2 Hodir native62/release50'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Hodir source79 ASAN/native62/release50:pinned whole instance echoes unsafe corpse poses through real release/ACK while ignoring the landing checkpoint. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS extracted Hodir graveyard107' -or $native -notmatch 'PASS Hodir native62/release50'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hodir_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Hodir cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Hodir cemetery:source107 ASAN/native62 with50 releases/ACKs,5654/5655 real floors/facing,all6 stages+completed,native landing52537 at2->3/early+duplicate denials,Human+Orc/active statuses,28 source refusals,retained Holy/MM/Blood/other/no-scenario;no callback progression/item/direct resurrection. Earlier state/final criterion endpoints controlled;client/dive/pet/group/corpse traversal separate.'
 }
}

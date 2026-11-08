param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/shadow-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('shadow-graveyard-native','shadow-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-ShadowGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Shadow cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ShadowGraveyard -ShadowGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Shadow cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinned41e2b7743 Shadow source107' -or $native -notmatch 'COUNTERFACTUAL pinned41e2b7743 Shadow native94/release82'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Shadow source107 ASAN/native94/release82:pinned whole instance revives all class stages at generic5281/zero-facing. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS Shadow graveyard135' -or $native -notmatch 'PASS Shadow native94/release82'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_shadow_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Shadow cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Shadow cemetery:source135 ASAN/native94 with82 releases/ACKs,early5281/new5335/5351 real floors/facing,all10 stages+completed,native48121 at1->2 and48393 at6->7/early+duplicate denials,Human+BloodElf/active statuses,28 source refusals,retained Arms/Holy/other/no-scenario;no callback progression/item/direct resurrection. Earlier state/final criterion endpoints controlled;client/dive/pet/group/corpse traversal separate.'
 }
}

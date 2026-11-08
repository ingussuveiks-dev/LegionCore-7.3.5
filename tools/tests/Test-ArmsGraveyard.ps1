param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/arms-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('arms-graveyard-native','arms-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-ArmsGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Arms cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ArmsGraveyard -ArmsGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Arms cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinned96ca7646c Arms source107' -or $native -notmatch 'COUNTERFACTUAL pinned96ca7646c Arms native94/release82'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Arms source107 ASAN/native94/release82:pinned whole instance revives all class stages at generic5281/zero-facing. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS Arms graveyard149' -or $native -notmatch 'PASS Arms native94/release82'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_arms_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Arms cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Arms cemetery:source149 ASAN/native94 with82 releases/ACKs,5339/5335/5705 real floors/facing,all10 stages+completed,native49022 at3->4 and49058 at6->7/early+duplicate denials,Human+Orc/active statuses,42 source refusals,retained Shadow/Holy/other/no-scenario;no callback progression/item/direct resurrection. Earlier state/final criterion endpoints controlled;client/dive/pet/group/corpse traversal separate.'
 }
}

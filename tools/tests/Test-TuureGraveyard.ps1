param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/tuure-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('tuure-graveyard-native','tuure-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-TuureGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Tuure cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -TuureGraveyard -TuureGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Tuure cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinned562084277 Tuure graveyard source80' -or $native -notmatch 'COUNTERFACTUAL pinned562084277 Tuure native64/release58'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Tuure source80 ASAN/native64/release58:pinned whole instance echoes corpse/foreign-world/below-map poses through actual release+ACK. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS extracted Tuure graveyard96' -or $native -notmatch 'PASS Tuure native64/release58'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_tuure_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Tuure cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Tuure cemetery:source96 ASAN/native64 with58 actual releases/ACKs,5480 floor/facing,all7 stages+completed,native Human+Undead pairs,invalid-source diagnostics,foreign/below-map death poses,shared940/972/no-scenario controls;no progression/item or direct resurrection from callback. Earlier state/final criterion endpoints controlled;full client/corpse/pet/group traversal separate.'
 }
}

param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/marksmanship-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('marksmanship-graveyard-native','marksmanship-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-MarksmanshipGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Tuure cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanGraveyard -MarksmanGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Tuure cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinnedf3c39b87a MM graveyard source84' -or $native -notmatch 'COUNTERFACTUAL pinnedf3c39b87a MM native74/release66'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL MM source84 ASAN/native74/release66:pinned whole instance echoes unsafe corpse poses through native release/ACK and has no acknowledged-Herald cemetery boundary. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS extracted MM graveyard112' -or $native -notmatch 'PASS MM native74/release66'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_marksmanship_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Tuure cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Marksmanship cemetery:source112 ASAN/native74 with66 releases/ACKs,5433/5434 real floors/facing,all8 stages+completed/native Herald47927 boundary4->5,early/repeated event denials,Human+Orc/active statuses,28 source refusals,retained Holy/Blood/no-scenario;no callback progression/item/direct resurrection. Earlier death/state/final event endpoints controlled;actual client/corpse/pet/group/Herald combat separate.'
 }
}

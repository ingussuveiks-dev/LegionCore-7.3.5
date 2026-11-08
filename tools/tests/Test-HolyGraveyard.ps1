param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned callback counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/holy-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('holy-graveyard-native','holy-graveyard-source','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-HolyGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Holy cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -HolyGraveyard -HolyGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Holy cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'COUNTERFACTUAL pinned2bd533cfd Holy source115' -or $native -notmatch 'COUNTERFACTUAL pinned2bd533cfd Holy native104/release90'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Holy source115 ASAN/native104/release90:pinned whole instance revives all class stages at generic5281/zero-facing. Current native support,not historical whole binary or safe ordinary recovery.'
 }else{
  if($source -notmatch 'PASS Holy graveyard157' -or $native -notmatch 'PASS Holy native104/release90'){throw 'Unexpected repaired variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_holy_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Holy cemetery failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Holy cemetery:source157 ASAN/native104 with90 releases/ACKs,5488/5693/5694 real floors/facing,all11 stages+completed,native50766 at1->2/50897 at4->5/50912 at6->7/early+duplicate denials,Human+BloodElf/active statuses,42 source refusals,retained Arms/Shadow/other/no-scenario;no callback progression/item/direct resurrection. Earlier state/final criterion endpoints controlled;client/dive/pet/group/corpse traversal separate.'
 }
}

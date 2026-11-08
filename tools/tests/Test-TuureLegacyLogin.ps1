param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned Prepare counterfactual cannot certify Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/tuure-legacy-login-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('tuure-legacy-source','tuure-legacy-login-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-TuureLegacySource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Tuure legacy source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -TuureLegacy -TuureLegacyBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Tuure legacy native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch '74' -or $native -notmatch 'COUNTERFACTUAL pinnedfdb24afb8 Tuure Prepare native50'){throw 'Unexpected pinned variant/count'}
  'COUNTERFACTUAL Tuure source74 ASAN/native50:pinned fdb24afb8 Prepare reproduces bad fresh/live/falling positions with legitimate/custom/completed/recycled-ID controls;native baseline uses a pinned method capsule and current support libraries,not a historical whole binary.'
 }else{
  if($source -notmatch '118' -or $native -notmatch 'PASS native Tuure legacy login94'){throw 'Unexpected repair variant/count'}
  if($VerifyReleaseBinary){
   $old=$env:PYTHONPATH
   try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_tuure_legacy_login_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked Tuure legacy repair failed'}}finally{$env:PYTHONPATH=$old}
  }
  'PASS Tuure legacy source118 ASAN/native94:both real Legion Priest race pairs,active incomplete/complete,legacy-only normalization,valid live pose retained,bad XY/falling pose recovered,44 native source/provenance refusals,completed cleanup,serialization and real destroyed-map/recycled-ID fresh constructor;no character DB writes or quest/item/criteria grants. Full LoadFromDB/group/client traversal remains separate.'
 }
}

param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$VerifyLiveData,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and ($VerifyReleaseBinary -or $VerifyLiveData)){throw 'Pinned source/legacy TEMP data cannot certify repaired Release/live data'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/sharasdal-entrance-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 & "$PSScriptRoot/Test-SharasdalLegacySource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Sharasdal legacy source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalEntrance -SharasdalEntranceBaseline:$Baseline -SharasdalEntranceLive:$VerifyLiveData *> "$out/native.log";if(!$?){throw 'Sharasdal entrance native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){
  if($source -notmatch 'Sharasdal login74' -or $native -notmatch 'Sharasdal Prepare native60'){throw 'Unexpected pinned source/native count'}
  'COUNTERFACTUAL pinned677430a07 source74 ASAN/native60:original1229 late records/fresh/live/falling legacy checkpoints remain wrong. Pinned Prepare capsule/current support;not historical whole binary.'
 }else{
  if($source -notmatch 'Sharasdal legacy login126' -or $native -notmatch 'Sharasdal legacy login112'){throw 'Unexpected repaired source/native count'}
  if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_sharasdal_legacy_login_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked legacy Sharasdal source missing'}}finally{$env:PYTHONPATH=$old}}
  'PASS Sharasdal entrance source126 ASAN/native112:actual207683/1229 loader and late Recorder,fresh/live/falling/valid/custom/completed/recycled-instance checkpoints,exact legacy-only source/tuple/header proof and52 native refusals,actual native1066 fresh constructor;no character reset/item/criteria grant. Full LoadFromDB/client/pet/group separate.'
 }
}

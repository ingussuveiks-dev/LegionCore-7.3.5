param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned reset counterfactual cannot certify current Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-reset-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('magnar-reset','magnar-reset-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ResetBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned reset counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL4b541a943: actual first-phase Reset leaves Hruthnir consumed start/497 and no offer; expected broken baseline'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ResetOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method reset regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ResetRecovery *> "$out/native.log"
  if(!$?){throw "Native reset integration failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_magnar_reset_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical reset certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS real acknowledged Magnar Reset/reserve cleanup -> original point6/full13s offer -> native cached option/point7/497/full6s/new roster; no victory grant. Full client traversal remains separate.'
 }
}

param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned cold corridor counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-cold-route-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-gossip','hruthnir-cold-route-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ColdRouteBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned cold corridor counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL05c9445bd: legitimate delivered instance/fresh Hruthnir at actual original DB entrance remains>400yd from chamber,nearest-boss60 prerequisite prevents reoffer; no real route after10s. Expected broken control.'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ColdRouteOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method cold route regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ColdRoute *> "$out/native.log"
  if(!$?){throw "Native cold corridor regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_cold_route_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical cold route certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS original delivered Hruthnir cold-entrance corridor: real native12-leg spline/arrival/normal mmap path,participant and movement deferrals/retry,original13s cached offer/point7/497/6s start; no old combat/credit/teleport grants. Actual client/dynamic gates remain separate.'
 }
}

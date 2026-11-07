param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Expected-broken cursor counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-primary-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-gossip','hruthnir-primary-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -PrimaryBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned cursor counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL 83a8cb6e3: original DoAction2/point6 approach loses unrecorded intent on same fullGUID AI replacement and remains idle. Expected broken control, not successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ColdRouteOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method cursor regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -PrimaryApproach *> "$out/native.log"
  if(!$?){throw "Native cursor regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_primary_approach_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical cursor certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS original Hruthnir DoAction2 approach retains leg12 intent and actual point6 arrival13 through same-fullGUID AI replacement; current instance epoch/receipt/actor/phase and real arrival guard progress. Original offer/start clocks remain; actual client traversal is separate.'
 }
}

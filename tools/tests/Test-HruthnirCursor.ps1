param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Expected-broken cursor counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-cursor-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-gossip','hruthnir-cursor-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -CursorBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned cursor counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL a0f4cc39b: same fullGUID AI replacement outside entrance/chamber loses native point4 and remains idle. Expected broken control, not successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ColdRouteOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method cursor regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Cursor *> "$out/native.log"
  if(!$?){throw "Native cursor regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_cursor_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical cursor certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS same-fullGUID Hruthnir AI replacement retains actual next native corridor leg; current instance epoch/receipt/actor/phase and real arrival guard progress. Original offer/start clocks remain; actual client traversal is separate.'
 }
}

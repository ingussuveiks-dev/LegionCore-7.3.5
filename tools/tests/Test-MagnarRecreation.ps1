param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Expected-broken combat counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-recreation-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('magnar-combat-state','hruthnir-reoffer','magnar-recreation-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -RecreationBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned combat counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL 8ac9eec48: actual original DB361400 removal/reload leaves the consumed companion claim and12 old native reserves;10s no reoffer/new start/46427. Expected broken control,not successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-MagnarCombatState.ps1" -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method combat regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ReofferOnly -AddressSanitizer *> "$out/reoffer.log"
  if(!$?){throw "Actual reoffer caller ASAN regression failed: $out/reoffer.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Recreation *> "$out/native.log"
  if(!$?){throw "Native combat regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_magnar_recreation_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical combat certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS observed original boss recreation retires only old owned reserves/start497,then actual point6/13s/menu/point7/497/6s/native waves and504;unobserved/epoch/pending/step4 paths refuse grants. Native72/source66/caller125;full client and whole-map restart remain separate.'
 }
}

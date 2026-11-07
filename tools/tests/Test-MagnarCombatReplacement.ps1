param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Expected-broken combat counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-combat-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('magnar-combat-state','magnar-combat-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -CombatBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned combat counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL 26fd47707: native10 owned deaths/46427/step4 are lost to a fresh first_phase=true AI,which discards lethal damage and cannot create504. Expected broken control, not successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-MagnarCombatState.ps1" -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method combat regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -CombatReplacement *> "$out/native.log"
  if(!$?){throw "Native combat regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_magnar_combat_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical combat certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS earned Magnar combat survives same-fullGUID AI replacement; native owned auxiliaries/504 interruption/ordinary fight timers and actual delayed handoff remain,while epoch/newGUID/unearned step reject restoration. Full client combat remains separate.'
 }
}

param([switch]$Baseline,[switch]$DeferredBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $DeferredBaseline) -and $VerifyReleaseBinary){throw 'Expected-broken combat counterfactual cannot certify installed Release'}
if($Baseline -and $DeferredBaseline){throw 'Choose one counterfactual'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-duel-recreation-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('magnar-combat-state','magnar-waves','magnar-finale','magnar-duel-recreation-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($DeferredBaseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -DuelDeferredBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Modeled deferred-resume counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL modeled missing NeedsReset fallback:6 real earned4/ticket-consumed controls cannot resume after a controlled health-callback disable/re-enable endpoint. Expected-broken control,not prior Git source or actual gameplay.'
 }elseif($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -DuelRecreationBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned combat counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL 89922acab: real10 deaths/46427/4 followed by actual original DB boss removal/reload discards a new lethal hit with first_phase=true;no504/45784.6 expected-broken player/pet/guardian/faction controls,not successful gameplay.'
 }else{
  & "$PSScriptRoot/Test-MagnarCombatState.ps1" -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method combat regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarWaves.ps1" -AddressSanitizer *> "$out/waves.log"
  if(!$?){throw "Actual health/wave caller ASAN regression failed: $out/waves.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -DuelRecreation *> "$out/native.log"
  if(!$?){throw "Native combat regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_magnar_duel_recreation_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical combat certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS observed earned4 boss recreation retains legitimate old identity/epoch/health authority,cleans current/suspended old504,then new real lethal504/full delay/45784 exactly once. Native132/ASAN110/7 health callback cases;readmission resumes without HP replay or a repeated reentrant Reset. Unobserved/unearned/new epoch/completed paths refuse recovery. No forced movement;actual client/whole-map restart remain separate.'
 }
}

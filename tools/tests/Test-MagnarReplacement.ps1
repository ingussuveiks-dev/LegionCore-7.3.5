param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned replacement counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-replacement-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('magnar-waves','magnar-reset','magnar-replacement-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ReplacementBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned AI replacement counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL7efbe0828: actual acknowledged boss/native AIM_Destroy+InitializeAI removes reserve but fresh Reset loses encounterStarted; Hruthnir keeps consumed request/497 and cannot reoffer. Expected-broken control.'
 }else{
  & "$PSScriptRoot/Test-MagnarWaves.ps1" -AddressSanitizer *> "$out/waves-source.log"
  if(!$?){throw "Actual-method Reset/wave regressions failed: $out/waves-source.log"}
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ResetOnly -AddressSanitizer *> "$out/consumer-source.log"
  if(!$?){throw "Actual acknowledged companion guard regressions failed: $out/consumer-source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Replacement *> "$out/native.log"
  if(!$?){throw "Native AI replacement regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_magnar_reset_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected installed Reset/replacement certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS actual first-phase Magnar AI replacement/owned reserve cleanup -> acknowledged same-boss Hruthnir reoffer -> original point6/13s/cached option/point7/497/6s/new roster; failed replacement/temporary unavailable AI do not revoke a claim. No victory grant; actual client/later-phase AI restoration remain separate.'
 }
}

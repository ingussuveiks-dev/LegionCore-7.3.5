param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned approach baseline cannot certify the current executable'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-approach-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-approach','hruthnir-approach-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ApproachBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned native stopped-point baseline failed: $out/counterfactual.log"}
  'COUNTERFACTUAL58134ab60: actual stopped-short point7 remains idle; expected broken baseline'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ApproachOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method approach regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Approach *> "$out/native.log"
  if(!$?){throw "Native approach regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_approach_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical approach certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS actual approach retry/deferral/cancellation, native stopped-point/arrival/497/full6s/Magnar receipt; actual client movement remains separate'
 }
}

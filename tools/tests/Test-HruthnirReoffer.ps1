param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned reoffer baseline cannot certify the current executable'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-reoffer-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-reoffer','hruthnir-reoffer-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ReofferBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned native missing-offer baseline failed: $out/counterfactual.log"}
  'COUNTERFACTUALe9ab2dd82: canceled native request/Reset leaves no ready offer; expected broken baseline'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ReofferOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method reoffer regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Reoffer *> "$out/native.log"
  if(!$?){throw "Native reoffer regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_reoffer_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical reoffer certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS actual reoffer/deferral/cancellation and native point6/full13s ready; source cached option/full6s start, client UI/traversal remain separate'
 }
}

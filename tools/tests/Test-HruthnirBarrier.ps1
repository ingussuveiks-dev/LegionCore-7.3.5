param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned barrier counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-barrier-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-barrier','hruthnir-barrier-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -BarrierBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned barrier counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL73b0da45c: actual handoff hides closed33309 collision; real DB/new GUID reload restores visible closed barrier despite delivered instance. Expected broken baseline.'
 }else{
  & "$PSScriptRoot/Test-HruthnirGuards.ps1" -BarrierOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method barrier regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Barrier *> "$out/native.log"
  if(!$?){throw "Native DB/model barrier regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_barrier_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical barrier certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS scoped original barrier/current guard handoff/pre-publication reload/native DOOR open and33309 mesh,shared class/custom phase/refusal/re-entry/terminal/repeat protections. Full client/cold NPC navigation remains separate.'
 }
}

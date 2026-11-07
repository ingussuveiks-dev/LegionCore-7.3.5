param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned chamber counterfactual cannot certify installed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-chamber-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-chamber','hruthnir-chamber-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ChamberBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned chamber counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL827bb28aa: actual new Hruthnir AI/actor retains delivered instance but loses local chamber and replays initial5s point1; expected broken baseline'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -ChamberOnly -AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method chamber recovery failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -ChamberRecovery *> "$out/native.log"
  if(!$?){throw "Native fresh chamber integration failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_chamber_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical chamber certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS actual fresh chamber receipt/current original actor,LOS replay prevention,native point6 including coincident endpoint/full13s/cached option/point7/497/full6s/acknowledged reserve and Reset. Cold far-spawn route/full client traversal remain separate.'
 }
}

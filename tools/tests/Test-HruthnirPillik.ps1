# Native Protection entrance/real Pillik death -> Hruthnir's authored handoff.
param([switch]$Baseline,[switch]$EntryBaseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $EntryBaseline) -and $VerifyReleaseBinary){throw 'Pinned counterfactual cannot certify the installed executable'}
if($Baseline -and $EntryBaseline){throw 'Choose one pinned counterfactual'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-pillik-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('hruthnir-pillik','hruthnir-pillik-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 if($EntryBaseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -EntryBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned native entry counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL 7dc957758: six actual out-of-range waypoint/returned-player cases stay at step0; expected broken baseline'
 }elseif($Baseline){
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -PillikBaseline *> "$out/counterfactual.log"
  if(!$?){throw "Pinned native Pillik counterfactual failed: $out/counterfactual.log"}
  'COUNTERFACTUAL cb1300a9f: six actual early-death/AI-owned5s cases remain stuck; expected broken baseline'
 }else{
  & "$PSScriptRoot/Test-HruthnirGossip.ps1" -PillikOnly -AddressSanitizer:$AddressSanitizer *> "$out/source.log"
  if(!$?){throw "Actual-method Pillik regression failed: $out/source.log"}
  & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -Pillik *> "$out/native.log"
  if(!$?){throw "Native Pillik regression failed: $out/native.log"}
  if($VerifyReleaseBinary){
   $previous=$env:PYTHONPATH
   try{
    $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous"
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_hruthnir_cleanup_release.py" "$repo/build-extractors/bin/Release" *> "$out/release.log"
    if($LASTEXITCODE){throw "Selected canonical Pillik/retained Hruthnir certificate failed: $out/release.log"}
   }finally{$env:PYTHONPATH=$previous}
  }
  'PASS actual-method/native Pillik deaths, original approach delay, owner/guardian/group progression and scoped scenery cleanup; full client traversal remains separate'
 }
}

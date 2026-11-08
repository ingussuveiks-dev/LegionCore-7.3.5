param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/sharasdal-wave-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 & "$PSScriptRoot/Test-SharasdalWaveSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Wave source regression failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalWave -SharasdalWaveBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Wave native regression failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){if($source -notmatch 'COUNTERFACTUAL Sharasdal wave source8' -or $native -notmatch 'COUNTERFACTUAL Sharasdal wave native12'){throw 'Missing expected original failure'};'COUNTERFACTUAL pinned dbb95ce8d source8 ASAN/native12:4 native vehicle-teardown waves retain original missing-kit cleanup defect;8 ordinary controls preserved. Not a passing ordinary gameplay run'}
 else{
  if($source -notmatch 'PASS Sharasdal wave source8' -or $native -notmatch 'PASS Sharasdal wave native12'){throw 'Missing repaired coverage'}
  if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_sharasdal_wave_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected repaired wave missing'}}finally{$env:PYTHONPATH=$prior}}
  'PASS Sharasdal wave source8 ASAN/native12:actual factory/manual summon/vehicle teardown/passenger ejection/100ms despawn,nonpoint/reset controls,whole actor scope/delta-repeat/unknown-delay refusal,no progress/item changes. Physical movement/contact/client separate.'
 }
}

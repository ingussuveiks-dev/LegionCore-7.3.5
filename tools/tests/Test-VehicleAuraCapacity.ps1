param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/vehicle-aura-capacity-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_vehicle_aura_capacity_source.py" *> "$out/source.log";if($LASTEXITCODE){throw 'Unrelated aura change'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -VehicleAuraCapacityBaseline *> "$out/baseline.log";if(!$?){throw 'Pinned expected-broken native full-seat counterfactual failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -VehicleAuraCapacity *> "$out/repaired.log";if(!$?){throw 'Repaired native capacity failed'}
 if((Get-Content "$out/repaired.log" -Raw) -notmatch 'PASS native vehicle aura capacity16:'){throw 'Unexpected capacity coverage'}
 if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_vehicle_aura_capacity_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected capacity fix absent'}}finally{$env:PYTHONPATH=$prior}}
 'PASS whole aura source scope/paired native original2+repaired16:full auto refusal retains all riders,explicit seat policy/own refresh/retry retained;client/shared vehicle traversal separate'
}

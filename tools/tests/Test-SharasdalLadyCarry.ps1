param([switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/sharasdal-lady-carry-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 if($VerifyLiveData){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalLadyCarryLive *> "$out/native.log"}else{& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalLadyCarry *> "$out/native.log"};if(!$?){throw 'Native carry regression failed'}
 if((Get-Content "$out/native.log" -Raw) -notmatch 'PASS (live native|native) Lady carry32'){throw 'Unexpected carry coverage'}
 if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_vehicle_aura_capacity_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Required selected native seat-capacity fix absent'}}finally{$env:PYTHONPATH=$prior}}
 'PASS native Lady carry32:real implicit target/queued vehicle acknowledgement and negative/retry controls;data-only selector repair,full encounter producer/threshold/dialogue/finisher/client remain OPEN'
}

param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/sharasdal-wave-contact-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_wave_contact_source.py" *> "$out/source-scope.log";if($LASTEXITCODE){throw 'Unreviewed native ride changes'}
 & "$PSScriptRoot/Test-SharasdalWaveRideScope.ps1" *> "$out/scope.log";if(!$?){throw 'Ride scope ASAN failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalWaveContact *> "$out/native.log";if(!$?){throw 'Native wave contact failed'}
 if((Get-Content "$out/native.log" -Raw) -notmatch 'PASS native wave contact32:'){throw 'Unexpected native contact coverage'}
 if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_sharasdal_wave_contact_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected wave contact implementation missing'}}finally{$env:PYTHONPATH=$prior}}
 'PASS wave contact:whole native source scope/ASAN30/native32,real geometric contact/cast/queued seat acknowledgement/retry/cleanup and foreign/shared controls;physical moving wave/client/camera/pet portrait/full1066 separate.'
}

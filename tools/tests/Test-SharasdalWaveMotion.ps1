param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/sharasdal-wave-motion-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -SharasdalWaveMotion *> "$out/native.log";if(!$?){throw 'Native moving-wave check failed'}
 if((Get-Content "$out/native.log" -Raw) -notmatch 'PASS native moving waves24:'){throw 'Unexpected native moving-wave coverage'}
 if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_sharasdal_wave_contact_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected native wave contact/cleanup implementation absent'}}finally{$env:PYTHONPATH=$prior}}
 'PASS native moving waves24:actual authored lanes/spline/map/attached contact/seat acknowledgement/pause-resumption/finalize/ejection/removal. Manual native clock and earlier quest/stage controlled;client/camera/pet portrait/full1066 remain separate.'
}

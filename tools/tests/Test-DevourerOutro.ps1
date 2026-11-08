param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/devourer-outro-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 & "$PSScriptRoot/Test-DevourerOutroSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Outro source regression failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -DevourerOutro -DevourerOutroBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Outro native regression failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){if($source -notmatch 'COUNTERFACTUAL Devourer outro:' -or $native -notmatch 'COUNTERFACTUAL Devourer outro native:10'){throw 'Missing pinned failure evidence'};'COUNTERFACTUAL original e59f20dd3 Update blocks natural completed7/cache6 reactions;source+native reproducible,not a passing ordinary run'}
 else{
  if($source -notmatch 'PASS Devourer outro source ASAN:' -or $native -notmatch 'PASS Devourer outro native:10'){throw 'Missing repaired evidence'}
  if($VerifyReleaseBinary){$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_devourer_outro_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected repaired outro missing'}}finally{$env:PYTHONPATH=$prior}}
  'PASS Devourer outro source ASAN/native10:real final event/cache6,ordinary native visual casts,cadence/identity/callback lifetime/cleanup,quest/item/progress unchanged. Earlier stages and poses controlled;client visual/full chain separate.'
 }
}

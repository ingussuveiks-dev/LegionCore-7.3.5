param([switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/marksmanship-return-recovery-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$roots=@('marksmanship-recovery-source','marksmanship-recovery-data','marksmanship-earned-native','legion-legendary-catalog')|ForEach-Object {"$repo/build-extractors/runtime-test/$_"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories $roots -Test {
 & "$PSScriptRoot/Test-MarksmanshipRecoverySource.ps1" *> "$out/source.log";if(!$?){throw 'Earned return source ASAN failed'}
 & "$PSScriptRoot/Test-MarksmanshipRecoveryData.ps1" -VerifyLiveData:$VerifyLiveData *> "$out/data.log";if(!$?){throw 'Earned return SQL graph failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -MarksmanEarned *> "$out/native.log";if(!$?){throw 'Earned return native failed'}
 if((Get-Content "$out/source.log" -Raw) -notmatch 'PASS recovery67 ASAN' -or (Get-Content "$out/native.log" -Raw) -notmatch 'PASS earned native80'){throw 'Unexpected earned recovery fixture/count'}
 if($VerifyReleaseBinary){
  $old=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$old";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_marksmanship_recovery_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Linked earned recovery failed'}}finally{$env:PYTHONPATH=$old}
 }
 'PASS earned Marksmanship return:source67 ASAN/native80,one SQL binding+12 no-mutation graph refusals,actual five-goal/owned loading-bow proof,condition dispatch and both active statuses;bounded real-caster normal205040 request/accepted-transfer-only cleanup. Native fixture controls quest state and poses;source cast/transfer endpoints controlled;full saved-character/group/client traversal remains separate.'
}

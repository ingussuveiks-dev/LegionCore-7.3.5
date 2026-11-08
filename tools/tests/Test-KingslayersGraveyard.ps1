param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned counterfactual cannot certify repaired Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/kingslayers-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw 'Missing actual Kingslayers callback'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
 . "$PSScriptRoot/KingslayersGraveyardScope.ps1"
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp';$old=(& git -C $repo show "148c610e4:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned whole instance absent'};$current=Restore-KingslayersGraveyardBoundary (Get-Content "$repo/$path" -Raw)
 if(($old -replace '\s','') -cne ($current -replace '\s','')){throw 'Unreviewed guard ledger/constructor/update/registration change'}
 & "$PSScriptRoot/Test-KingslayersGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Kingslayers source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -KingslayersGraveyard -KingslayersGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Kingslayers native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){if($source -notmatch 'Kingslayers source88' -or $native -notmatch 'Kingslayers native184/release176'){throw 'Unexpected pinned variant'};'COUNTERFACTUAL Kingslayers source88 ASAN/native184/release176:all original0..9 checkpoints preserved,natural completed10 retains cached9/5530,zero facing;source cached10 is a separate explicit input. Current support,not historical executable.'}
 else{
  if($source -notmatch 'Kingslayers graveyard154' -or $native -notmatch 'Kingslayers native192/release176'){throw 'Unexpected repaired variant'}
  if($VerifyReleaseBinary){$oldPath=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$oldPath";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_kingslayers_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected Kingslayers Release missing'}}finally{$env:PYTHONPATH=$oldPath}}
  'PASS Kingslayers source154 ASAN/native192/release176:all10 stages+real completed10,four native checkpoints/facing,Human+Orc/both42504+42627,real release/ACK/Horde2479 retained,guard ledger unchanged,all ordered criteria/AND both orders,56 source diagnostic refusals and foreign+missing null. Earlier state/endpoints controlled;full client/pet/group/route separate.'
 }
}

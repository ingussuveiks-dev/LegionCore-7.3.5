param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned counterfactual cannot certify repaired Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/devourer-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw 'Missing actual Devourer callback'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp';$old=(& git -C $repo show "3465a1d48:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned whole instance absent'};$current=Get-Content "$repo/$path" -Raw
 $current=$current.Replace((Block $current 'WorldLocation* GetClosestGraveYard('),(Block $old 'WorldLocation* GetClosestGraveYard(')).Replace('#include "MapManager.h"','').Replace('#include "Scenario.h"','').Replace('#include "ScenarioMgr.h"','')
 if(($old -replace '\s','') -cne ($current -replace '\s','')){throw 'Unreviewed constructor/actor-list/outro/update/registration change'}
 & "$PSScriptRoot/Test-DevourerGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Devourer source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -DevourerGraveyard -DevourerGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Devourer native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){if($source -notmatch 'Devourer source64' -or $native -notmatch 'Devourer native68/release64'){throw 'Unexpected pinned variant'};'COUNTERFACTUAL Devourer source64 ASAN/native68/release64:original0..6 checkpoints preserved,natural completed7 retains cached6/5470,zero facing;source cached7 is a separate explicit input. Current support,not historical executable.'}
 else{
  if($source -notmatch 'Devourer graveyard130' -or $native -notmatch 'Devourer native72/release64'){throw 'Unexpected repaired variant'}
  if($VerifyReleaseBinary){$oldPath=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$oldPath";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_devourer_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected Devourer Release missing'}}finally{$env:PYTHONPATH=$oldPath}}
  'PASS Devourer source130 ASAN/native72/release64:all7 stages+real completed7,four native checkpoints/facing,Human+Orc/two active41924 statuses,real release/ACK,outro list/timer preserved,ordered criteria/AND/OR variants,56 source diagnostic refusals and foreign+missing null. Earlier state/endpoints controlled;full client/combat/pet/group/route separate.'
 }
}

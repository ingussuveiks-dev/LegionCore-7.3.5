param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned counterfactual cannot certify repaired Release'}
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
$out="$repo/build-extractors/runtime-test/dreadblades-graveyard-check/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @("$repo/build-extractors/runtime-test") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw 'Missing actual Dreadblades instance'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$old=(& git -C $repo show "9a8f9340d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned shared source absent'};$current=Get-Content "$repo/$path" -Raw
 $a=$old.Replace((Block $old 'struct instance_dreadblades : public InstanceScript'),'SCOPED_INSTANCE').Trim();$b=$current.Replace((Block $current 'struct instance_dreadblades : public InstanceScript'),'SCOPED_INSTANCE').Replace('#include "MapManager.h"','').Trim()
 if(($a -replace '\s','') -cne ($b -replace '\s','')){throw 'Unreviewed shared Mage/Priest/Monk/other instance change'}
 & "$PSScriptRoot/Test-DreadbladesGraveyardSource.ps1" -Baseline:$Baseline *> "$out/source.log";if(!$?){throw 'Dreadblades cemetery source failed'}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -DreadbladesGraveyard -DreadbladesGraveyardBaseline:$Baseline *> "$out/native.log";if(!$?){throw 'Dreadblades cemetery native failed'}
 $source=Get-Content "$out/source.log" -Raw;$native=Get-Content "$out/native.log" -Raw
 if($Baseline){if($source -notmatch 'Dreadblades source48' -or $native -notmatch 'Dreadblades native52/release48'){throw 'Unexpected counterfactual variant'};'COUNTERFACTUAL Dreadblades source48 ASAN/native52/release48:original start/hill routing and zero facing reproduced;natural completed5 retains cached4/hill;source cached5 is a separate explicit input;current native support,not historical whole binary.'}
 else{
  if($source -notmatch 'Dreadblades graveyard114' -or $native -notmatch 'Dreadblades native56/release48'){throw 'Unexpected repaired variant'}
  if($VerifyReleaseBinary){$oldPath=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$oldPath";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/run_linked_release_verifier.py" "$PSScriptRoot/verify_dreadblades_graveyard_release.py" $ReleaseDirectory *> "$out/release.log";if($LASTEXITCODE){throw 'Selected Dreadblades cemetery Release missing'}}finally{$env:PYTHONPATH=$oldPath}}
  'PASS Dreadblades source114 ASAN/native56/release48:actual five-step route/four authored checkpoints and facing,completed final beach,real release/ACK and native criteria boundaries,56 source diagnostic refusals and absent/foreign Scenario null;shared instances unchanged. Earlier state/combat inputs controlled;full client/pet/group/route separate.'
 }
}

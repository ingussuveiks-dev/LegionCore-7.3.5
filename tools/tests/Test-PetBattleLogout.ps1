param([switch]$Baseline,[ValidateSet('all','detached','webbed','offline')][string]$Case='all')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $Case -eq 'all'){throw 'Select one independently expected-broken complete native control'}
$mode=if($Baseline){'baseline'}else{'fixed'};$out="$repo/.codex/pet-logout-2026-11-13/$mode-$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory("$out/logs")
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw;$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$source='../tests/PetBattleLogoutNative.cpp';$probe='PetBattleLogoutNative'
if($Baseline){$old=(& git -C $repo show '59489d127:src/server/game/PetBattle/PetBattle.cpp')-join "`n";if($LASTEXITCODE){throw 'Pinned full PetBattle baseline unavailable'};[IO.File]::WriteAllText("$out/PetBattle.cpp",$old);[IO.File]::WriteAllText("$out/Baseline.cpp",('#include "'+$repo.Replace('\','/')+'/src/server/game/PrecompiledHeaders/gamePCH.h"'+"`n"+'#include "'+$out.Replace('\','/')+'/PetBattle.cpp"'+"`n"+'#include "'+$repo.Replace('\','/')+'/tools/tests/PetBattleLogoutNative.cpp"'+"`n"));$source='../../'+[IO.Path]::GetRelativePath($repo,"$out/Baseline.cpp").Replace('\','/');$probe='PetBattleLogoutBaselineNative'}
$previous=$env:CL;$oldConfig=$env:LEGION_DIALOGUE_CONFIG;$oldCase=$env:LEGION_PET_LOGOUT_CASE
try{$env:CL="$previous /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`"";$env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_PET_LOGOUT_CASE=if($Case -eq 'all'){$null}else{$Case}
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$PSScriptRoot/Prepare-PetBattleOwnedNative.ps1" -Out $out;$failure=$null
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $source -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($Baseline){$message=@{detached='Bound detached logout must preserve native penalty and actual journal before save';webbed='Genuine detached logout must settle a webbed battle instead of orphaning it';offline='Detached ordinary Finish must save owned journal without a filtered Player lookup'}[$Case];if(!$failure -or !$failure.Exception.Message.Contains('failed with exit 1') -or !(Get-Content "$out/build.log" -Raw).Contains($message)){Get-Content "$out/build.log" -Tail 14;throw 'Unexpected baseline failure'};"PASS expected-broken full59489d127 PetBattle control:$Case;exit1;paired repaired run required"}
  else{if($failure){Get-Content "$out/build.log" -Tail 16;throw $failure};if(!(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native logout cases=')){throw 'Native logout cases did not complete'};Get-Content "$out/native.tsv" -Tail 1}
 }
}finally{$env:CL=$previous;$env:LEGION_DIALOGUE_CONFIG=$oldConfig;$env:LEGION_PET_LOGOUT_CASE=$oldCase}
$global:LASTEXITCODE=0

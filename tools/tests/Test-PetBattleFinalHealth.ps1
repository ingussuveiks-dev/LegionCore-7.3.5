param([switch]$Baseline,[ValidateSet('sacrifice','forfeit','undeadexpiry','all')][string]$Case='all')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $Case -notin 'sacrifice','forfeit','all'){throw 'Pinned failing baseline is limited to sacrifice/forfeit; Undead expiry is an already-correct retained regression'}
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/.codex/pet-final-health-open-2026-11-13/$mode-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
[void][IO.Directory]::CreateDirectory("$out/logs")
$config=[IO.File]::ReadAllText("$repo/build-extractors/bin/Release/worldserver.conf")
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$source='../tests/PetBattleFinalHealthNative.cpp';$probe='PetBattleFinalHealthNative'
if($Baseline){
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){
  $raw=& git -C $repo show "31af6b87f:src/server/game/PetBattle/$name"
  if($LASTEXITCODE){throw 'Pinned complete native pet implementation unavailable'}
  [IO.File]::WriteAllText("$out/$name",($raw -join "`n"))
 }
 $unit='#include "'+$repo.Replace('\','/')+'/src/server/game/PrecompiledHeaders/gamePCH.h"'+"`n"
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){$unit+='#include "'+$out.Replace('\','/')+'/'+$name+'"'+"`n"}
 $unit+='#include "'+$repo.Replace('\','/')+'/tools/tests/PetBattleFinalHealthNative.cpp"'+"`n"
 [IO.File]::WriteAllText("$out/Baseline.cpp",$unit)
 $source='../../'+[IO.Path]::GetRelativePath($repo,"$out/Baseline.cpp").Replace('\','/')
 $probe='PetBattleFinalHealthBaselineNative'
}
$previousCompiler=$env:CL;$previousConfig=$env:LEGION_DIALOGUE_CONFIG;$previousCase=$env:LEGION_PET_FINAL_HEALTH_CASE
try{
 $env:CL="$previousCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`""
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:LEGION_PET_FINAL_HEALTH_CASE=if($Case -eq 'all'){$null}else{$Case}
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$PSScriptRoot/Prepare-PetBattleOwnedNative.ps1" -Out $out
  $failure=$null
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $source -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($Baseline){
   if(!$failure -or !(Test-Path "$out/native.tsv")){throw 'Expected complete native failing baseline was not reached; compiler/diagnostics retained'}
   $text=[IO.File]::ReadAllText("$out/build.log")
   $messages=@{sacrifice='A final-dead owned pet must not retain positive journal health after actual Finish';forfeit='A final-dead owned pet must not retain positive journal health after actual Finish';all='A final-dead owned pet must not retain positive journal health after actual Finish'}
   if(!$messages.ContainsKey($Case) -or !$text.Contains($messages[$Case]) -or !$failure.Exception.Message.Contains('failed with exit 1')){throw 'Unexpected baseline failure; retained output requires review'}
   "PASS expected-broken complete native31af6b87f counterfactual:$Case; assertion=$($messages[$Case]); paired repaired run required"
  }else{
   if($failure){Get-Content "$out/build.log" -Tail 14;throw $failure}
   if($LASTEXITCODE -or !(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native final-health cases:')){throw 'Native lifecycle regression did not complete'}
   Get-Content "$out/native.tsv" -Tail 1
  }
 }
}finally{$env:CL=$previousCompiler;$env:LEGION_DIALOGUE_CONFIG=$previousConfig;$env:LEGION_PET_FINAL_HEALTH_CASE=$previousCase}
$global:LASTEXITCODE=0

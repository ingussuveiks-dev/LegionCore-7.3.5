param([switch]$Baseline,[ValidateSet('team','clone','failed','holes','expiry','reapply','mechanical','round','all')][string]$Case='all')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/.codex/pet-unholy-ascension-open-2026-11-12/$mode-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
[void][IO.Directory]::CreateDirectory("$out/logs")
$config=[IO.File]::ReadAllText("$repo/build-extractors/bin/Release/worldserver.conf")
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$source='../tests/PetBattleUnholyAscensionNative.cpp';$probe='PetBattleUnholyAscensionNative'
if($Baseline){
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){
  $raw=& git -C $repo show "b7168da63:src/server/game/PetBattle/$name"
  if($LASTEXITCODE){throw 'Pinned complete native pet implementation unavailable'}
  [IO.File]::WriteAllText("$out/$name",($raw -join "`n"))
 }
 $unit='#include "'+$repo.Replace('\','/')+'/src/server/game/PrecompiledHeaders/gamePCH.h"'+"`n"
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){$unit+='#include "'+$out.Replace('\','/')+'/'+$name+'"'+"`n"}
 $unit+='#include "'+$repo.Replace('\','/')+'/tools/tests/PetBattleUnholyAscensionNative.cpp"'+"`n"
 [IO.File]::WriteAllText("$out/Baseline.cpp",$unit)
 $source='../../'+[IO.Path]::GetRelativePath($repo,"$out/Baseline.cpp").Replace('\','/')
 $probe='PetBattleUnholyAscensionBaselineNative'
}
$previousConfig=$env:LEGION_DIALOGUE_CONFIG;$previousCase=$env:LEGION_PET_UNHOLY_CASE
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:LEGION_PET_UNHOLY_CASE=if($Case -eq 'all'){$null}else{$Case}
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  $failure=$null
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $source -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($Baseline){
   if(!$failure -or !(Test-Path "$out/native.tsv")){throw 'Expected complete native failing baseline was not reached; compiler/diagnostics retained'}
   $text=[IO.File]::ReadAllText("$out/build.log")
   $messages=@{team='Unholy Ascension must leave each enemy alive with its own native twenty-five-percent aura';clone='Unholy Ascension must honor its native caster immunity';round='Actual Unholy Ascension round must preserve the living enemy and sacrifice its user';all='Unholy Ascension must leave each enemy alive with its own native twenty-five-percent aura'}
   if(!$messages.ContainsKey($Case) -or !$text.Contains($messages[$Case]) -or !$failure.Exception.Message.Contains('failed with exit 1')){throw 'Unexpected baseline failure; retained output requires review'}
   "PASS expected-broken complete nativeb7168da63 counterfactual:$Case; assertion=$($messages[$Case]); paired repaired run required"
  }else{
   if($failure){Get-Content "$out/build.log" -Tail 14;throw $failure}
   if($LASTEXITCODE -or !(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native Unholy Ascension cases:')){throw 'Native lifecycle regression did not complete'}
   Get-Content "$out/native.tsv" -Tail 1
  }
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig;$env:LEGION_PET_UNHOLY_CASE=$previousCase}
$global:LASTEXITCODE=0

param([switch]$Baseline,[ValidateSet('lethal','round','idle','periodic','capture','otherdeath','foreign','context','multiple','equal','mechanical','expiry','unrelated','all')][string]$Case='all')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/.codex/pet-cheat-death-open-2026-11-12/$mode-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
[void][IO.Directory]::CreateDirectory("$out/logs")
$config=[IO.File]::ReadAllText("$repo/build-extractors/bin/Release/worldserver.conf")
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$source='../tests/PetBattleCheatDeathNative.cpp';$probe='PetBattleCheatDeathNative'
if($Baseline){
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){
  $raw=& git -C $repo show "94b0133fb:src/server/game/PetBattle/$name"
  if($LASTEXITCODE){throw 'Pinned complete native pet implementation unavailable'}
  [IO.File]::WriteAllText("$out/$name",($raw -join "`n"))
 }
 $unit='#include "'+$repo.Replace('\','/')+'/src/server/game/PrecompiledHeaders/gamePCH.h"'+"`n"
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){$unit+='#include "'+$out.Replace('\','/')+'/'+$name+'"'+"`n"}
 $unit+='#include "'+$repo.Replace('\','/')+'/tools/tests/PetBattleCheatDeathNative.cpp"'+"`n"
 [IO.File]::WriteAllText("$out/Baseline.cpp",$unit)
 $source='../../'+[IO.Path]::GetRelativePath($repo,"$out/Baseline.cpp").Replace('\','/')
 $probe='PetBattleCheatDeathBaselineNative'
}
$previousConfig=$env:LEGION_DIALOGUE_CONFIG;$previousCase=$env:LEGION_PET_CHEAT_CASE
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:LEGION_PET_CHEAT_CASE=if($Case -eq 'all'){$null}else{$Case}
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  $failure=$null
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $source -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($Baseline){
   if(!$failure -or !(Test-Path "$out/native.tsv")){throw 'Expected complete native failing baseline was not reached; compiler/diagnostics retained'}
   $text=[IO.File]::ReadAllText("$out/build.log")
   $messages=@{lethal='Native Cheat Death controller must survive its holder death until round end';all='Native Cheat Death controller must survive its holder death until round end'}
   if(!$messages.ContainsKey($Case) -or !$text.Contains($messages[$Case]) -or !$failure.Exception.Message.Contains('failed with exit 1')){throw 'Unexpected baseline failure; retained output requires review'}
   "PASS expected-broken complete native94b0133fb counterfactual:$Case; assertion=$($messages[$Case]); paired repaired run required"
  }else{
   if($failure){Get-Content "$out/build.log" -Tail 14;throw $failure}
   if($LASTEXITCODE -or !(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native Cheat Death cases:')){throw 'Native lifecycle regression did not complete'}
   Get-Content "$out/native.tsv" -Tail 1
  }
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig;$env:LEGION_PET_CHEAT_CASE=$previousCase}
$global:LASTEXITCODE=0

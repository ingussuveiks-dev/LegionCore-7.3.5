param([switch]$Baseline,[ValidateSet('roots','damage','overlap','reset','round','cleanup','receipts','foreign','guards','racial','miss','death','all')][string]$Case='all')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/.codex/pet-ward-rot-open-2026-11-12/$mode-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
[void][IO.Directory]::CreateDirectory("$out/logs")
$config=[IO.File]::ReadAllText("$repo/build-extractors/bin/Release/worldserver.conf")
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$source='../tests/PetBattleWardRotNative.cpp';$probe='PetBattleWardRotNative'
if($Baseline){
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){
  $raw=& git -C $repo show "72e54ac8c:src/server/game/PetBattle/$name"
  if($LASTEXITCODE){throw 'Pinned complete native pet implementation unavailable'}
  [IO.File]::WriteAllText("$out/$name",($raw -join "`n"))
 }
 $unit='#include "'+$repo.Replace('\','/')+'/src/server/game/PrecompiledHeaders/gamePCH.h"'+"`n"
 foreach($name in 'PetBattle.cpp','PetBattleAbilityEffect.cpp'){$unit+='#include "'+$out.Replace('\','/')+'/'+$name+'"'+"`n"}
 $unit+='#include "'+$repo.Replace('\','/')+'/tools/tests/PetBattleWardRotNative.cpp"'+"`n"
 [IO.File]::WriteAllText("$out/Baseline.cpp",$unit)
 $source='../../'+[IO.Path]::GetRelativePath($repo,"$out/Baseline.cpp").Replace('\','/')
 $probe='PetBattleWardRotBaselineNative'
}
$previousConfig=$env:LEGION_DIALOGUE_CONFIG;$previousCase=$env:LEGION_PET_WARD_ROT_CASE
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:LEGION_PET_WARD_ROT_CASE=if($Case -eq 'all'){$null}else{$Case}
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  $failure=$null
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile $source -ProbeName $probe -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($Baseline){
   if(!$failure -or !(Test-Path "$out/native.tsv")){throw 'Expected complete native failing baseline was not reached; compiler/diagnostics retained'}
   $text=[IO.File]::ReadAllText("$out/build.log")
   $messages=@{roots='Native Ward/Rot root must create its real controller and defensive mark on the actual holder';damage='Native defensive mark must change actual family damage without changing species';overlap='Categorical type IDs cannot be added; actual last mark must select one type';all='Native Ward/Rot root must create its real controller and defensive mark on the actual holder'}
   if(!$messages.ContainsKey($Case) -or !$text.Contains($messages[$Case]) -or !$failure.Exception.Message.Contains('failed with exit 1')){throw 'Unexpected baseline failure; retained output requires review'}
   "PASS expected-broken complete native72e54ac8c counterfactual:$Case; assertion=$($messages[$Case]); paired repaired run required"
  }else{
   if($failure){Get-Content "$out/build.log" -Tail 14;throw $failure}
   if($LASTEXITCODE -or !(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native Ward/Rot cases:')){throw 'Native lifecycle regression did not complete'}
   Get-Content "$out/native.tsv" -Tail 1
  }
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig;$env:LEGION_PET_WARD_ROT_CASE=$previousCase}
$global:LASTEXITCODE=0

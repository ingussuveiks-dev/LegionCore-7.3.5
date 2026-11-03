function Get-PreRagnvaldSecondBattleSource([string]$path,[string]$source) {
 $changed=@('src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp','src/server/scripts/Scenario/Artifacts/Warrior/Intro/ritual_transition.h','src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp')
 if($changed -contains $path -and (Get-Content "$repo/$($changed[2])" -Raw).Contains('ragnvaldSecondBattle') -and $source.Replace("`r",'') -ceq (Get-Content "$repo/$path" -Raw).Replace("`r",'')) {
  $key=(@($changed|ForEach-Object {(Get-FileHash -LiteralPath "$repo/$_").Hash}) -join ':')
  if($global:RagnvaldSecondBattleCertificate -cne $key){& "$PSScriptRoot/Test-WarriorSecondBattle.ps1" -ScopeOnly | Out-Host;$global:RagnvaldSecondBattleCertificate=$key}
  $source=(& git -C $repo show "2c6952ecf:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-second-battle source missing'}
 }
 return $source
}

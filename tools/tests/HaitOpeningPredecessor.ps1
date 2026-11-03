function Get-HaitPreOpeningSource([string]$path,[string]$source) {
 $changed=@('src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp','src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_hait_click.h','src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp')
 if($changed -contains $path -and (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp" -Raw).Contains('#include "hait_opening.h"') -and $source.Replace("`r",'') -ceq (Get-Content "$repo/$path" -Raw).Replace("`r",'')) {
  $key=(@($changed|ForEach-Object {(Get-FileHash -LiteralPath "$repo/$_").Hash}) -join ':')
  if($global:HaitOpeningCertificateKey -cne $key){& "$PSScriptRoot/Test-HaitOpening.ps1" -ScopeOnly | Out-Host;$global:HaitOpeningCertificateKey=$key}
  $source=(& git -C $repo show "c0f7fae1a:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-opening source missing'}
 }
 return $source
}

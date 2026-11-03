function Get-HaitPreDefeatSource([string]$path,[string]$source) {
 $changed=@('src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp','src/server/scripts/Scenario/Artifacts/Monk/Intro/hait_opening.h','src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp')
 if($changed -contains $path -and (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/hait_opening.h" -Raw).Contains('void UpdateDefeat()') -and $source.Replace("`r",'') -ceq (Get-Content "$repo/$path" -Raw).Replace("`r",'')) {
  $key=(@($changed|ForEach-Object {(Get-FileHash -LiteralPath "$repo/$_").Hash}) -join ':')
  if($global:HaitDefeatCertificateKey -cne $key){& "$PSScriptRoot/Test-HaitDefeat.ps1" -ScopeOnly | Out-Host;$global:HaitDefeatCertificateKey=$key}
  $source=(& git -C $repo show "8314d0379:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-defeat source missing'}
 }
 return $source
}

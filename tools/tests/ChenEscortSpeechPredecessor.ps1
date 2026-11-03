. "$PSScriptRoot/HaitOpeningPredecessor.ps1"
# A complete successor certificate precedes every old full-file comparison.
# Exporters still read and execute the actual current production source.
function Get-ChenPreEscortSource([string]$path,[string]$source) {
 $source=Get-HaitPreOpeningSource $path $source
 $changed=@('src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_presentation.h','src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_rescue.h','src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp')
 if($changed -contains $path -and (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_presentation.h" -Raw).Contains('void UpdateEscortSpeech(') -and $source.Replace("`r",'') -ceq (Get-HaitPreOpeningSource $path (Get-Content "$repo/$path" -Raw)).Replace("`r",'')) {
  $key=(@($changed|ForEach-Object {(Get-FileHash -LiteralPath "$repo/$_").Hash}) -join ':')
  if($global:ChenEscortCertificateKey -cne $key){
   & "$PSScriptRoot/Test-ChenEscortSpeech.ps1" -ScopeOnly | Out-Host
   $global:ChenEscortCertificateKey=$key
  }
  $source=(& git -C $repo show "f5122dd06:$path") -join "`n";if($LASTEXITCODE){throw 'Certified complete pre-escort source missing'}
 }
 return $source
}

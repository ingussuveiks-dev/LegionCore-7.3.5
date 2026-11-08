param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-certificates/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 . "$PSScriptRoot/DevourerDoorCertificate.ps1"
 . "$PSScriptRoot/DevourerEscapeCertificate.ps1"
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
 $body=Block $source 'class npc_akaari_shadowgore :';$mutated=$source.Replace($body,$body.Insert($body.IndexOf('{')+1,"`n uint32 unrelatedCertificateMutation;`n"));$cases=0
 foreach($name in @('Restore-DevourerEscapeBoundary','Restore-DevourerDoorBoundary')){
  $once=& $name $source;++$cases;$twice=& $name $once
  if(($once-replace '\s','') -cne ($twice-replace '\s','')){throw 'Certificate repeat changed original boundary'};++$cases
  $rejected=$false;try{[void](& $name $mutated)}catch{$rejected=$true}
  if(!$rejected){throw 'Unknown capture edit bypassed exact certificate'};++$cases
 }
 if($cases -ne 6){throw 'Unexpected certificate case count'}
 'PASS Devourer certificate6:current exact escape+door composition,repeated normalization and two unknown capture-field refusals;only pinned pre-opening/pre-clone classes accepted. No production/runtime data change.'
}

param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
& "$PSScriptRoot/Test-XalatathOpening.ps1" -Stromkar -Baseline:$Baseline -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Stromkar opening regression failed'}

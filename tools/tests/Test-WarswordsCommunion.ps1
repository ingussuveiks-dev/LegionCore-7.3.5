param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-WarswordsFinalAdmission.ps1" -CommunionBaseline
if($LASTEXITCODE){throw 'Pinned communion selection/damage baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -CommunionBaseline
if($LASTEXITCODE){throw 'Pinned communion victim-tick baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalAdmission.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Repaired communion admission failed'}
& "$PSScriptRoot/Test-WarswordsFinalIntro.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Repaired communion callback/retry failed'}

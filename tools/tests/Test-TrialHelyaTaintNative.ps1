param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialHelyaTaint -Baseline:$Baseline -AdminUser $AdminUser
if($VerifyReleaseBinary){& "$PSScriptRoot/Test-TrialHelyaTaintRelease.ps1"}

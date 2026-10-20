param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialHelyaMistwatcher -Baseline:$Baseline -AdminUser $AdminUser
if($VerifyReleaseBinary){& "$PSScriptRoot/Test-TrialHelyaMistwatcherRelease.ps1"}

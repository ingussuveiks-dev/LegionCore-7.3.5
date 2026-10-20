param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialHelyaMariner -Baseline:$Baseline -AdminUser $AdminUser
if($VerifyReleaseBinary){& "$PSScriptRoot/Test-TrialHelyaMarinerRelease.ps1"}

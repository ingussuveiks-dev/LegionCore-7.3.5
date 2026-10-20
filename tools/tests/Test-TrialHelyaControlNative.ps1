param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialHelyaControl -Baseline:$Baseline -AdminUser $AdminUser
if($VerifyReleaseBinary){& "$PSScriptRoot/Test-TrialHelyaControlLoginRelease.ps1"}

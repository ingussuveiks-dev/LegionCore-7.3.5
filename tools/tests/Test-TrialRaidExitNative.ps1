param([switch]$Baseline,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialExit -Baseline:$Baseline -AdminUser $AdminUser

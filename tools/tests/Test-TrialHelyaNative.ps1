param([switch]$Baseline,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialHelya -Baseline:$Baseline -AdminUser $AdminUser

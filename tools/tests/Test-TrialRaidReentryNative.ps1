param([switch]$Baseline,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialReentry -Baseline:$Baseline -AdminUser $AdminUser

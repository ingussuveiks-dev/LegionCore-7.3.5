param([switch]$Baseline,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialGarm -Baseline:$Baseline -AdminUser $AdminUser

param([switch]$Baseline,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialChest -Baseline:$Baseline -AdminUser $AdminUser

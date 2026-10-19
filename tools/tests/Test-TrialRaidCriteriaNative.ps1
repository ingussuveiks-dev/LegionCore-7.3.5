param([switch]$Baseline, [string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -TrialCriteria -AdminUser $AdminUser -Baseline:$Baseline

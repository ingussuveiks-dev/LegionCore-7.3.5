param([switch]$Baseline, [string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -RaidFragments -Baseline:$Baseline -AdminUser $AdminUser

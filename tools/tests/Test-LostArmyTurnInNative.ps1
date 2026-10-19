param([switch]$Baseline, [string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -QuestTurnIn -Baseline:$Baseline -AdminUser $AdminUser

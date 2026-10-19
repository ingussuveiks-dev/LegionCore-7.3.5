param([switch]$Baseline)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-RockawayChannelNative.ps1" -MasterLoot -Baseline:$Baseline

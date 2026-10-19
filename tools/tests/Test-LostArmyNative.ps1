param([switch]$Baseline)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-RockawayChannelNative.ps1" -LostArmy -Baseline:$Baseline

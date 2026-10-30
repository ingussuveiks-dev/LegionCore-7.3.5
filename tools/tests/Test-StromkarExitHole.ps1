#requires -Version 7.0
param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary)
# Retain both existing receiver suites when compiling their shared native host.
& "$PSScriptRoot/Test-NexusAzuregosReceiver.ps1" -WithPriestHealing -WithExitHole @PSBoundParameters

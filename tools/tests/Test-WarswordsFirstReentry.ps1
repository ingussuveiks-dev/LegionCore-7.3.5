param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-WarswordsFinalVisibility.ps1" -FirstBoss -Baseline
if(!$? -or $LASTEXITCODE){throw 'Pinned first Vigfus re-entry baseline failed'}
& "$PSScriptRoot/Test-WarswordsFinalVisibility.ps1" -FirstBoss -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$? -or $LASTEXITCODE){throw 'First Vigfus re-entry admission failed'}

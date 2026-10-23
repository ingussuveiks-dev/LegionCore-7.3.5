param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-TalonclawPresentation.ps1" -Marksmanship -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Marksmanship presentation regression failed'}

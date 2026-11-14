param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-pickup-native",[switch]$VerifyLiveData=$true)
$ErrorActionPreference='Stop'
# Preserve the existing command while exercising the current return sequence.
& "$PSScriptRoot/Test-FuZanReturnNative.ps1" -OutputDirectory $OutputDirectory -VerifyLiveData:$VerifyLiveData

param([switch]$VerifyReleaseBinary,[string]$AdminUser='root')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-GroupRollLifecycleNative.ps1" -CorpseRestart -AdminUser $AdminUser
if($VerifyReleaseBinary){& "$PSScriptRoot/Test-CorpseRestartRelease.ps1"}

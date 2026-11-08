param([switch]$Baseline,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyLiveData){throw 'Choose explicitly broken counterfactual or repaired live data'}
& "$PSScriptRoot/Test-HodirLandingSource.ps1" -Baseline:$Baseline
if($Baseline){& "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -HodirLandingBaseline}
else{
 & "$PSScriptRoot/Test-HodirLandingData.ps1" -VerifyLiveData:$VerifyLiveData
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -HodirLanding -HodirLandingLive:$VerifyLiveData
}

param([switch]$Baseline,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DTWIN_EMPERORS_BASELINE"}
 if($VerifyLiveData){if($Baseline){throw 'Original broken control cannot certify live data'};$env:CL="$previous /DTWIN_EMPERORS_LIVE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe TwinEmperorsNative
 if(!$? -or $LASTEXITCODE){throw 'Native Twin Emperors regression failed'}
}finally{$env:CL=$previous}

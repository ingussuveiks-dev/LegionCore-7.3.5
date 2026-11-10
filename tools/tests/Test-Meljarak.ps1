param([switch]$Baseline,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DMELJARAK_BASELINE"}
 if($VerifyLiveData){if($Baseline){throw 'Broken counterfactual cannot certify live data'};$env:CL="$previous /DMELJARAK_LIVE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe MeljarakNative
 if(!$? -or $LASTEXITCODE){throw 'Native Meljarak regression failed'}
}finally{$env:CL=$previous}

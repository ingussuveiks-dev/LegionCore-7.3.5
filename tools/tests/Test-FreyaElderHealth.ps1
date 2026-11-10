param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DFREYA_ELDER_HEALTH_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe FreyaElderHealthNative
 if(!$? -or $LASTEXITCODE){throw 'Native Freya elder-health regression failed'}
}finally{$env:CL=$previous}

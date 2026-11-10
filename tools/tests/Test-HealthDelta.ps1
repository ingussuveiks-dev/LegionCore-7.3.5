param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DHEALTH_DELTA_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe HealthDeltaNative
 if(!$? -or $LASTEXITCODE){throw 'Native health-delta regression failed'}
}finally{$env:CL=$previous}

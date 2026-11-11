param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DNATIVE_HEAL_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe NativeHealApi
 if(!$? -or $LASTEXITCODE){throw 'Native heal amount/gain/absorption regression failed'}
}finally{$env:CL=$previous}

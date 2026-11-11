param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DMELJARAK_MEND_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe MeljarakMendNative
 if(!$? -or $LASTEXITCODE){throw 'Native Mending profile regression failed'}
}finally{$env:CL=$previous}

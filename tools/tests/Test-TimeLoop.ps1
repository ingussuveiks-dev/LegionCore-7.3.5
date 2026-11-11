param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DTIME_LOOP_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe TimeLoopNative
 if(!$? -or $LASTEXITCODE){throw 'Native TimeLoop decision regression failed'}
}finally{$env:CL=$previous}

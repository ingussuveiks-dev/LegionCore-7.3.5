param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DTIME_LOOP_HEAL_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe TimeLoopHealingNative
 if(!$? -or $LASTEXITCODE){throw 'Native full TimeLoop healing regression failed'}
}finally{$env:CL=$previous}

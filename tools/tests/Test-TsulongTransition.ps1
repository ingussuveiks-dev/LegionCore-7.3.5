param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DTSULONG_TRANSITION_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe TsulongTransitionNative
 if(!$? -or $LASTEXITCODE){throw 'Native Tsulong transition regression failed'}
}finally{$env:CL=$previous}

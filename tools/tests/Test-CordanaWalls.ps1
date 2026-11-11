param([switch]$Baseline)
$ErrorActionPreference='Stop';$previous=$env:CL
try{
 if($Baseline){$env:CL="$previous /DCORDANA_WALL_BASELINE"}
 & "$PSScriptRoot/Test-MagnarReadinessNative.ps1" -AdditionalProbe CordanaWallsNative
 if(!$? -or $LASTEXITCODE){throw 'Cordana full-method geometry regression failed'}
}finally{$env:CL=$previous}

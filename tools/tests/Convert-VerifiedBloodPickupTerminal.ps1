param([Parameter(Mandatory)][string[]]$Rows)
$ErrorActionPreference='Stop'
$matches=@($Rows|Where-Object {$f=$_.Split("`t");$f.Length -eq 32 -and $f[0] -ceq 'live-smart_scripts' -and $f[1] -eq '247566' -and $f[2] -eq '1' -and $f[3] -eq '0' -and $f[4] -eq '0'})
if(!$matches.Count){return ,$Rows}
if($matches.Count -ne 1){throw 'Ambiguous Blood pickup terminal row'}
# Reconstruct only the old comparison field after complete current-context and
# RELEASED proof. Never change live data or skip other historical differences.
& "$PSScriptRoot/Test-SmartTerminalData.ps1" -ContextFile "$PSScriptRoot/BloodPickupTerminalContext.json" -FixtureFile "$PSScriptRoot/BloodPickupTerminalOriginal.json" -OutputDirectory "$PSScriptRoot/../../build-extractors/runtime-test/blood-pickup-terminal-live-data" -VerifyLiveData | Out-Host
return ,@($Rows|ForEach-Object {if($_ -ceq $matches[0]){$f=$_.Split("`t");$f[4]='1';$f -join "`t"}else{$_}})

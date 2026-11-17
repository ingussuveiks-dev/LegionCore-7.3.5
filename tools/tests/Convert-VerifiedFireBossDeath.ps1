param([Parameter(Mandatory)][string[]]$Rows)
$ErrorActionPreference='Stop'
$matches=@($Rows|Where-Object {$f=$_.Split("`t");$f.Length -eq 32 -and $f[0] -ceq 'live-smart_scripts' -and $f[1] -eq '99615' -and $f[2] -eq '0' -and $f[3] -eq '31' -and $f[4] -eq '0'})
if(!$matches.Count){return ,$Rows}
if($matches.Count -ne 1){throw 'Ambiguous Fire death terminal row'}
# Permit only the independently checked full native context and RELEASED hash.
& "$PSScriptRoot/Test-SmartTerminalData.ps1" -ContextFile "$PSScriptRoot/FireBossDeathContext.json" -FixtureFile "$PSScriptRoot/FireBossDeathOriginal.json" -OutputDirectory "$PSScriptRoot/../../build-extractors/runtime-test/fire-death-live-data" -VerifyLiveData | Out-Host
return ,@($Rows|ForEach-Object {if($_ -ceq $matches[0]){$f=$_.Split("`t");$f[4]='32';$f -join "`t"}else{$_}})

param([Parameter(Mandatory=$true)][string[]]$Rows,[Parameter(Mandatory=$true)]$Fixture)
$ErrorActionPreference='Stop'
$column=@($Fixture.creature_template[0].PSObject.Properties.Name).IndexOf('ScriptName')+1
if($column -lt 1){throw 'Missing template ScriptName column in pinned baseline'}
$tag='live-creature_template'
$approved=@($Rows|Where-Object {$f=$_.Split("`t");$f.Count -gt $column -and $f[0] -ceq $tag -and $f[1] -eq '99763' -and $f[$column] -ceq 'npc_fire_artifact_aethas'})
if(!$approved.Count){return ,$Rows}
if($approved.Count -ne 1){throw 'Ambiguous approved Aethas binding'}
# An older focused regression predates this deliberate one-field binding.
# Validate the later migration's entire six-table live scope and released hash
# before comparing that single field against its original empty baseline.
& "$PSScriptRoot/Test-FireReturnRecoveryData.ps1" -VerifyLiveData | ForEach-Object {Write-Host $_}
$result=@($Rows|ForEach-Object {$f=$_.Split("`t");if($f.Count -gt $column -and $f[0] -ceq $tag -and $f[1] -eq '99763' -and $f[$column] -ceq 'npc_fire_artifact_aethas'){$f[$column]='';$f -join "`t"}else{$_}})
return ,([string[]]$result)

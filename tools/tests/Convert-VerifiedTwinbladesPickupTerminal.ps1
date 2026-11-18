param([Parameter(Mandatory)][string[]]$Rows)
$ErrorActionPreference='Stop'
$matches=@($Rows|Where-Object {$f=$_.Split("`t");$f.Length -eq 32 -and $f[0] -ceq 'live-smart_scripts' -and $f[1] -eq '251878' -and $f[2] -eq '1' -and $f[3] -eq '4' -and $f[4] -eq '0'})
if(!$matches.Count){return ,$Rows}
if($matches.Count -ne 1){throw 'Ambiguous Twinblades terminal row'}
& "$PSScriptRoot/Test-SmartTerminalData.ps1" -ContextFile "$PSScriptRoot/TwinbladesPickupTerminalContext.json" -FixtureFile "$PSScriptRoot/TwinbladesPickupTerminalOriginal.json" -OutputDirectory "$PSScriptRoot/../../build-extractors/runtime-test/twinblades-pickup-terminal-live-data" -VerifyLiveData | Out-Host
# Four later native broadcast fallbacks in the older shared speaker fixture.
# The full successor above fingerprints them. Check their exact committed
# before/after provenance too, then restore only comparison Text fields.
$plan=Get-Content "$PSScriptRoot/ArtifactTenTen20261116FallbackRepairs.json" -Raw|ConvertFrom-Json
$allowed=@('94836/1/0','110556/0/0','110556/1/0','110556/4/0');$fallback=@{}
foreach($row in $plan.repairs){$key="$($row.key.CreatureID)/$($row.key.GroupID)/$($row.key.ID)";if($row.table -eq 'creature_text' -and $key -in $allowed){if($fallback.ContainsKey($key)){throw 'Ambiguous native fallback provenance'};$fallback[$key]=$row}}
if($fallback.Count -ne 4){throw 'Expected four native fallback records'}
return ,@($Rows|ForEach-Object {
 $f=$_.Split("`t")
 if($_ -ceq $matches[0]){$f[4]='5'}
 if($f[0] -ceq 'live-creature_text'){$key="$($f[1])/$($f[2])/$($f[3])";if($fallback.ContainsKey($key)){$known=$fallback[$key];if($f[4] -cne $known.after_text){throw 'Verified native fallback differs'};$f[4]=$known.before_text}}
 $f -join "`t"
})

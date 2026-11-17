param([Parameter(Mandatory)][string[]]$Rows)
$ErrorActionPreference='Stop'
$matches=@($Rows|Where-Object {$f=$_.Split("`t");$f.Length -eq 32 -and $f[0] -ceq 'live-smart_scripts' -and $f[1] -eq '108168' -and $f[2] -eq '0' -and $f[3] -eq '7' -and $f[4] -eq '0'})
if(!$matches.Count){return ,$Rows}
if($matches.Count -ne 1){throw 'Ambiguous Balaadur death terminal row'}
# Validate the complete successor and updater before reconstructing old input.
& "$PSScriptRoot/Test-SmartTerminalData.ps1" -ContextFile "$PSScriptRoot/BalaadurDeathTerminalContext.json" -FixtureFile "$PSScriptRoot/BalaadurDeathTerminalOriginal.json" -OutputDirectory "$PSScriptRoot/../../build-extractors/runtime-test/balaadur-death-live-data" -VerifyLiveData | Out-Host
$historical=Get-Content "$PSScriptRoot/ArtifactBossSpeakersOriginal.json" -Raw|ConvertFrom-Json
$fallback=@($historical.creature_text|Where-Object {$_.CreatureID -eq 108168 -and $_.GroupID -eq 4 -and $_.ID -eq 0})
if($fallback.Count -ne 1){throw 'Historical Balaadur fallback is ambiguous'}
# The complete successor above also validates the later native-broadcast text
# correction (2026_11_16_06_artifact_fallback_batch06). Restore only this one
# field when comparing against these older wave fixtures; retain its live text.
return ,@($Rows|ForEach-Object {
 $f=$_.Split("`t")
 if($_ -ceq $matches[0]){$f[4]='8';$f -join "`t"}
 elseif($f[0] -ceq 'live-creature_text' -and $f[1] -eq '108168' -and $f[2] -eq '4' -and $f[3] -eq '0'){
  if($f[4] -cne "You possess the might of an archmage! But I'm not out of tricks yet..."){throw 'Verified Balaadur fallback row differs'}
  $f[4]=$fallback[0].Text;$f -join "`t"
 }else{$_}
})

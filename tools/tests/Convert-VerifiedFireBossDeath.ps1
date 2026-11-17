param([Parameter(Mandatory)][string[]]$Rows)
$ErrorActionPreference='Stop'
$matches=@($Rows|Where-Object {$f=$_.Split("`t");$f.Length -eq 32 -and $f[0] -ceq 'live-smart_scripts' -and $f[1] -eq '99615' -and $f[2] -eq '0' -and $f[3] -eq '31' -and $f[4] -eq '0'})
if(!$matches.Count){return ,$Rows}
if($matches.Count -ne 1){throw 'Ambiguous Fire death terminal row'}
# Permit only the independently checked full native context and RELEASED hash.
& "$PSScriptRoot/Test-SmartTerminalData.ps1" -ContextFile "$PSScriptRoot/FireBossDeathContext.json" -FixtureFile "$PSScriptRoot/FireBossDeathOriginal.json" -OutputDirectory "$PSScriptRoot/../../build-extractors/runtime-test/fire-death-live-data" -VerifyLiveData | Out-Host
# The full successor was checked above, including all13 tables and its updater
# hash. Reverse only its four known fields in this comparison snapshot; live
# tables are never edited and unrelated columns retain their exact values.
$successor=Get-Content "$PSScriptRoot/FireLyandraPhaseContext.json" -Raw|ConvertFrom-Json -AsHashtable
return ,@($Rows|ForEach-Object {
 $f=$_.Split("`t")
 foreach($edit in $successor.edits){
  if($f[0] -cne "live-$($edit.table)"){continue}
  $columns=@($successor.columns[$edit.table]);$same=$true
  foreach($key in $edit.key.Keys){$index=[Array]::IndexOf($columns,$key)+1;if($index -lt 1 -or $f[$index] -cne [string]$edit.key[$key]){$same=$false}}
  if(!$same){continue}
  $index=[Array]::IndexOf($columns,$edit.field)+1
  if($index -lt 1 -or $f[$index] -cne [string]$edit.after){throw 'Caller snapshot disagrees with verified Fire phase successor'}
  $f[$index]=[string]$edit.before
 }
 if($_ -ceq $matches[0]){$f[4]='32'}
 $f -join "`t"
})

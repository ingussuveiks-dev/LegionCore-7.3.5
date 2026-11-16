# Historical host view only. The new whole native suite executes current code.
function Remove-SharasdalFinaleScope([string]$source,[string]$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp') {
 if(!$script:SharasdalFinaleScopeVerified){
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/sharasdal_living_finale_scope.py" | Out-Null
  if($LASTEXITCODE){throw 'Actual living-finale source delta is not certified'}
  $script:SharasdalFinaleScopeVerified=$true
 }
 $source=$source.Replace("`r",'')
 $evade=Get-Content "$PSScriptRoot/SharasdalErunakEvadeRepair.json" -Raw|ConvertFrom-Json
 if($path -ceq $evade.path -and $source.Contains('#include "sharasdal_erunak_evade.h"')){
  foreach($edit in $evade.edits){
   if([regex]::Matches($source,[regex]::Escape($edit.new)).Count -ne 1){throw 'Partial Erunak evade amendment in historical source view'}
   $source=$source.Replace($edit.new,$edit.old)
  }
 }
 $plan=Get-Content "$PSScriptRoot/SharasdalLivingFinaleRepair.json" -Raw|ConvertFrom-Json
 foreach($edit in $plan.edits){if($edit.path -ceq $path -and $source.Contains($edit.new)){$source=$source.Replace($edit.new,$edit.old)}}
 return $source
}

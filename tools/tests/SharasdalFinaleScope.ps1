# Historical host view only. The new whole native suite executes current code.
function Remove-SharasdalFinaleScope([string]$source,[string]$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp') {
 if(!$script:SharasdalFinaleScopeVerified){
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/sharasdal_living_finale_scope.py" | Out-Null
  if($LASTEXITCODE){throw 'Actual living-finale source delta is not certified'}
  $script:SharasdalFinaleScopeVerified=$true
 }
 $source=$source.Replace("`r",'')
 $plan=Get-Content "$PSScriptRoot/SharasdalLivingFinaleRepair.json" -Raw|ConvertFrom-Json
 foreach($edit in $plan.edits){if($edit.path -ceq $path -and $source.Contains($edit.new)){$source=$source.Replace($edit.new,$edit.old)}}
 return $source
}

# Historical host boundary only; the whole native suite executes current code.
function Remove-AshbringerStorageScope([string]$source) {
 if(!$script:AshbringerStorageVerified){
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/ashbringer_storage_scope.py" | Out-Null
  if($LASTEXITCODE){throw 'Actual storage amendment is not certified'}
  $script:AshbringerStorageVerified=$true
 }
 $source=$source.Replace("`r",'')
 $plan=Get-Content "$PSScriptRoot/AshbringerStorageRepair.json" -Raw|ConvertFrom-Json
 foreach($edit in $plan.edits){if($source.Contains($edit.new)){$source=$source.Replace($edit.new,$edit.old)}}
 return $source
}

# Preserve pinned counterfactual bodies while adapting only their return declaration
# to the current host ABI. Actual current methods are exported without rewriting.
function Get-HealApiBlock([string]$source){
 $marker=if($source.Contains('int64 Unit::DealHeal(')){'int64 Unit::DealHeal('}else{'int32 Unit::DealHeal('}
 (Block $source $marker).Replace('int32 Unit::DealHeal(','int64 Unit::DealHeal(')
}
function Get-HealApiScopeSource([string]$source){
 if($source.Contains('int64 /*gain*/') -and !$script:NativeHealApiScopeVerified){
  # Only an unused parameter declaration is normalized for historical views.
  # The independent whole-file certificate retains every current payload.
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_native_heal_scope.py" | Out-Null
  if($LASTEXITCODE){throw 'Native heal gain amendment is not certified'}
  $script:NativeHealApiScopeVerified=$true
 }
 $source.Replace('int64 /*gain*/','int32 /*gain*/')
}

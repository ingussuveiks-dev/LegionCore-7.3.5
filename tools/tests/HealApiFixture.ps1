# Preserve pinned counterfactual bodies while adapting only their return declaration
# to the current host ABI. Actual current methods are exported without rewriting.
function Get-HealApiBlock([string]$source){
 if($source.Contains('uint64 Unit::DealHeal(')){
  # Historical quest fixtures have bounded32 health services and AI stubs.
  # Keep this explicit test-only ABI adapter out of every real native probe.
  # Test-HealingWidthCallbacks/TimeLoopHealing/NativeHealApi execute the actual
  # unsigned64 methods separately. A wide amount here must fail,never truncate.
  $block=Block $source 'uint64 Unit::DealHeal('
  $block=$block.Replace('addhealth','wideHealth').Replace('uint64 Unit::DealHeal(','int64 Unit::DealHeal(').Replace('uint64 wideHealth','uint32 addhealth').Replace('uint64 gain = 0;','uint64 wideHealth = addhealth; int64 gain = 0;')
  return $block.Replace('victim->AddHealth(wideHealth, this)','(assert(wideHealth <= INT32_MAX), victim->ModifyHealth(int64(wideHealth), this))').Replace('NotifyHealApplied64(','NotifyHealApplied(')
 }
 $marker=if($source.Contains('int64 Unit::DealHeal(')){'int64 Unit::DealHeal('}else{'int32 Unit::DealHeal('}
 (Block $source $marker).Replace('int32 Unit::DealHeal(','int64 Unit::DealHeal(')
}
function Get-HealApiNotifyBlock([string]$source){
 $body=Block $source 'void Creature::NotifyHealApplied('
 if($source.Contains('void Creature::NotifyHealApplied64(')){$body+="`n"+(Block $source 'void Creature::NotifyHealApplied64(')}
 $body
}
function Get-HealApiScopeSource([string]$source){
 if(($source.Contains('int64 /*gain*/') -or $source.Contains('uint64 /*gain*/')) -and !$script:NativeHealApiScopeVerified){
  # Only an unused parameter declaration is normalized for historical views.
  # The independent whole-file certificate retains every current payload.
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_native_heal_scope.py" | Out-Null
  if($LASTEXITCODE){throw 'Native heal gain amendment is not certified'}
  $script:NativeHealApiScopeVerified=$true
 }
 $source.Replace('uint64 /*amount*/, uint64 /*gain*/','uint32 /*amount*/, int32 /*gain*/').Replace('int64 /*gain*/','int32 /*gain*/')
}

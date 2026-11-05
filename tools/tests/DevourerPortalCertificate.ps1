# Only accepted41924 Valzuun work and the native248901 use gate may differ.
function Restore-DevourerPortalBoundary([string]$source){
 $source=$source.Replace('#include "CombatAI.h"','')
 $old=(& git -C $repo show '45360fd90:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned portal baseline missing'}
 $body=Block $source 'class npc_valzuun :';$prior=Block $old 'class npc_valzuun :'
 $source=$source.Replace($body,$prior)
 $body=Block $source 'class go_devourer_misc';$prior=Block $old 'class go_devourer_misc'
 if($body.Contains('bool PortalPlayer(')){
  $normalized=$body.Replace((Block $body 'bool PortalPlayer('),'')
  $use=Block $body 'bool GossipUse(';$before=Block $prior 'bool GossipUse('
  $plain=$use.Replace('if (go->GetEntry() == 248901)','').Replace('return !PortalPlayer(player); // Keep native GO credit/209066.','')
  if(($plain-replace '\s','') -cne ($before-replace '\s','')){throw 'Original native cage/cache/door use unexpectedly changed'}
  $normalized=$normalized.Replace($use,$before)
  if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed GO change outside248901 gate'}
  $source=$source.Replace($body,$prior)
 }
 return $source
}

# Restore only the independently checked Lucas108399 callback ownership delta.
function Restore-KingslayersLucasBoundary([string]$source){
 $old=(& git -C $repo show '3cf03e7a9:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Lucas baseline missing'}
 $marker='class npc_kingslayers_small_events : public CreatureScript';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body.Replace('EventMap lucasEvents;','').Replace((Block $body 'void UpdateLucas(uint32 diff)'),'').Replace('UpdateLucas(diff);','')
 $branch=Block $normalized 'if (me->GetEntry() == 108399 &&';$before=Block $prior 'if (me->GetEntry() == 108399 &&';$normalized=$normalized.Replace($branch,$before)
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed shared AI change outside Lucas queue ownership'}
 return $source.Replace($body,$prior)
}

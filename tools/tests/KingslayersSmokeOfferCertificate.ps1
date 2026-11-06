# Restore only the separately certified map1620 Craggle offer delta.
function Restore-KingslayersSmokeOfferBoundary([string]$source) {
 . "$PSScriptRoot/KingslayersLucasCertificate.ps1"
 $source=Restore-KingslayersLucasBoundary $source
 $old=(& git -C $repo show 'e7c890b41:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned smoke-offer boundary missing'}
 $marker='class npc_kingslayers_small_events : public CreatureScript'
 $body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body.Replace((Block $body 'void UpdateSmokeOffer(uint32 diff)'),'').Replace('        bool smokeOfferWarning = false;','')
 $normalized=$normalized.Replace("if (me->GetEntry() == 108177 && me->GetMapId() == 1620 && me->GetDBTableGUIDLow() == 360801)`r`n                UpdateSmokeOffer(diff);`r`n            else if (me->GetEntry() == 108177 && !check)",'if (me->GetEntry() == 108177 && !check)').Replace("if (me->GetEntry() == 108177 && me->GetMapId() == 1620 && me->GetDBTableGUIDLow() == 360801)`n                UpdateSmokeOffer(diff);`n            else if (me->GetEntry() == 108177 && !check)",'if (me->GetEntry() == 108177 && !check)')
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed shared small-events change outside scoped smoke offer'}
 return $source.Replace($body,$prior)
}

# Only native105609 mirror-image retirement at first-Akaari Reset may differ.
function Restore-DevourerCloneResetBoundary([string]$source){
 $old=(& git -C $repo show 'c48f300b0:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-clone-reset source missing'}
 $marker='class npc_akaari_shadowgore :';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 # An enclosing certificate may already have removed both clone reset and
 # the independently checked opening. Accept only that exact pinned class.
 $opening=(& git -C $repo show '48c20d9c6:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-opening capture source missing'}
 if(($body-replace '\s','') -ceq ((Block $opening $marker)-replace '\s','')){return $source}
 $normalized=$body.Replace((Block $body 'void ResetClones('),'')
 $reset=Block $body 'void Reset(';$before=Block $prior 'void Reset('
 if(($reset.Replace('ResetClones();','')-replace '\s','') -cne ($before-replace '\s','')){throw 'Original capture Reset unexpectedly changed'}
 $normalized=$normalized.Replace($reset,$before)
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed capture change outside native clone Reset'}
 return $source.Replace($body,$prior).Replace('#include "TemporarySummon.h"','')
}

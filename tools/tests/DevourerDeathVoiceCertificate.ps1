# Certify only the independently tested1947 source actor subtype/allocation.
function Restore-DevourerDeathVoiceBoundary([string]$source){
 $old=(& git -C $repo show '0deb4788b:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned original Akaari death source absent'}
 $marker='class npc_akaari_shadowgore_last :';$body=Block $source $marker;$before=Block $old $marker
 if(($body-replace '\s','') -ceq ($before-replace '\s','')){return $source}
 $actor=Block $body 'class DeathConversation final : public Conversation'
 $normalized=$body.Replace($actor+';','').Replace('new DeathConversation(me->GetGUID())','new Conversation')
 if((($normalized-replace '//[^\n]*','')-replace '\s','') -cne (($before-replace '//[^\n]*','')-replace '\s','')){throw 'Unreviewed Akaari change outside1947 actor subtype/allocation'}
 return $source.Replace($body,$before)
}

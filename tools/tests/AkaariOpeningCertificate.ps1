# Only independently covered opening1864 admission/publication/retry may differ.
function Restore-AkaariOpeningBoundary([string]$source){
 $old=(& git -C $repo show '48c20d9c6:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Akaari opening source missing'}
 $marker='class npc_akaari_shadowgore :';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body
 foreach($name in @('Player* IntroPlayer(','void TryIntro(','void UpdateIntro(')){$normalized=$normalized.Replace((Block $body $name),'')}
 foreach($field in @('uint32 introTimer = 0, introGeneration = 0;','bool introBusy = false, introWarning = false;')){$normalized=$normalized.Replace($field,'')}
 $reset=Block $body 'void Reset(';$before=Block $prior 'void Reset('
 if(($reset.Replace('++introGeneration;','').Replace('introTimer = 0;','').Replace('introWarning = false;','')-replace '\s','') -cne ($before-replace '\s','')){throw 'Original capture Reset changed'}
 $normalized=$normalized.Replace($reset,$before)
 $update=Block $body 'void UpdateAI(';$before=Block $prior 'void UpdateAI('
 if(($update.Replace('UpdateIntro(diff);','')-replace '\s','') -cne ($before-replace '\s','')){throw 'Original capture dispatcher changed'}
 $normalized=$normalized.Replace($update,$before).Replace((Block $body 'void MoveInLineOfSight('),(Block $prior 'void MoveInLineOfSight('))
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed change outside Akaari1864 opening'}
 return $source.Replace($body,$prior)
}

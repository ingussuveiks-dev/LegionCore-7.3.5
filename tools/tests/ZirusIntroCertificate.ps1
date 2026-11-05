# Only independently tested Zirus1884 qualification/receipt/retry may differ.
function Restore-ZirusIntroBoundary([string]$source){
 $old=(& git -C $repo show '2cd89325a:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Zirus intro source missing'}
 $marker='class npc_devourer_zirus :';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body
 foreach($name in @('Player* IntroPlayer(','void TryIntro(','void UpdateIntro(')){$normalized=$normalized.Replace((Block $body $name),'')}
 foreach($field in @('uint32 introTimer = 0, introGeneration = 0;','bool introBusy = false, introWarning = false;')){$normalized=$normalized.Replace($field,'')}
 $reset=Block $body 'void Reset(';$before=Block $prior 'void Reset('
 $plain=$reset.Replace('++introGeneration;','').Replace('introTimer = 0;','').Replace('introWarning = false;','')
 if(($plain-replace '\s','') -cne ($before-replace '\s','')){throw 'Original Zirus Reset unexpectedly changed'}
 $normalized=$normalized.Replace($reset,$before)
 $update=Block $body 'void UpdateAI(';$before=Block $prior 'void UpdateAI('
 if(($update.Replace('UpdateIntro(diff);','')-replace '\s','') -cne ($before-replace '\s','')){throw 'Original Zirus combat tail unexpectedly changed'}
 $normalized=$normalized.Replace($update,$before).Replace((Block $body 'void MoveInLineOfSight('),(Block $prior 'void MoveInLineOfSight('))
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed Zirus change outside1884 intro'}
 return $source.Replace($body,$prior)
}

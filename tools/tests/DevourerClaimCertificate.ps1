# The independently regressed pending claim may change only these callbacks.
function Restore-DevourerClaimBoundary([string]$source){
 . "$PSScriptRoot/DevourerDoorCertificate.ps1"
 $source=Restore-DevourerDoorBoundary $source
 $old=(& git -C $repo show 'd8973a49b:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Devourer claimant source absent'}
 $marker='class go_art_devourer';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body.Replace((Block $body 'Player* ClaimantPlayer('),'').Replace('std::shared_ptr<QuestCancellationReceipt const> claimReceipt;','').Replace('bool starting = false;','')
 foreach($name in @('void Cancel(','void SetGUID(','void UpdateAI(')){$normalized=$normalized.Replace((Block $body $name),(Block $prior $name))}
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed pickup constructor/admission/loot/actor change outside claim callbacks'}
 return $source.Replace($body,$prior)
}

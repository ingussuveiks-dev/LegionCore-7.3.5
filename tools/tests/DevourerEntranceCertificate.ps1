# The entrance delta has its own full-class native/ASAN coverage. Preserve the
# original death/rotation/Reset and earlier independent capture certificates.
function Restore-DevourerEntranceBoundary([string]$source){
 . "$PSScriptRoot/DevourerDeathVoiceCertificate.ps1"
 $source=Restore-DevourerDeathVoiceBoundary $source
 $old=(& git -C $repo show '698d34495:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Devourer entrance boundary absent'}
 $marker='class npc_akaari_shadowgore_last :';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body
 foreach($m in @('bool EntryLeaf(','Player* EntryPlayer(','Player* EntryRecipient(','void UpdateEntry(')){$normalized=$normalized.Replace((Block $body $m),'')}
 foreach($field in @('ObjectGuid entryPlayer;','std::shared_ptr<QuestCancellationReceipt const> entryReceipt;','bool entryBusy = false, entryVisual = false, entryWarning = false, entryCreditWarning = false;')){$normalized=$normalized.Replace($field,'')}
 $update=Block $body 'void UpdateAI('; $before=Block $prior 'void UpdateAI('
 $oldGate=Block $before 'if (instance->getScenarionStep() == 4)'
 if((($update.Replace('UpdateEntry(diff);',''))-replace '\s','') -cne (($before.Replace($oldGate,''))-replace '\s','')){throw 'Unreviewed original combat tail change'}
 $normalized=$normalized.Replace($update,$before)
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed final Akaari death/Reset/combat/constructor change'}
 return $source.Replace($body,$prior)
}

# The exact1942 source/owner/pending state is independently covered here.
function Restore-DevourerOpeningBoundary([string]$source){
 . "$PSScriptRoot/DevourerClaimCertificate.ps1"
 $source=Restore-DevourerClaimBoundary $source
 . "$PSScriptRoot/ZirusIntroCertificate.ps1"
 $source=Restore-ZirusIntroBoundary $source
 $old=(& git -C $repo show 'c000a548d:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned opening source missing'}
 $marker='class npc_akaari_shadowgore_last :';$body=Block $source $marker;$before=Block $old $marker
 if(!$body.Contains('ObjectGuid combatOpener;')){
  if(($body-replace '\s','') -ceq ($before-replace '\s','')){return $source}
  foreach($pin in @('48ec2ea80','0deb4788b','698d34495')){
   $historical=(& git -C $repo show "${pin}:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp") -join "`n"
   if($LASTEXITCODE){throw 'Historical opening boundary missing'}
   if(($body-replace '\s','') -ceq ((Block $historical $marker)-replace '\s','')){return $source}
  }
  throw 'Unknown final-Akaari class without opening state'
 }
 $a=$body.IndexOf('        ObjectGuid combatOpener;');$last=Block $body 'class OpeningConversation final : public Conversation';$b=$body.IndexOf($last,$a)+$last.Length+1
 $normalized=$body.Remove($a,$b-$a)
 foreach($name in @('void ResetCombatVoice(','void UpdateCombatVoice(','void RequestCombatVoice(','void EnterCombat(')){
  $normalized=$normalized.Replace((Block $body $name),(Block $before $name))
 }
 if(($source.Replace($body,$normalized)-replace '\s','') -cne ($old-replace '\s','')){throw 'Unreviewed change outside1942 opening and its existing voice lane'}
 return $source.Replace($body,$before)
}

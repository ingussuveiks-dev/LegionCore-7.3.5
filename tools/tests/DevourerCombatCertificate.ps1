# Original final-Akaari combat changed only by this separately tested voice lane.
function Restore-DevourerCombatBoundary([string]$source){
 $old=(& git -C $repo show '48ec2ea80:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned combat source missing'}
 $marker='class npc_akaari_shadowgore_last :';$body=Block $source $marker;$before=Block $old $marker
 if(($body-replace '\s','') -ceq ($before-replace '\s','')){return $source}
 if(!$body.Contains('uint32 combatVoiceDelay')){
  foreach($pin in @('0deb4788b','698d34495')){
   $historical=(& git -C $repo show "${pin}:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp") -join "`n"
   if($LASTEXITCODE){throw 'Historical independently certified boundary missing'}
   if(($body-replace '\s','') -ceq ((Block $historical $marker)-replace '\s','')){return $source}
  }
  throw 'Unknown final-Akaari class without combat helpers'
 }
 $a=$body.IndexOf('        uint32 combatVoiceDelay');$last=Block $body 'void RequestCombatVoice('
 $b=$body.IndexOf($last,$a)+$last.Length
 if($a -lt 0 -or $b -le $a){throw 'Dedicated combat voice boundary missing'}
 $normalized=$body.Remove($a,$b-$a)
 foreach($name in @('void Reset(','void EnterCombat(','void UpdateAI(')){
  $current=Block $body $name;$prior=Block $before $name
  if($name -like '*UpdateAI*'){
   $oldVoice='convers++;'+(Block $prior 'if (convers < 1947)')
   $compact=$prior-replace '\s','';$voice=$oldVoice-replace '\s',''
   $expected=$compact.Replace($voice,'RequestCombatVoice();').Replace('events.Update(diff);','UpdateCombatVoice(diff);events.Update(diff);')
  }
  if($name -notlike '*UpdateAI*'){$expected=$prior-replace '\s','';$actual=($current.Replace('ResetCombatVoice();',''))-replace '\s',''}else{$actual=$current-replace '\s',''}
  if($actual -cne $expected){throw "Original combat callback unexpectedly changed: $name"}
  $normalized=$normalized.Replace($current,$prior)
 }
 if(($source.Replace($body,$normalized)-replace '\s','') -cne ($old-replace '\s','')){throw 'Unreviewed change outside final Akaari combat voice'}
 return $source.Replace($body,$before)
}

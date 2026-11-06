# Verify the exact Priest-only addition before older whole-instance comparisons.
function Remove-SilverHandHandoffScope([string]$source) {
 $source=$source.Replace("`r",'')
 $include='#include "../../Priest/slaghammer_handoff.h"'
 $base='struct instance_silver_hand_InstanceMapScript : public InstanceScript, public SlaghammerHandoff::Instance'
 $enter=@'
        void OnPlayerEnter(Player* player) override
        {
            slaghammer.Rejoin(player);
        }
'@
 $actor=@'
class npc_slaghammer_handoff : public CreatureScript
{
public:
    npc_slaghammer_handoff() : CreatureScript("npc_slaghammer_handoff") { }
    CreatureAI* GetAI(Creature* creature) const override { return new SlaghammerHandoff::AI(creature); }
};
'@
 foreach($part in @($include,$base,$enter,$actor,'    new npc_slaghammer_handoff();')) {
  if(!$source.Contains($part.Replace("`r",''))){throw 'Exact Priest instance-owned handoff binding differs'}
 }
 $source=$source.Replace($include,'').Replace($base,'struct instance_silver_hand_InstanceMapScript : public InstanceScript').Replace($enter.Replace("`r",''),'').Replace($actor.Replace("`r",''),'').Replace('    new npc_slaghammer_handoff();','')
 $pinned=(& git -C "$PSScriptRoot/../.." show 'dafaace6d:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp') -join "`n"
 if($LASTEXITCODE -or ($source -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Shared instance changed beyond the exact Priest handoff addition'}
 return $source
}

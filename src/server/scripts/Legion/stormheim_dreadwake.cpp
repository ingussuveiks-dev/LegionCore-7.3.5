#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GossipDef.h"

struct npc_stormheim_dreadwake_cullen : SmartAI
{
    explicit npc_stormheim_dreadwake_cullen(Creature* c):SmartAI(c){}
    void sGossipSelect(Player* p,uint32 menu,uint32 option) override
    {
        if(menu!=18657 || option!=0){SmartAI::sGossipSelect(p,menu,option);return;}
        if(!p || p->GetTeam()!=HORDE || !p->IsInWorld() || !p->IsAlive() || p->GetMapId()!=1220 ||
            p->GetInstanceId() || p->GetZoneId()!=7541 || p->GetMap()!=me->GetMap() ||
            me->GetEntry()!=93624 || me->GetDBTableGUIDLow()!=291298 || !me->IsAlive() ||
            !me->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP) || !p->InSamePhase(me) ||
            !p->IsWithinDistInMap(me,INTERACTION_DISTANCE) || p->IsBeingTeleported() ||
            p->GetVehicle() || p->GetTransport() || p->isInFlight())return;
        auto status=p->GetQuestStatus(38872);
        // The only objective is optional/hidden: native acceptance can already
        // complete this delivery quest before the player asks for the flight.
        if(status!=QUEST_STATUS_INCOMPLETE && status!=QUEST_STATUS_COMPLETE)return;
        auto& gossip=p->PlayerTalkClass->GetGossipMenu();
        if(gossip.GetMenuId()!=18657 || !gossip.GetItem(0))return;
        // Native quest taxi5262 has the Riding Bat/Duchess display60307,
        // zero cost and seven authored points; keep normal taxi cleanup.
        if(p->ActivateTaxiPathTo(5262))p->PlayerTalkClass->SendCloseGossip();
    }
};

void AddSC_stormheim_dreadwake()
{
    TC_LOG_INFO("server.loading","Dreadwake: Cullen native quest taxi5262");
    RegisterCreatureAI(npc_stormheim_dreadwake_cullen);
}

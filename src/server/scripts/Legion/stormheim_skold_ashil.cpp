#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "GameObject.h"
#include "Player.h"

namespace StormheimSkoldAshil
{
struct Shield {uint32 entry;ObjectGuid::LowType spawn;};
Shield const Shields[]={{242673,115401},{251557,115389},{251558,115385},{251559,115377},{251560,115394},{251561,115380}};
bool NativeShield(GameObject* go)
{
    if(go->GetMapId()!=1220 || go->GetZoneId()!=7541 || go->GetInstanceId())return false;
    for(auto const& s:Shields)if(go->GetEntry()==s.entry && go->GetDBTableGUIDLow()==s.spawn)return true;
    return false;
}
}

class go_stormheim_engraved_shield : public GameObjectScript
{
public:
    go_stormheim_engraved_shield():GameObjectScript("go_stormheim_engraved_shield"){}
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* go):GameObjectAI(go){}
        bool GossipUse(Player* p) override
        {
            if(!StormheimSkoldAshil::NativeShield(go))return false;
            if(!p || !p->IsInWorld() || !p->IsAlive() || p->GetMap()!=go->GetMap() || !p->InSamePhase(go) ||
                !go->isSpawned() || go->IsQuestObjectExpired() || go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE) ||
                !p->IsWithinDistInMap(go,go->GetInteractionDistance()))return true;
            // Native goober Use immediately follows this and credits this GO.
            // Its distinct objective is the per-player repeat gate; the old
            // group event credited every click and nearby non-reading players.
            if(p->GetQuestStatus(39063)==QUEST_STATUS_INCOMPLETE && !p->GetQuestObjectiveData(39063,go->GetEntry()))
                p->KilledMonsterCredit(109346);
            return false; // Preserve native gossip, GO credit and open/close.
        }
    };
    GameObjectAI* GetAI(GameObject* go) const override {return new AI(go);}
};

void AddSC_stormheim_skold_ashil()
{
    TC_LOG_INFO("server.loading","Skold-Ashil: distinct shield readings");
    new go_stormheim_engraved_shield();
}

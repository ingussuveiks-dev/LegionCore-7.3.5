#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "GameObject.h"
#include "Player.h"
#include "Creature.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include <memory>

namespace StormheimSkoldAshil
{
struct Shield {uint32 entry;ObjectGuid::LowType spawn;};
Shield const Shields[]={{242673,115401},{251557,115389},{251558,115385},{251559,115377},{251560,115394},{251561,115380}};
uint32 CommunionQuest(Player* p) {return p && p->GetTeam()==HORDE?39155:39092;}
bool Shrines(Player* p)
{
    for(auto entry:{242994u,242995u,242996u,242998u})if(!p->GetQuestObjectiveData(CommunionQuest(p),entry))return false;
    return true;
}
bool Communion(Player* p,Creature* c)
{
    return p && c && p->IsInWorld() && p->IsAlive() && c->IsInWorld() && c->IsAlive() && p->GetMapId()==1220 &&
        p->GetZoneId()==7541 && !p->GetInstanceId() && c->GetMapId()==1220 && !c->GetInstanceId() &&
        c->GetEntry()==94393 && c->GetDBTableGUIDLow()==293383 && p->InSamePhase(c) && p->IsWithinDistInMap(c,INTERACTION_DISTANCE) &&
        p->GetQuestStatus(CommunionQuest(p))==QUEST_STATUS_INCOMPLETE && Shrines(p) && !p->GetQuestObjectiveData(CommunionQuest(p),94477);
}
Creature* Statue(Player* p)
{
    if(!p)return nullptr;
    std::list<Creature*> actors;GetCreatureListWithEntryInGrid(actors,p,94393,15);
    for(auto c:actors)if(Communion(p,c))return c;
    return nullptr;
}
void RepairPrematureCommunion(Player* p)
{
    // The old group/click action could save final credit before any rites.
    // That state is provably out of order. Preserve finished/rewarded history
    // and an otherwise valid four-shrine run; never reset shrine progress.
    if(!p || p->GetQuestStatus(CommunionQuest(p))!=QUEST_STATUS_INCOMPLETE || !p->GetQuestObjectiveData(CommunionQuest(p),94477) || Shrines(p))return;
    auto q=sQuestDataStore->GetQuestTemplate(CommunionQuest(p));if(!q)return;
    for(auto const& o:q->GetObjectives())if(o.Type==QUEST_OBJECTIVE_MONSTER && o.ObjectID==94477 && o.StorageIndex==4)
    {
        p->SetQuestObjectiveData(q,&o,0);p->SendQuestUpdateAddCredit(q,ObjectGuid::Empty,o,0);
        TC_LOG_INFO("scripts","Skold-Ashil: cleared premature communion credit");
        break;
    }
}
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

struct npc_stormheim_ascendant_statue : SmartAI
{
    explicit npc_stormheim_ascendant_statue(Creature* c):SmartAI(c){}
    void sGossipSelect(Player* p,uint32 menu,uint32 option) override
    {
        if(menu!=18543 || option!=0){SmartAI::sGossipSelect(p,menu,option);return;}
        if(!StormheimSkoldAshil::Communion(p,me))return;
        p->PlayerTalkClass->SendCloseGossip();
        p->CastSpell(p,186982,false); // Native Communing: two seconds, not an instant trigger.
    }
};

class spell_stormheim_communing : public SpellScript
{
    PrepareSpellScript(spell_stormheim_communing);
    ObjectGuid statue;
    uint32 quest=0;
    std::weak_ptr<uint8> lifetime;
    bool captured=false,credited=false;
    bool Load() override {return GetCaster()->IsPlayer() && GetCaster()->GetMapId()==1220 && GetCaster()->GetZoneId()==7541;}
    bool Valid()
    {
        auto p=GetCaster()->ToPlayer();
        return captured && !lifetime.expired() && p && p->IsInWorld() && p->GetMapId()==1220 && !p->GetInstanceId() &&
            quest==StormheimSkoldAshil::CommunionQuest(p) && StormheimSkoldAshil::Communion(p,ObjectAccessor::GetCreature(*p,statue)) &&
            lifetime.lock()==p->getQuestStatus(quest)->ScriptLifetime;
    }
    SpellCastResult Check()
    {
        auto p=GetCaster()->ToPlayer();
        if(!captured)
        {
            auto actor=StormheimSkoldAshil::Statue(p);if(!actor)return SPELL_FAILED_BAD_TARGETS;
            quest=StormheimSkoldAshil::CommunionQuest(p);statue=actor->GetGUID();p->AddQuestDelayedEvent(quest,0,[]{});
            lifetime=p->getQuestStatus(quest)->ScriptLifetime;captured=true;
        }
        return Valid()?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Credit(SpellEffIndex index)
    {
        // Native effect134 shares group credit. Communion is a personal final
        // interaction, so retain its cast/visual but award the valid caster only.
        PreventHitDefaultEffect(index);
        auto p=GetCaster()->ToPlayer();if(credited || GetHitPlayer()!=p || !Valid())return;
        auto q=sQuestDataStore->GetQuestTemplate(quest);if(!q)return;
        for(auto const& o:q->GetObjectives())if(o.Type==QUEST_OBJECTIVE_MONSTER && o.ObjectID==94477 && o.StorageIndex==4 && o.Amount==1)
        {
            credited=true;p->SetQuestObjectiveData(q,&o,1);p->SendQuestUpdateAddCredit(q,statue,o,1);
            if(p->CanCompleteQuest(quest))p->CompleteQuest(quest);
            break; // Do not credit a stale opposite-faction quest with the same bunny.
        }
    }
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_stormheim_communing::Check);
        OnEffectHitTarget+=SpellEffectFn(spell_stormheim_communing::Credit,EFFECT_0,SPELL_EFFECT_KILL_CREDIT2);
    }
};

class player_stormheim_communion_recovery : public PlayerScript
{
public:
    player_stormheim_communion_recovery():PlayerScript("player_stormheim_communion_recovery"){}
    void OnLogin(Player* p) override {StormheimSkoldAshil::RepairPrematureCommunion(p);}
};

void AddSC_stormheim_skold_ashil()
{
    TC_LOG_INFO("server.loading","Skold-Ashil: distinct shield readings");
    new go_stormheim_engraved_shield();
    TC_LOG_INFO("server.loading","Skold-Ashil: ordered native communion");
    TC_LOG_INFO("server.loading","Skold-Ashil: faction-specific communion quest lifetime");
    RegisterCreatureAI(npc_stormheim_ascendant_statue);
    RegisterSpellScript(spell_stormheim_communing);
    new player_stormheim_communion_recovery();
}

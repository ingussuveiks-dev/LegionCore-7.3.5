#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "GameObject.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "SpellScript.h"
#include "Spell.h"
#include "Item.h"
#include <memory>

namespace StormheimGift
{
bool Ready(Player* p)
{
    return p && p->IsInWorld() && p->IsAlive() && p->GetTeam()==HORDE && p->GetMapId()==1220 &&
        !p->GetInstanceId() && p->GetZoneId()==7541 && p->GetAreaId()==7773 &&
        p->GetQuestStatus(39385)==QUEST_STATUS_INCOMPLETE && p->GetQuestObjectiveData(39385,243455)>=3 &&
        !p->GetQuestObjectiveData(39385,95412);
}
void Refresh(Player* p)
{
    // Reuse native spell_area189152 and its objective condition/CreateItem.
    if(Ready(p) && !p->GetItemCount(128287,true))p->SetQuestUpdate(39385);
}
void Queue(Player* p,uint32 delay)
{
    if(!p || p->GetQuestStatus(39385)!=QUEST_STATUS_INCOMPLETE)return;
    auto guid=p->GetGUID();
    p->AddQuestDelayedEvent(39385,delay,[guid]{Refresh(ObjectAccessor::FindPlayer(guid));});
}
bool Source(GameObject* go)
{
    auto id=go->GetDBTableGUIDLow();
    return go->GetEntry()==243455 && go->GetMapId()==1220 && !go->GetInstanceId() &&
        (id==127738 || id==127739 || id==127740);
}
}

struct go_stormheim_gift_planting : GameObjectAI
{
    explicit go_stormheim_gift_planting(GameObject* object):GameObjectAI(object){}
    bool GossipUse(Player* p) override
    {
        if(StormheimGift::Source(go) && p && p->IsInWorld() && p->IsAlive() && p->GetMap()==go->GetMap() &&
            p->InSamePhase(go) && go->isSpawned() && !go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE) &&
            p->IsWithinDistInMap(go,go->GetInteractionDistance()))
            StormheimGift::Queue(p,1); // Native goober credit follows this callback.
        return false;
    }
};

class spell_stormheim_gift_detonator : public SpellScript
{
    PrepareSpellScript(spell_stormheim_gift_detonator);
    std::weak_ptr<uint8> lifetime;
    bool captured=false;
    bool Load() override {return GetCaster()->IsPlayer();}
    SpellCastResult Check()
    {
        auto p=GetCaster()->ToPlayer();
        if(!StormheimGift::Ready(p) || !GetCastItem() || GetCastItem()->GetEntry()!=128287)return SPELL_FAILED_BAD_TARGETS;
        if(!captured)
        {
            p->AddQuestDelayedEvent(39385,0,[]{});lifetime=p->getQuestStatus(39385)->ScriptLifetime;captured=true;
        }
        if(lifetime.expired() || lifetime.lock()!=p->getQuestStatus(39385)->ScriptLifetime)return SPELL_FAILED_BAD_TARGETS;
        // Native target46 uses an explicit Unit referer before grid search.
        // Item use need not supply one, and a selected unit can be unrelated.
        // Use the caster without inventing a fixed explosion site.
        GetSpell()->m_targets.SetUnitTarget(p);
        GetSpell()->m_targets.SetDst(*p); // Do not retain an unrelated item's destination during native CheckCast.
        return SPELL_CAST_OK;
    }
    void Register() override {OnCheckCast+=SpellCheckCastFn(spell_stormheim_gift_detonator::Check);}
};

class player_stormheim_gift_refresh : public PlayerScript
{
public:
    player_stormheim_gift_refresh():PlayerScript("player_stormheim_gift_refresh"){}
    void OnLogin(Player* p) override {StormheimGift::Queue(p,1000);}
};

void AddSC_stormheim_gift()
{
    TC_LOG_INFO("server.loading","Stormheim: Gift detonator refresh and native self destination");
    RegisterGameObjectAI(go_stormheim_gift_planting);
    RegisterSpellScript(spell_stormheim_gift_detonator);
    new player_stormheim_gift_refresh();
}

#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "EventObject.h"
#include "EventObjectData.h"
#include "Map.h"
#include <list>
#include <map>
#include <mutex>

namespace StormheimHelheimDisguise
{
constexpr uint32 QuestId=38347, NextQuest=39848, Disguise=181705, Reaction=181709;
Position const GuardPoint={455.686f,526.597f,5.13701f,4.87301f};
Position const ExitPoint={454.412f,565.988f,0.083255f,0};
struct Run {
    Map* map=nullptr;
    std::weak_ptr<uint8> lifetime;
    ObjectGuid spirit, gate;
    uint32 timer=0;
    bool summoning=false, approached=false;
};
std::map<ObjectGuid,Run> runs;
std::recursive_mutex mutex;
bool Scope(Player* p){return p&&p->IsInWorld()&&p->GetMapId()==1463&&!p->GetInstanceId()&&p->GetZoneId()==7588&&!p->HasPhaseId(8306);}
bool Definition()
{
    auto q=sQuestDataStore->GetQuestTemplate(QuestId);
    auto actor=sObjectMgr->GetCreatureTemplate(97419);
    auto gate=sEventObjectDataStore->GetEventObjectTemplate(334);
    if(!q||q->StartScript||q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)||!actor||!gate||
        sObjectMgr->GetScriptName(actor->ScriptID)!="npc_stormheim_helheim_ashildir"||
        sObjectMgr->GetScriptName(gate->ScriptID)!="eventobject_stormheim_helheim_passage"||gate->radius!=4||gate->SpellID||gate->WorldSafeLocID)return false;
    uint32 count=0;
    for(auto const& o:q->GetObjectives()){
        if(o.Type!=QUEST_OBJECTIVE_MONSTER||o.ObjectID!=91386||o.StorageIndex!=1||o.Amount!=1||o.Flags)return false;
        ++count;
    }
    return count==1;
}
uint32 Stage(Player* p)
{
    if(!Scope(p)||!Definition()||p->GetQuestStatus(NextQuest)!=QUEST_STATUS_NONE||p->GetQuestRewardStatus(NextQuest))return 0;
    auto status=p->GetQuestStatus(QuestId);
    if(status==QUEST_STATUS_INCOMPLETE)return 1;
    if(status==QUEST_STATUS_COMPLETE)return 2;
    return p->GetQuestRewardStatus(QuestId)?3:0;
}
bool Ready(Player* p){return Stage(p)&&p->IsAlive()&&!p->IsBeingTeleported()&&!p->GetVehicle()&&!p->GetTransport()&&!p->isInFlight();}
void RemoveSelfAuras(Player* p)
{
    // Retire the old controller by its own aura handler (192298 minions only).
    // Do not cast192299: its second effect triggers deleted spell81040.
    for(uint32 id:{192299u,Disguise,Reaction})p->RemoveAurasDueToSpell(id,p->GetGUID());
}
void Clear(Player* p)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end())return;
    auto old=it->second;runs.erase(it);RemoveSelfAuras(p);
    if(old.map)if(auto c=old.map->GetCreature(old.spirit))c->DespawnOrUnsummon();
}
bool Current(Player* p,Run const& r)
{
    if(!Ready(p)||r.map!=p->GetMap())return false;
    if(Stage(p)==3)return true; // Keep the actual giver until39848 is accepted.
    auto status=p->getQuestStatus(QuestId);
    return status&&!r.lifetime.expired()&&status->ScriptLifetime==r.lifetime.lock();
}
Creature* Guard(Player* p)
{
    std::list<Creature*> list;GetCreatureListWithEntryInGrid(list,p,91386,110);
    for(auto c:list)if(c->GetDBTableGUIDLow()==11546644&&c->GetMapId()==1463&&c->IsInWorld()&&c->IsAlive()&&
        c->GetExactDist(&GuardPoint)<1&&p->InSamePhase(c)&&p->canSeeOrDetect(c))return c;
    return nullptr;
}
bool Gate(EventObject* e)
{
    return e&&e->IsInWorld()&&e->GetEntry()==334&&e->GetDBTableGUIDLow()==400326&&e->GetMapId()==1463&&
        e->GetZoneId()==7588&&e->GetExactDist(&ExitPoint)<0.5f;
}
void Credit(Player* p,Run& r)
{
    auto gate=r.map->GetEventObject(r.gate);
    if(Stage(p)!=1||!Current(p,r)||!r.approached||p->isInCombat()||!p->GetAura(Disguise,p->GetGUID())||
        !p->GetAura(Reaction,p->GetGUID())||!Guard(p)||!Gate(gate)||!p->InSamePhase(gate)||p->GetExactDist(&ExitPoint)>4)return;
    auto q=sQuestDataStore->GetQuestTemplate(QuestId);
    for(auto const& o:q->GetObjectives())if(!p->GetQuestObjectiveData(QuestId,91386)){
        p->SetQuestObjectiveData(q,&o,1);p->SendQuestUpdateAddCredit(q,gate->GetGUID(),o,1);
        if(p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);
    }
}
void Update(Player* p,uint32 diff)
{
    auto it=runs.find(p->GetGUID());
    if(it!=runs.end()&&!Current(p,it->second)){Clear(p);it=runs.end();}
    if(!Ready(p))return;
    if(it==runs.end()){
        RemoveSelfAuras(p);Run r;r.map=p->GetMap();
        if(Stage(p)!=3){p->AddQuestDelayedEvent(QuestId,0,[]{});r.lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;}
        it=runs.emplace(p->GetGUID(),r).first;
    }
    auto& r=it->second;if(r.timer>diff){r.timer-=diff;return;}r.timer=500;
    if(!p->GetAura(Disguise,p->GetGUID())||!p->GetAura(Reaction,p->GetGUID())||p->isInCombat())r.approached=false;
    if(!p->isInCombat()){
        if(!p->GetAura(Disguise,p->GetGUID()))p->CastSpell(p,Disguise,true);
        if(p->GetAura(Disguise,p->GetGUID())&&!p->GetAura(Reaction,p->GetGUID()))p->CastSpell(p,Reaction,true);
    }
    auto spirit=r.map->GetCreature(r.spirit);
    if(spirit&&(!spirit->IsAlive()||!p->InSamePhase(spirit)||p->GetExactDist(spirit)>70)){
        spirit->DespawnOrUnsummon();r.spirit.Clear();spirit=nullptr;
    }
    if(!spirit){
        r.summoning=true;
        auto c=p->SummonCreature(97419,p->GetPositionX(),p->GetPositionY(),p->GetPositionZ(),p->GetOrientation(),TEMPSUMMON_MANUAL_DESPAWN,0,p->GetGUID());
        r.summoning=false;
        if(c&&r.spirit!=c->GetGUID())c->DespawnOrUnsummon();
    }else if(!spirit->HasAura(Disguise))spirit->CastSpell(spirit,Disguise,true);
    // Record the living native guard's approach side, never his proximity as
    // completion. A fresh incomplete login must approach again before exit334.
    auto guard=Guard(p);if(!guard)r.approached=false;
    if(guard&&Stage(p)==1&&!p->isInCombat()&&p->GetAura(Disguise,p->GetGUID())&&p->GetAura(Reaction,p->GetGUID())&&
        p->GetExactDist(guard)<=40&&p->GetPositionY()<=GuardPoint.GetPositionY()&&p->IsWithinLOSInMap(guard))r.approached=true;
    Credit(p,r);
}
}

struct npc_stormheim_helheim_ashildir : SmartAI
{
    explicit npc_stormheim_helheim_ashildir(Creature* c):SmartAI(c){}
    ObjectGuid owner;bool owned=false;
    void IsSummonedBy(Unit* summoner)override
    {
        using namespace StormheimHelheimDisguise;std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p=summoner?summoner->ToPlayer():nullptr;auto it=p?runs.find(p->GetGUID()):runs.end();
        if(it==runs.end()||!Current(p,it->second)||!it->second.summoning||me->GetEntry()!=97419){SmartAI::IsSummonedBy(summoner);return;}
        owner=p->GetGUID();owned=true;it->second.spirit=me->GetGUID();
        me->AddPlayerInPersonnalVisibilityList(owner);me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);me->setActive(true);
        me->GetMotionMaster()->MoveFollow(p,1.5f,3.14159f);me->CastSpell(me,Disguise,true);
    }
    void UpdateAI(uint32 diff)override
    {
        using namespace StormheimHelheimDisguise;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(owned){auto p=ObjectAccessor::GetPlayer(*me,owner);auto it=runs.find(owner);
            if(!p||it==runs.end()||!Current(p,it->second)||it->second.spirit!=me->GetGUID()){me->DespawnOrUnsummon();return;}}
        SmartAI::UpdateAI(diff);
    }
};
class eventobject_stormheim_helheim_passage : public EventObjectScript
{
public:
    eventobject_stormheim_helheim_passage():EventObjectScript("eventobject_stormheim_helheim_passage"){}
    bool IsTriggerMeets(Player* p,EventObject* e)override
    {return StormheimHelheimDisguise::Gate(e)&&StormheimHelheimDisguise::Ready(p)&&StormheimHelheimDisguise::Stage(p)==1;}
    bool OnTrigger(Player* p,EventObject* e,bool enter)override
    {
        using namespace StormheimHelheimDisguise;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!enter||!Gate(e)||!Ready(p)||Stage(p)!=1)return false;
        Update(p,0);auto it=runs.find(p->GetGUID());if(it==runs.end())return false;
        it->second.gate=e->GetGUID();Credit(p,it->second);return true;
    }
};
class player_stormheim_helheim_disguise : public PlayerScript
{
public:
    player_stormheim_helheim_disguise():PlayerScript("player_stormheim_helheim_disguise"){}
    void OnUpdate(Player* p,uint32 diff)override{std::lock_guard<std::recursive_mutex> lock(StormheimHelheimDisguise::mutex);StormheimHelheimDisguise::Update(p,diff);}
    void OnLogout(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimHelheimDisguise::mutex);StormheimHelheimDisguise::Clear(p);}
    void OnMapChanged(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimHelheimDisguise::mutex);StormheimHelheimDisguise::Clear(p);}
    void OnLogin(Player* p)override
    {
        using namespace StormheimHelheimDisguise;std::lock_guard<std::recursive_mutex> lock(mutex);
        Clear(p);
        // Compatibility with a saved pre-fix controller/disguise. Foreign
        // casters are untouched; the quest run rebuilds only in Helheim.
        if(p->HasPhaseId(8306))return;
        auto s=p->GetQuestStatus(QuestId);
        if(Scope(p)||s==QUEST_STATUS_INCOMPLETE||s==QUEST_STATUS_COMPLETE||p->GetAura(192299,p->GetGUID()))RemoveSelfAuras(p);
    }
};
void AddSC_stormheim_helheim_disguise()
{
    RegisterCreatureAI(npc_stormheim_helheim_ashildir);new eventobject_stormheim_helheim_passage();new player_stormheim_helheim_disguise();
}

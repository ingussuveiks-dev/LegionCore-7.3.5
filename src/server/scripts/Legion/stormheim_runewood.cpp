#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "SpellMgr.h"
#include "Map.h"
#include <array>
#include <map>
#include <mutex>

namespace StormheimRunewood
{
constexpr uint32 QuestId = 38778;
struct Site { uint32 entry, marker, controller, summon; Position point; };
Site const Sites[] = {
    {93182,93185,192155,184582,{3116.57f,1247.91f,125.037f,0}},
    {93342,93340,192156,184792,{2992.34f,1126.41f,130.259f,0}},
    {93343,93341,192157,184794,{3181.52f,1054.34f,117.037f,0}}
};
struct Attempt {
    Map* scene = nullptr;
    std::weak_ptr<uint8> lifetime;
    std::array<ObjectGuid,3> actors;
    std::array<uint32,3> retry{};
    int pending = -1;
    uint32 elapsed = 0, completed = 0;
};
std::map<ObjectGuid,Attempt> attempts;
std::recursive_mutex mutex;
bool Scope(Player* p) { return p && p->IsInWorld() && p->GetMapId()==1220 && !p->GetInstanceId() && p->GetZoneId()==7541; }
bool Definition()
{
    auto quest=sQuestDataStore->GetQuestTemplate(QuestId);if(!quest||quest->StartScript)return false;
    bool total=false;uint32 markers=0;
    for(auto const& o:quest->GetObjectives()) {
        if(o.Type!=QUEST_OBJECTIVE_MONSTER) return false;
        if(o.ObjectID==93196) total=o.StorageIndex==0&&o.Amount==3&&!(o.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL);
        else for(size_t i=0;i<3;++i) if(o.ObjectID==Sites[i].marker&&o.StorageIndex==int8(i+1)&&o.Amount==1&&(o.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL)) markers|=1u<<i;
    }
    if(!total||markers!=7)return false;
    for(auto const& site:Sites) {
        auto info=sObjectMgr->GetCreatureTemplate(site.entry);auto pos=sSpellMgr->GetSpellTargetPosition(site.summon);
        if(!info||sObjectMgr->GetScriptName(info->ScriptID)!="npc_stormheim_runewood_stone"||!pos||pos->target_mapId!=1220||
            std::fabs(pos->target_X-site.point.GetPositionX())>0.01f||std::fabs(pos->target_Y-site.point.GetPositionY())>0.01f||std::fabs(pos->target_Z-site.point.GetPositionZ())>0.01f)return false;
    }
    return true;
}
bool Eligible(Player* p) { return Scope(p)&&p->IsAlive()&&!p->IsBeingTeleported()&&p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE&&Definition(); }
bool Current(Player* p,Attempt const& r)
{
    auto status=p->getQuestStatus(QuestId);auto state=p->GetQuestStatus(QuestId);
    return Scope(p)&&status&&!r.lifetime.expired()&&status->ScriptLifetime==r.lifetime.lock()&&
        (state==QUEST_STATUS_INCOMPLETE||state==QUEST_STATUS_COMPLETE);
}
void Set(Player* p,uint32 entry,uint32 count)
{
    auto quest=sQuestDataStore->GetQuestTemplate(QuestId);
    for(auto const& o:quest->GetObjectives()) if(o.ObjectID==entry) {
        if(p->GetQuestObjectiveData(QuestId,entry)==int32(count))return;
        p->SetQuestObjectiveData(quest,&o,count);
        p->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,o,count);return;
    }
}
uint32 Unique(Player* p) { uint32 n=0;for(auto const& site:Sites)n+=p->GetQuestObjectiveData(QuestId,site.marker)>0;return n; }
void Normalize(Player* p)
{
    if(!Scope(p)||!Definition())return;auto status=p->GetQuestStatus(QuestId);
    if(status!=QUEST_STATUS_INCOMPLETE&&status!=QUEST_STATUS_COMPLETE)return;
    uint32 unique=Unique(p);Set(p,93196,unique);
    if(status==QUEST_STATUS_COMPLETE&&unique<3)p->SetQuestStatus(QuestId,QUEST_STATUS_INCOMPLETE);
    else if(status==QUEST_STATUS_INCOMPLETE&&unique==3&&p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);
}
void Clear(Player* p)
{
    auto it=attempts.find(p->GetGUID());if(it==attempts.end())return;
    Attempt old=it->second;attempts.erase(it);
    for(size_t i=0;i<3;++i) {
        if(old.scene)if(auto c=old.scene->GetCreature(old.actors[i]))c->DespawnOrUnsummon();
        // Native controller cleanup filters m_Controlled; ordinary wild personal
        // summons need the explicit original-map GUID cleanup above as well.
        p->RemoveAurasDueToSpell(Sites[i].controller);
    }
}
int Index(Creature* c) { if(c)for(int i=0;i<3;++i)if(c->GetEntry()==Sites[i].entry)return i;return -1; }
bool Interact(Player* p,Creature* c,int index)
{
    if(index<0||!Eligible(p)||!c||!c->IsInWorld()||!c->IsAlive()||c->IsQuestObjectExpired()||
        p->GetVehicle()||p->GetTransport()||p->isInCombat()||p->isInFlight()||
        !c->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK)||p->GetExactDist(c)>6||
        !p->InSamePhase(c)||!p->canSeeOrDetect(c)||!p->IsWithinLOSInMap(c)||c->GetExactDist(&Sites[index].point)>1||
        p->GetQuestObjectiveData(QuestId,Sites[index].marker))return false;
    auto it=attempts.find(p->GetGUID());return it!=attempts.end()&&Current(p,it->second)&&it->second.actors[index]==c->GetGUID();
}
void Update(Player* p,uint32 diff)
{
    auto it=attempts.find(p->GetGUID());
    if(it!=attempts.end()) {
        if(!Current(p,it->second)||!p->IsAlive()||p->IsBeingTeleported()){Clear(p);return;}
        if(p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE){it->second.completed+=diff;if(it->second.completed>=15000)Clear(p);return;}
        it->second.elapsed+=diff;if(it->second.elapsed<500)return;it->second.elapsed=0;
    }else {
        if(!Eligible(p)||p->GetVehicle()||p->GetTransport())return;
        bool nearby=false;for(auto const& site:Sites)nearby|=p->GetExactDist(&site.point)<80;if(!nearby)return;
        Normalize(p);if(!Eligible(p))return;
        p->AddQuestDelayedEvent(QuestId,0,[]{});Attempt attempt;attempt.scene=p->GetMap();attempt.lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;
        it=attempts.emplace(p->GetGUID(),attempt).first;
    }
    auto& r=it->second;
    for(size_t i=0;i<3;++i) {
        auto const& site=Sites[i];auto c=r.scene->GetCreature(r.actors[i]);
        if(p->GetExactDist(&site.point)>=100) {if(c)c->DespawnOrUnsummon();r.actors[i].Clear();p->RemoveAurasDueToSpell(site.controller);continue;}
        if(c&&c->IsAlive()&&!c->IsQuestObjectExpired()&&p->InSamePhase(c)&&p->canSeeOrDetect(c))continue;
        if(c)c->DespawnOrUnsummon();
        r.actors[i].Clear();
        if(p->GetQuestObjectiveData(QuestId,site.marker)||p->GetExactDist(&site.point)>=80)continue;
        if(r.retry[i]>500){r.retry[i]-=500;continue;}r.retry[i]=2000;
        p->RemoveAurasDueToSpell(site.controller);r.pending=int(i);
        p->CastSpell(p,site.controller,true);r.pending=-1;
    }
}
}

struct npc_stormheim_runewood_stone : SmartAI
{
    explicit npc_stormheim_runewood_stone(Creature* c):SmartAI(c){}
    bool owned=false;
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimRunewood;std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p=summoner?summoner->ToPlayer():nullptr;int i=Index(me);
        auto it=p?attempts.find(p->GetGUID()):attempts.end();
        if(i>=0&&Eligible(p)&&it!=attempts.end()&&Current(p,it->second)&&it->second.pending==i&&me->GetExactDist(&Sites[i].point)<1) {
            owned=true;it->second.actors[i]=me->GetGUID();p->BindQuestObject(QuestId,me);
            me->AddPlayerInPersonnalVisibilityList(p->GetGUID());me->SetReactState(REACT_PASSIVE);me->setActive(true);
        }else SmartAI::IsSummonedBy(summoner);
    }
    void SpellHit(Unit* caster,SpellInfo const* spell) override
    {
        if(owned&&spell&&spell->Id==184579)return; // No repeatable native SAI credit on an owned attempt.
        SmartAI::SpellHit(caster,spell);
    }
};

class spell_stormheim_runewood_summon : public SpellScript
{
    PrepareSpellScript(spell_stormheim_runewood_summon);
    SpellCastResult Check()
    {
        using namespace StormheimRunewood;std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p=GetCaster()->ToPlayer();if(p&&!p->IsInWorld())return SPELL_FAILED_BAD_TARGETS;if(!Scope(p))return SPELL_CAST_OK;
        auto it=attempts.find(p->GetGUID());if(!Eligible(p)||it==attempts.end()||!Current(p,it->second))return SPELL_FAILED_BAD_TARGETS;
        int i=it->second.pending;return i>=0&&Sites[i].summon==m_scriptSpellId&&p->GetExactDist(&Sites[i].point)<80&&!p->GetQuestObjectiveData(QuestId,Sites[i].marker)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Register() override {OnCheckCast+=SpellCheckCastFn(spell_stormheim_runewood_summon::Check);}
};
class spell_stormheim_runewood_turn : public SpellScript
{
    PrepareSpellScript(spell_stormheim_runewood_turn);
    ObjectGuid actor;std::weak_ptr<uint8> lifetime;int index=-1;bool scoped=false,captured=false,hit=false,consumed=false;
    bool Valid() {
        using namespace StormheimRunewood;auto p=GetCaster()->ToPlayer();auto status=p?p->getQuestStatus(QuestId):nullptr;
        return captured&&status&&!lifetime.expired()&&status->ScriptLifetime==lifetime.lock()&&Interact(p,ObjectAccessor::GetCreature(*p,actor),index);
    }
    SpellCastResult Check() {
        using namespace StormheimRunewood;std::lock_guard<std::recursive_mutex> lock(mutex);auto p=GetCaster()->ToPlayer();
        if(!captured){scoped=Scope(p);if(!scoped)return SPELL_CAST_OK;auto c=GetOriginalTarget()?GetOriginalTarget()->ToCreature():nullptr;index=Index(c);
            if(!Interact(p,c,index))return SPELL_FAILED_BAD_TARGETS;actor=c->GetGUID();lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;captured=true;}
        return Valid()?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex) {std::lock_guard<std::recursive_mutex> lock(StormheimRunewood::mutex);if(scoped&&Valid()&&GetHitCreature()&&GetHitCreature()->GetGUID()==actor)hit=true;}
    void Complete() {
        using namespace StormheimRunewood;std::lock_guard<std::recursive_mutex> lock(mutex);if(consumed||!hit||!Valid())return;consumed=true;
        auto p=GetCaster()->ToPlayer();auto c=ObjectAccessor::GetCreature(*p,actor);
        c->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);c->SetFacingTo(c->GetOrientation()+1.570796327f);
        Set(p,Sites[index].marker,1);Set(p,93196,Unique(p));if(p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);
    }
    void Register() override {OnCheckCast+=SpellCheckCastFn(spell_stormheim_runewood_turn::Check);OnEffectHitTarget+=SpellEffectFn(spell_stormheim_runewood_turn::Hit,EFFECT_0,SPELL_EFFECT_DUMMY);AfterCast+=SpellCastFn(spell_stormheim_runewood_turn::Complete);}
};
class player_stormheim_runewood : public PlayerScript
{
public:
    player_stormheim_runewood():PlayerScript("player_stormheim_runewood"){}
    void OnUpdate(Player* p,uint32 diff) override {std::lock_guard<std::recursive_mutex> lock(StormheimRunewood::mutex);StormheimRunewood::Update(p,diff);}
    void OnLogout(Player* p) override {std::lock_guard<std::recursive_mutex> lock(StormheimRunewood::mutex);StormheimRunewood::Clear(p);}
    void OnMapChanged(Player* p) override {std::lock_guard<std::recursive_mutex> lock(StormheimRunewood::mutex);StormheimRunewood::Clear(p);}
    void OnLogin(Player* p) override {std::lock_guard<std::recursive_mutex> lock(StormheimRunewood::mutex);if(StormheimRunewood::Definition()){for(auto const& site:StormheimRunewood::Sites)p->RemoveAurasDueToSpell(site.controller);StormheimRunewood::Normalize(p);}}
};
void AddSC_stormheim_runewood()
{
    RegisterCreatureAI(npc_stormheim_runewood_stone);RegisterSpellScript(spell_stormheim_runewood_summon);RegisterSpellScript(spell_stormheim_runewood_turn);new player_stormheim_runewood();
}

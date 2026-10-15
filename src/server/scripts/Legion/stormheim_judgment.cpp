#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "Map.h"
#include <map>
#include <mutex>

namespace StormheimJudgment
{
constexpr uint32 QuestId = 38811;
struct Site { uint32 bowl; ObjectGuid::LowType spawn; Position bowlPoint; uint32 king; Position kingPoint; };
Site const Sites[] = {
    {241869,115152,{3513.71f,1113.86f,153.034f,0},92889,{3513.62f,1102.43f,157.74f,1.51f}},
    {241868,115115,{3375.76f,1272.61f,126.355f,0},92918,{3377.39f,1279.22f,128.48f,4.5f}},
    {241864,115119,{3406.63f,1153.15f,136.178f,0},92920,{3417.11f,1162.67f,139.0f,3.86f}}
};
struct Run {
    Map* map=nullptr;
    std::weak_ptr<uint8> lifetime;
    ObjectGuid bowl, king, incense;
    int site=-1;
    uint32 stage=0, timer=7500, age=0;
    bool summoning=false;
};
std::map<ObjectGuid,Run> runs;
std::recursive_mutex mutex;
bool Scope(Player* p) { return p&&p->IsInWorld()&&p->GetMapId()==1220&&!p->GetInstanceId()&&p->GetZoneId()==7541; }
int Identity(GameObject* go)
{
    if(go&&go->IsInWorld()&&go->GetMapId()==1220&&!go->GetInstanceId()&&go->GetZoneId()==7541)
        for(int i=0;i<3;++i)if(go->GetEntry()==Sites[i].bowl&&go->GetDBTableGUIDLow()==Sites[i].spawn)return i;
    return -1;
}
int Index(GameObject* go) { int i=Identity(go);return i>=0&&go->GetExactDist(&Sites[i].bowlPoint)<1?i:-1; }
bool Definition()
{
    auto q=sQuestDataStore->GetQuestTemplate(QuestId);if(!q||q->StartScript)return false;
    uint32 mask=0;
    for(auto const& o:q->GetObjectives()) {
        bool known=false;for(int i=0;i<3;++i)if(o.ObjectID==Sites[i].king&&o.Type==QUEST_OBJECTIVE_MONSTER&&o.StorageIndex==i&&o.Amount==1&&!o.Flags){mask|=1u<<i;known=true;}
        if(!known)return false;
    }
    if(mask!=7)return false;
    for(auto const& site:Sites) {
        auto go=sObjectMgr->GetGameObjectTemplate(site.bowl);auto king=sObjectMgr->GetCreatureTemplate(site.king);
        if(!go||!king||go->type!=GAMEOBJECT_TYPE_GOOBER||go->goober.open!=1691||go->goober.questID!=QuestId||
            sObjectMgr->GetScriptName(go->ScriptId)!="go_stormheim_judgment"||sObjectMgr->GetScriptName(king->ScriptID)!="npc_stormheim_judgment_king")return false;
    }
    return true;
}
bool Active(Player* p) { return Scope(p)&&p->IsAlive()&&!p->IsBeingTeleported()&&p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE&&Definition(); }
bool Source(Player* p,GameObject* go,int site,bool opening=true)
{
    return site>=0&&Index(go)==site&&go->isSpawned()&&!go->IsQuestObjectExpired()&&
        !go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE)&&
        p->InSamePhase(go)&&p->canSeeOrDetect(go)&&!go->ShouldHideFor(p->GetGUID())&&(!opening||p->IsWithinLOSInMap(go));
}
bool Current(Player* p,Run const& r)
{
    auto status=p?p->getQuestStatus(QuestId):nullptr;
    return Active(p)&&status&&!r.lifetime.expired()&&status->ScriptLifetime==r.lifetime.lock()&&
        !p->GetVehicle()&&!p->GetTransport()&&p->GetExactDist(&Sites[r.site].bowlPoint)<80&&
        !p->GetQuestObjectiveData(QuestId,Sites[r.site].king)&&Source(p,r.map->GetGameObject(r.bowl),r.site,false);
}
void Clear(Player* p,uint32 corpseDelay=0)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end())return;auto old=it->second;runs.erase(it);
    if(old.map) {
        if(auto c=old.map->GetCreature(old.incense))c->DespawnOrUnsummon();
        if(auto c=old.map->GetCreature(old.king))c->DespawnOrUnsummon(corpseDelay);
    }
}
void Begin(Player* p,GameObject* go)
{
    int i=Index(go);
    if(i<0||!Active(p)||p->GetVehicle()||p->GetTransport()||p->isInCombat()||!Source(p,go,i)||
        !p->IsWithinDistInMap(go,go->GetInteractionDistance())||p->GetQuestObjectiveData(QuestId,Sites[i].king))return;
    auto it=runs.find(p->GetGUID());if(it!=runs.end()){if(Current(p,it->second))return;Clear(p);}
    p->AddQuestDelayedEvent(QuestId,0,[]{});Run r;r.map=p->GetMap();r.lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;r.bowl=go->GetGUID();r.site=i;
    auto inserted=runs.emplace(p->GetGUID(),r).first;
    // A private visual helper cannot run the shared35845 SmartAI update spells.
    // Failure of this optional incense effect must not consume progression.
    if(auto helper=p->SummonCreature(35845,go->GetPositionX(),go->GetPositionY(),go->GetPositionZ(),0,TEMPSUMMON_MANUAL_DESPAWN,0,p->GetGUID())) {
        helper->IsAIEnabled=false;helper->SetReactState(REACT_PASSIVE);helper->setActive(true);
        helper->AddPlayerInPersonnalVisibilityList(p->GetGUID());p->BindQuestObject(QuestId,helper);inserted->second.incense=helper->GetGUID();
        helper->CastSpell(helper,184831,true);
    }
}
void Credit(Player* p,Run const& r,ObjectGuid guid)
{
    auto q=sQuestDataStore->GetQuestTemplate(QuestId);
    for(auto const& o:q->GetObjectives())if(o.ObjectID==Sites[r.site].king) {
        p->SetQuestObjectiveData(q,&o,1);p->SendQuestUpdateAddCredit(q,guid,o,1);
        if(p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);return;
    }
}
void Update(Player* p,uint32 diff)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end())return;auto& r=it->second;
    if(!Current(p,r)){Clear(p);return;}r.age+=diff;if(r.age>=180000){Clear(p);return;}
    auto king=r.map->GetCreature(r.king);
    if(r.stage&&(!king||!king->IsAlive()||king->IsQuestObjectExpired()||!p->InSamePhase(king))){Clear(p);return;}
    if(r.stage==3)return;
    if(r.timer>diff){r.timer-=diff;return;}
    // Preserve one dialogue transition per tick, even after a long update.
    if(r.stage==0) {
        auto const& site=Sites[r.site];r.summoning=true;
        king=p->SummonCreature(site.king,site.kingPoint,TEMPSUMMON_MANUAL_DESPAWN,0,0,p->GetGUID());r.summoning=false;
        if(!king||r.king!=king->GetGUID()){if(king)king->DespawnOrUnsummon();Clear(p);return;}
        r.stage=1;r.timer=r.site==2?4000:6000;king->AI()->Talk(0,p->GetGUID());
    }else if(r.stage==1&&r.site!=2) {
        king->AI()->Talk(1,p->GetGUID());r.stage=2;r.timer=5000;
    }else {
        r.stage=3;king->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
        king->SetReactState(REACT_AGGRESSIVE);king->AI()->AttackStart(p);
    }
}
}

struct npc_stormheim_judgment_king : SmartAI
{
    explicit npc_stormheim_judgment_king(Creature* c):SmartAI(c){}
    ObjectGuid owner;bool owned=false;
    Player* Owner()const{return ObjectAccessor::GetPlayer(*me,owner);}
    bool Valid(bool fighting=false)const
    {
        using namespace StormheimJudgment;auto p=Owner();auto it=p?runs.find(owner):runs.end();
        return owned&&it!=runs.end()&&Current(p,it->second)&&it->second.king==me->GetGUID()&&!me->IsQuestObjectExpired()&&
            p->InSamePhase(me)&&p->canSeeOrDetect(me)&&(!fighting||it->second.stage==3);
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);auto p=summoner?summoner->ToPlayer():nullptr;
        auto it=p?runs.find(p->GetGUID()):runs.end();
        if(it==runs.end()||!Current(p,it->second)||!it->second.summoning||me->GetEntry()!=Sites[it->second.site].king||
            me->GetExactDist(&Sites[it->second.site].kingPoint)>1){SmartAI::IsSummonedBy(summoner);return;}
        owner=p->GetGUID();owned=true;it->second.king=me->GetGUID();p->BindQuestObject(QuestId,me);
        me->AddPlayerInPersonnalVisibilityList(owner);me->setActive(true);me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
    }
    bool CanAIAttack(Unit const* target)const override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);
        return owned?Valid(true)&&target&&target->GetCharmerOrOwnerPlayerOrPlayerItself()==Owner():SmartAI::CanAIAttack(target);
    }
    void DamageTaken(Unit* attacker,uint32& damage,DamageEffectType type)override
    {
        if(!owned){SmartAI::DamageTaken(attacker,damage,type);return;}
        if(!CanAIAttack(attacker))damage=0;
    }
    void JustDied(Unit* killer)override
    {
        using namespace StormheimJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!owned){SmartAI::JustDied(killer);return;}
        auto p=Owner();if(Valid(true)&&killer&&killer->GetCharmerOrOwnerPlayerOrPlayerItself()==p)Credit(p,runs.find(owner)->second,me->GetGUID());
        if(p)Clear(p,3000);
    }
    void EnterEvadeMode()override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);
        if(!owned){SmartAI::EnterEvadeMode();return;}if(auto p=Owner())StormheimJudgment::Clear(p);else me->DespawnOrUnsummon();
    }
    void UpdateAI(uint32 diff)override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);
        if(!owned){SmartAI::UpdateAI(diff);return;}
        if(!Valid()){if(auto p=Owner())StormheimJudgment::Clear(p);else me->DespawnOrUnsummon();return;}
        if(Valid(true)&&UpdateVictim())DoMeleeAttackIfReady();
    }
};
struct go_stormheim_judgment_ai : SmartGameObjectAI
{
    explicit go_stormheim_judgment_ai(GameObject* go):SmartGameObjectAI(go){}
    bool GossipHello(Player* p,bool opened)override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);
        if(StormheimJudgment::Identity(go)<0)return SmartGameObjectAI::GossipHello(p,opened);
        if(opened)StormheimJudgment::Begin(p,go);return true;
    }
    bool GossipUse(Player* p)override
    {
        return StormheimJudgment::Identity(go)>=0?true:SmartGameObjectAI::GossipUse(p);
    }
};
class go_stormheim_judgment : public GameObjectScript
{
public:
    go_stormheim_judgment():GameObjectScript("go_stormheim_judgment"){}
    bool OnGossipHello(Player*,GameObject* go)override{return StormheimJudgment::Identity(go)>=0;}
    GameObjectAI* GetAI(GameObject* go)const override{return new go_stormheim_judgment_ai(go);}
};
// The ordinary client Opening3365 can outlive abandonment/reacceptance.
// Retain the original bowl and acceptance before its cast starts, then check
// them again before the native lock effect is allowed to dispatch.
class spell_stormheim_judgment_opening : public SpellScript
{
    PrepareSpellScript(spell_stormheim_judgment_opening);
    ObjectGuid bowl;std::weak_ptr<uint8> lifetime;bool scoped=false,captured=false;
    bool Valid()
    {
        using namespace StormheimJudgment;auto p=GetCaster()->ToPlayer();auto status=p?p->getQuestStatus(QuestId):nullptr;
        auto go=p?ObjectAccessor::GetGameObject(*p,bowl):nullptr;int i=Index(go);
        return captured&&i>=0&&Active(p)&&status&&!lifetime.expired()&&status->ScriptLifetime==lifetime.lock()&&
            !p->GetVehicle()&&!p->GetTransport()&&!p->isInCombat()&&Source(p,go,i)&&
            p->IsWithinDistInMap(go,go->GetInteractionDistance())&&!p->GetQuestObjectiveData(QuestId,Sites[i].king);
    }
    SpellCastResult Check()
    {
        using namespace StormheimJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!captured){auto go=GetExplTargetGObj();scoped=Identity(go)>=0;if(!scoped)return SPELL_CAST_OK;
            auto p=GetCaster()->ToPlayer();if(!Active(p))return SPELL_FAILED_BAD_TARGETS;
            p->AddQuestDelayedEvent(QuestId,0,[]{});bowl=go->GetGUID();lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;captured=true;}
        return Valid()?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex)
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);
        if(scoped&&(!Valid()||!GetHitGObj()||GetHitGObj()->GetGUID()!=bowl))PreventHitDefaultEffect(EFFECT_0);
    }
    void Register()override
    {
        OnCheckCast+=SpellCheckCastFn(spell_stormheim_judgment_opening::Check);
        OnEffectHitTarget+=SpellEffectFn(spell_stormheim_judgment_opening::Hit,EFFECT_0,SPELL_EFFECT_OPEN_LOCK);
    }
};
class player_stormheim_judgment : public PlayerScript
{
public:
    player_stormheim_judgment():PlayerScript("player_stormheim_judgment"){}
    void OnUpdate(Player* p,uint32 diff)override{std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);StormheimJudgment::Update(p,diff);}
    void OnLogout(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);StormheimJudgment::Clear(p);}
    void OnMapChanged(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimJudgment::mutex);StormheimJudgment::Clear(p);}
};
void AddSC_stormheim_judgment()
{
    RegisterCreatureAI(npc_stormheim_judgment_king);RegisterSpellScript(spell_stormheim_judgment_opening);new go_stormheim_judgment();new player_stormheim_judgment();
}

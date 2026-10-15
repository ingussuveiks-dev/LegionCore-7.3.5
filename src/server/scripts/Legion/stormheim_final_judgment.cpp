#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "Map.h"
#include <algorithm>
#include <map>
#include <mutex>

namespace StormheimFinalJudgment
{
constexpr uint32 QuestId = 38818;
Position const FaljarPoint = {3626.52f,791.312f,38.7806f,5.36266f};
Position const AshildirPoint = {3639.2f,777.75f,40.2274f,2.50259f};
struct Run
{
    Map* map = nullptr;
    std::weak_ptr<uint8> lifetime;
    ObjectGuid boss, spirit, originalBoss, originalSpirit;
    uint32 stage = 0, timer = 0, age = 0;
    bool summoning = false;
};
std::map<ObjectGuid,Run> runs;
std::recursive_mutex mutex;
bool Definition()
{
    auto q=sQuestDataStore->GetQuestTemplate(QuestId);
    if(!q||q->StartScript||q->GetObjectives().size()!=1)return false;
    auto const& o=q->GetObjectives().front();
    auto t=sObjectMgr->GetCreatureTemplate(93093);
    return o.Type==QUEST_OBJECTIVE_MONSTER&&o.ObjectID==93442&&o.Amount==1&&o.StorageIndex==0&&!o.Flags&&
        t&&sObjectMgr->GetScriptName(t->ScriptID)=="npc_stormheim_final_judgment";
}
bool Active(Player* p)
{
    return p&&p->IsInWorld()&&p->IsAlive()&&p->GetMapId()==1220&&!p->GetInstanceId()&&p->GetZoneId()==7541&&
        !p->GetVehicle()&&!p->GetTransport()&&
        (p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE||p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE)&&Definition();
}
bool Current(Player* p,Run const& r)
{
    auto status=p?p->getQuestStatus(QuestId):nullptr;
    if(!Active(p)||!status||r.lifetime.expired()||status->ScriptLifetime!=r.lifetime.lock()||
        p->GetExactDist(&FaljarPoint)>=80)return false;
    if(r.boss.IsEmpty())return r.summoning;
    auto boss=r.map->GetCreature(r.boss);
    return boss&&!boss->IsQuestObjectExpired()&&p->InSamePhase(boss)&&p->canSeeOrDetect(boss);
}
void Clear(Player* p)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end())return;
    auto old=it->second;runs.erase(it);
    p->CancelQuestObjects(QuestId);
    if(old.map){
        if(auto c=old.map->GetCreature(old.boss))c->DespawnOrUnsummon();
        if(auto c=old.map->GetCreature(old.spirit))c->DespawnOrUnsummon();
        if(auto c=old.map->GetCreature(old.originalBoss))c->RemoveFromHideList(p->GetGUID());
        if(auto c=old.map->GetCreature(old.originalSpirit))c->RemoveFromHideList(p->GetGUID());
    }
}
void Credit(Player* p)
{
    if(p->GetQuestStatus(QuestId)!=QUEST_STATUS_INCOMPLETE||p->GetQuestObjectiveData(QuestId,93442))return;
    auto q=sQuestDataStore->GetQuestTemplate(QuestId);auto const& o=q->GetObjectives().front();
    p->SetQuestObjectiveData(q,&o,1);p->SendQuestUpdateAddCredit(q,ObjectGuid::Empty,o,1);
    if(p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);
}
void Finish(Player* p,Creature* boss)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end()||it->second.stage||!Current(p,it->second)||it->second.boss!=boss->GetGUID())return;
    // The native objective is "Faljar confronted". Save that completed
    // confrontation before the delayed transfer, so logout cannot lose it.
    it->second.stage=1;it->second.timer=5000;
    boss->CombatStop(true);boss->SetReactState(REACT_PASSIVE);
    boss->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
    boss->AI()->Talk(4,p->GetGUID());
    Credit(p);
}
void Begin(Player* p)
{
    if(!Active(p)||p->IsBeingTeleported()||p->GetExactDist(&FaljarPoint)>40)return;
    Creature* original=nullptr;Creature* spirit=nullptr;std::list<Creature*> actors;
    GetCreatureListWithEntryInGrid(actors,p,93093,60);
    for(auto c:actors)if(c->GetDBTableGUIDLow()==377348&&c->IsInWorld())original=c;
    actors.clear();GetCreatureListWithEntryInGrid(actors,p,93428,60);
    for(auto c:actors)if(c->GetDBTableGUIDLow()==292001&&c->IsInWorld())spirit=c;
    // The DB Faljar carries Quest Invisibility22. Its phase and exact spawn
    // identify the source; the owned replacement removes that hidden pose.
    if(!original||!spirit||!p->InSamePhase(original)||!p->IsWithinLOSInMap(original))return;
    p->AddQuestDelayedEvent(QuestId,0,[]{});
    Run r;r.map=p->GetMap();r.lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;
    r.originalBoss=original->GetGUID();r.originalSpirit=spirit->GetGUID();
    auto it=runs.emplace(p->GetGUID(),r).first;it->second.summoning=true;
    auto boss=p->SummonCreature(93093,FaljarPoint,TEMPSUMMON_MANUAL_DESPAWN,0,0,p->GetGUID());
    it->second.summoning=false;
    if(!boss||it->second.boss!=boss->GetGUID()){if(boss)boss->DespawnOrUnsummon();Clear(p);return;}
    auto helper=p->SummonCreature(93428,AshildirPoint,TEMPSUMMON_MANUAL_DESPAWN,0,0,p->GetGUID());
    if(!helper){Clear(p);return;}
    helper->SetReactState(REACT_PASSIVE);helper->setActive(true);p->BindQuestObject(QuestId,helper);
    it->second.spirit=helper->GetGUID();
    original->AddToHideList(p->GetGUID());spirit->AddToHideList(p->GetGUID());
    original->DestroyForPlayer(p);spirit->DestroyForPlayer(p);
    // A completed confrontation in Haustvald resumes only its pending ending.
    // Helheim is an outdoor map, not a Scenario checkpoint.
    if(p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE)Finish(p,boss);
}
void Update(Player* p,uint32 diff)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end()){Begin(p);return;}
    auto& r=it->second;
    if(!Current(p,r)){Clear(p);return;}
    r.age+=diff;if(r.age>=180000){Clear(p);return;}
    auto boss=r.map->GetCreature(r.boss);auto spirit=r.map->GetCreature(r.spirit);
    if(!boss||!spirit||!boss->IsAlive()||!spirit->IsAlive()||boss->IsQuestObjectExpired()||spirit->IsQuestObjectExpired()||!p->InSamePhase(spirit)) {Clear(p);return;}
    if(!r.stage)return;
    if(r.timer>diff){r.timer-=diff;return;}
    if(r.stage==1){
        // Preserve the old five-second pause and independent twelve-second
        // ending. Runic Bulwark208544 is not a player banishment spell.
        boss->AI()->Talk(2,p->GetGUID());r.stage=2;r.timer=12000;return;
    }
    if(p->IsBeingTeleported()){r.timer=1000;return;}
    p->CombatStopWithPets(true);
    r.stage=3;r.timer=5000;
    bool accepted=p->TeleportTo(1463,343.62f,354.5f,30.54f,4.57f);
    // Teleport callbacks can clear the run synchronously.
    if(!accepted){auto pending=runs.find(p->GetGUID());if(pending!=runs.end())pending->second.timer=3000;}
}
}

struct npc_stormheim_final_judgment : SmartAI
{
    explicit npc_stormheim_final_judgment(Creature* c):SmartAI(c) { }
    ObjectGuid owner;bool owned=false,spoken=false;
    Player* Owner()const{return ObjectAccessor::GetPlayer(*me,owner);}
    bool Valid()const
    {
        using namespace StormheimFinalJudgment;auto p=Owner();auto it=p?runs.find(owner):runs.end();
        return owned&&it!=runs.end()&&Current(p,it->second)&&it->second.boss==me->GetGUID()&&
            !me->IsQuestObjectExpired()&&p->InSamePhase(me)&&p->canSeeOrDetect(me);
    }
    void IsSummonedBy(Unit* summoner)override
    {
        using namespace StormheimFinalJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p=summoner?summoner->ToPlayer():nullptr;auto it=p?runs.find(p->GetGUID()):runs.end();
        if(it==runs.end()||!it->second.summoning||!Current(p,it->second)||me->GetEntry()!=93093||me->GetExactDist(&FaljarPoint)>1){SmartAI::IsSummonedBy(summoner);return;}
        owner=p->GetGUID();owned=true;it->second.boss=me->GetGUID();p->BindQuestObject(QuestId,me);me->setActive(true);
        me->RemoveAurasDueToSpell(89319);me->RemoveAurasDueToSpell(209363);
        me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC);me->SetReactState(REACT_AGGRESSIVE);
    }
    bool CanAIAttack(Unit const* target)const override
    {
        using namespace StormheimFinalJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!owned)return SmartAI::CanAIAttack(target);
        return Valid()&&!runs.find(owner)->second.stage&&target&&target->GetCharmerOrOwnerPlayerOrPlayerItself()==Owner();
    }
    void EnterCombat(Unit* target)override
    {
        using namespace StormheimFinalJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!owned){SmartAI::EnterCombat(target);return;}
        if(CanAIAttack(target)&&!spoken){spoken=true;me->RemoveAurasDueToSpell(209164);Talk(0,owner);}
    }
    void DamageTaken(Unit* attacker,uint32& damage,DamageEffectType type)override
    {
        if(!owned){SmartAI::DamageTaken(attacker,damage,type);return;}
        if(!CanAIAttack(attacker))damage=0;
        else EnterCombat(attacker);
    }
    void DamageTakenAfterScaling(Unit* attacker,uint32& damage,DamageEffectType)override
    {
        using namespace StormheimFinalJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        if(!owned)return;
        if(!CanAIAttack(attacker)){damage=0;return;}
        uint64 health=me->GetHealth(attacker),floor=std::max<uint64>(1,me->GetMaxHealth(attacker)/100);
        if(damage>=health||health-damage<=floor){damage=0;me->SetHealth(1);Finish(Owner(),me);}
    }
    void EnterEvadeMode()override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);
        if(!owned){SmartAI::EnterEvadeMode();return;}
        if(Valid()&&StormheimFinalJudgment::runs.find(owner)->second.stage){me->CombatStop(true);return;}
        if(auto p=Owner())StormheimFinalJudgment::Clear(p);else me->DespawnOrUnsummon();
    }
    void UpdateAI(uint32 diff)override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);
        if(!owned){SmartAI::UpdateAI(diff);return;}
        if(!Valid()){if(auto p=Owner())StormheimFinalJudgment::Clear(p);else me->DespawnOrUnsummon();return;}
        if(!StormheimFinalJudgment::runs.find(owner)->second.stage&&UpdateVictim())DoMeleeAttackIfReady();
    }
};
class player_stormheim_final_judgment : public PlayerScript
{
public:
    player_stormheim_final_judgment():PlayerScript("player_stormheim_final_judgment") { }
    void OnUpdate(Player* p,uint32 diff)override{std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);StormheimFinalJudgment::Update(p,diff);}
    void OnMapChanged(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);StormheimFinalJudgment::Clear(p);}
    void OnLogout(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);StormheimFinalJudgment::Clear(p);}
};
void AddSC_stormheim_final_judgment()
{
    RegisterCreatureAI(npc_stormheim_final_judgment);new player_stormheim_final_judgment();
}

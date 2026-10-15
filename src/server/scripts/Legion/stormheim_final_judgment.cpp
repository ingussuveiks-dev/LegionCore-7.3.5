#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
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
Position const RunePoints[] = {{3630.62f,759.943f,38.7806f,5.05479f},{3655.53f,784.203f,39.1235f,3.80961f}};
struct Run
{
    Map* map = nullptr;
    std::weak_ptr<uint8> lifetime;
    ObjectGuid boss, spirit, originalBoss, originalSpirit;
    ObjectGuid rune, originalRunes[2];
    uint32 stage = 0, timer = 0, age = 0;
    uint32 runes = 0, line = 0;
    bool shield = false, summoningRune = false, banishOwned = false;
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
    auto rune=sObjectMgr->GetCreatureTemplate(97222);
    return o.Type==QUEST_OBJECTIVE_MONSTER&&o.ObjectID==93442&&o.Amount==1&&o.StorageIndex==0&&!o.Flags&&
        t&&sObjectMgr->GetScriptName(t->ScriptID)=="npc_stormheim_final_judgment"&&
        rune&&sObjectMgr->GetScriptName(rune->ScriptID)=="npc_stormheim_faljar_rune";
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
    if(old.banishOwned)p->RemoveAurasDueToSpell(209241,p->GetGUID());
    if(old.map){
        if(auto c=old.map->GetCreature(old.boss))c->DespawnOrUnsummon();
        if(auto c=old.map->GetCreature(old.spirit))c->DespawnOrUnsummon();
        if(auto c=old.map->GetCreature(old.rune))c->DespawnOrUnsummon();
        for(auto const& guid:old.originalRunes)if(auto c=old.map->GetCreature(guid))c->RemoveFromHideList(p->GetGUID());
        if(auto c=old.map->GetCreature(old.originalBoss))c->RemoveFromHideList(p->GetGUID());
        if(auto c=old.map->GetCreature(old.originalSpirit))c->RemoveFromHideList(p->GetGUID());
    }
}
void EnsureRune(Player* p,Run& r)
{
    if(!r.shield||r.runes>=2||!r.rune.IsEmpty())return;
    std::list<Creature*> list;GetCreatureListWithEntryInGrid(list,p,97221,80);
    Creature* source=nullptr;
    for(auto c:list)if(c->GetDBTableGUIDLow()==291988+r.runes&&c->IsInWorld())source=c;
    if(!source)return; // loaded-source/summon failures remain retryable
    r.summoningRune=true;
    auto rune=p->SummonCreature(97222,RunePoints[r.runes],TEMPSUMMON_MANUAL_DESPAWN,0,0,p->GetGUID());
    r.summoningRune=false;
    if(!rune||r.rune!=rune->GetGUID()){if(rune)rune->DespawnOrUnsummon();return;}
    r.originalRunes[r.runes]=source->GetGUID();source->AddToHideList(p->GetGUID());source->DestroyForPlayer(p);
}
void Shield(Player* p,Creature* boss)
{
    auto& r=runs.find(p->GetGUID())->second;if(r.shield||r.runes>=2)return;
    r.shield=true;boss->SetHealth(std::max<uint64>(1,boss->GetMaxHealth()*(r.runes?35:70)/100));
    boss->AttackStop();boss->GetMotionMaster()->MoveIdle();boss->SetReactState(REACT_PASSIVE);
    boss->CastSpell(boss,208544,true);
    // State, not success of a cosmetic cast, enforces the rune gate.
    if(auto spirit=r.map->GetCreature(r.spirit))spirit->AI()->Talk(r.runes?12:11,p->GetGUID());
    boss->AI()->Talk(1,p->GetGUID());
}
void RuneDestroyed(Player* p,ObjectGuid guid)
{
    auto it=runs.find(p->GetGUID());if(it==runs.end()||!Current(p,it->second))return;
    auto& r=it->second;if(r.stage||!r.shield||r.rune!=guid)return;
    r.rune.Clear();r.shield=false;
    if(auto source=r.map->GetCreature(r.originalRunes[r.runes]))source->RemoveFromHideList(p->GetGUID());
    ++r.runes;
    if(auto boss=r.map->GetCreature(r.boss)){boss->RemoveAurasDueToSpell(208544);boss->SetReactState(REACT_AGGRESSIVE);boss->AI()->AttackStart(p);}
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
    it->second.stage=1;it->second.timer=5000;it->second.age=0;
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
    if(!r.stage){EnsureRune(p,r);return;}
    if(r.timer>diff){r.timer-=diff;return;}
    if(r.stage==1){
        // Use the actual native dialogue speakers. The old conversation1867
        // rows overwrite indices and include removed native line3956.
        spirit->AI()->Talk(10,p->GetGUID());r.stage=2;r.line=1;r.timer=4200;return;
    }
    if(r.stage==2){
        static uint32 const Delays[]={7971,4452,5000,5000};
        if(r.line==2)spirit->AI()->Talk(13,p->GetGUID());
        else boss->AI()->Talk(r.line==1?10:r.line==3?12:13,p->GetGUID());
        r.timer=Delays[r.line-1];
        if(++r.line==5)r.stage=3;
        return;
    }
    if(r.stage==3){
        p->CombatStopWithPets(true);
        // Native20-second209241 owns its transform/fly/no-actions effects.
        // Do not remove an already-existing copy from an independent source.
        if(!p->HasAura(209241)){p->CastSpell(p,209241,true);r.banishOwned=p->HasAura(209241);}
        r.stage=4;r.timer=20000;return;
    }
    if(p->IsBeingTeleported()){r.timer=1000;return;}
    if(r.banishOwned){p->RemoveAurasDueToSpell(209241,p->GetGUID());r.banishOwned=false;}
    p->CombatStopWithPets(true);
    r.stage=5;r.timer=5000;
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
        auto& r=runs.find(owner)->second;
        if(r.shield){damage=0;return;}
        uint64 health=me->GetHealth(attacker),floor=std::max<uint64>(1,me->GetMaxHealth(attacker)/100);
        uint64 threshold=me->GetMaxHealth(attacker)*(r.runes?35:70)/100;
        if(r.runes<2&&(damage>=health||health-damage<=threshold)){damage=0;Shield(Owner(),me);return;}
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
        auto const& r=StormheimFinalJudgment::runs.find(owner)->second;
        if(!r.stage&&!r.shield&&UpdateVictim())DoMeleeAttackIfReady();
    }
};
struct npc_stormheim_faljar_rune : SmartAI
{
    explicit npc_stormheim_faljar_rune(Creature* c):SmartAI(c) { }
    ObjectGuid owner;bool owned=false;
    void IsSummonedBy(Unit* summoner)override
    {
        using namespace StormheimFinalJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p=summoner?summoner->ToPlayer():nullptr;auto it=p?runs.find(p->GetGUID()):runs.end();
        if(it==runs.end()||!Current(p,it->second)||!it->second.summoningRune||!it->second.shield||it->second.runes>=2||me->GetEntry()!=97222||me->GetExactDist(&RunePoints[it->second.runes])>1){SmartAI::IsSummonedBy(summoner);return;}
        owner=p->GetGUID();owned=true;it->second.rune=me->GetGUID();p->BindQuestObject(QuestId,me);
        me->setFaction(16);me->SetReactState(REACT_PASSIVE);me->SetMaxHealth(1);me->SetHealth(1);me->setActive(true);
        me->CastSpell(me,208811,true);me->CastSpell(me,208815,true);
    }
    bool Valid(Unit* attacker)const
    {
        using namespace StormheimFinalJudgment;auto p=ObjectAccessor::GetPlayer(*me,owner);auto it=p?runs.find(owner):runs.end();
        return owned&&it!=runs.end()&&Current(p,it->second)&&!it->second.stage&&it->second.shield&&
            it->second.rune==me->GetGUID()&&!me->IsQuestObjectExpired()&&p->InSamePhase(me)&&
            attacker&&attacker->GetCharmerOrOwnerPlayerOrPlayerItself()==p;
    }
    void DamageTaken(Unit* attacker,uint32& damage,DamageEffectType type)override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);
        if(!owned){SmartAI::DamageTaken(attacker,damage,type);return;}
        if(!Valid(attacker))damage=0;
    }
    void JustDied(Unit* killer)override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimFinalJudgment::mutex);
        if(!owned){SmartAI::JustDied(killer);return;}
        if(Valid(killer))StormheimFinalJudgment::RuneDestroyed(ObjectAccessor::GetPlayer(*me,owner),me->GetGUID());
        else {auto it=StormheimFinalJudgment::runs.find(owner);if(it!=StormheimFinalJudgment::runs.end()&&it->second.rune==me->GetGUID())it->second.rune.Clear();}
        me->DespawnOrUnsummon(1000);
    }
};
class spell_stormheim_faljar_bulwark : public SpellScript
{
    PrepareSpellScript(spell_stormheim_faljar_bulwark);
    void Filter(std::list<WorldObject*>& targets)
    {
        using namespace StormheimFinalJudgment;std::lock_guard<std::recursive_mutex> lock(mutex);
        auto caster=GetCaster()->ToCreature();if(!caster||caster->GetEntry()!=93093)return;
        auto ai=dynamic_cast<npc_stormheim_final_judgment*>(caster->AI());if(!ai||!ai->owned)return;
        for(auto const& pair:runs)if(pair.second.boss==caster->GetGUID()){
            auto p=ObjectAccessor::GetPlayer(*caster,pair.first);
            if(!Current(p,pair.second)||!pair.second.shield){targets.clear();return;}
            targets.remove_if([p](WorldObject* object){auto unit=object?object->ToUnit():nullptr;return !unit||unit->GetCharmerOrOwnerPlayerOrPlayerItself()!=p;});return;
        }
        targets.clear(); // a queued tick cannot escape after its run is cleared
    }
    void Register()override{OnObjectAreaTargetSelect+=SpellObjectAreaTargetSelectFn(spell_stormheim_faljar_bulwark::Filter,EFFECT_0,TARGET_UNIT_SRC_AREA_ENEMY);}
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
    RegisterCreatureAI(npc_stormheim_final_judgment);RegisterCreatureAI(npc_stormheim_faljar_rune);
    RegisterSpellScript(spell_stormheim_faljar_bulwark);new player_stormheim_final_judgment();
}

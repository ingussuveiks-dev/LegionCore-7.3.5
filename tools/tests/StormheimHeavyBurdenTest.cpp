#include <crtdbg.h>
#include <cstdlib>
#include <mutex>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,GAMEOBJECT_TYPE_GOOBER=10;
constexpr int SPELL_EFFECT_REMOVE_AURA_2=203,SPELL_EFFECT_HANDLE_HIT_TARGET=1;
#include "BurdenFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
#include "NativeQuestSkill.inc"
enum {TARGET_FLAG_UNIT_MASK=0x3ff,TARGET_FLAG_CORPSE_MASK=1<<10,TARGET_FLAG_GAMEOBJECT_MASK=1<<11};
Unit* NativeSelfTarget(Unit* unit)
{
    uint32 neededTargets=0;
    struct {Unit* target;WorldObject* GetObjectTarget(){return target;}void RemoveObjectTarget(){target=nullptr;}} m_targets{unit};
#include "NativeExplicitAdmission.inc"
    return m_targets.target;
}
struct ScriptManager {bool OnGossipHello(Player*,GameObject*){return false;}} scriptManager;auto sScriptMgr=&scriptManager;
#include "NativeObjectUse.inc"
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}};
struct FishSpell {
    Unit* caster=nullptr,*target=nullptr,*hit=nullptr;std::set<int> prevented;
    Hook OnCheckCast,OnEffectHitTarget;
    Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}Unit* GetHitUnit(){return hit;}
    void PreventHitDefaultEffect(int i){prevented.insert(i);}virtual void Register(){}
};
#define SpellScript FishSpell
#define SpellCheckCastFn(x) 1
#define SpellEffectFn(x,i,t) (i*1000+t)
#include "HeavyBurden.inc"
#undef SpellScript
struct Spell {
    int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;Unit* unitTarget=nullptr;SpellInfo data;uint32 m_diffMode=0;SpellInfo* m_spellInfo=&data;
    void EffectKillCredit(SpellEffIndex);
};
#include "NativeSpellGroupCredit.inc"
using namespace StormheimHeavyBurden;
GameObject barrel,spear;
std::unique_ptr<go_stormheim_heavy_burden> barrelAI,spearAI;
uint64 serial=50000;
struct Visit {
    Player p;player_stormheim_heavy_burden script;
    std::vector<std::unique_ptr<spell_stormheim_heavy_burden>> casts;
    bool failFish=false,failCast=false;
    Visit(uint32 team=ALLIANCE){
        p.guid={++serial};p.map=1220;p.zone=7541;p.team=team;p.race=team==ALLIANCE?1:2;
        p.m_RewardedQuests.insert(39855);p.Accept(QuestId);players[p.guid]=&p;
        p.castTargetHook=[&](Unit* target,uint32 spell)->int{
            if(spell==Fish){if(failFish)return SPELL_FAILED_BAD_TARGETS;target->roots[{spell,p.guid.value}]={};return SPELL_CAST_OK;}
            assert(spell==Skewer&&target==&p);if(failCast)return SPELL_FAILED_BAD_TARGETS;
            auto cast=std::make_unique<spell_stormheim_heavy_burden>();cast->caster=cast->hit=&p;cast->target=NativeSelfTarget(&p);assert(!cast->target);
            auto result=cast->Check();if(result==SPELL_CAST_OK){p.casting=true;casts.push_back(std::move(cast));}return result;
        };
    }
    ~Visit(){script.OnLogout(&p);players.erase(p.guid);}
    void At(GameObject& go){static_cast<Position&>(p)=static_cast<Position&>(go);}
    void Collect(){At(barrel);barrel.Use(&p);}
    void Begin(){At(spear);spear.Use(&p);}
    void Finish(){auto& c=*casts.back();c.RemoveFish(0);c.Complete(1);p.casting=false;}
    void Tick(){script.OnUpdate(&p,500);}
    bool FishAura(){return p.GetAura(Fish,p.guid)!=nullptr;}
};
int main()
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.objectives={{0,CreditId,1,0,1}};
    store.quests[39855].id=39855;
    q.PrevQuestID=39855;q.prevQuests={39855};q.NextQuestID=40001;q.NextQuestIdChain=40001;
    auto& next=store.quests[40001];next.id=40001;next.PrevQuestID=QuestId;next.prevQuests={QuestId};store.starters.emplace(92539,40001);
    for(auto go:{&barrel,&spear}){go->map=1220;go->zone=7541;go->info.type=10;go->guid={++serial};objects[go->guid]=go;}
    barrel.entry=244867;barrel.dbGuid=147040078;static_cast<Position&>(barrel)=Barrel;
    spear.entry=244872;spear.dbGuid=115015;spear.info.goober.spell=193577;static_cast<Position&>(spear)=Spear;
    barrelAI=std::make_unique<go_stormheim_heavy_burden>(&barrel);spearAI=std::make_unique<go_stormheim_heavy_burden>(&spear);
    spell_stormheim_heavy_burden registration;registration.Register();assert(registration.OnCheckCast.values==std::vector<int>{1}&&registration.OnEffectHitTarget.values==std::vector<int>({203,1134}));
    {Visit v;v.p.m_RewardedQuests.clear();q.RequiredSkillId=185;q.RequiredSkillPoints=1;q.prevQuests.clear();q.PrevQuestID=0;
        assert(!v.p.CanTakeQuest(&q,false));v.p.cooking=1;assert(v.p.CanTakeQuest(&q,false)); // old admission: cooking gates story, no Helheim prerequisite
        q.RequiredSkillId=q.RequiredSkillPoints=0;q.PrevQuestID=39855;q.prevQuests={39855};v.p.cooking=0;assert(!v.p.CanTakeQuest(&q,false));
        v.p.quests[39855].Status=QUEST_STATUS_COMPLETE;assert(!v.p.CanTakeQuest(&q,false));v.p.quests.erase(39855);v.p.m_RewardedQuests.insert(39855);assert(v.p.CanTakeQuest(&q,false));}
    for(auto team:{ALLIANCE,HORDE}){
        Visit v(team);assert(v.p.CanTakeQuest(&q,false)&&!v.p.CanRewardQuest(&q,false)&&!v.p.CanTakeQuest(&next,false));
        v.Begin();assert(v.casts.empty()&&!v.p.packets);v.Collect();assert(v.FishAura()&&runs.count(v.p.guid)&&!v.p.packets);
        auto n=v.p.casts.size();v.Collect();assert(v.p.casts.size()==n);v.Begin();assert(v.p.casting&&v.casts.size()==1&&v.FishAura()&&!v.p.packets);
        v.Begin();assert(v.casts.size()==1);v.Finish();assert(!v.FishAura()&&!runs.count(v.p.guid)&&v.p.packets==1&&v.p.completions==1&&v.p.CanRewardQuest(&q,false));
        auto& c=*v.casts.back();assert(c.prevented==std::set<int>({0,1}));c.Complete(1);assert(v.p.packets==1);v.Collect();assert(!v.FishAura());
        v.p.m_RewardedQuests.insert(QuestId);v.p.quests.erase(QuestId);Creature havi;havi.entry=92539;havi.guid={99};creatures[havi.guid]=&havi;
        assert(v.p.GetNextQuest(havi.guid,&q)==&next&&v.p.CanTakeQuest(&next,false));creatures.erase(havi.guid);
    }
    {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group{&first};a.p.group=b.p.group=&group;
        a.Collect();b.Collect();a.Begin();a.Finish();assert(a.p.completions==1&&!b.p.packets&&b.FishAura());b.Begin();b.Finish();assert(b.p.completions==1);}
    {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group{&first};a.p.group=b.p.group=&group;
        Spell native;native.unitTarget=&a.p;native.EffectKillCredit(1);assert(!a.p.packets&&!b.p.packets);
        auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,CreditId,1,0,0}};a.p.Accept(999);b.p.Accept(999);native.EffectKillCredit(1);assert(a.p.GetQuestObjectiveData(999,CreditId)==1&&b.p.GetQuestObjectiveData(999,CreditId)==1&&!a.p.GetQuestObjectiveData(QuestId,CreditId)&&!b.p.GetQuestObjectiveData(QuestId,CreditId));}
    {Visit v;Creature alias;alias.entry=123;alias.guid={9999};creatures[alias.guid]=&alias;v.p.KilledMonsterCredit(CreditId,alias.guid);v.p.TalkedToCreature(CreditId,{});assert(!v.p.packets);creatures.erase(alias.guid);}
    {Visit v;v.failFish=true;v.Collect();assert(!v.FishAura()&&!runs.count(v.p.guid));v.failFish=false;v.Collect();assert(v.FishAura());v.failCast=true;v.Begin();assert(v.FishAura()&&v.casts.empty());v.failCast=false;v.Begin();v.Finish();assert(v.p.completions==1);}
    {Visit v;v.Collect();v.Begin();v.p.casting=false;v.Begin();auto& old=*v.casts.front();assert(old.Check()==SPELL_FAILED_BAD_TARGETS);old.RemoveFish(0);old.Complete(1);assert(v.FishAura()&&!v.p.packets);v.Finish();assert(v.p.completions==1);} // interrupted cast ticket replaced; repeated CheckCast cannot adopt it
    {Visit v;v.Collect();v.Begin();v.p.Accept(QuestId);v.Tick();v.Finish();assert(!v.FishAura()&&!v.p.packets);v.Collect();v.Begin();v.Finish();assert(v.p.completions==1);}
    {Visit v;v.Collect();v.Begin();v.At(barrel);v.Finish();assert(!v.p.packets&&v.FishAura());v.Begin();v.Finish();assert(v.p.completions==1);} // finish rechecks source distance
    {Visit v;v.Collect();v.Begin();v.p.roots.erase({Fish,v.p.guid.value});v.Finish();assert(!v.p.packets);} // dispelled during cast
    for(int failure=0;failure<15;++failure){Visit v;v.Collect();v.Begin();
        if(failure==0)v.p.alive=false;if(failure==1)v.p.map=1463;if(failure==2)v.p.zone=1;if(failure==3)v.p.teleport=true;
        if(failure==4)v.p.vehicle=true;if(failure==5)v.p.transport=true;if(failure==6)v.p.combat=true;if(failure==7)v.p.flying=true;
        if(failure==8)v.p.phase=false;if(failure==9)v.p.los=false;if(failure==10)v.p.visible=false;if(failure==11)v.p.phases.insert(8306);
        if(failure==12)v.p.quests.erase(QuestId);if(failure==13)spear.flags=GO_FLAG_NOT_SELECTABLE;if(failure==14)spear.phases.insert(7510);
        v.Finish();assert(!v.p.packets);spear.flags=0;spear.phases.clear();
    }
    {Visit v;v.p.roots[{Fish,999}]={};v.Begin();assert(v.casts.empty());v.Collect();assert(v.FishAura());v.Begin();v.Finish();assert(v.p.completions==1&&v.p.GetAura(Fish,999));}
    {Visit v;v.Collect();v.script.OnLogout(&v.p);assert(!v.FishAura());v.p.roots[{Fish,v.p.guid.value}]={};v.script.OnLogin(&v.p);assert(!v.FishAura()&&!runs.count(v.p.guid));v.Begin();assert(v.casts.empty());}
    {Visit v;v.Collect();v.p.map=1463;v.script.OnMapChanged(&v.p);assert(!v.FishAura()&&!runs.count(v.p.guid));}
    {Visit v;v.Collect();v.p.quests.erase(QuestId);v.Tick();assert(!v.FishAura());v.p.roots[{Fish,999}]={};v.script.OnLogin(&v.p);assert(v.p.GetAura(Fish,999));}
    {Visit v;v.p.quests[QuestId].Status=QUEST_STATUS_COMPLETE;v.p.data[{QuestId,CreditId}]=1;v.p.roots[{Fish,v.p.guid.value}]={};v.script.OnLogin(&v.p);assert(!v.FishAura()&&v.p.CanRewardQuest(&q,false)&&v.p.GetQuestObjectiveData(QuestId,CreditId)==1);}
    {Visit v;v.p.phases.insert(8306);v.p.roots[{Fish,v.p.guid.value}]={};v.script.OnLogin(&v.p);assert(v.FishAura());v.Collect();assert(!runs.count(v.p.guid));}
    {Visit v;v.p.phases.insert(7510);v.Collect();v.Begin();v.Finish();assert(v.p.completions==1);} // unrelated later Havi raid phase must not block the base story quest
    {Visit v;q.objectives[0].Flags=1;v.Collect();assert(!v.FishAura());q.objectives[0].Flags=0;barrel.flags=GO_FLAG_IN_USE;v.Collect();assert(!v.FishAura());barrel.flags=0;}
    {Visit v;v.At(barrel);barrelAI->GossipHello(&v.p,false);assert(!v.FishAura());}
    {Visit v;auto old=spearAI->delegated;spear.dbGuid=123;assert(!spearAI->GossipUse(&v.p)&&spearAI->delegated==old+1);spear.dbGuid=115015;}
    {Visit v;spell_stormheim_heavy_burden unrelated;unrelated.caster=unrelated.target=unrelated.hit=&v.p;assert(unrelated.Check()==SPELL_CAST_OK);unrelated.RemoveFish(0);unrelated.Complete(1);assert(unrelated.prevented.empty());}
    std::cout<<"PASS: production40078 actual barrel/spear use, native quest skill/previous/reward/next and native spell134 group path; both factions/owners, self fish proof, separate cast ticket/acceptance, real effect203/134 hook registration, finish/range/phase/abandon/retry/reentry/foreign-aura/cleanup boundaries. World, cast scheduling/1500ms interruption, aura engine and client networking remain fixtures.\n";
}

#include <crtdbg.h>
#include <array>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "DrowningValkyraFixture.inc"
#include "Talked.inc"
void Creature::GetPlayerListInGrid(std::list<Player*>& list,float range)
{for(auto const& pair:players)if(pair.second->world&&pair.second->map==map&&pair.second->GetExactDist(this)<=range)list.push_back(pair.second);}
struct SpellClickInfo {uint32 spellId=192387;bool IsFitToRequirements(Player const*,Creature const*)const{return true;}};
using SpellClickInfoContainer=std::multimap<uint32,SpellClickInfo>;using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct NativeTemplates {SpellClickInfoContainer clicks{{97469,{192387}}};SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 id){return clicks.equal_range(id);}std::string GetScriptName(uint32 id){return id==2?"npc_stormheim_drowning_valkyra":"custom";}} templates;auto sObjectMgr=&templates;
using ConditionList=std::vector<int>;
struct ConditionSourceInfo {Player* player;ConditionSourceInfo(Player* p,Creature*):player(p){}};
struct Conditions {ConditionList GetConditionsForSpellClickEvent(uint32,uint32){return {9};}bool IsObjectMeetToConditions(ConditionSourceInfo const& info,ConditionList const&){Player* player=info.player;uint32 ConditionValue1=39851;bool condMeets=false;
#include "NativeQuestTaken.inc"
return condMeets;}} conditions;auto sConditionMgr=&conditions;
#include "NativeClickVisibility.inc"
uint32 NativeClickField(Player* target,Creature const* creature){uint32 appendValue=UNIT_NPC_FLAG_SPELLCLICK;
#include "NativeClickField.inc"
return appendValue;}
struct SmartScript {Creature* me;bool IsSmart(Creature*);};
#include "NativeIsSmart.inc"
struct LanternScript {Unit* caster=nullptr,*target=nullptr,*hit=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget;Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}Unit* GetHitUnit(){return hit;}void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}};
#define SpellScript LanternScript
#define SpellCheckCastFn(x) 1
#define SpellEffectFn(x,i,t) (i*100+t)
#include "DrowningValkyra.inc"
#undef SpellScript
using namespace StormheimDrowningValkyra;
uint64 serial=1000;
struct Visit {
    Player p;
    Visit(uint32 team=ALLIANCE){p.guid={++serial};p.map=1463;p.zone=7588;p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(QuestId);players[p.guid]=&p;}
    ~Visit(){players.erase(p.guid);}
    void At(Creature& c){static_cast<Position&>(p)=static_cast<Position&>(c);}
};
std::array<Creature,10> actors;
std::array<std::unique_ptr<npc_stormheim_drowning_valkyra>,10> ai;
spell_stormheim_drowning_valkyra Cast(Visit& v,Creature& actor,bool finish=true)
{
    spell_stormheim_drowning_valkyra spell;spell.caster=&v.p;spell.target=&actor;spell.hit=&actor;
    if(spell.Check()==SPELL_CAST_OK&&finish)spell.Hit(0);
    return spell;
}
void Refresh(uint32 diff=1){for(auto& a:ai)a->UpdateAI(diff);}
int main()
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.NextQuestID=39853;q.ExclusiveGroup=-39850;q.objectives={{0,CreditId,8,0,1}};
    auto& parallel=store.quests[39850];parallel.id=39850;parallel.ExclusiveGroup=-39850;
    auto& next=store.quests[39853];next.id=39853;next.PrevQuestID=39850;next.prevQuests={39850};
    store.mExclusiveQuestGroups.emplace(-39850,QuestId);store.mExclusiveQuestGroups.emplace(-39850,39850);
    for(int i=0;i<10;++i){auto& c=actors[i];c.entry=97469;c.dbGuid=Sites[i].spawn;c.guid={uint64(100+i)};c.map=1463;c.zone=7588;static_cast<Position&>(c)=Sites[i].point;c.SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);c.roots[{192388,c.guid.value}]={};creatures[c.guid]=&c;ai[i]=std::make_unique<npc_stormheim_drowning_valkyra>(&c);}
    for(auto team:{ALLIANCE,HORDE}){
        Visit a(team),b(team);
        for(int i=0;i<8;++i){auto& c=actors[i];a.At(c);b.At(c);auto pending=Cast(a,c,false);assert(a.p.packets==uint32(i));pending.Hit(0);assert(a.p.GetQuestObjectiveData(QuestId,CreditId)==i+1);pending.Hit(0);assert(pending.prevented&&a.p.packets==uint32(i+1));assert(!c.ShouldHideFor(a.p.guid)&&c.visible);Cast(b,c);assert(b.p.packets==uint32(i+1));}
        assert(a.p.GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE&&a.p.completions==1&&a.p.CanRewardQuest(&q,false));
        a.At(actors[8]);Cast(a,actors[8]);assert(a.p.packets==8&&a.p.GetQuestObjectiveData(QuestId,CreditId)==8);
        assert(!a.p.CanTakeQuest(&next,false));a.p.m_RewardedQuests.insert(QuestId);a.p.quests.erase(QuestId);assert(!a.p.CanTakeQuest(&next,false));a.p.m_RewardedQuests.insert(39850);assert(a.p.CanTakeQuest(&next,false));
        b.p.m_RewardedQuests.insert(39850);assert(!b.p.CanTakeQuest(&next,false));b.p.m_RewardedQuests.insert(QuestId);b.p.quests.erase(QuestId);assert(b.p.CanTakeQuest(&next,false));Refresh();
    }
    {Visit a,b;a.At(actors[0]);b.At(actors[0]);Cast(a,actors[0]);ai[0]->UpdateAI(4499);assert(!actors[0].ShouldHideFor(a.p.guid));Cast(a,actors[0]);assert(a.p.packets==1);ai[0]->UpdateAI(1);assert(actors[0].ShouldHideFor(a.p.guid)&&!actors[0].ShouldHideFor(b.p.guid)&&actors[0].visible);Cast(b,actors[0]);assert(b.p.packets==1);ai[0]->UpdateAI(13999);assert(actors[0].ShouldHideFor(a.p.guid));ai[0]->UpdateAI(1);assert(!actors[0].ShouldHideFor(a.p.guid)&&actors[0].ShouldHideFor(b.p.guid));Cast(a,actors[0]);assert(a.p.packets==2);assert(actors[0].world&&actors[0].roots.size()==1&&actors[0].HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK));}
    {Visit v;v.At(actors[0]);auto old=Cast(v,actors[0],false);v.p.Accept(QuestId);old.Hit(0);assert(old.prevented&&!v.p.packets);Cast(v,actors[0]);assert(v.p.packets==1);ai[0]->UpdateAI(HideDelay);assert(actors[0].ShouldHideFor(v.p.guid));v.p.Accept(QuestId);Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));Cast(v,actors[0]);assert(v.p.packets==2);}
    {Visit v;v.At(actors[0]);v.p.SetQuestObjectiveData(&q,&q.objectives[0],5);Cast(v,actors[0]);assert(v.p.GetQuestObjectiveData(QuestId,CreditId)==6&&v.p.packets==1);} // saved count resumes; transient per-source cycle is not serialized
    for(int boundary=0;boundary<4;++boundary){Visit v;v.At(actors[0]);Cast(v,actors[0]);ai[0]->UpdateAI(HideDelay);assert(actors[0].ShouldHideFor(v.p.guid));if(boundary==0)v.p.quests.erase(QuestId);if(boundary==1)v.p.world=false;if(boundary==2)v.p.map=1220;if(boundary==3)v.p.phases.insert(8306);Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));}
    for(int invalid=0;invalid<16;++invalid){Visit v;v.At(actors[0]);if(invalid==0)v.p.alive=false;if(invalid==1)v.p.map=1220;if(invalid==2)v.p.zone=1;if(invalid==3)v.p.teleport=true;if(invalid==4)v.p.vehicle=true;if(invalid==5)v.p.transport=true;if(invalid==6)v.p.combat=true;if(invalid==7)v.p.flying=true;if(invalid==8)v.p.phase=false;if(invalid==9)v.p.los=false;if(invalid==10)v.p.visible=false;if(invalid==11)v.p.x+=11;if(invalid==12)v.p.phases.insert(8306);if(invalid==13)v.p.quests.erase(QuestId);if(invalid==14)actors[0].RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);if(invalid==15)actors[0].IsAIEnabled=false;Cast(v,actors[0]);assert(!v.p.packets);actors[0].IsAIEnabled=true;actors[0].SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);}
    {Visit v;v.At(actors[0]);auto pending=Cast(v,actors[0],false);v.p.combat=true;pending.Hit(0);assert(pending.prevented&&!v.p.packets);v.p.combat=false;Cast(v,actors[0]);assert(v.p.packets==1);}
    {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group;group.first=&first;a.p.group=&group;b.p.group=&group;for(auto entry:{CreditId}){a.p.KilledMonsterCredit(entry);a.p.TalkedToCreature(entry,actors[0].guid);a.p.RewardPlayerAndGroupAtEvent(entry,&actors[0]);}assert(!a.p.packets&&!b.p.packets);Unit pet;pet.ownerPlayer=&a.p;spell_stormheim_drowning_valkyra cast;cast.caster=&pet;cast.target=&actors[0];assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);}
    {Visit v;auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,CreditId,1,0,0}};v.p.Accept(999);v.p.KilledMonsterCredit(CreditId);assert(v.p.GetQuestObjectiveData(999,CreditId)==1&&!v.p.GetQuestObjectiveData(QuestId,CreditId));}
    {Visit v;v.At(actors[0]);q.objectives[0].Amount=7;Cast(v,actors[0]);assert(!v.p.packets);q.objectives[0].Amount=8;auto pending=Cast(v,actors[0],false);pending.hit=&actors[1];pending.Hit(0);assert(pending.prevented&&!v.p.packets);}

    {Visit a,b;assert(NativeClickField(&a.p,&actors[0])==UNIT_NPC_FLAG_SPELLCLICK);b.p.quests.erase(QuestId);assert(!NativeClickField(&b.p,&actors[0]));a.p.quests[QuestId].Status=QUEST_STATUS_COMPLETE;assert(!NativeClickField(&a.p,&actors[0]));a.p.Accept(QuestId);assert(NativeClickField(&a.p,&actors[0])==UNIT_NPC_FLAG_SPELLCLICK);SmartScript native{&actors[0]};assert(native.IsSmart(&actors[0]));actors[0].aiName="";assert(!native.IsSmart(&actors[0]));actors[0].aiName="SmartAI";}
    {Creature later;later.entry=97469;later.dbGuid=12742796;later.map=1463;later.zone=7588;static_cast<Position&>(later)=Sites[7].point;npc_stormheim_drowning_valkyra native(&later);native.UpdateAI(500);SpellInfo hit;hit.Id=192387;native.SpellHit(&later,&hit);assert(native.delegated==2);spell_stormheim_drowning_valkyra cast;cast.caster=&later;cast.target=&later;assert(cast.Check()==SPELL_CAST_OK);}
    std::cout<<"PASS: extracted production39851 actor/spell/core credit handlers,8-count cap and both factions/owners, personal4500/18500ms cycle, recast/stale/failed hits, count reload, abandon/logout/map/phase cleanup, group/pet guards and39853 negative-group handoff in both reward orders. Later-spawn native dispatch is delegated. World, casting, talk, network and persistence are fixtures; client checks pending.\n";
}

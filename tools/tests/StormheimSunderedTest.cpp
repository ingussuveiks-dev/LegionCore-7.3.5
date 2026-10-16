#include <crtdbg.h>
#include <array>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "SunderedFixture.inc"
#include "Talked.inc"
void Creature::GetPlayerListInGrid(std::list<Player*>& list,float range)
{for(auto const& pair:players)if(pair.second->map==map&&pair.second->GetExactDist(this)<=range)list.push_back(pair.second);}
struct LanternScript {Unit* caster=nullptr,*target=nullptr,*hit=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget;Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}Unit* GetHitUnit(){return hit;}void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}};
struct NativeTemplates {std::string GetScriptName(uint32 id){return id==2?"npc_stormheim_sundered":"custom";}} templates;auto sObjectMgr=&templates;
#define SpellScript LanternScript
#define SpellCheckCastFn(x) 1
#define SpellEffectFn(x,i,t) (i*100+t)
#include "Sundered.inc"
#undef SpellScript
using namespace StormheimSundered;
// Actual native explicit-target admission: TARGET_UNIT_TARGET_ANY25 does not
// enter the assist-only branch that rejected the different New Blood spell.
namespace NativeTarget
{
enum {TARGET_FLAG_UNIT_MASK=0x3FF,TARGET_FLAG_GAMEOBJECT_MASK=1<<10,TARGET_FLAG_CORPSE_MASK=1<<11,
 TARGET_FLAG_DEST_LOCATION=1<<12,TARGET_FLAG_GAMEOBJECT_ITEM=1<<13,TARGET_FLAG_UNIT_ENEMY=1<<2,
 TARGET_FLAG_UNIT_ALLY=1<<3,TARGET_FLAG_UNIT_RAID=1<<4,TARGET_FLAG_UNIT_PARTY=1<<5,TARGET_FLAG_UNIT_MINIPET=1<<6,TARGET_FLAG_UNIT_PASSENGER=1<<7};
struct Unit;struct Item{};struct WorldObject {virtual Unit const* ToUnit()const{return nullptr;}};
struct Unit:WorldObject {mutable uint32 assists=0;uint64 GetGUID()const{return 1;}Unit const* ToUnit()const override{return this;}bool _IsValidAttackTarget(Unit const*,void const*)const{return false;}bool _IsValidAssistTarget(Unit const*,void const*)const{++assists;return false;}bool IsInPartyWith(Unit const*)const{return false;}bool IsInRaidWith(Unit const*)const{return false;}uint64 GetCritterGUID()const{return 0;}bool IsOnVehicle(Unit const*)const{return false;}};
struct SpellInfo {uint32 mask=2;uint32 GetExplicitTargetMask()const{return mask;}SpellCastResult CheckExplicitTarget(Unit const*,WorldObject const*,Item const*)const;};
#include "NativeExplicitTarget.inc"
void Verify(){SpellInfo spell;Unit caster,target;assert(spell.CheckExplicitTarget(&caster,&target,nullptr)==SPELL_CAST_OK&&!caster.assists);assert(spell.CheckExplicitTarget(&caster,nullptr,nullptr)==SPELL_FAILED_BAD_TARGETS);spell.mask=TARGET_FLAG_UNIT_ALLY;assert(spell.CheckExplicitTarget(&caster,&target,nullptr)==SPELL_FAILED_BAD_TARGETS&&caster.assists==1);}
}
uint64 serial=1000;
struct Visit {
    Player p;
    Visit(uint32 team=ALLIANCE){p.guid={++serial};p.map=1463;p.zone=7588;p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(QuestId);players[p.guid]=&p;}
    ~Visit(){players.erase(p.guid);}
    void At(Creature& c){static_cast<Position&>(p)=static_cast<Position&>(c);}
};
std::array<Creature,3> actors;
std::array<std::unique_ptr<npc_stormheim_sundered>,3> ai;
spell_stormheim_sundered Cast(Visit& v,Creature& actor,bool finish=true)
{
    spell_stormheim_sundered spell;spell.caster=&v.p;spell.target=&actor;spell.hit=&actor;
    if(spell.Check()==SPELL_CAST_OK&&finish)spell.Hit(0);
    return spell;
}
void Refresh(){for(auto& a:ai)a->UpdateAI(500);}
int main()
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    NativeTarget::Verify();
    auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.NextQuestID=39853;q.ExclusiveGroup=-39850;q.objectives={{0,97482,1,0,1},{0,97483,1,0,2},{0,97484,1,0,3}};
    auto& parallel=store.quests[39851];parallel.id=39851;parallel.ExclusiveGroup=-39850;
    auto& next=store.quests[39853];next.id=39853;next.PrevQuestID=QuestId;next.prevQuests={QuestId};
    store.mExclusiveQuestGroups.emplace(-39850,QuestId);store.mExclusiveQuestGroups.emplace(-39850,39851);
    for(int i=0;i<3;++i){auto& c=actors[i];auto const& f=Fragments[i];c.entry=f.entry;c.dbGuid=f.spawn;c.guid={uint64(100+i)};c.map=1463;c.zone=7588;static_cast<Position&>(c)=f.point;c.SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);c.roots[{f.pose,c.guid.value}]={};creatures[c.guid]=&c;ai[i]=std::make_unique<npc_stormheim_sundered>(&c);ai[i]->UpdateAI(0);assert(c.casts==std::vector<uint32>{192500});}
    for(auto team:{ALLIANCE,HORDE}){
        Visit a(team),b(team);a.p.castResult=1; // optional conversation/summon failure
        for(int i=0;i<3;++i){auto& c=actors[i];a.At(c);b.At(c);auto packets=a.p.packets;auto attempt=Cast(a,c,false);assert(!a.p.GetQuestObjectiveData(QuestId,Fragments[i].credit));attempt.Hit(0);assert(a.p.packets==packets+1&&!attempt.prevented);attempt.Hit(0);assert(attempt.prevented&&a.p.packets==packets+1);Refresh();assert(c.ShouldHideFor(a.p.guid)&&!c.ShouldHideFor(b.p.guid)&&c.world&&c.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK)&&c.roots.size()==1);Cast(b,c);assert(b.p.GetQuestObjectiveData(QuestId,Fragments[i].credit)==1);assert(c.world&&!c.despawnDelay);}
        assert(a.p.GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE&&a.p.completions==1&&b.p.completions==1&&a.p.CanRewardQuest(&q,false));
        assert(!a.p.CanTakeQuest(&next,false));a.p.m_RewardedQuests.insert(QuestId);a.p.quests.erase(QuestId);assert(!a.p.CanTakeQuest(&next,false));a.p.m_RewardedQuests.insert(39851);assert(a.p.CanTakeQuest(&next,false));
        b.p.m_RewardedQuests.insert(39851);assert(!b.p.CanTakeQuest(&next,false));b.p.m_RewardedQuests.insert(QuestId);b.p.quests.erase(QuestId);assert(b.p.CanTakeQuest(&next,false));
        Refresh();
    }
    {Visit v;v.At(actors[0]);auto old=Cast(v,actors[0],false);v.p.Accept(QuestId);old.Hit(0);assert(old.prevented&&!v.p.packets);Cast(v,actors[0]);assert(v.p.packets==1);Refresh();v.p.Accept(QuestId);Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));Cast(v,actors[0]);assert(v.p.packets==2);}
    {Visit v;v.At(actors[0]);Cast(v,actors[0]);assert(actors[0].ShouldHideFor(v.p.guid)&&ai[0]->hidden.count(v.p.guid));v.p.Accept(QuestId);Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));} // abandonment before the first periodic scan
    {Visit v;v.At(actors[0]);v.p.SetQuestObjectiveData(&q,&q.objectives[0],1);Refresh();assert(actors[0].ShouldHideFor(v.p.guid)&&!v.p.packets);v.p.quests.erase(QuestId);Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));} // persisted counters and abandonment
    {Visit v;v.At(actors[0]);Cast(v,actors[0]);Refresh();v.p.map=1220;Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));v.p.map=1463;Refresh();assert(actors[0].ShouldHideFor(v.p.guid));v.p.world=false;Refresh();assert(!actors[0].ShouldHideFor(v.p.guid));}
    for(int invalid=0;invalid<15;++invalid){Visit v;v.At(actors[0]);if(invalid==0)v.p.alive=false;if(invalid==1)v.p.map=1220;if(invalid==2)v.p.zone=1;if(invalid==3)v.p.teleport=true;if(invalid==4)v.p.vehicle=true;if(invalid==5)v.p.transport=true;if(invalid==6)v.p.combat=true;if(invalid==7)v.p.flying=true;if(invalid==8)v.p.phase=false;if(invalid==9)v.p.los=false;if(invalid==10)v.p.visible=false;if(invalid==11)v.p.x+=11;if(invalid==12)v.p.phases.insert(8306);if(invalid==13)v.p.quests.erase(QuestId);if(invalid==14)actors[0].RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);Cast(v,actors[0]);assert(!v.p.packets);actors[0].SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);}
    {Visit v;v.At(actors[0]);auto cast=Cast(v,actors[0],false);v.p.combat=true;cast.Hit(0);assert(cast.prevented&&!v.p.packets);v.p.combat=false;Cast(v,actors[0]);assert(v.p.packets==1);} // failed completion can retry stationary
    {Visit v;v.At(actors[0]);auto cast=Cast(v,actors[0],false);cast.hit=&actors[1];cast.Hit(0);assert(cast.prevented&&!v.p.packets);q.objectives[0].Flags=1;Cast(v,actors[0]);assert(!v.p.packets);q.objectives[0].Flags=0;}
    {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group;group.first=&first;a.p.group=&group;b.p.group=&group;for(auto const& f:Fragments){a.p.KilledMonsterCredit(f.credit);a.p.TalkedToCreature(f.credit,actors[0].guid);a.p.RewardPlayerAndGroupAtEvent(f.credit,&actors[0]);}assert(!a.p.packets&&!b.p.packets);Unit pet;pet.ownerPlayer=&a.p;spell_stormheim_sundered cast;cast.caster=&pet;cast.target=&actors[0];assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);}
    {Visit v;auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,97482,1,0,0}};v.p.Accept(999);v.p.KilledMonsterCredit(97482);assert(v.p.GetQuestObjectiveData(999,97482)==1&&!v.p.GetQuestObjectiveData(QuestId,97482));}
    {Visit v;v.At(actors[0]);Creature copy;copy.guid={9999};copy.entry=97356;copy.map=1463;copy.zone=7588;copy.SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);copy.roots[{192214,0}]={};v.p.judgmentSummon=[&](uint32 e,Position const& pos,ObjectGuid viewer){assert(e==97356&&viewer==v.p.guid);static_cast<Position&>(copy)=pos;return &copy;};Cast(v,actors[0]);assert(v.p.packets==1&&copy.bound&&copy.personal==v.p.guid&&copy.roots.empty()&&!copy.gravityDisabled&&copy.tier==AnimTier::Ground&&copy.motion.falls==1&&!copy.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK)&&copy.casts==std::vector<uint32>{192495});assert(actors[0].roots.size()==1&&actors[0].world);}
    {Creature later;later.entry=97356;later.guid={555};later.dbGuid=1;later.map=1463;later.zone=7588;npc_stormheim_sundered a(&later);a.UpdateAI(500);SpellInfo hit;hit.Id=192493;a.SpellHit(&later,&hit);assert(a.delegated==2&&later.casts.empty());}
    std::cout<<"PASS: extracted production Sundered spell admission/effect and actor AI, native objective completion/reward, negative-group next39853 in both reward orders, both factions/owners, repeat/stale casts, failure retries, saved counters, abandon/login/map/phase boundaries, group/pet/generic guards, optional failures and private release without public despawn. World/cast engine/packets/motion are fixtures; client verification pending.\n";
}

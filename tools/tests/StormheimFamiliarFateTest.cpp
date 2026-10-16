#include <crtdbg.h>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,QUEST_ITEM_COUNT=4,SPELL_EFFECT_HANDLE_HIT_TARGET=1;
constexpr float INTERACTION_DISTANCE=5;
#include "FamiliarFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
void Creature::GetPlayerListInGrid(std::list<Player*>& out,float range){for(auto const& pair:players)if(pair.second->world&&pair.second->map==map&&GetExactDist(pair.second)<=range)out.push_back(pair.second);}
struct ItemTemplate{int GetMaxCount()const{return 0;}uint32 GetMaxStackSize()const{return 1;}} itemTemplate;
struct ObjectMgr{ItemTemplate* GetItemTemplate(uint32){return &itemTemplate;}} objectMgr;auto sObjectMgr=&objectMgr;
#include "HasQuestForItem.inc"
struct Item{uint32 entry=129106;ObjectGuid owner;uint32 GetEntry(){return entry;}ObjectGuid GetOwnerGUID(){return owner;}};
struct PlayerScript{explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}};
struct ShardSpell{
 Unit* caster=nullptr,*hit=nullptr;Item* item=nullptr;std::set<int> prevented;Hook OnEffectHitTarget;
 Unit* GetCaster(){return caster;}Unit* GetHitUnit(){return hit;}Item* GetCastItem(){return item;}
 void PreventHitDefaultEffect(int i){prevented.insert(i);}virtual void Register(){}
};
#define SpellScript ShardSpell
#define SpellEffectFn(x,i,t) (i*1000+t)
#include "FamiliarFate.inc"
#undef SpellScript
struct Spell{int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;Unit* unitTarget=nullptr;uint32 m_diffMode=0;SpellInfo data;SpellInfo* m_spellInfo=&data;void EffectKillCredit(SpellEffIndex);};
#include "SpellGroupCredit.inc"
enum{SMART_ACTION_CALL_KILLEDMONSTER=33};using ObjectList=std::vector<WorldObject*>;
bool IsUnit(WorldObject* o){return o&&o->ToUnit();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}
struct SmartDispatch{
 Creature* me=nullptr;struct Event{struct{struct{uint32 creature=97981;} killedMonster;} action;} e;
 WorldObject* GetBaseObject(){return me;}ObjectList* GetTargets(Event const&,Unit* p){return new ObjectList{p};}
 void Run(Unit* unit){bool trigger=false;switch(SMART_ACTION_CALL_KILLEDMONSTER){
#include "SmartGroupCredit.inc"
 }}
};
using namespace StormheimFamiliarFate;
Creature circle;std::unique_ptr<npc_stormheim_familiar_binding> circleAI;
uint64 serial=1000;
struct Visit{
 Player p;Item shard;player_stormheim_familiar_fate script;
 Visit(uint32 team=ALLIANCE){p.guid={++serial};p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(QuestId);p.menu.menu.menu=18828;p.menu.menu.item=true;static_cast<Position&>(p)=Circle;shard.owner=p.guid;players[p.guid]=&p;}
 ~Visit(){players.erase(p.guid);circleAI->UpdateAI(500);}
 void Collect(){p.bags[Shard]=1;spell_stormheim_familiar_shard spell;spell.caster=spell.hit=&p;spell.item=&shard;spell.Register();assert(spell.OnEffectHitTarget.values==std::vector<int>({134}));spell.Collect(0);assert(spell.prevented.count(0));}
 void Release(){circleAI->sGossipSelect(&p,18828,0);}
};
int main()
{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.objectives={{0,Collected,1,0,0},{0,Freed,1,0,1}};q.ItemDrop[0]=Shard;q.ItemDropQuantity[0]=1;q.NextQuestIdChain=40003;
 auto& next=store.quests[40003];next.id=40003;next.PrevQuestID=QuestId;next.prevQuests={int32(QuestId)};next.objectives={{0,97983,1,0,0}};store.starters.emplace(97979,40003);
 circle.entry=91743;circle.dbGuid=291393;circle.guid={291393};static_cast<Position&>(circle)=Circle;circle.fields[UNIT_FIELD_NPC_FLAGS]=1;circleAI=std::make_unique<npc_stormheim_familiar_binding>(&circle);
 for(auto team:{ALLIANCE,HORDE}){
  Visit v(team);assert(v.p.HasQuestForItem(Shard));v.Release();assert(!v.p.packets);v.Collect();assert(v.p.GetQuestObjectiveData(QuestId,Collected)==1&&v.p.packets==1&&!v.p.CanCompleteQuest(QuestId)&&!v.p.HasQuestForItem(Shard));
  v.Collect();assert(v.p.packets==1);v.Release();assert(v.p.packets==2&&v.p.completions==1&&v.p.CanRewardQuest(&q,false)&&v.p.bags[Shard]==1&&circle.hidden.count(v.p.guid));v.Release();assert(v.p.packets==2);
  Creature vethir;vethir.entry=97979;vethir.guid={291392};creatures[vethir.guid]=&vethir;
  assert(!v.p.CanTakeQuest(&next,false));v.p.m_RewardedQuests.insert(QuestId);v.p.quests.erase(QuestId);assert(v.p.GetNextQuest(vethir.guid,&q)==&next&&v.p.CanTakeQuest(&next,false));creatures.erase(vethir.guid);
 }
 {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group g{&first};a.p.group=b.p.group=&g;a.Collect();assert(a.p.packets==1&&!b.p.packets);a.Release();assert(a.p.completions==1&&!b.p.packets);b.Collect();b.Release();assert(b.p.completions==1);}
 // Execute both actual old group dispatchers:40002 is now guarded, unrelated
 // quest999 retains native party sharing with these same credit entries.
 {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group g{&first};a.p.group=b.p.group=&g;Spell native;native.unitTarget=&a.p;native.EffectKillCredit(0);SmartDispatch smart;smart.me=&circle;smart.Run(&a.p);assert(!a.p.packets&&!b.p.packets);
  auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives=q.objectives;a.p.Accept(999);b.p.Accept(999);native.EffectKillCredit(0);smart.Run(&a.p);assert(a.p.GetQuestObjectiveData(999,Collected)==1&&b.p.GetQuestObjectiveData(999,Collected)==1&&a.p.GetQuestObjectiveData(999,Freed)==1&&b.p.GetQuestObjectiveData(999,Freed)==1&&!a.p.GetQuestObjectiveData(QuestId,Collected)&&!b.p.GetQuestObjectiveData(QuestId,Freed));}
 {Visit v;v.p.KilledMonsterCredit(Collected);v.p.KilledMonsterCredit(Freed);v.p.TalkedToCreature(Collected,circle.guid);v.p.TalkedToCreature(Freed,circle.guid);assert(!v.p.packets);}
 {Visit v;v.Collect();v.p.bags.clear();assert(v.p.HasQuestForItem(Shard));v.Release();assert(v.p.packets==1);v.Collect();v.Release();assert(v.p.completions==1);} // lost shard after menu preparation
 {Visit v;v.p.bags[Shard]=1;v.p.inventoryVisible=false;v.script.OnUpdate(&v.p,1);assert(!v.p.packets);v.p.inventoryVisible=true;v.script.OnUpdate(&v.p,1);assert(v.p.packets==1);v.script.OnUpdate(&v.p,1);assert(v.p.packets==1);v.Release();assert(v.p.completions==1);} // loaded bag retry
 {Visit v;v.Collect();v.p.quests.erase(QuestId);v.p.bags.clear();v.Release();assert(v.p.packets==1);v.p.Accept(QuestId);v.Release();assert(v.p.packets==1);v.Collect();v.Release();assert(v.p.completions==1);} // native abandonment storage/item deletion modeled
 {Visit v;v.Collect();v.Release();auto status=v.p.quests;v.p.quests=status;v.script.OnUpdate(&v.p,1);v.Release();assert(v.p.packets==2&&v.p.completions==1);} // saved completed state
 {Visit a,b;a.Collect();a.Release();assert(circle.hidden.count(a.p.guid)&&!circle.hidden.count(b.p.guid)&&circle.world);a.p.quests.erase(QuestId);a.p.bags.clear();circleAI->UpdateAI(500);assert(!circle.hidden.count(a.p.guid));a.p.Accept(QuestId);a.Collect();a.Release();a.p.world=false;circleAI->UpdateAI(500);assert(!circle.hidden.count(a.p.guid));a.p.world=true;circleAI->UpdateAI(500);assert(circle.hidden.count(a.p.guid));} // private visibility cleanup/re-entry
 {Visit v;v.Collect();v.Release();v.p.map=1463;circleAI->UpdateAI(500);assert(!circle.hidden.count(v.p.guid));v.p.map=1220;circleAI->UpdateAI(500);assert(circle.hidden.count(v.p.guid));v.p.m_RewardedQuests.insert(QuestId);v.p.quests.erase(QuestId);circleAI->UpdateAI(500);assert(circle.hidden.count(v.p.guid));} // map transfer/rewarded state
 for(int fail=0;fail<18;++fail){Visit v;v.Collect();
  if(fail==0)v.p.alive=false;if(fail==1)v.p.world=false;if(fail==2)v.p.map=1463;if(fail==3)v.p.zone=1;if(fail==4)v.p.teleport=true;
  if(fail==5)v.p.combat=true;if(fail==6)v.p.flying=true;if(fail==7)v.p.vehicle=true;if(fail==8)v.p.transport=true;
  if(fail==9)v.p.phase=false;if(fail==10)v.p.visible=false;if(fail==11)v.p.los=false;if(fail==12)v.p.x+=18;
  if(fail==13)v.p.menu.menu.menu=12618;if(fail==14)v.p.menu.menu.item=false;if(fail==15)v.p.phases.insert(8306);if(fail==16)circle.alive=false;if(fail==17)circle.fields[UNIT_FIELD_NPC_FLAGS]=0;
  v.Release();assert(v.p.packets==1&&!v.p.completions);circle.alive=true;circle.fields[UNIT_FIELD_NPC_FLAGS]=1;
 }
 {Visit v;v.Collect();circleAI->sGossipSelect(&v.p,18828,1);circleAI->sGossipSelect(&v.p,12618,0);assert(v.p.packets==1);}
 {Visit v;v.Collect();v.p.x+=10;v.Release();assert(v.p.completions==1);} // native NPC interaction includes large binding combat reach
 {Visit v;v.p.KilledMonsterCredit(97890);assert(!v.p.packets&&v.p.HasQuestForItem(Shard));} // Dravax kill alone is not collection
 {Visit v;v.Collect();circle.dbGuid=999;auto calls=circleAI->delegated;v.Release();assert(circleAI->delegated==calls+1&&v.p.packets==1);circle.dbGuid=291393;}
 {Visit v;v.p.bags[Shard]=1;spell_stormheim_familiar_shard s;s.caster=s.hit=&v.p;s.Collect(0);assert(!v.p.packets);s.item=&v.shard;v.shard.owner={999};s.Collect(0);assert(!v.p.packets);v.shard.owner=v.p.guid;s.Collect(0);assert(v.p.packets==1);}
 {Visit v;v.p.bags[Shard]=1;auto saved=q.objectives;q.objectives[0].Amount=2;v.script.OnUpdate(&v.p,1);v.Release();assert(!v.p.packets);q.objectives=saved;v.p.phases.insert(7510);v.script.OnUpdate(&v.p,1);assert(v.p.packets==1);} // unrelated phase is not a base-story veto
 std::cout<<"PASS: full production shard/circle/reconciliation handlers and extracted native source-item eligibility, spell134, SmartAI33 and group/individual credit handlers. Both factions/party separation, stale item/menu, invalid source/context, repeat, abandonment, loaded-bag retry, saved complete and40003 handoff. Inventory storage/loading/reward mutation, NPC dispatch/network and condition admission are fixtures; renderer and actual client sequence remain pending.\n";
}

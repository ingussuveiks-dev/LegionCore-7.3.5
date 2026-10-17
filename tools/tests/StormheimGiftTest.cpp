#include <array>
#include <sstream>
#include "ShieldsFixture.inc"

// Native spell_area reevaluation and source17/condition41 are executed here.
// Area-map lookup, spell casting and inventory storage are fixture boundaries.
enum {CONDITION_QUEST_OBJECTIVE_DONE=41,CONDITION_ITEM=2};
bool NoDetonator(Player* object)
{
 uint32 ConditionValue1=128287,ConditionValue2=1,ConditionValue3=0;
 bool condMeets=false;switch(CONDITION_ITEM){
#include "NativeGiftItemCondition.inc"
 }return !condMeets; // Existing source17 row has NegativeCondition=1.
}
bool CreatorCondition(Player* object)
{
 uint32 ConditionValue1=39385,ConditionValue2=243455,ConditionValue3=3;
 bool condMeets=false;switch(CONDITION_QUEST_OBJECTIVE_DONE){
#include "NativeGiftCondition.inc"
 }return condMeets;
}
struct SpellArea {uint32 areaId=7773,spellId=189152;bool autocast=true;
 bool IsFitToRequirements(Player* p,uint32,uint32 area){return area==areaId&&p->GetQuestStatus(39385)==QUEST_STATUS_INCOMPLETE;}} creatorArea;
using SpellAreaForAreaMap=std::multimap<uint32,SpellArea*>;
using SpellAreaForQuestMapBounds=std::pair<SpellAreaForAreaMap::const_iterator,SpellAreaForAreaMap::const_iterator>;
struct GiftAreaMgr {SpellAreaForAreaMap starts,ends;SpellAreaForQuestMapBounds GetSpellAreaForQuestMapBounds(uint32 q){return starts.equal_range(q);}SpellAreaForQuestMapBounds GetSpellAreaForQuestEndMapBounds(uint32 q){return ends.equal_range(q);}} areaMgr;
auto sSpellMgr=&areaMgr;
#include "NativeQuestUpdate.inc"
int Player::CastSpell(Player*,uint32 spell,bool)
{
 assert(spell==189152);casts.push_back(spell);
 if(!castResult&&!bagFull&&CreatorCondition(this)&&NoDetonator(this))++bags[128287];
 return castResult;
}
struct GiftCast {struct Targets {Player* target=nullptr;Player* destination=nullptr;void SetUnitTarget(Player* p){target=p;}void SetDst(Player& p){destination=&p;}} m_targets;};
struct GiftSpellBase {Unit* caster=nullptr;Item* item=nullptr;GiftCast spell;Hook OnCheckCast;
 virtual bool Load(){return true;}virtual void Register(){}Unit* GetCaster(){return caster;}Item* GetCastItem(){return item;}GiftCast* GetSpell(){return &spell;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}};
#define SpellScript GiftSpellBase
#define SpellCheckCastFn(F) 1
#include "GiftProduction.inc"
#undef SpellScript
#include "GiftTargeting.inc"

// Actual SAI owner target and action33; selection of JUST_SUMMONED event54
// and creation/ownership of the summon are fixtures checked against world.tsv.
enum {SMART_TARGET_OWNER_OR_SUMMONER=23,SMART_ACTION_CALL_KILLEDMONSTER=33};
using ObjectList=std::list<WorldObject*>;
bool IsUnit(WorldObject* o){return o&&o->ToUnit();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}
struct Event {struct {struct {uint32 creature=95412;} killedMonster;} action;};
struct Controller {
 Creature* me;bool trigger=false;WorldObject* GetBaseObject(){return me;}
 ObjectList* GetTargets(Event const&,Unit*){auto l=new ObjectList;switch(SMART_TARGET_OWNER_OR_SUMMONER){
#include "NativeGiftOwner.inc"
 }return l;}
 void Credit(Unit* unit=nullptr){Event e;switch(SMART_ACTION_CALL_KILLEDMONSTER){
#include "NativeGiftCredit.inc"
 }}
};
void Init(Player& p,uint32 id=1)
{
 p.team=HORDE;p.race=2;p.area=7773;p.guid={id};players[p.guid]=&p;p.Accept(39385);
}
void Tick(Player& p){auto q=std::move(p.events);p.events.clear();for(auto& e:q)e();}
void Reset(GameObject& go){go.flags=0;go.distance=0;}
void ReadyPlayer(Player& p,uint32 id=1){Init(p,id);p.data[{39385,243455}]=3;}
void TargetingTest()
{
 Targeting::SpellImplicitTargetInfo target;
 {Targeting::Spell s;s.SelectImplicitNearbyTargets(0,target,1);assert(s.failed&&s.searches==1&&!s.m_targets.has);}
 {Targeting::Spell s;Targeting::Unit unrelated;unrelated.x=999;s.found=&unrelated;s.SelectImplicitNearbyTargets(0,target,1);assert(!s.failed&&s.m_targets.dst.target_X==999&&s.searches==1);}
 for(float x:{2396.f,2402.f,2427.f}){Targeting::Spell s;s.caster.x=x;s.m_targets.target=&s.caster;s.SelectImplicitNearbyTargets(0,target,1);assert(!s.failed&&!s.searches&&s.m_targets.has&&s.m_targets.dst.target_X==x);}
}
int main(int argc,char** argv)
{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;unsigned controller=0;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,ex,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 id,qid;int index;QuestObjective o;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[qid].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
  if(kind=="summon-credit"){uint32 event,action,credit,target;in>>event>>action>>credit>>target;assert(event==54&&action==33&&credit==95412&&target==23);++controller;}
 }
 assert(controller==1);areaMgr.starts.emplace(39385,&creatorArea);auto& q=store.quests[39385];q.special|=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;assert(q.objectives.size()==2);
 QuestMap _questTemplates;for(auto& r:store.quests)_questTemplates[r.first]=&r.second;for(auto& r:_questTemplates){auto qinfo=r.second;
#include "QuestPrevLoader.inc"
 }
 std::array<GameObject,3> gos;std::vector<std::unique_ptr<go_stormheim_gift_planting>> ais;
 for(unsigned i=0;i<3;++i){auto& g=gos[i];g.entry=g.info.entry=243455;g.info.goober.questID=0;g.dbGuid=127738+i;g.area=7773;ais.emplace_back(std::make_unique<go_stormheim_gift_planting>(&g));g.scripted=ais.back().get();}
 // Reproduce old stationary third-use failure; manual area reevaluation works.
 {Player p;Init(p);p.SetQuestUpdate(39385);assert(!p.bags[128287]);for(auto& g:gos){Reset(g);auto ai=g.scripted;g.scripted=nullptr;g.Use(&p);g.scripted=ai;}assert(p.GetQuestObjectiveData(39385,243455)==3&&!p.GetItemCount(128287)&&p.casts.size()==1);p.SetQuestUpdate(39385);assert(p.GetItemCount(128287)==1);}
 // Every planting order, delayed native GO credit, item suppression and final summon.
 std::array<unsigned,3> order{0,1,2};unsigned permutations=0;
 do{Player p;Init(p);unsigned n=0;for(auto i:order){Reset(gos[i]);gos[i].Use(&p);++n;assert(p.GetQuestObjectiveData(39385,243455)==int(n)&&!p.GetItemCount(128287));Tick(p);assert(p.GetItemCount(128287)==uint32(n==3));}
  StormheimGift::Refresh(&p);assert(p.GetItemCount(128287)==1&&p.casts.size()==1&&!p.CanCompleteQuest(39385));
  Item item;spell_stormheim_gift_detonator s;s.caster=&p;s.item=&item;Player unrelated;s.spell.m_targets.target=s.spell.m_targets.destination=&unrelated;assert(s.Load()&&s.Check()==SPELL_CAST_OK&&s.spell.m_targets.target==&p&&s.spell.m_targets.destination==&p);assert(!p.GetQuestObjectiveData(39385,95412)); // Casting alone is no credit.
  Creature c;c.owner=&p;Controller{&c}.Credit();assert(p.GetQuestObjectiveData(39385,95412)==1&&p.GetQuestStatus(39385)==QUEST_STATUS_COMPLETE&&p.CanRewardQuest(&q,false));Controller{&c}.Credit();assert(p.completions==1&&s.Check()!=SPELL_CAST_OK);++permutations;
 }while(std::next_permutation(order.begin(),order.end()));assert(permutations==6);
 {Player p;ReadyPlayer(p);p.bagFull=true;StormheimGift::Refresh(&p);assert(!p.GetItemCount(128287));p.bagFull=false;Reset(gos[0]);gos[0].Use(&p);Tick(p);assert(p.GetItemCount(128287)==1&&p.GetQuestObjectiveData(39385,243455)==3);}
 {Player p;ReadyPlayer(p);p.bank[128287]=1;StormheimGift::Refresh(&p);assert(p.casts.empty()&&!p.bags[128287]);}
 {Player p;ReadyPlayer(p);StormheimGift::Queue(&p,1);p.Accept(39385);p.data[{39385,243455}]=3;Tick(p);assert(!p.bags[128287]);player_stormheim_gift_refresh login;login.OnLogin(&p);Tick(p);assert(p.bags[128287]==1);}
 {Player p;ReadyPlayer(p);StormheimGift::Queue(&p,1);p.area=1;Tick(p);assert(!p.bags[128287]);}
 {Player p;ReadyPlayer(p);player_stormheim_gift_refresh login;login.OnLogin(&p);p.world=false;Tick(p);assert(!p.bags[128287]);}
 {Player p;ReadyPlayer(p);Item item;spell_stormheim_gift_detonator s;s.caster=&p;s.item=&item;assert(s.Check()==SPELL_CAST_OK);p.Accept(39385);p.data[{39385,243455}]=3;assert(s.Check()!=SPELL_CAST_OK);}
 for(unsigned why=0;why<13;++why){Player p;ReadyPlayer(p);Item item;spell_stormheim_gift_detonator s;s.caster=&p;s.item=&item;
  if(why==0)p.team=ALLIANCE;if(why==1)p.map=1463;if(why==2)p.zone=1;if(why==3)p.area=1;if(why==4)p.world=false;if(why==5)p.alive=false;if(why==6)p.quests.erase(39385);if(why==7)p.data[{39385,243455}]=2;if(why==8)p.data[{39385,95412}]=1;if(why==9)s.item=nullptr;if(why==10)item.entry=1;if(why==11)p.quests[39385].Status=QUEST_STATUS_COMPLETE;
  if(why==12)p.instance=1;
  assert(s.Check()!=SPELL_CAST_OK&&s.spell.m_targets.target==nullptr);StormheimGift::Refresh(&p);if(why!=9&&why!=10)assert(p.casts.empty());
 }
 for(unsigned why=0;why<6;++why){Player p;Init(p);auto& g=gos[0];Reset(g);if(why==0)p.phase=false;if(why==1)g.distance=6;if(why==2)g.dbGuid=1;if(why==3)g.map=1;if(why==4)g.spawned=false;if(why==5)g.flags=GO_FLAG_IN_USE;ais[0]->GossipUse(&p);assert(p.events.empty());g.dbGuid=127738;g.map=1220;g.spawned=true;Reset(g);}
 // Preserve original group behavior while mandatory GO3 remains unshared.
 {Player p,near;ReadyPlayer(p,1);Init(near,2);GroupReference b{&near,nullptr},a{&p,&b};Group group;group.first=&a;p.group=&group;Creature c;c.owner=&p;Controller{&c}.Credit();assert(p.GetQuestStatus(39385)==QUEST_STATUS_COMPLETE&&near.GetQuestObjectiveData(39385,95412)==1&&!near.CanCompleteQuest(39385)&&!near.GetQuestObjectiveData(39385,243455));}
 {Player p;ReadyPlayer(p);Creature c;Controller{&c}.Credit();assert(!p.GetQuestObjectiveData(39385,95412));}
 // Native delivery prerequisite and zero-objective completion.
 {Player p;Init(p);p.quests.clear();assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(38872);assert(p.CanTakeQuest(&q,false));auto& travel=store.quests[39154];auto& shields=store.quests[38878];assert(!p.CanTakeQuest(&travel,false));p.m_RewardedQuests.insert(39385);assert(p.CanTakeQuest(&travel,false));p.Accept(39154);assert(p.CanCompleteQuest(39154)&&!p.CanTakeQuest(&shields,false));p.quests.clear();p.m_RewardedQuests.insert(39154);assert(p.CanTakeQuest(&shields,false));Creature n;n.entry=93603;n.guid={293893};creatures[n.guid]=&n;assert(p.GetNextQuest(n.guid,&q)==&travel);n.entry=94227;assert(p.GetNextQuest(n.guid,&travel)==&shields);Player alliance;alliance.m_RewardedQuests=p.m_RewardedQuests;assert(!alliance.CanTakeQuest(&q,false)&&!alliance.CanTakeQuest(&travel,false));}
 TargetingTest();
 std::cout<<"PASS: stationary third-use failure reproduced; actual Gift callbacks, native goober/quest-update/condition41/target46/owner23/action33/group/prerequisite branches; six planting orders, bag-full retry, bank suppression, login/abandon/area/invalid-cast boundaries, summon-only final credit and39385->39154->38878 handoff. Casting/inventory/world/event selection are fixtures; client pending.\n";
}

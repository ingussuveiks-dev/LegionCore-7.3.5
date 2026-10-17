#include <sstream>
#include "ShieldsFixture.inc"
constexpr float INTERACTION_DISTANCE=5;
#include "DreadwakeProduction.inc"
struct ConditionSourceInfo {WorldObject* targets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b=nullptr):targets{a,b}{}};
struct Condition {uint32 ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ElseGroup=0,ReferenceId=0;bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo& info){auto object=info.targets[ConditionTarget];bool condMeets=false;switch(ConditionType){
#include "QuestConditions.inc"
}return condMeets;}};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(WorldObject*,WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const;} conditions;
#include "ShieldsConditionGroups.inc"
namespace MenuAdmission {
#include "DreadwakeGossipEnums.inc"
struct GossipMenu {static uint64 GetRequiredNpcFlagForOption(GossipOptionNpc);};
#include "DreadwakeGossipFlags.inc"
bool Admits(uint32 npcflags){for(auto optionNpc:{GossipOptionNpc(0)}){
#include "DreadwakeGossipAdmission.inc"
 return true;}return false;}
}

// Execute the full native taxi activation. DB lookups, session packets,
// movement dispatch and aura/vehicle cleanup are observable fixtures.
namespace Taxi {
using int64=int64_t;
enum {ERR_TAXINOSUCHPATH=1,ERR_TAXIPLAYERBUSY,UNIT_STATE_STUNNED=1,UNIT_STATE_ROOT=2,
 UNIT_FLAG_REMOVE_CLIENT_CONTROL=4,SPELL_AURA_MOUNTED,SPELL_AURA_MOD_SHAPESHIFT,CURRENT_GENERIC_SPELL,
 CURRENT_AUTOREPEAT_SPELL,CURRENT_CHANNELED_SPELL,ERR_TAXIUNSPECIFIEDSERVERERROR,ERR_TAXINOTVISITED,
 TAXI_NODE_FLAG_USE_FAVORITE_MOUNT=4,CLASS_DEATH_KNIGHT=6,ERR_TAXINOTENOUGHMONEY=17,
 CRITERIA_TYPE_FLIGHT_PATHS_TAKEN,CONFIG_INSTANT_TAXI,CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING,ERR_TAXIOK};
struct Spell {struct Info {uint32 Id=0;} info;Info* m_spellInfo=&info;};
struct TaxiNodesEntry {struct {float X=0,Y=0,Z=0;}Pos;uint32 Flags=0,ContinentID=1220;};
struct Nodes {TaxiNodesEntry node;bool present=true;TaxiNodesEntry const* LookupEntry(uint32 id){return present&&id==1722?&node:nullptr;}} nodes;
auto& sTaxiNodesStore=nodes;
struct TaxiPathEntry {uint32 FromTaxiNode=1722,ToTaxiNode=1722;};struct Paths {TaxiPathEntry entry;TaxiPathEntry const* LookupEntry(uint32 id){return id==5262?&entry:nullptr;}} paths;auto& sTaxiPathStore=paths;
struct ObjectMgr {bool path=true,mount=true;uint32 cost=0;void GetTaxiPath(uint32 from,uint32 to,uint32& id,uint32& price){assert(from==1722&&to==1722);id=path?5262:0;price=cost;}uint32 GetTaxiMountDisplayId(uint32 id,uint32 team,bool allow){assert(id==1722&&team==HORDE&&allow);return mount?60307:0;}} mgr;auto sObjectMgr=&mgr;
struct World {bool instant=false;bool getBoolConfig(int){return instant;}} world;auto sWorld=&world;
struct Session {bool loggingOut=false;uint32 flights=0,mount=0,path=0,reply=0;bool isLogingOut(){return loggingOut;}void SendActivateTaxiReply(uint32 n){reply=n;}void SendDoFlight(uint32 m,uint32 p){++flights;mount=m;path=p;}};
struct TaxiPlayer {
 Session session;bool combat=false,root=false,stun=false,blocked=false;uint32 cleanups=0;int64 money=0;std::vector<std::function<void()>> callbacks;
 struct TaxiState {std::vector<uint32> destinations;void ClearTaxiDestinations(){destinations.clear();}void AddTaxiDestination(uint32 n){destinations.push_back(n);}}m_taxi;
 bool ActivateTaxiPathTo(std::vector<uint32>const&,Creature* =nullptr,uint32=0,uint32=0);
 bool ActivateTaxiPathTo(uint32,uint32=0);
 Session* GetSession(){return &session;}bool isInCombat(){return combat;}bool HasUnitState(int f){return f==UNIT_STATE_ROOT?root:stun;}bool HasFlag(int,int){return blocked;}
 void RemoveAurasByType(int){++cleanups;}bool IsInDisallowedMountForm(){return false;}Spell* GetCurrentSpell(int){return nullptr;}void InterruptSpell(int,bool){}
 void CombatStop(){++cleanups;}void StopCastingCharm(){++cleanups;}void StopCastingBindSight(){++cleanups;}void ExitVehicle(){++cleanups;}void TradeCancel(bool){++cleanups;}
 uint32 GetTeam(){return HORDE;}uint32 getClass(){return 1;}uint64 GetMoney(){return money;}float GetReputationPriceDiscount(Creature*){return 1;}
 void ModifyMoney(int64 n){money+=n;}template<class...T>void UpdateAchievementCriteria(T...){}void AddDelayedEvent(uint32,std::function<void()> f){callbacks.push_back(f);}void TeleportTo(uint32,float,float,float,float){}float GetOrientation(){return 0;}
};
#include "DreadwakeNativeTaxi.inc"
#include "DreadwakeTaxiOverload.inc"
}
bool Player::ActivateTaxiPathTo(uint32 path)
{
 assert(path==5262);++taxiCalls;Taxi::TaxiPlayer native;native.combat=combat;native.root=rooted;native.stun=stunned;native.session.loggingOut=loggingOut;native.blocked=controlBlocked;
 bool result=native.ActivateTaxiPathTo(path);
 if(result){assert(native.session.mount==60307&&native.session.path==5262&&native.session.flights==1&&native.money==0);flight=true;++taxiStarts;}
 return result;
}
// The existing catapult uses spell effect134, not a custom kill callback.
struct CreditSpell {int effectHandleMode=1,m_diffMode=0;Unit* unitTarget=nullptr;struct Info {struct Effect {int32 MiscValue=95391;}effect;Effect* GetEffect(int,int){return &effect;}}info;Info* m_spellInfo=&info;void EffectKillCredit(int);};
constexpr int SPELL_EFFECT_HANDLE_HIT_TARGET=1;
#define Spell CreditSpell
#include "DreadwakeNativeCredit.inc"
#undef Spell
enum {NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4};
ObjectGuid GetOwnerGUID(){return {};}
void Fire(Creature* catapult,Unit* clicker,bool castSucceeded=true)
{
 struct Click {uint32 castFlags=1;};std::map<unsigned,Click> rows{{95212,{}}};auto itr=rows.begin();
#include "DreadwakeClickActors.inc"
 assert(caster==clicker&&target==catapult&&origCasterGUID==clicker->GetGUID());
 // Native target1 is caster; the successful hit dispatch is a fixture.
 if(castSucceeded){CreditSpell spell;spell.unitTarget=caster;spell.EffectKillCredit(1);}
}
void Init(Player& p){p.team=HORDE;p.race=2;p.menu.menu.menu=18657;p.menu.menu.item=true;p.Accept(38872);}
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;unsigned conditionsSeen=0;std::list<Condition> menuRows;ConditionList gates;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,ex,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 qid,id;int index;QuestObjective o;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[qid].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,qid;in>>entry>>qid;store.starters.emplace(entry,qid);}
  if(kind=="condition"){unsigned group,type,target,quest,v2,v3,negative;in>>group>>type>>target>>quest>>v2>>v3>>negative;assert(quest==38872&&!v2&&!v3&&!negative&&!target&&((group==0&&type==9)||(group==1&&type==28)));menuRows.emplace_back();auto& c=menuRows.back();c.ConditionType=type;c.ConditionValue1=quest;c.ElseGroup=group;gates.push_back(&c);++conditionsSeen;}
 }assert(conditionsSeen==2);
 for(auto id:{38872u,38873u})store.quests[id].special|=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;
 QuestMap _questTemplates;for(auto& r:store.quests)_questTemplates[r.first]=&r.second;for(auto& r:_questTemplates){auto qinfo=r.second;
#include "QuestPrevLoader.inc"
 }
 auto& delivery=store.quests[38872];auto& clear=store.quests[38873];auto& travel=store.quests[39154];assert(delivery.objectives.size()==1&&clear.objectives.size()==1);
 Creature c;c.entry=93624;c.dbGuid=291298;c.fields[UNIT_FIELD_NPC_FLAGS]=3;npc_stormheim_dreadwake_cullen ai(&c);
 assert(MenuAdmission::Admits(3)&&MenuAdmission::Admits(1)&&!MenuAdmission::Admits(2));
 for(int status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){Player p;Init(p);p.quests[38872].Status=status;assert(conditions.IsObjectMeetToConditions(&p,gates)==(status==QUEST_STATUS_INCOMPLETE||status==QUEST_STATUS_COMPLETE));}
 // Original unbound template/empty SmartAI consumes no transport action.
 {Player p;Init(p);SmartAI old(&c);old.sGossipSelect(&p,18657,0);assert(!p.flight&&!p.taxiCalls);}
 for(int status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){Player p;Init(p);p.quests[38872].Status=status;ai.sGossipSelect(&p,18657,0);assert(p.flight&&p.taxiStarts==1&&p.menu.closed==1&&!p.GetQuestObjectiveData(38872,93624));ai.sGossipSelect(&p,18657,0);assert(p.taxiStarts==1);}
 // Optional/hidden credit does not block delivery, and native taxi failure retries.
 {Player p;Init(p);assert(p.CanCompleteQuest(38872)&&!p.GetQuestObjectiveData(38872,93624));p.combat=true;ai.sGossipSelect(&p,18657,0);assert(p.taxiCalls==1&&!p.flight&&!p.menu.closed);p.combat=false;ai.sGossipSelect(&p,18657,0);assert(p.flight&&p.taxiCalls==2);}
 for(unsigned why=0;why<19;++why){Player p;Init(p);if(why==0)p.team=ALLIANCE;if(why==1)p.map=1463;if(why==2)p.instance=1;if(why==3)p.zone=1;if(why==4)p.world=false;if(why==5)p.alive=false;if(why==6)p.phase=false;if(why==7)c.distance=6;if(why==8)p.teleport=true;if(why==9)p.vehicle=true;if(why==10)p.transport=true;if(why==11)p.flight=true;if(why==12)p.quests.erase(38872);if(why==13)p.quests[38872].Status=QUEST_STATUS_REWARDED;if(why==14)p.menu.menu.menu=1;if(why==15)p.menu.menu.item=false;if(why==16)c.dbGuid=1;if(why==17)c.alive=false;if(why==18)c.fields[UNIT_FIELD_NPC_FLAGS]=2;
  ai.sGossipSelect(&p,18657,0);assert(!p.taxiCalls);c.distance=0;c.dbGuid=291298;c.alive=true;c.fields[UNIT_FIELD_NPC_FLAGS]=3;}
 {Player p;Init(p);auto n=ai.delegated;ai.sGossipSelect(&p,1,0);assert(ai.delegated==n+1&&!p.taxiCalls);}
 for(unsigned why=0;why<7;++why){Taxi::TaxiPlayer p;if(why==0)p.root=true;if(why==1)p.stun=true;if(why==2)p.session.loggingOut=true;if(why==3)p.blocked=true;if(why==4)Taxi::nodes.present=false;if(why==5)Taxi::mgr.path=false;if(why==6)Taxi::mgr.mount=false;assert(!p.ActivateTaxiPathTo({1722,1722})&&!p.session.flights);Taxi::nodes.present=true;Taxi::mgr.path=Taxi::mgr.mount=true;}
 {Taxi::TaxiPlayer p;assert(!p.ActivateTaxiPathTo({1722}));Taxi::mgr.cost=1;assert(!p.ActivateTaxiPathTo({1722,1722}));Taxi::mgr.cost=0;assert(p.ActivateTaxiPathTo({1722,1722}));}
 // Preserve native effect134 group credit and the four mandatory firings.
 {Player p,near;Init(p);Init(near);p.quests.clear();near.quests.clear();p.Accept(38873);near.Accept(38873);GroupReference b{&near,nullptr},a{&p,&b};Group group;group.first=&a;p.group=&group;Creature catapult;catapult.entry=95212;
  Fire(&catapult,&p,false);assert(!p.GetQuestObjectiveData(38873,95391));for(int n=1;n<=4;++n){Fire(&catapult,&p);assert(p.GetQuestObjectiveData(38873,95391)==n&&near.GetQuestObjectiveData(38873,95391)==n&&p.completions==unsigned(n==4));}Fire(&catapult,&p);assert(p.completions==1&&p.GetQuestObjectiveData(38873,95391)==4);Unit pet;pet.ownerPlayer=&p;Fire(&catapult,&pet);assert(p.completions==1);}
 // Both positive incoming39154 alternatives must be loaded, not just PrevQuestID39385.
 for(unsigned reward:{0u,38872u,38873u,39385u}){Player p;Init(p);p.quests.clear();if(reward)p.m_RewardedQuests.insert(reward);assert(p.CanTakeQuest(&travel,false)==(reward==38873||reward==39385));}
 for(unsigned previous:{38873u,39385u})for(int status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){Player p;Init(p);p.quests.clear();p.Accept(previous);p.quests[previous].Status=status;assert(!p.CanTakeQuest(&travel,false));}
 {Player p;Init(p);p.quests.clear();p.m_RewardedQuests.insert(38873);p.m_RewardedQuests.insert(39385);assert(p.CanTakeQuest(&travel,false));}
 {Player p;Init(p);p.quests.clear();assert(!p.CanTakeQuest(&clear,false));p.m_RewardedQuests.insert(38872);assert(p.CanTakeQuest(&clear,false));p.m_RewardedQuests.insert(38873);Creature n;n.entry=93603;n.guid={293893};creatures[n.guid]=&n;assert(p.GetNextQuest(n.guid,&clear)==&travel);p.Accept(39154);assert(p.CanCompleteQuest(39154));Player a;a.m_RewardedQuests=p.m_RewardedQuests;assert(!a.CanTakeQuest(&clear,false)&&!a.CanTakeQuest(&travel,false));}
 std::cout<<"PASS: actual Cullen selector + full native taxi activation reproduce old no-op, free route/mount, incomplete/complete delivery, busy/retry/duplicate/source guards; native effect134 group4/4, cap/pet boundaries and both38873/39385 alternative39154 prerequisites. Session/movement/world/inventory remain fixtures; client pending.\n";
}

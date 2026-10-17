// Executes extracted production handlers. World, spell engine, inventory slot
// allocation, SmartAI target selection and packet serialization are fixtures.
#include <vector>
#include <cstdint>
struct Item;struct OutdoorPvP;struct SmartScript;
using ItemPosCountVec=std::vector<uint32_t>;using InventoryResult=int;
enum {NULL_BAG=0,NULL_SLOT=0,EQUIP_ERR_OK=0,OBJECT_FIELD_GUID=0};
#include "OverlookFixture.inc"
#include "NativeDilemmaGetters.inc"
enum {QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE=99,QUEST_DEFAULT_SAVE_TYPE=1};
struct CriteriaTree{};struct CriteriaMgr {CriteriaTree const* GetCriteriaTree(uint32){return nullptr;}} criteriaMgr;auto sAchievementMgr=&criteriaMgr;
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple {uint32 QuestID=0,ObjectID=0,ObjectiveType=0;int Write(){return 0;}};}}
#include "NativeDilemmaSetter.inc"
#include "NativeRewardPlayer.inc"
enum {CONFIG_QUEST_IGNORE_AUTO_ACCEPT=0,QUEST_FLAGS_AUTO_ACCEPT=0x80000};
struct World {bool ignoreAutoAccept=false;bool getBoolConfig(int){return ignoreAutoAccept;}} world;auto sWorld=&world;
#include "NativeAutoAccept.inc"
void DispatchRewardHandoff(Player* _player,Object* object,Quest const* quest,ObjectGuid guid){struct {ObjectGuid QuestGiverGUID;uint32 ItemChoiceID=0;} packet{guid};
#include "NativeRewardHandoff.inc"
}
#include "NativeItemAddedQuestCheck.inc"
#include "NativeItemRemovedQuestCheck.inc"
struct ChatHandler {Player* player;explicit ChatHandler(Player* p):player(p){}void PSendSysMessage(char const*){++player->storageErrors;}};
InventoryResult Player::CanStoreNewItem(int,int,ItemPosCountVec& dest,uint32,uint32 count,uint32* missing){*missing=storageFull?count:0;if(!storageFull)dest.push_back(count);return storageFull?1:EQUIP_ERR_OK;}
Item* Player::StoreNewItem(ItemPosCountVec const& dest,uint32 entry,bool,int){if(storeFails)return nullptr;static Item item;bags[entry]+=dest.at(0);ItemAddedQuestCheck(entry,dest.at(0));return &item;}
#include "NativeAddItem.inc"
enum {SMART_EVENT_ON_SPELLCLICK=73,SMART_ACTION_ADD_ITEM=56,NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,MAX_SPELL_EFFECTS=32,SPELL_AURA_CONTROL_VEHICLE=236,SPELLVALUE_BASE_POINT0=0,MAX_EFFECT_MASK=0xffffffff};
int SpellValueMod(int n){return n;}
struct OutdoorPvP {void HandleSpellClick(Player*,Unit*){}};
struct SpellClickInfo {uint32 spellId=181532,castFlags=1;bool IsFitToRequirements(Unit*,Unit*)const{return true;}};
using SpellClickInfoContainer=std::multimap<uint32,SpellClickInfo>;
using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr {SpellClickInfoContainer clicks;SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 entry){return clicks.equal_range(entry);}} objectMgr;auto sObjectMgr=&objectMgr;
using ConditionList=std::vector<int>;struct ConditionSourceInfo {ConditionSourceInfo(Unit*,Unit*){}};
struct ConditionMgr {bool allowed=true;ConditionList GetConditionsForSpellClickEvent(uint32,uint32){return {};}bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList&){return allowed;}} conditionMgr;auto sConditionMgr=&conditionMgr;
struct SpellMgr {SpellInfo spell;SpellInfo const* GetSpellInfo(uint32 id){spell.Id=id;return &spell;}} spellMgr;auto sSpellMgr=&spellMgr;
int Unit::CastSpell(Unit* target,SpellInfo const* spell,bool triggered,void*,void*,ObjectGuid){return CastSpell(target,spell->Id,triggered);}
using ObjectList=std::vector<WorldObject*>;
struct Event {struct {struct {uint32 cooldownMin=0,cooldownMax=0;} spellclick;} event;struct {struct {uint32 entry=122581,count=1;} item;} action;};
struct SmartScript {
 Event e;uint32 timers=0;
 bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}
 ObjectList* GetTargets(Event&,Unit* u){auto list=new ObjectList;if(u)list->push_back(u);return list;}
 void RecalcTimer(Event&,uint32 min,uint32 max){assert(!min&&!max);++timers;}
 void ProcessAction(Event& e,Unit* unit,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr){switch(SMART_ACTION_ADD_ITEM){
#include "NativeAddItemAction.inc"
 }}
 void ProcessEventsFor(int kind,Unit* unit){uint32 var0=0,var1=0;bool bvar=false;SpellInfo const* spell=nullptr;GameObject* gob=nullptr;switch(kind){
#include "NativeClickEvent.inc"
 }}
} smart;
SmartScript* SmartAI::GetScript(){return &smart;}
#include "NativeOnSpellClick.inc"
#include "NativeSpellClick.inc"

// Native waypoint loader, with map movement/spline and manager adapters.
struct PatrolCreature {uint32 path=0,entry=91460;uint32 GetWaypointPath(){return path;}uint32 GetEntry(){return entry;}};
struct WaypointManager {std::map<uint32,std::vector<Position>> paths;auto GetPath(uint32 id){auto it=paths.find(id);return it==paths.end()?nullptr:&it->second;}auto GetPathScript(uint32 id){return GetPath(id);}} waypointMgr;auto sWaypointMgr=&waypointMgr;
struct Time {uint32 value=0;void Reset(uint32 n){value=n;}};
template<class T>struct WaypointMovementGenerator;
template<>struct WaypointMovementGenerator<PatrolCreature> {bool _loadedFromDB=true,_forceTPToStart=false;uint32 _pathId=0,starts=0;std::vector<Position>* _path=nullptr;Time _nextMoveTime;bool CanMove(PatrolCreature&){return true;}void StartMoveNow(PatrolCreature&){++starts;}void LoadPath(PatrolCreature&);};
#define TC_LOG_WARN(...) (void)0
#include "NativePatrolLoad.inc"

void Init(Player& p,unsigned id=100){p.guid={id};p.guid.high=HighGuid::Player;p.team=HORDE;p.race=2;}
void Kill(Player& p,uint32 entry,bool petKill,uint64 serial){Creature v;v.entry=entry;v.guid={serial};v.data.Entry=entry;creatures[v.guid]=&v;Unit pet;pet.ownerPlayer=&p;assert(ResolveKiller(petKill?pet:static_cast<Unit&>(p),&v).first==&p);KillRewarder r;r._victim=&v;r._RewardPlayer(&p,false);Deliver(p);assert(v.IsPlayerRewarded(p.GetGUID()));r._RewardPlayer(&p,false);assert(p.events.empty());creatures.erase(v.guid);}
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream file(argv[1]);assert(file);std::string line;uint32 boundPath=0;
 while(std::getline(file,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,ex,item,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>item>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 qid,id;int slot;QuestObjective o;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>slot>>o.Flags;o.StorageIndex=int8(slot);store.quests[qid].objectives.push_back(o);}
  if(kind=="patrol-addon"){uint32 guid,path;in>>guid>>path;if(guid==288637)boundPath=path;}
  if(kind=="patrol"){uint32 id,point;Position p;in>>id>>point>>p.x>>p.y>>p.z;waypointMgr.paths[id].push_back(p);}
 }
 QuestMap _questTemplates;for(auto& r:store.quests)_questTemplates[r.first]=&r.second;for(auto& r:_questTemplates){auto qinfo=r.second;
#include "QuestPrevLoader.inc"
 }
 auto& intel=store.quests[38308];auto& leaders=store.quests[38317];intel.special|=QUEST_SPECIAL_FLAGS_DELIVER;leaders.special|=QUEST_SPECIAL_FLAGS_KILL;
 assert(intel.HasFlag(QUEST_FLAGS_AUTOCOMPLETE)&&!leaders.HasFlag(QUEST_FLAGS_AUTOCOMPLETE)&&leaders.IsAutoAccept()&&intel.objectives.at(0).StorageIndex==3&&leaders.objectives.size()==2);
 {Player p;Init(p);p.m_RewardedQuests.insert(38308);world.ignoreAutoAccept=true;DispatchRewardHandoff(&p,&p,&intel,p.guid);assert(p.GetQuestStatus(38317)==QUEST_STATUS_NONE&&p.popup==38317&&p.menu.details==1);world.ignoreAutoAccept=false;}
 objectMgr.clicks.emplace(91470,SpellClickInfo{});Creature corpse;corpse.entry=91470;SmartAI corpseAI(&corpse);
 // Failed/full storage is retryable; the cosmetic cast is independent of ADD_ITEM.
 for(int failure=0;failure<3;++failure){Player p;Init(p,100+failure);p.m_RewardedQuests.insert(38362);assert(p.CanTakeQuest(&intel,false));p.AddQuestAndCheckCompletion(&intel,nullptr);assert(!p.CanRewardQuest(&intel,false));
  p.storageFull=failure==0;p.storeFails=failure==1;p.castResult=failure==2?1:0;
  assert(corpse.HandleSpellClick(&p));assert(p.casts.back()==181532);
  if(failure<2){assert(!p.GetQuestObjectiveData(38308,122581)&&!p.completions&&!p.bags[122581]);p.storageFull=p.storeFails=false;assert(corpse.HandleSpellClick(&p));}
  assert(p.GetQuestStatus(38308)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(38308,122581)==1&&p.slotCounters.at(3)==1&&p.m_QuestStatusSave.count(38308));
  auto completed=p.completions;assert(corpse.HandleSpellClick(&p));assert(p.completions==completed&&p.GetQuestObjectiveData(38308,122581)==1&&p.bags[122581]==2); // MaxCount0: native item is not unique.
  assert(p.CanRewardQuest(&intel,false));p.bank[122581]=p.bags[122581];p.bags[122581]=0;assert(!p.CanRewardQuest(&intel,false));p.bags[122581]=p.bank[122581];p.bank[122581]=0;
  assert(!p.CanTakeQuest(&leaders,false));WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward req{p.guid,38308};session.HandleQuestGiverRequestReward(req);assert(p.menu.offers==1);assert(p.GetNextQuest(p.guid,&intel)==&leaders);
  // Reward persistence is a fixture, after actual CanReward admission.
  p.quests.clear();p.bags[122581]=0;p.m_RewardedQuests.insert(38308);assert(p.CanTakeQuest(&leaders,false));DispatchRewardHandoff(&p,&p,&intel,p.guid);assert(p.GetQuestStatus(38317)==QUEST_STATUS_INCOMPLETE&&p.menu.details==1&&!p.popup&&!p.CanCompleteQuest(38317));
 }
 {Player p;Init(p);p.Accept(38308);assert(corpse.HandleSpellClick(&p));assert(p.bags[122581]==1);p.ItemRemovedQuestCheck(122581,1);p.bags[122581]=0;assert(p.GetQuestStatus(38308)==QUEST_STATUS_INCOMPLETE&&!p.CanRewardQuest(&intel,false));assert(corpse.HandleSpellClick(&p));assert(p.completions==2);p.quests.clear();p.bags.clear();p.Accept(38308);assert(corpse.HandleSpellClick(&p));assert(p.GetQuestObjectiveData(38308,122581)==1);}
 {Player p;Init(p);assert(corpse.HandleSpellClick(&p));assert(!p.completions&&p.bags[122581]==1);} // Existing behavior: click is not quest-conditioned.
 {Unit pet;assert(corpse.HandleSpellClick(&pet));} // ADD_ITEM ignores non-player invokers.
 Creature bat;bat.entry=97250;bat.guid={340420};bat.fields[UNIT_FIELD_NPC_FLAGS]=3;creatures[bat.guid]=&bat;SmartAI batAI(&bat);
 for(uint32 first:{91460u,91468u})for(bool petKill:{false,true}){Player p;Init(p);p.m_RewardedQuests.insert(38308);p.AddQuestAndCheckCompletion(&leaders,nullptr);Kill(p,first,petKill,10000+first);assert(p.packets==1&&!p.CanCompleteQuest(38317)&&!p.CanRewardQuest(&leaders,false));Kill(p,first,petKill,20000+first);assert(p.packets==1);Kill(p,first==91460?91468:91460,petKill,30000+first);assert(p.packets==2&&p.GetQuestStatus(38317)==QUEST_STATUS_COMPLETE&&p.completions==1&&p.slotCounters.at(0)==1&&p.slotCounters.at(1)==1);assert(p.CanRewardQuest(&leaders,false));WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward req{p.guid,38317};session.HandleQuestGiverRequestReward(req);assert(!p.menu.offers);req.QuestGiverGUID=bat.guid;session.HandleQuestGiverRequestReward(req);assert(p.menu.offers==1&&!p.GetNextQuest(bat.guid,&leaders));bat.near=false;session.HandleQuestGiverRequestReward(req);assert(p.menu.offers==1);bat.near=true;}
 {Player p;Init(p);p.Accept(38317);Kill(p,91460,true,50000);p.quests.clear();p.Accept(38317);assert(!p.GetQuestObjectiveData(38317,91460));p.SetQuestObjectiveData(&leaders,&leaders.objectives.at(0),1,true);Kill(p,91460,true,50001);assert(p.GetQuestStatus(38317)==QUEST_STATUS_COMPLETE);} // Reconstructed saved counter, not a DB login.
 {Player a,b,far,released,offline;unsigned id=200;for(auto p:{&a,&b,&far,&released,&offline}){Init(*p,id++);p->Accept(38317);}far.rewardRange=false;released.alive=false;released.corpse=true;offline.contact=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},bb{&b,&c},aa{&a,&bb};Group g;g.first=&aa;Creature v;v.guid={60000};v.entry=v.data.Entry=91468;creatures[v.guid]=&v;KillRewarder r;r._victim=&v;r._group=&g;r.SelectGroup();for(auto p:{&a,&b,&far,&released,&offline})Deliver(*p);assert(a.packets==1&&b.packets==1&&!far.packets&&!released.packets&&!offline.packets);creatures.erase(v.guid);}
 for(auto id:{38308u,38317u}){auto& q=store.quests[id];for(int state:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){Player p;Init(p);if(state)p.quests[q.PrevQuestID].Status=state;assert(!p.CanTakeQuest(&q,false));}unsigned allowed=0;for(unsigned bit=0;bit<32;++bit){Player p;Init(p);p.race=uint64(1)<<bit;p.m_RewardedQuests.insert(q.PrevQuestID);assert(p.CanTakeQuest(&q,false)==bool(q.AllowableRaces&p.race));allowed+=bool(q.AllowableRaces&p.race);}assert(allowed==9);}
 PatrolCreature actor;WaypointMovementGenerator<PatrolCreature> broken;broken.LoadPath(actor);assert(!broken._path&&!broken.starts);assert(boundPath==288637);actor.path=boundPath;WaypointMovementGenerator<PatrolCreature> fixed;fixed.LoadPath(actor);assert(fixed._path&&fixed._path->size()==5&&fixed.starts==1&&fixed._nextMoveTime.value==3000);
 std::cout<<"PASS: native spellclick -> SmartAI event73 -> action56 -> AddItem/item objective3, full/store-failure retry, optional cast failure, repeated click cap, remove/recollect, bank reward rejection and popup next selector; both leader kill orders/player/pet/caps/group/save-reconstruction/abandon/prerequisite/race boundaries; native waypoint loader fails unbound and starts the restored five-point patrol. Spell engine, inventory allocation, target lookup, reward persistence and world/spline/packets remain fixtures; client pending.\n";
}

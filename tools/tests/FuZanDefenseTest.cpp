// Production SmartScript dispatch/actions/text timer, criteria evaluator and
// old/current instance factories. Spawn, path, search, chat and credit endpoints
// are controlled; this is not a client/combat/pathfinding traversal.
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using uint8=uint8_t;using ReactStates=uint32;
#include "EventConstants.inc"
struct Unit;struct Creature;struct Player;struct GameObject;struct SmartScript;struct World;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;void Clear(){unit=nullptr;}};
ObjectGuid const ObjectGuid::Empty{};
struct FakeAI {
 SmartScript* script=nullptr;unsigned attempts=0,started=0;uint32 path=0,mEscortQuestID=999,despawn=999;bool fail=false;
 bool StartPath(bool run,uint32 entry,bool repeat,Unit* invoker){assert(run&&!repeat&&!invoker);++attempts;path=entry;if(fail)return false;++started;return true;}
 void SetDespawnTime(uint32 value){despawn=value;}void SetData(uint32,uint32);
};
struct Unit {bool player=false,creature=false;Creature* ToCreature();Player* ToPlayer();GameObject* ToGameObject();ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player(){player=true;}};
struct Creature:Unit {
 uint32 entry;bool combat=false,expired=false,IsAIEnabled=true;Unit* victim=nullptr;FakeAI ai;ReactStates reaction=9;
 explicit Creature(uint32 value=0):entry(value){creature=true;}
 FakeAI* AI(){return IsAIEnabled?&ai:nullptr;}uint32 GetEntry()const{return entry;}
 bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}
 void SetReactState(ReactStates value){reaction=value;}Unit* getVictim(){return victim;}
};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}bool IsGameObject(Unit*){return false;}
struct GameObject:Unit {FakeAI ai;FakeAI* AI(){return &ai;}};GameObject* Unit::ToGameObject(){return static_cast<GameObject*>(this);}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};
struct SmartScriptHolder {
 uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,targetEntry=0,parameter=0,second=0;
 bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
  struct {uint32 pointID=0,pathID=0;} waypoint;
 } event;
 struct {
  struct {uint32 textGroupID=0,duration=0;} talk;
  struct {uint32 field=0,data=0;} setData;
  struct {uint32 run=0,pathID=0,repeat=0,quest=0,despawnTime=0,reactState=0;} wpStart;
 } action;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}
 uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}
};
struct CriteriaTreeEntry {uint32 ID=0,Parent=0,Operator=0;uint64 Amount=0;};
struct CriteriaTree {CriteriaTreeEntry* Entry=nullptr;bool Criteria=false;std::vector<CriteriaTree const*> Children;uint32 ID=0;};
struct CriteriaProgress {uint64 Counter=0;};
struct AchievementGlobalMgr {
 template<class F> static void WalkCriteriaTree(CriteriaTree const* tree,F&& fn){fn(tree);for(auto child:tree->Children)WalkCriteriaTree(child,fn);}
};
template<class T> struct AchievementMgr {
 std::map<uint32,CriteriaProgress> savedProgress;
 CriteriaProgress const* GetCriteriaProgress(uint32 id){auto it=savedProgress.find(id);return it==savedProgress.end()?nullptr:&it->second;}
 bool IsCompletedCriteria(CriteriaTree const* tree,uint64 amount){auto p=GetCriteriaProgress(tree->ID);return p&&p->Counter>=std::max(uint64(1),amount);}
 bool IsCompletedCriteriaTree(CriteriaTree const*);
};
#include "CriteriaEvaluator.inc"
struct ConditionRow {uint32 entry,id,type,target,value1,value2,value3,negative;};
struct ConditionSourceInfo {Unit* unit;Creature* base;ConditionSourceInfo(Unit* u,Creature* b):unit(u),base(b){}};
using ConditionList=std::vector<ConditionRow>;
struct Conditions {
 std::vector<ConditionRow> rows;World* world=nullptr;
 ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32){ConditionList r;for(auto c:rows)if(c.entry==entry&&c.id==id+1)r.push_back(c);return r;}
 bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&);
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* unit,Creature* actor){ConditionSourceInfo info(unit,actor);return IsObjectMeetToConditions(info,GetConditionsForSmartEvent(entry,id,source));}
} conditions;auto sConditionMgr=&conditions;
struct Speech {uint32 speaker,group;bool operator==(Speech const& b)const{return speaker==b.speaker&&group==b.group;}};
struct Chat {std::vector<Speech> requests;void SendChat(Creature* talker,uint8 group,ObjectGuid){requests.push_back({talker->GetEntry(),group});}} chat;auto sCreatureTextMgr=&chat;
bool roll_chance_i(uint32){return true;}
struct InstanceMap {Creature* yulon=nullptr;Creature* GetCreature(ObjectGuid guid){return yulon&&guid.unit==yulon?yulon:nullptr;}};
struct InstanceScript {
 InstanceMap* instance;explicit InstanceScript(InstanceMap* map):instance(map){}virtual ~InstanceScript()=default;
 virtual void Initialize(){}virtual void OnCreatureCreate(Creature*){}virtual void CreatureDies(Creature*,Unit*){}virtual void onScenarionNextStep(uint32){}
};
struct InstanceMapScript {InstanceMapScript(char const*,uint32){}virtual ~InstanceMapScript()=default;virtual InstanceScript* GetInstanceScript(InstanceMap*)const=0;};
#include "OriginalInstance.inc"
#include "CurrentInstance.inc"
struct World {
 Creature yulon{109391},boss{109821};Player player;InstanceMap map;AchievementMgr<int> criteria;
 CriteriaTreeEntry rootEntry{49567,49171,9,100},strongEntry{49568,49567,0,5},impEntry{49620,49567,0,2};
 CriteriaTree strong{&strongEntry,true,{},49568},imp{&impEntry,true,{},49620},root{&rootEntry,false,{&strong,&imp},49567};
 std::map<uint32,unsigned> alive;std::vector<uint32> creditRequests;std::unique_ptr<SmartScript> dragon,demon;
 std::vector<SmartScriptHolder> data;std::unique_ptr<InstanceScript> instance;uint32 step=5,waves=0;
 bool eligible=true,actorsPresent=true,original=false,pumpDeaths=true;World(std::vector<SmartScriptHolder> const&,bool);~World();
 bool Complete(){return criteria.IsCompletedCriteriaTree(&root);}unsigned Alive(){unsigned n=0;for(auto pair:alive)n+=pair.second;return n;}
 void Credit(uint32);void Kill(uint32,Unit*);void Tick(uint32);
};
struct SmartScript {
 World* world;Creature* me;std::vector<SmartScriptHolder> mEvents;std::unique_ptr<ObjectList> escortTargets;
 ObjectGuid mLastInvoker,mTextGUID;unsigned errors=0;uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false;
 SmartScript(World* w,Creature* c):world(w),me(c){for(auto e:w->data)if(e.entryOrGuid==c->entry)mEvents.push_back(e);c->ai.script=this;}
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
 bool IsSmart()const{return me!=nullptr;}bool IsInPhase(uint32)const{return true;}
 uint32 GetPathId(){return me->ai.path;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 void StoreTargetList(ObjectList* list,uint32 id){assert(id==SMART_ESCORT_TARGETS);escortTargets.reset(list);}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
#include "TargetSelfVictim.inc"
  case 18:case 21:if(world->eligible)l->push_back(&world->player);break;
  case 19:if(world->actorsPresent){if(e.targetEntry==109391)l->push_back(&world->yulon);if(e.targetEntry==109821)l->push_back(&world->boss);}break;
 }return l;}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Update(uint32 diff);
};
#define CAST_AI(type,ai) (ai)
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "NoParamEvents.inc"
#include "TextOverEvent.inc"
#include "DataSetEvent.inc"
#include "WaypointEvent.inc"
#include "UpdateEvent.inc"
 }
}
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "TalkAction.inc"
#include "SetDataAction.inc"
#include "WaypointAction.inc"
 case 12:++world->alive[e.parameter];if(e.event_id==0||e.event_id==8)++world->waves;break;
 case 205:if(world->eligible)world->Credit(e.second);break;
 case 8:case 11:case 28:case 41:case 49:case 117:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){for(auto& e:mEvents)if(e.type==SMART_EVENT_UPDATE)ProcessEvent(e);
#include "TextTimer.inc"
void FakeAI::SetData(uint32 id,uint32 value){if(script)script->ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,id,value);}
bool Conditions::IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const& list){
 for(auto c:list){bool ok=false;
  switch(c.type){case 44:ok=c.value1==1137&&world->step==c.value2;break;
   case 52:assert(c.value1==49567);ok=world->Complete();break;
   case 29:ok=world->alive[c.value1]!=0;break;default:assert(false);
  }if(c.negative)ok=!ok;if(!ok)return false;
 }return true;
}
World::World(std::vector<SmartScriptHolder> const& rows,bool old):data(rows),original(old){
 conditions.world=this;chat.requests.clear();map.yulon=&yulon;
 if(old){OriginalTempleFactory factory;instance.reset(factory.GetInstanceScript(&map));}
 else{instance_temple_of_the_jade_serpent factory;instance.reset(factory.GetInstanceScript(&map));}
 instance->Initialize();instance->OnCreatureCreate(&yulon);
 dragon=std::make_unique<SmartScript>(this,&yulon);demon=std::make_unique<SmartScript>(this,&boss);
}
World::~World()=default;
void World::Credit(uint32 event){
 creditRequests.push_back(event);if(event==52970)++criteria.savedProgress[49568].Counter;if(event==53046)++criteria.savedProgress[49620].Counter;
 if(event==52981)criteria.savedProgress[49177].Counter=1;if(event==52590){criteria.savedProgress[49351].Counter=1;step=6;}
}
void World::Kill(uint32 entry,Unit* killer){
 assert(alive[entry]);--alive[entry];Creature dead(entry);SmartScript ai(this,&dead);
 ai.ProcessEventsFor(SMART_EVENT_DEATH,killer);instance->CreatureDies(&dead,killer);
 assert(!ai.errors);if(pumpDeaths)Tick(1);
}
void World::Tick(uint32 diff){dragon->Update(diff);demon->Update(diff);}
static std::vector<std::string> Fields(std::string const& line){std::stringstream ss(line);std::vector<std::string> f;std::string cell;while(std::getline(ss,cell,'\t'))f.push_back(cell);return f;}
static std::vector<SmartScriptHolder> Read(char const* path){std::ifstream input(path);assert(input);std::string line;std::vector<SmartScriptHolder> rows;
 while(std::getline(input,line)){auto f=Fields(line);assert(f.size()==31);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};SmartScriptHolder e;
  e.entryOrGuid=n(0);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
  e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};e.event.waypoint={n(9),n(10)};
  e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.action.setData={n(15),n(16)};e.action.wpStart={n(15),n(16),n(17),n(18),n(19),n(20)};
  e.targetType=n(21);e.targetEntry=n(22);rows.push_back(e);
 }return rows;
}
static unsigned Speeches(uint32 speaker,uint32 group){unsigned n=0;for(auto s:chat.requests)if(s.speaker==speaker&&s.group==group)++n;return n;}
static void Waves(World& w,Unit* killer,bool reversed=false){
 w.Tick(1);assert(w.waves==1&&w.Alive()==8);
 std::vector<uint32> waveActors{109438u,109511u,109933u,110471u,110471u,110471u,110471u,110471u};if(reversed)std::reverse(waveActors.begin(),waveActors.end());
 for(unsigned wave=0;wave<4;++wave){for(uint32 entry:waveActors)w.Kill(entry,killer);
  assert(w.waves==(wave==3?4:wave+2));
 }assert(w.Complete()&&!w.Alive()&&w.criteria.savedProgress[49568].Counter==12&&w.criteria.savedProgress[49620].Counter==20);
 if(!w.pumpDeaths)w.Tick(1);
}
static void Dialogue(World& w,bool old){
 assert(Speeches(109391,0)==1&&Speeches(109391,1)==unsigned(old));
 w.Tick(5999);assert(!w.yulon.ai.started);w.Tick(2);assert(w.yulon.ai.started==1&&w.yulon.ai.path==109391);
 assert(w.criteria.savedProgress[49177].Counter==1);
 w.dragon->ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1,109391);assert(w.step==6);
 w.dragon->ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,109391);assert(Speeches(109821,0)==1);
 w.Tick(6001);assert(Speeches(109391,1)==1);w.Tick(6001);
 assert(Speeches(109821,1)==unsigned(!old));assert(!w.dragon->errors&&!w.demon->errors);
}
int main(int argc,char** argv){
 if(argc!=4)return 1;auto rows=Read(argv[1]);assert(rows.size()==51);
 std::ifstream input(argv[2]);std::string line;while(std::getline(input,line)){auto f=Fields(line);assert(f.size()==14);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};conditions.rows.push_back({n(2),n(1),n(5),n(6),n(7),n(8),n(9),n(10)});}
 assert(conditions.rows.size()==20);bool baseline=std::string(argv[3])=="baseline";
 Player player;Creature pet(1),guardian(2),ally(3);unsigned passed=0;
 for(Unit* killer:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&ally),static_cast<Unit*>(nullptr)})for(bool reversed:{false,true})for(bool pump:{false,true}){
  World w(rows,baseline);w.pumpDeaths=pump;Waves(w,killer,reversed);Dialogue(w,baseline);++passed;
  if(!baseline){auto speeches=chat.requests;w.dragon->ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,2,2);w.demon->ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,1,1);w.Tick(20000);assert(chat.requests==speeches);}
 }
 if(!baseline){
  World w(rows,false);w.eligible=false;w.Tick(1);for(unsigned wave=0;wave<4;++wave)for(uint32 entry:{109438u,109511u,109933u,110471u,110471u,110471u,110471u,110471u})w.Kill(entry,nullptr);
  assert(!w.Complete()&&chat.requests.empty()&&w.creditRequests.empty());++passed;
  World expired(rows,false);expired.yulon.expired=true;expired.Tick(1);assert(!expired.waves&&chat.requests.empty());++passed;
  World missing(rows,false);missing.map.yulon=nullptr;missing.yulon.IsAIEnabled=false;
  for(unsigned i=0;i<300;++i){Creature dead(110471);missing.instance->CreatureDies(&dead,nullptr);}assert(chat.requests.empty());++passed;
  World boundary(rows,false);boundary.criteria.savedProgress[49568].Counter=19;boundary.criteria.savedProgress[49620].Counter=2;
  assert(!boundary.Complete());boundary.Tick(1);assert(chat.requests.empty());boundary.criteria.savedProgress[49620].Counter=3;assert(boundary.Complete());boundary.Tick(1);assert(Speeches(109391,0)==1&&!Speeches(109391,1));++passed;
  // A new instance has no inherited raw death count or one-shot speech state.
  World retry(rows,false);assert(!retry.Complete());Waves(retry,&player);Dialogue(retry,false);++passed;
 }
 std::printf("%s %u Fu Zan native-dispatch/instance cases: weighted100 defense, four8-actor waves, authored speech/path/52981/52590/Korithis reply, repeated callbacks and player/pet/guardian/ally/null endpoints. Native chat/search/spawn/path/credit services controlled; no client traversal.\n",baseline?"EXPECTED_BROKEN":"PASS",passed);
}

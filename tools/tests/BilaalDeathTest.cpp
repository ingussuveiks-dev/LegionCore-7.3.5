#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int8=int8_t;
using QuestStatus=uint32;
constexpr uint32 CLASS_PRIEST=5,CLASS_MAGE=8,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
constexpr uint32 UNIT_FIELD_CREATED_BY_SPELL=0,UNIT_FIELD_DEMON_CREATOR=1,TEMPSUMMON_MANUAL_DESPAWN=8,CRITERIA_TYPE_SCRIPT_EVENT_2=92;
uint32 warnings=0,infos=0;
#define TC_LOG_ERROR(...) (++warnings)
#define TC_LOG_INFO(...) (++infos)
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid{uint64 high=1,low=0;static ObjectGuid Empty;bool operator==(ObjectGuid const&o)const{return high==o.high&&low==o.low;}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}bool operator<(ObjectGuid const&o)const{return std::tie(high,low)<std::tie(o.high,o.low);}};ObjectGuid ObjectGuid::Empty{0,0};
struct Position{float x,y,z,o;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}};
struct Map;struct Creature;struct Player;struct TempSummon;struct Unit;
struct GroupReference{Player*p;GroupReference*following=nullptr;Player*getSource(){return p;}GroupReference*next(){return following;}};struct Group{GroupReference*first=nullptr;GroupReference*GetFirstMember(){return first;}};
struct AI{std::function<void(Unit*)>death;void JustDied(Unit*k){if(death)death(k);}};
struct InstanceScript{std::function<void(Creature*,Unit*)>death;uint32 nativeFollowup=0;void CreatureDies(Creature*c,Unit*k){if(death)death(c,k);}void CreatureDiesForScript(Creature*,Unit*){++nativeFollowup;}};
struct QuestStatusData{std::shared_ptr<uint8> ScriptLifetime=std::make_shared<uint8>(uint8{0});std::map<int8,int32> ObjectiveData;};
struct QuestObjective{uint32 ObjectID;int8 StorageIndex;};
struct Quest{uint32 id;std::vector<QuestObjective> objectives;uint32 GetQuestId()const{return id;}auto const&GetObjectives()const{return objectives;}};
struct QuestStore{std::map<uint32,Quest> quests;Quest const*GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}}questStore;auto*sQuestDataStore=&questStore;
struct Unit{virtual ~Unit()=default;ObjectGuid unitGuid{7,3};virtual ObjectGuid GetGUID()const{return unitGuid;}Player* credit=nullptr;virtual Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return credit;}void NativeDeath(Creature*);};
struct Creature:Unit{
 uint32 m_PlayerDamageReq=1;
#include "RewardAllowed.inc"
 ObjectGuid guid{1,1};Map*map=nullptr;uint32 entry=104502,spawn=370522,mask=1;std::set<uint32> phases{6140};bool world=true,alive=false,predelete=false,scoped=false,IsAIEnabled=true;Player*loot=nullptr;
 virtual TempSummon*ToTempSummon(){return nullptr;}Creature*ToCreature(){return this;}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return predelete;}uint32 GetMapId()const;Map*GetMap(){return map;}bool HasQuestObjectScope()const{return scoped;}uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return spawn;}auto const&GetPhases()const{return phases;}uint32 GetPhaseMask()const{return mask;}ObjectGuid GetGUID()const{return guid;}Player*GetLootRecipient(){return loot;}
 ::AI ai;InstanceScript*instanceScript=nullptr;void KillAllDelayedCombats(){}::AI*AI(){return &ai;}InstanceScript*GetInstanceScript(){return instanceScript;}
};
void Unit::NativeDeath(Creature*creature){
#include "NativeDeathDispatch.inc"
}
struct Player:Unit{
 ObjectGuid guid{9,2};Map*map=nullptr;Group*group=nullptr;uint32 klass=5,status=3;bool world=true,contact=true,gm=false,teleport=false,samePhase=true,range=true,rewarded=false;std::map<uint32,QuestStatusData> quests;Group*GetGroup(){return group;}
 Player(){credit=this;}ObjectGuid GetGUID()const{return guid;}bool IsInWorld()const{return world;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return teleport;}uint32 getClass()const{return klass;}Map*GetMap(){return map;}bool InSamePhase(Creature*)const{return samePhase;}bool InSamePhase(uint32)const{return samePhase;}bool InSamePhaseId(std::set<uint32>const&,bool playerRules)const{assert(!playerRules);return samePhase;}bool IsAtGroupRewardDistance(Creature*)const{return range;}bool GetQuestRewardStatus(uint32)const{return rewarded;}QuestStatus GetQuestStatus(uint32 id)const{return quests.count(id)?status:0;}QuestStatusData*getQuestStatus(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}int32 GetQuestObjectiveData(uint32,uint32)const;int32 GetQuestObjectiveData(Quest const*,int8)const;void UpdateAchievementCriteria(uint32,uint32,uint32,uint32,Unit*,bool);uint32 GetGUIDLow()const{return uint32(guid.low);}
};
#include "QuestObjective.inc"
#include "QuestStorage.inc"
struct Reference{Player*p;Player*getSource()const{return p;}};
struct ProgressValue{uint32 Counter=0;};struct Achievement{std::map<uint32,ProgressValue> values;auto const*GetCriteriaProgressMap(){return &values;}};
struct Scenario{uint32 id=1065,step=4,tree=47117,requests=0;Unit*lastReference=nullptr;bool accept=true;Achievement achievements;std::function<void()>onCredit;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}Achievement&GetAchievementMgr(){return achievements;}};
struct ScenarioMgr{std::map<uint32,Scenario*>scenarios;Scenario*GetScenario(uint32 id){auto i=scenarios.find(id);return i==scenarios.end()?nullptr:i->second;}}scenarioMgr;auto*sScenarioMgr=&scenarioMgr;
enum class HighGuid{Creature};struct Generator{uint64 value=100;uint64 Generate(){return ++value;}};
struct ObjectMgr{Generator generator;template<HighGuid>Generator*GetGenerator(){return &generator;}}objectMgr;auto*sObjectMgr=&objectMgr;
struct Map{
 uint32 id=1583,instance=77,creates=0,inserts=0,inits=0,failCreate=0,failInsert=0;bool unloadSourceOnCredit=false;std::vector<Reference>players;std::map<ObjectGuid,Creature*>creatures;std::map<ObjectGuid,Unit*>units;std::vector<TempSummon*>owned;std::function<void()>onInsert;
 uint32 GetInstanceId(){return instance;}auto const&GetPlayers(){return players;}Creature*GetCreature(ObjectGuid const&g){auto i=creatures.find(g);return i==creatures.end()?nullptr:i->second;}bool AddToMap(Creature*);~Map();
};
uint32 Creature::GetMapId()const{return map?map->id:0;}
void Player::UpdateAchievementCriteria(uint32 type,uint32 event,uint32 a,uint32 b,Unit*victim,bool force){assert(type==92&&event==49810&&!a&&!b&&force);auto*s=sScenarioMgr->GetScenario(map->instance);s->lastReference=victim;++s->requests;if(s->onCredit)s->onCredit();if(s->accept){s->step=5;s->achievements.values[s->tree].Counter=1;}}
struct ObjectAccessor{static Unit*GetUnit(Player&p,ObjectGuid const&g){auto i=p.map->units.find(g);return i==p.map->units.end()?nullptr:i->second;}};
uint32 allocated=0,deleted=0,cleaned=0;uint32 sourceCallbacks=0;
struct TempSummon:Creature{
 ObjectGuid summoner,creator;uint32 summonType=0;bool initialized=false;Position home{};explicit TempSummon(void*properties,Creature*actor,bool obj){assert(!properties&&!obj);++allocated;world=false;alive=true;entry=104546;spawn=0;summoner=actor?actor->guid:ObjectGuid::Empty;}~TempSummon(){++deleted;}TempSummon*ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID()const{return summoner;}void SetSummonerGUID(ObjectGuid g){summoner=g;}
 bool Create(uint64 low,Map*m,uint32 phase,uint32 e,uint32 veh,uint32 team,float x,float y,float z,float o){assert(!veh&&!team&&e==104546);assert(x==4219.38f&&y==7124.89f&&z==268.198f&&o==4.28748f);map=m;guid={5,low};mask=phase;entry=e;++map->creates;if(map->failCreate){--map->failCreate;return false;}return true;}
 void SetPhaseId(std::set<uint32>const&p,bool update){assert(!update&&!world);phases=p;}void SetUInt32Value(uint32 f,uint32 v){assert(f==0&&!v);}void SetGuidValue(uint32 f,ObjectGuid g){assert(f==1);creator=g;}void SetTargetGUID(ObjectGuid g){assert(g==ObjectGuid::Empty);}void SetHomePosition(Position p){home=p;}void InitStats(uint32 d){assert(!d&&!world);}void CleanupsBeforeDelete(){assert(!world);++cleaned;}void InitSummon(){assert(world);initialized=true;++map->inits;}void SetTempSummonType(uint32 t){assert(t==8&&initialized);summonType=t;}
};
bool Map::AddToMap(Creature*c){++inserts;assert(c->phases.count(6140)||c->phases.count(6068));assert(c->mask==3);if(failInsert){--failInsert;return false;}c->world=true;creatures[c->guid]=c;owned.push_back(c->ToTempSummon());if(onInsert)onInsert();return true;}
Map::~Map(){for(auto*c:owned)delete c;}
#include "Bilaal.inc"
struct Test{
 Map map;Scenario scenario;Player player;Creature boss;Unit pet,guardian;BilaalDeath::State state;uint32 quest;
 explicit Test(bool mage):quest(mage?42011:41631){auto const&r=mage?BilaalDeath::mage:BilaalDeath::priest;boss.map=&map;boss.mask=3;boss.entry=r.entry;boss.spawn=r.spawn;boss.phases={r.phase,999};map.creatures[boss.guid]=&boss;player.map=&map;player.klass=r.klass;player.quests.try_emplace(quest);player.quests[quest].ObjectiveData[0]=1;map.players.push_back({&player});scenario.id=r.scenario;scenario.tree=r.victory;sScenarioMgr->scenarios[77]=&scenario;pet.credit=&player;guardian.credit=&player;questStore.quests[quest]={quest,{{r.entry,0},{105109,1}}};}
 void death(Unit*killer=nullptr){state.Death(&map,&boss,killer?killer:&player);}void ticks(uint32 n=4){for(uint32 i=0;i<n;++i)state.Update(&map,2000);}void success(){assert(state.earned&&state.done&&scenario.requests==1&&map.owned.size()==1);auto*c=map.owned[0];assert(c->phases==boss.phases&&c->summoner==boss.guid&&c->creator==boss.guid&&c->summonType==8);}
};
int main(){
 for(bool mage:{false,true}){
  for(bool enabled:{false,true}){Test baseline(mage);InstanceScript old;baseline.boss.instanceScript=&old;baseline.boss.IsAIEnabled=enabled;baseline.boss.ai.death=[&](Unit*){++baseline.scenario.requests;};baseline.pet.NativeDeath(&baseline.boss);assert(baseline.scenario.requests==(enabled?1u:0u)&&old.nativeFollowup==1);Test repaired(mage);InstanceScript script;script.death=[&](Creature*c,Unit*k){repaired.state.Death(&repaired.map,c,k);};repaired.boss.instanceScript=&script;repaired.boss.IsAIEnabled=enabled;repaired.guardian.NativeDeath(&repaired.boss);repaired.success();assert(script.nativeFollowup==1);}
  for(uint32 killer=0;killer<4;++killer){Test t(mage);Unit unrelated;if(killer==3)t.boss.loot=&t.player;Unit*actual=killer==1?&t.pet:killer==2?&t.guardian:killer==3?&unrelated:&t.player;t.death(actual);t.success();assert(t.scenario.lastReference==actual);t.ticks();t.death();assert(t.scenario.requests==1&&t.map.creates==1);}
  for(uint32 denial=0;denial<25;++denial){Test t(mage);switch(denial){case 0:t.boss.alive=true;break;case 1:t.boss.world=false;break;case 2:t.boss.predelete=true;break;case 3:t.map.id=1;break;case 4:t.boss.entry=104546;break;case 5:++t.boss.spawn;break;case 6:t.boss.phases={1};break;case 7:t.boss.scoped=true;break;case 8:t.scenario.id=99;break;case 9:t.scenario.step=3;break;case 10:t.scenario.step=5;break;case 11:t.player.world=false;break;case 12:t.player.contact=false;break;case 13:t.player.gm=true;break;case 14:t.player.teleport=true;break;case 15:t.player.klass=1;break;case 16:t.player.map=nullptr;break;case 17:t.player.samePhase=false;break;case 18:t.player.range=false;break;case 19:t.player.rewarded=true;break;case 20:t.player.status=0;break;case 21:t.player.quests.clear();break;case 22:t.player.quests[t.quest].ObjectiveData[0]=0;break;case 23:t.player.quests[t.quest].ObjectiveData[1]=1;t.player.quests[t.quest].ObjectiveData[0]=0;break;case 24:t.boss.m_PlayerDamageReq=0;break;}t.death();t.ticks();assert(!t.scenario.requests&&!t.map.creates);}
  {Test t(mage);t.boss.m_PlayerDamageReq=0;assert(!t.boss.IsDamageEnoughForLootingAndReward());t.boss.LowerPlayerDamageReq(1);assert(t.boss.IsDamageEnoughForLootingAndReward());t.death();t.success();}
  {Test t(mage);t.map.failCreate=2;t.death();assert(t.state.earned&&!t.state.done&&t.map.creates==1);t.map.creatures.erase(t.boss.guid);t.boss.world=false;t.map.players.clear();t.state.Update(&t.map,1999);assert(t.map.creates==1);t.state.Update(&t.map,1);assert(t.map.creates==2);t.state.Update(&t.map,2000);assert(t.state.done&&t.map.creates==3&&t.map.inits==1&&t.scenario.requests==1);assert(t.map.owned[0]->phases==t.boss.phases&&t.map.owned[0]->summoner==t.boss.guid);}
  {Test t(mage);t.map.failInsert=2;uint32 old=cleaned;t.death();t.ticks();t.success();assert(t.map.creates==3&&cleaned==old+2);}
  {Test t(mage);t.boss.IsAIEnabled=false;t.death(&t.pet);t.success();}
  {Test t(mage);t.scenario.accept=false;t.death();assert(t.scenario.requests==1&&!t.state.earned&&!t.map.creates);t.player.quests.erase(t.quest);t.player.quests.try_emplace(t.quest);t.player.quests[t.quest].ObjectiveData[0]=1;t.scenario.accept=true;t.ticks();assert(t.state.done&&t.scenario.requests==1&&!t.map.creates);}
  {Test t(mage);t.scenario.accept=false;t.death();t.scenario.accept=true;t.ticks();assert(t.state.earned&&t.state.done&&t.scenario.requests==2&&t.map.creates==1);}
  {Test t(mage);t.map.units[t.pet.GetGUID()]=&t.pet;t.scenario.accept=false;t.death(&t.pet);assert(t.scenario.lastReference==&t.pet);t.scenario.accept=true;t.ticks();assert(t.scenario.requests==2&&t.scenario.lastReference==&t.pet&&t.state.earned);}
  {Test t(mage);t.scenario.accept=false;t.death(&t.pet);t.scenario.accept=true;t.ticks();assert(t.scenario.requests==2&&!t.scenario.lastReference&&t.state.earned);}
  {Test t(mage);t.scenario.accept=false;t.death(&t.pet);t.map.creatures.erase(t.boss.guid);t.boss.world=false;t.player.range=false;t.scenario.accept=true;t.ticks();assert(t.scenario.requests==2&&t.state.earned&&t.state.done&&t.map.creates==1);}
  {Test t(mage);t.scenario.accept=false;t.death();t.player.samePhase=false;t.scenario.accept=true;t.ticks();assert(t.scenario.requests==1&&!t.state.earned);t.player.samePhase=true;t.ticks();assert(t.scenario.requests==2&&t.state.earned);}
  {Test t(mage);t.map.failCreate=1;t.death();t.player.quests.clear();t.map.players.clear();t.ticks();assert(t.state.earned&&t.state.done&&t.map.creates==2&&t.scenario.requests==1);}
  {Test t(mage);t.map.failCreate=1;t.death();t.scenario.step=6;t.ticks();assert(t.state.done&&t.map.creates==1);}
  {Test t(mage);t.scenario.onCredit=[&]{t.map.creatures.erase(t.boss.guid);t.boss.world=false;};t.death();assert(t.state.earned&&t.state.done&&t.map.creates==1&&t.map.owned[0]->summoner==t.boss.guid);}
  {Test t(mage);t.map.onInsert=[&]{t.state.Update(&t.map,2000);t.death();};t.scenario.onCredit=[&]{t.state.Update(&t.map,2000);t.death();};t.death();t.success();assert(t.map.creates==1);}
  {Test t(mage);Player root;root.klass=1;root.map=&t.map;GroupReference member{&t.player};Group party{&member};root.group=&party;t.boss.loot=&root;Unit unrelated;t.death(&unrelated);t.success();}
  {Test t(mage);Player root;root.klass=1;root.map=&t.map;t.boss.loot=&root;t.death(&t.pet);t.ticks();assert(!t.scenario.requests&&!t.map.creates);}
  {Test t(mage);t.scenario.accept=false;t.death();t.player.quests.erase(t.quest);t.player.quests.try_emplace(t.quest);t.scenario.accept=true;t.ticks();t.death();assert(t.scenario.requests==1&&!t.map.creates);t.player.quests[t.quest].ObjectiveData[0]=1;t.death();assert(t.state.earned&&t.state.done&&t.scenario.requests==2&&t.map.creates==1);}
  {Test baseline(mage);InstanceScript old;baseline.boss.instanceScript=&old;baseline.map.failCreate=1;baseline.state.route=mage?&BilaalDeath::mage:&BilaalDeath::priest;baseline.state.source=baseline.boss.guid;baseline.state.phaseMask=baseline.boss.mask;baseline.state.phases=baseline.boss.phases;baseline.boss.ai.death=[&](Unit*){assert(!baseline.state.CreateReceiver(&baseline.map));};baseline.pet.NativeDeath(&baseline.boss);for(uint32 tick=0;tick<10;++tick){assert(baseline.map.creates==1&&!baseline.map.inits);}Test repaired(mage);repaired.map.failCreate=1;repaired.death(&repaired.pet);repaired.ticks();repaired.success();assert(repaired.map.creates==2);std::cout<<"COUNTERFACTUAL native DEATH callback invokes one controlled failed receiver creation; original graph has no UPDATE retry. Repaired instance receipt retries.\n";}
 }
 assert(allocated==deleted);std::cout<<"PASS both native-route death receipts,50 denials,native damage/reward and objective counters,pet/guardian/loot owner,disabled AI,original quest,accepted world recovery,source removal,creation/insertion retry,phase identity and cleanup; controlled criterion/factory endpoints only\n";
}

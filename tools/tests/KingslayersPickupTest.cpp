#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint64=uint64_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
using LootState=uint32;using SpellCastResult=uint32;struct ItemPosCountVec:std::vector<uint32>{void emplace_back(std::uint16_t,uint32){assert(false);}};
enum {QUEST_STATUS_NONE=0,CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,GO_READY=1,GO_ACTIVATED=2,GO_JUST_DEACTIVATED=3,
 GAMEOBJECT_FIELD_FLAGS=17,GO_FLAG_NOT_SELECTABLE=16,INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15,EQUIPMENT_SLOT_OFFHAND=16,
 SPELL_CAST_OK=0,TYPEID_PLAYER=4,NULL_BAG=255,NULL_SLOT=255,GAMEOBJECT_TYPE_FISHINGNODE=17};
#include "CriteriaTypes.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid{uint32 value=0;bool IsEmpty()const{return !value;}void Clear(){value=0;}bool operator==(ObjectGuid b)const{return value==b.value;}bool operator!=(ObjectGuid b)const{return value!=b.value;}bool operator<(ObjectGuid b)const{return value<b.value;}};
struct Position{float x,y,z,o;};struct Creature;struct Map{Creature*GetCreature(ObjectGuid); } liveMap,otherMap;
struct Player;struct GameObject;struct Creature;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime=std::make_shared<uint8>();};
struct Unit{virtual Player* ToPlayer(){return nullptr;}virtual ~Unit()=default;virtual uint32 GetTypeId(){return 3;}ObjectGuid guid{1};ObjectGuid GetGUID()const{return guid;}uint32 CastSpell(Unit*,uint32,bool);virtual Creature*SummonCreature(uint32,float,float,float,float);};
struct Item{uint32 entry=128870;ObjectGuid guid{21},child{22};uint32 GetEntry(){return entry;}ObjectGuid GetChildItem(){return child;}ObjectGuid GetGUID(){return guid;}};
struct CriteriaProgress{uint32 Counter=0;};
struct Scenario{uint32 id=1123,step=8;bool accept=true;std::map<uint32,CriteriaProgress> progress{{48589,{1}}};uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Scenario& GetAchievementMgr(){return *this;}auto const* GetCriteriaProgressMap(){return &progress;}};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}} manager;
auto* sScenarioMgr=&manager;
enum InventoryResult{EQUIP_ERR_OK,EQUIP_ERR_INVENTORY_FULL};
constexpr InventoryResult EQUIP_ERR_INV_FULL=EQUIP_ERR_INVENTORY_FULL;

struct LootItem{struct{uint32 ItemID=128870,RandomPropertiesID=0;struct{std::vector<uint32> BonusListIDs;uint32 Context=0;}ItemBonus;}item;uint32 count=1;bool freeforall=false,is_looted=false;std::set<uint32> GetAllowedLooters(){return {};}};
struct Loot{uint32 objGuid=0;LootItem item;uint32 gold=0,unlootedCount=1;
#include "IsLooted.inc"
};
struct GameObject;struct Unit;
struct ScriptMgr{ template<class... Args> bool CanAutoEquipLoot(Args&&...){return false;}void OnGameObjectLootStateChanged(GameObject* g,uint32 s,Unit* u);}scripts;auto* sScriptMgr=&scripts;
struct Player:Unit{
 bool world=true,contact=true,alive=true,teleport=false,rewarded=false,owned=false,bank=false,paired=false,room=true,storage=true,phase=true,visible=true,los=true;
 uint32 questId=42504;QuestStatusData quest;std::vector<std::pair<uint64_t,std::function<void()>>>tasks;uint64_t clock=0;std::vector<uint32>conversations;bool failOutro=false;uint32 directCredits=0;std::function<void()>onCredit;
 uint32 klass=4,status=3,instance=42,casts=0,castResult=0,events=0,explicitRewards=0,grants=0,errors=0,mapId=1620;float distance=1;Map* map=&liveMap;
 Item parent,child{128869,{22},{}};
 struct Phase{uint32 changes=0;void SetCustomPhase(uint32 phase){assert(phase==1);++changes;}}phases;
 Player* ToPlayer()override{return this;}uint32 GetTypeId()override{return 4;}
 bool IsInWorld(){return world;}bool CanContact(){return contact;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}bool HasPendingTeleport(){return teleport;}uint32 GetMapId(){return mapId;}uint32 getClass(){return klass;}
 Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}bool InSamePhase(GameObject*){return phase;}bool InSamePhase(Creature*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}
 bool IsWithinLOSInMap(GameObject*){return los;}bool IsWithinDistInMap(GameObject*,float r){return distance<=r;}
 bool GetQuestRewardStatus(uint32 id){return id==questId&&rewarded;}uint32 GetQuestStatus(uint32 id){return id==questId?status:0;}
 bool HasItemCount(uint32 id,uint32=1,bool withBank=false){assert(id==128870);return owned||withBank&&bank;}
 Item* GetItemByEntry(uint32 id){assert(id==128870);return owned?&parent:nullptr;}
 Item* GetItemByPos(uint32,uint32 slot){return paired&&owned?(slot==15?&parent:&child):nullptr;}
 Phase& GetPhaseMgr(){return phases;}
 Item* AddItem(uint32,uint32){++grants;if(!room||!storage)return nullptr;owned=true;return &parent;}
 void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32=0){if(type==CRITERIA_TYPE_SCRIPT_EVENT_2){assert(asset==51616);++events;if(onCredit)onCredit();if(manager.scenario.accept){manager.scenario.progress[48591].Counter=1;if(manager.scenario.progress[48589].Counter)manager.scenario.step=9;}}else{assert(type==CRITERIA_TYPE_COMPLETE_SCENARIO);++explicitRewards;}}
 InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec& dest,uint32 id,uint32 count,void*,bool){assert(id==128870&&count==1);if(!room)return EQUIP_ERR_INVENTORY_FULL;dest.push_back(1);return EQUIP_ERR_OK;}
 Item* StoreNewItem(ItemPosCountVec const&,uint32,bool,uint32,std::set<uint32> const&,std::vector<uint32> const&,uint32,bool refund=false,bool autoEquip=false) {assert(!refund&&!autoEquip);if(!storage)return nullptr;owned=true;return &parent;}
 void SendEquipError(InventoryResult,void*,void*,uint32){++errors;}
 QuestStatusData*getQuestStatus(uint32 id){quest.Status=status;return id==questId&&status?&quest:nullptr;}
 void AddDelayedEvent(uint64_t delay,std::function<void()>&&fn){tasks.push_back({clock+delay,std::move(fn)});}void AddQuestDelayedEvent(uint32,uint64_t,std::function<void()>&&);
 void Advance(uint64_t diff);
 void CreateConversation(uint32 id){conversations.push_back(id);}void KilledMonsterCredit(uint32){++directCredits;}
 Creature*SummonCreature(uint32,float,float,float,float)override;
 void StoreLootItem(Loot* loot){LootItem* item=&loot->item;void* qitem=nullptr;
#include "StoreInventory.inc"
#include "ConsumeLoot.inc"
 }
};
uint32 Unit::CastSpell(Unit* target,uint32 spell,bool triggered){assert(target==this&&spell==215112&&triggered);auto* p=ToPlayer();assert(p);++p->casts;return p->castResult;}
struct Motion{std::vector<Position>moves;void MovePoint(uint32,float x,float y,float z){moves.push_back({x,y,z,0});}};
struct Creature{ObjectGuid guid,source;bool removed=false,alive=true,world=true,preDelete=false;uint32 entry=0;ObjectGuid viewer;ObjectGuid GetGUID(){return guid;}uint32 delay=0;void DespawnOrUnsummon(uint32 value=0){delay=value;removed=true;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){viewer=g;}void SetPhaseMask(uint32 phase,bool){assert(phase==1);}bool IsInWorld(){return world&&!removed;}bool IsAlive(){return alive;}bool IsPreDelete(){return preDelete;}uint32 GetEntry(){return entry;}Creature*ToTempSummon(){return this;}ObjectGuid GetSummonerGUID(){return source;}Motion motion;Motion*GetMotionMaster(){return &motion;}GameObject*FindNearestGameObject(uint32,float);Creature*FindNearestCreature(uint32,float,bool=true);std::vector<std::pair<uint64_t,std::function<void()>>>tasks;uint64_t clock=0;template<class F>void AddDelayedEvent(uint32 d,F&&f){tasks.push_back({clock+d,std::forward<F>(f)});}void Advance(uint64_t d){clock+=d;for(auto&t:tasks)if(t.second&&t.first<=clock){auto f=std::move(t.second);f();}} };
static std::map<ObjectGuid,Creature> actors;static std::map<ObjectGuid,Player*> players;static uint32 serial=100;
Creature*Map::GetCreature(ObjectGuid g){auto i=actors.find(g);return i==actors.end()?nullptr:&i->second;}
struct Maps{bool present=true;Map*FindMap(uint32 map,uint32 instance){assert(map==1620);return present&&instance==42?&liveMap:nullptr;}}maps;auto*sMapMgr=&maps;
void Player::Advance(uint64_t diff){clock+=diff;for(size_t i=0;i<tasks.size();++i)if(tasks[i].second&&tasks[i].first<=clock){auto f=std::move(tasks[i].second);f();}for(auto&entry:actors)entry.second.Advance(diff);}
Creature* Player::SummonCreature(uint32 id,float,float,float,float){if(failOutro)return nullptr;auto&c=actors[{++serial}];c.guid={serial};c.source=guid;c.entry=id;return &c;}
Creature* Unit::SummonCreature(uint32 id,float x,float y,float z,float o){return ToPlayer()?ToPlayer()->SummonCreature(id,x,y,z,o):nullptr;}
struct GameObjectAI;
struct GameObject{
 bool world=true,spawned=true,notSelectable=false;uint32 phase=1;void SetPhaseMask(uint32 p,bool){phase=p;}uint32 entry=251107,mapId=1620,instance=42,m_lootState=GO_READY,failEntry=0,spawnCount=0;Map* map=&liveMap;GameObjectAI* ai=nullptr;
 bool IsInWorld(){return world;}bool isSpawned(){return spawned;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapId;}Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}
 float GetInteractionDistance(){return 5;}bool HasFlag(uint32,uint32){return notSelectable;}uint32 getLootState(){return m_lootState;}GameObjectAI* AI(){return ai;}
 void SetLootState(LootState,Unit* =nullptr);uint32 GetGoType(){return 3;}
 Creature* SummonCreature(uint32 id,Position){if(id==failEntry)return nullptr;auto& c=actors[{++serial}];c.guid={serial};c.entry=id;++spawnCount;return &c;}
 Creature* SummonCreature(uint32 id,float x,float y,float z,float o){return SummonCreature(id,{x,y,z,o});}
 Creature* visual=nullptr;Creature* FindNearestCreature(uint32 id,float distance){assert(id==105843&&distance==10);return visual;}
};
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* g):go(g){}virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual void OnStateChanged(uint32,Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual void SetGUID(ObjectGuid const&,int32){}virtual uint32 GetData(uint32){return 0;}virtual void UpdateAI(uint32){};};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual GameObjectAI* GetAI(GameObject* g)const{return new GameObjectAI(g);}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual void OnLootStateChanged(GameObject*,uint32,Unit*){}};
GameObjectScript* currentScript;
void ScriptMgr::OnGameObjectLootStateChanged(GameObject* g,uint32 s,Unit* u){currentScript->OnLootStateChanged(g,s,u);}
struct ObjectAccessor{static Player*FindPlayer(ObjectGuid id){auto i=players.find(id);return i==players.end()?nullptr:i->second;}static Player* GetPlayer(GameObject&,ObjectGuid id){return FindPlayer(id);}template<class T>static Creature* GetCreature(T&,ObjectGuid id){auto i=actors.find(id);return i==actors.end()||i->second.removed?nullptr:&i->second;}};
GameObject door;Creature garona;
GameObject*Creature::FindNearestGameObject(uint32 id,float distance){assert(id==251106&&distance==60);return &door;}
Creature*Creature::FindNearestCreature(uint32 id,float distance,bool alive){assert(id==108222&&distance==70&&alive);return &garona;}
#include "QuestQueue.inc"

enum class HighGuid{Conversation};
struct Generator{uint32 Generate(){return 1;}}generator;
struct ObjectMgr{template<HighGuid>Generator* GetGenerator(){return &generator;}}objects;auto* sObjectMgr=&objects;
struct Conversation{bool CreateConversation(uint32,uint32 id,Unit*,void*,Unit&){return false;}};
#include "Acquisition.inc"
#include "Callback.inc"
#include "SetLootState.inc"
bool CompletedLoot(Loot* loot,GameObject* go){
#include "CompleteLoot.inc"
}
struct Fixture{Player p;GameObject go;go_the_kingslayers script;std::unique_ptr<GameObjectAI> ai;Fixture(){manager={};maps.present=true;actors.clear();players.clear();p.guid={1};players[p.guid]=&p;currentScript=&script;ai.reset(script.GetAI(&go));go.ai=ai.get();}void LootWeapon(){Loot loot;p.StoreLootItem(&loot);if(CompletedLoot(&loot,&go))go.SetLootState(GO_JUST_DEACTIVATED,&p);}void Finish(){p.paired=true;ai->UpdateAI(2000);}};

int main(){
#ifdef BASELINE
 Fixture f;f.p.room=false;f.go.SetLootState(GO_ACTIVATED,&f.p);assert(!f.p.owned&&f.p.events==1&&f.p.explicitRewards==1&&f.p.grants==1&&f.p.casts==1&&f.p.directCredits==3);std::cout<<"COUNTERFACTUAL pinned8cb3e9cb activation grants51616/explicit1123 and quest credits before failed AddItem; native loot/equip absent.\n";
#else
 for(uint32 quest:{42504u,42627u}){
  {Fixture f;f.p.questId=quest;f.p.room=false;f.LootWeapon();assert(!f.p.events&&!f.p.owned);f.p.room=true;f.LootWeapon();assert(f.p.owned&&f.p.casts==1&&!f.p.events&&f.go.m_lootState==GO_READY);f.p.paired=true;f.ai->UpdateAI(2000);assert(f.p.events==1&&manager.scenario.step==9&&!f.p.explicitRewards&&!f.p.directCredits&&!f.p.grants);assert(f.p.tasks.size()==2);f.p.Advance(6999);assert(f.p.conversations.empty());f.p.Advance(1);assert(f.p.conversations==std::vector<uint32>{2452});f.p.Advance(6000);assert(f.p.conversations==std::vector<uint32>({2452,2453}));f.p.Advance(14000);assert(door.phase==2&&garona.delay==15000);f.ai->UpdateAI(10000);assert(f.p.events==1);}
  for(int failure=0;failure<4;++failure){Fixture f;f.p.questId=quest;if(failure==0)f.p.storage=false;if(failure==1)f.p.castResult=1;if(failure==2)f.p.bank=true;if(failure==3)f.p.room=false;if(failure==2)f.script.OnGossipHello(&f.p,&f.go);else f.LootWeapon();assert(!f.p.events&&!f.p.grants);f.p.storage=f.p.room=true;f.p.bank=false;f.p.castResult=0;if(f.p.owned)f.script.OnGossipHello(&f.p,&f.go);else f.LootWeapon();f.p.paired=true;f.ai->UpdateAI(2000);assert(f.p.events==1);}
  {Fixture f;f.p.questId=quest;f.LootWeapon();f.ai->UpdateAI(15000);assert(!f.p.events&&f.ai->GetGUID(0).IsEmpty());f.script.OnGossipHello(&f.p,&f.go);f.p.paired=true;f.ai->UpdateAI(1);assert(f.p.events==1);}
  {Fixture f;f.p.questId=quest;f.p.failOutro=true;f.LootWeapon();f.p.paired=true;f.ai->UpdateAI(2000);assert(f.p.events==1&&f.p.tasks.empty());f.p.distance=60;f.ai->UpdateAI(2000);assert(f.p.events==1);f.p.failOutro=false;f.ai->UpdateAI(2000);assert(f.p.tasks.size()==2&&f.p.events==1);}
  {Fixture f;f.p.questId=quest;manager.scenario.accept=false;f.LootWeapon();f.p.paired=true;f.ai->UpdateAI(1);assert(f.p.events==1&&f.p.tasks.empty());f.ai->UpdateAI(1999);assert(f.p.events==1);manager.scenario.accept=true;f.ai->UpdateAI(1);assert(f.p.events==2&&f.p.tasks.size()==2);}
  for(int invalid=0;invalid<10;++invalid){Fixture f;f.p.questId=quest;f.LootWeapon();f.p.paired=true;f.ai->UpdateAI(2000);if(invalid==0)players.clear();if(invalid==1)actors.clear();if(invalid==2)f.p.quest.ScriptLifetime.reset();if(invalid==3)f.p.instance=43;if(invalid==4)f.p.mapId=1;if(invalid==5)f.p.alive=false;if(invalid==6)f.p.phase=false;if(invalid==7)f.p.status=0;if(invalid==8)f.p.quest.ScriptLifetime=std::make_shared<uint8>();if(invalid==9)manager.scenario.id=1;door.phase=1;f.p.Advance(30000);assert(f.p.conversations.empty()&&door.phase==((invalid==1||invalid==9)?1u:2u));}
  {Fixture f;f.p.questId=quest;f.LootWeapon();f.p.quest.ScriptLifetime=std::make_shared<uint8>();f.p.paired=true;f.ai->UpdateAI(2000);assert(!f.p.events);}
 }
 {Fixture f;f.LootWeapon();f.p.paired=true;f.p.onCredit=[&]{f.ai->UpdateAI(2000);};f.ai->UpdateAI(2000);assert(f.p.events==1&&f.p.tasks.size()==2);}
 for(uint32 invalid=0;invalid<8;++invalid){Fixture f;f.LootWeapon();f.p.paired=true;f.ai->UpdateAI(2000);assert(actors.size()==1);auto&actor=actors.begin()->second;if(invalid==0)actor.source={99};if(invalid==1)actor.entry=108222;if(invalid==2)actor.alive=false;if(invalid==3)actor.preDelete=true;if(invalid==4)actor.world=false;if(invalid==5)manager.scenario.step=8;if(invalid==6)manager.scenario.progress[48591].Counter=0;if(invalid==7)maps.present=false;door.phase=1;f.p.Advance(27000);assert(door.phase==1&&f.p.events==1);}
 for(uint32 invalid=0;invalid<27;++invalid){Fixture f;switch(invalid){case 0:f.p.world=false;break;case 1:f.p.contact=false;break;case 2:f.p.alive=false;break;case 3:f.p.teleport=true;break;case 4:f.p.klass=1;break;case 5:f.go.world=false;break;case 6:f.go.spawned=false;break;case 7:f.go.entry=1;break;case 8:f.go.mapId=1;break;case 9:f.p.map=&otherMap;break;case 10:f.p.instance=43;break;case 11:f.p.phase=false;break;case 12:f.p.visible=false;break;case 13:f.p.los=false;break;case 14:f.p.distance=6;break;case 15:f.go.notSelectable=true;break;case 16:f.p.rewarded=true;break;case 17:f.p.status=0;break;case 18:f.p.status=5;break;case 19:f.p.status=6;break;case 20:manager.present=false;break;case 21:manager.scenario.id=1;break;case 22:manager.scenario.step=7;break;case 23:manager.scenario.step=9;break;case 24:manager.scenario.progress.clear();break;case 25:manager.scenario.progress[48589].Counter=0;break;case 26:f.p.map=nullptr;break;}assert(f.script.OnGossipHello(&f.p,&f.go));f.p.owned=true;if(invalid==23)manager.scenario.step=10;f.go.SetLootState(3,&f.p);f.p.paired=true;f.ai->UpdateAI(2000);assert(!f.p.events&&!f.p.grants);}
 std::cout<<"PASS actual native loot storage/release/GO callbacks,27 denials,both quests,full bags/null store/bank/failed scene/equip/native-credit/outro retry,quest identity,reentrant credit,7/13s owner dialogue and27s earned-world cleanup after owner death/logout,8 world denials; native scene/client/save remain separate.\n";
#endif
}

#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
using LootState=uint32;using SpellCastResult=uint32;using ItemPosCountVec=std::vector<uint32>;
enum {CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,GO_READY=1,GO_ACTIVATED=2,GO_JUST_DEACTIVATED=3,
 GAMEOBJECT_FIELD_FLAGS=17,GO_FLAG_NOT_SELECTABLE=16,INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15,EQUIPMENT_SLOT_OFFHAND=16,
 SPELL_CAST_OK=0,TYPEID_PLAYER=4,NULL_BAG=255,NULL_SLOT=255,GAMEOBJECT_TYPE_FISHINGNODE=17};
#include "CriteriaTypes.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid{uint32 value=0;bool IsEmpty()const{return !value;}void Clear(){value=0;}bool operator==(ObjectGuid b)const{return value==b.value;}bool operator<(ObjectGuid b)const{return value<b.value;}};
struct Position{float x,y,z,o;};struct Map{} liveMap,otherMap;
struct Player;struct GameObject;
struct Unit{virtual Player* ToPlayer(){return nullptr;}virtual ~Unit()=default;virtual uint32 GetTypeId(){return 3;}ObjectGuid guid{1};ObjectGuid GetGUID()const{return guid;}uint32 CastSpell(Unit*,uint32,bool);};
struct Item{uint32 entry=128872;ObjectGuid guid{21},child{22};uint32 GetEntry(){return entry;}ObjectGuid GetChildItem(){return child;}ObjectGuid GetGUID(){return guid;}};
struct CriteriaProgress{uint32 Counter=0;};
struct Scenario{uint32 id=1012,step=4;std::map<uint32,CriteriaProgress> progress{{46707,{1}}};uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Scenario& GetAchievementMgr(){return *this;}auto const* GetCriteriaProgressMap(){return &progress;}};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}} manager;
auto* sScenarioMgr=&manager;
enum InventoryResult{EQUIP_ERR_OK,EQUIP_ERR_INVENTORY_FULL};
struct LootItem{struct{uint32 ItemID=128872,RandomPropertiesID=0;struct{std::vector<uint32> BonusListIDs;uint32 Context=0;}ItemBonus;}item;uint32 count=1;bool freeforall=false,is_looted=false;std::set<uint32> GetAllowedLooters(){return {};}};
struct Loot{LootItem item;uint32 gold=0,unlootedCount=1;
#include "IsLooted.inc"
};
struct Player:Unit{
 bool world=true,contact=true,alive=true,teleport=false,rewarded=false,owned=false,bank=false,paired=false,room=true,storage=true,phase=true,visible=true,los=true;
 uint32 klass=4,status=3,instance=42,casts=0,castResult=0,events=0,explicitRewards=0,grants=0,errors=0,mapId=1545;float distance=1;Map* map=&liveMap;
 Item parent,child{134552,{22},{}};
 struct Phase{uint32 changes=0;void SetCustomPhase(uint32 phase){assert(phase==1);++changes;}}phases;
 Player* ToPlayer()override{return this;}uint32 GetTypeId()override{return 4;}
 bool IsInWorld(){return world;}bool CanContact(){return contact;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}uint32 getClass(){return klass;}
 Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}
 bool IsWithinLOSInMap(GameObject*){return los;}bool IsWithinDistInMap(GameObject*,float r){return distance<=r;}
 bool GetQuestRewardStatus(uint32 id){assert(id==40849);return rewarded;}uint32 GetQuestStatus(uint32 id){assert(id==40849);return status;}
 bool HasItemCount(uint32 id,uint32=1,bool withBank=false){assert(id==128872);return owned||withBank&&bank;}
 Item* GetItemByEntry(uint32 id){assert(id==128872);return owned?&parent:nullptr;}
 Item* GetItemByPos(uint32,uint32 slot){return paired&&owned?(slot==15?&parent:&child):nullptr;}
 Phase& GetPhaseMgr(){return phases;}
 void AddItem(uint32,uint32){++grants;owned=true;}
 void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32=0){if(type==CRITERIA_TYPE_SCRIPT_EVENT_2){assert(asset==48553);++events;manager.scenario.progress[46691].Counter=1;if(manager.scenario.progress[46707].Counter)manager.scenario.step=5;}else{assert(type==CRITERIA_TYPE_COMPLETE_SCENARIO);++explicitRewards;}}
 InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec& dest,uint32 id,uint32 count,void*,bool){assert(id==128872&&count==1);if(!room)return EQUIP_ERR_INVENTORY_FULL;dest.push_back(1);return EQUIP_ERR_OK;}
 Item* StoreNewItem(ItemPosCountVec const&,uint32,bool,uint32,std::set<uint32> const&,std::vector<uint32> const&,uint32){if(!storage)return nullptr;owned=true;return &parent;}
 void SendEquipError(InventoryResult,void*,void*,uint32){++errors;}
 void StoreLootItem(Loot* loot){LootItem* item=&loot->item;void* qitem=nullptr;
#include "StoreInventory.inc"
#include "ConsumeLoot.inc"
 }
};
uint32 Unit::CastSpell(Unit* target,uint32 spell,bool triggered){assert(target==this&&spell==202040&&triggered);auto* p=ToPlayer();assert(p);++p->casts;return p->castResult;}
struct Creature{ObjectGuid guid;bool removed=false;uint32 entry=0;ObjectGuid viewer;ObjectGuid GetGUID(){return guid;}void DespawnOrUnsummon(){removed=true;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){viewer=g;}void SetPhaseMask(uint32 phase,bool){assert(phase==1);}};
static std::map<ObjectGuid,Creature> actors;static std::map<ObjectGuid,Player*> players;static uint32 serial=100;
struct GameObjectAI;
struct GameObject{
 bool world=true,spawned=true,notSelectable=false;uint32 entry=254087,mapId=1545,instance=42,m_lootState=GO_READY,failEntry=0,spawnCount=0;Map* map=&liveMap;GameObjectAI* ai=nullptr;
 bool IsInWorld(){return world;}bool isSpawned(){return spawned;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapId;}Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}
 float GetInteractionDistance(){return 5;}bool HasFlag(uint32,uint32){return notSelectable;}uint32 getLootState(){return m_lootState;}GameObjectAI* AI(){return ai;}
 void SetLootState(LootState,Unit* =nullptr);uint32 GetGoType(){return 3;}
 Creature* SummonCreature(uint32 id,Position){if(id==failEntry)return nullptr;auto& c=actors[{++serial}];c.guid={serial};c.entry=id;++spawnCount;return &c;}
 Creature* SummonCreature(uint32 id,float x,float y,float z,float o){return SummonCreature(id,{x,y,z,o});}
 Creature* FindNearestCreature(uint32 id,float distance){assert(id==112817&&distance==20);return nullptr;}
};
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* g):go(g){}virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual void OnStateChanged(uint32,Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual void SetGUID(ObjectGuid const&,int32){}virtual uint32 GetData(uint32){return 0;}virtual void UpdateAI(uint32){};};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual GameObjectAI* GetAI(GameObject* g)const{return new GameObjectAI(g);}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual void OnLootStateChanged(GameObject*,uint32,Unit*){}};
GameObjectScript* currentScript;
struct ScriptMgr{void OnGameObjectLootStateChanged(GameObject* g,uint32 s,Unit* u){currentScript->OnLootStateChanged(g,s,u);}}scripts;auto* sScriptMgr=&scripts;
struct ObjectAccessor{static Player* GetPlayer(GameObject&,ObjectGuid id){auto i=players.find(id);return i==players.end()?nullptr:i->second;}static Creature* GetCreature(GameObject&,ObjectGuid id){auto i=actors.find(id);return i==actors.end()||i->second.removed?nullptr:&i->second;}};
enum class HighGuid{Conversation};
struct Generator{uint32 Generate(){return 1;}}generator;
struct ObjectMgr{template<HighGuid>Generator* GetGenerator(){return &generator;}}objects;auto* sObjectMgr=&objects;
struct Conversation{bool CreateConversation(uint32,uint32 id,Unit*,void*,Unit&){assert(id==1721);return false;}};
#include "Acquisition.inc"
#include "Callback.inc"
#include "SetLootState.inc"
bool CompletedLoot(Loot* loot,GameObject* go){
#include "CompleteLoot.inc"
}
struct Fixture{Player p;GameObject go;go_scenario_dreadblades script;std::unique_ptr<GameObjectAI> ai;Fixture(){manager={};actors.clear();players.clear();p.guid={1};players[p.guid]=&p;currentScript=&script;ai.reset(script.GetAI(&go));go.ai=ai.get();}void LootWeapon(){Loot loot;p.StoreLootItem(&loot);if(CompletedLoot(&loot,&go))go.SetLootState(GO_JUST_DEACTIVATED,&p);}void Finish(){p.paired=true;ai->UpdateAI(2000);}};
int main(){
#ifdef BASELINE
 Fixture f;f.go.SetLootState(GO_ACTIVATED,&f.p);assert(!f.p.owned&&f.p.events==1&&f.p.explicitRewards==1);f.p.owned=true;f.go.SetLootState(GO_ACTIVATED,&f.p);assert(f.p.grants==1);
 std::cout<<"PASS pinned349d886d counterfactual: empty activation credits completion, owned activation duplicates parent.\n";
#else
 for(uint32 invalid=0;invalid<27;++invalid){Fixture f;switch(invalid){case 0:f.p.world=false;break;case 1:f.p.contact=false;break;case 2:f.p.alive=false;break;case 3:f.p.teleport=true;break;case 4:f.p.klass=1;break;case 5:f.go.world=false;break;case 6:f.go.spawned=false;break;case 7:f.go.entry=1;break;case 8:f.go.mapId=1;break;case 9:f.p.map=&otherMap;break;case 10:f.p.instance=43;break;case 11:f.p.phase=false;break;case 12:f.p.visible=false;break;case 13:f.p.los=false;break;case 14:f.p.distance=6;break;case 15:f.go.notSelectable=true;break;case 16:f.p.rewarded=true;break;case 17:f.p.status=0;break;case 18:f.p.status=5;break;case 19:f.p.status=6;break;case 20:manager.present=false;break;case 21:manager.scenario.id=1;break;case 22:manager.scenario.step=3;break;case 23:manager.scenario.step=5;break;case 24:manager.scenario.progress.clear();break;case 25:manager.scenario.progress[46707].Counter=0;break;case 26:f.p.map=nullptr;break;}assert(f.script.OnGossipHello(&f.p,&f.go)&&f.ai->GossipHello(&f.p,false));f.p.owned=true;f.go.SetLootState(GO_JUST_DEACTIVATED,&f.p);f.Finish();assert(!f.p.casts&&!f.p.events&&!f.p.grants&&!f.go.spawnCount);}
 {Fixture f;Unit pet,guardian;f.script.OnLootStateChanged(nullptr,3,&f.p);f.go.SetLootState(3,nullptr);f.go.SetLootState(3,&pet);f.go.SetLootState(3,&guardian);assert(!f.p.events);}
 for(bool storeFailure:{false,true}){Fixture f;Loot loot;if(storeFailure)f.p.storage=false;else f.p.room=false;f.go.SetLootState(GO_ACTIVATED,&f.p);f.p.StoreLootItem(&loot);assert(!loot.isLooted()&&!CompletedLoot(&loot,&f.go)&&!f.p.events&&!f.p.casts);f.p.room=f.p.storage=true;f.p.StoreLootItem(&loot);assert(loot.isLooted()&&f.p.owned);f.go.SetLootState(3,&f.p);assert(!f.p.events&&f.p.casts==1&&f.go.m_lootState==GO_READY);f.ai->UpdateAI(1999);assert(!f.p.events);f.Finish();assert(f.p.events==1&&!f.p.explicitRewards&&!f.p.grants&&f.go.spawnCount==4);f.ai->UpdateAI(5000);assert(f.p.events==1&&f.go.spawnCount==4);}
 {Fixture f;f.p.bank=true;assert(f.script.OnGossipHello(&f.p,&f.go));assert(!f.p.grants&&!f.p.casts);}
 {Fixture f;f.p.owned=true;f.p.castResult=1;warnings=0;assert(f.script.OnGossipHello(&f.p,&f.go));assert(warnings==1&&!f.p.events&&f.go.m_lootState==GO_READY);f.p.castResult=0;f.script.OnGossipHello(&f.p,&f.go);f.Finish();assert(f.p.events==1&&f.p.casts==2&&!f.p.grants);}
 {Fixture f;f.LootWeapon();warnings=0;f.ai->UpdateAI(15000);assert(warnings==1&&!f.p.events&&f.go.m_lootState==GO_READY);f.script.OnGossipHello(&f.p,&f.go);f.Finish();assert(f.p.events==1);}
 for(uint32 missing:{102357u,102356u,102355u,102352u}){Fixture f;f.go.failEntry=missing;f.LootWeapon();f.Finish();assert(!f.p.events&&f.go.m_lootState==GO_READY);uint32 prior=f.go.spawnCount;f.ai->UpdateAI(1000);assert(f.go.spawnCount==prior);f.go.failEntry=0;f.ai->UpdateAI(1000);assert(f.p.events==1&&f.go.spawnCount==4);}
 for(uint32 abort=0;abort<5;++abort){Fixture f;f.LootWeapon();switch(abort){case 0:f.p.alive=false;break;case 1:f.p.status=0;break;case 2:f.p.instance=43;break;case 3:players.clear();break;case 4:f.p.owned=false;break;}f.ai->UpdateAI(3000);assert(!f.p.events&&f.go.m_lootState==GO_READY&&f.ai->GetGUID(0).IsEmpty());}
 {Fixture f;f.LootWeapon();f.p.paired=true;f.p.child.guid={99};f.ai->UpdateAI(2000);assert(!f.p.events);f.p.child.guid={22};f.ai->UpdateAI(1);assert(f.p.events==1);}
 std::cout<<"PASS native loot storage/consumption/release/state dispatcher and complete pickup handlers: admission/real death, full bag/storage retry, native cast/equip boundary, pair identity, no grants/early/double reward, missing actors and stale claimant cleanup. Actual equip spell scheduling/scene/network/client are separate.\n";
#endif
}

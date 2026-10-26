#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
using LootState=uint32;using SpellCastResult=uint32;using SpellEffIndex=uint32;using ItemPosCountVec=std::vector<uint32>;
enum {CLASS_PRIEST=5,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,GO_READY=1,GO_ACTIVATED=2,GO_JUST_DEACTIVATED=3,
 GAMEOBJECT_FIELD_FLAGS=17,GO_FLAG_NOT_SELECTABLE=16,INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,TYPEID_PLAYER=4,NULL_BAG=255,NULL_SLOT=255,GAMEOBJECT_TYPE_FISHINGNODE=17,
 EFFECT_0=0,EFFECT_ALL=255,SPELL_EFFECT_ANY=255,SPELL_EFFECT_TELEPORT_L=252,SCENE_COMPLETE=3,
 SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2};
#include "CriteriaTypes.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
#include "Functions.inc"
struct ObjectGuid{uint32 value=0;uint64 high=0;bool IsEmpty()const{return !value&&!high;}void Clear(){value=0;high=0;}
 bool operator==(ObjectGuid b)const{return value==b.value&&high==b.high;}bool operator!=(ObjectGuid b)const{return !(*this==b);}bool operator<(ObjectGuid b)const{return std::tie(high,value)<std::tie(b.high,b.value);}};
struct WorldObject{virtual ~WorldObject()=default;};struct Unit;struct Player;struct GameObject;struct Spell;struct SpellScript;
struct ZoneScript{void ProcessEvent(WorldObject*,uint32){}};using InstanceScript=ZoneScript;
struct Map{void ScriptsStart(uint32,uint32,Unit*,WorldObject*){}} liveMap,otherMap;uint32 sEventScripts=0;
struct Unit:WorldObject{ObjectGuid guid{1};FunctionProcessor functions;virtual Player* ToPlayer(){return nullptr;}virtual uint32 GetTypeId(){return 3;}
 ObjectGuid GetGUID()const{return guid;}virtual Map* GetMap(){return &liveMap;}ZoneScript* GetZoneScript(){return nullptr;}InstanceScript* GetInstanceScript(){return nullptr;}
 SpellCastResult CastSpell(Unit*,uint32,bool=false);void AddDelayedEvent(uint64 delay,std::function<void()>&& fn){functions.AddDelayedEvent(delay,std::move(fn));}};
struct Item{uint32 entry=128825;ObjectGuid guid{21},child;uint16 position=0xFF20;uint32 GetEntry()const{return entry;}ObjectGuid GetChildItem()const{return child;}ObjectGuid GetGUID()const{return guid;}bool IsEquipped()const{return position==0xFF0F;}uint16 GetPos()const{return position;}};
struct Progress{uint32 Counter=0;};
struct Scenario{uint32 id=1085,step=6;std::map<uint32,Progress> progress{{47656,{1}}};uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}
 Scenario& GetAchievementMgr(){return *this;}auto const* GetCriteriaProgressMap(){return &progress;}};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} manager;auto sScenarioMgr=&manager;
enum InventoryResult{EQUIP_ERR_OK,EQUIP_ERR_INVENTORY_FULL};
struct LootItem{struct{uint32 ItemID=128825,RandomPropertiesID=0;struct{std::vector<uint32> BonusListIDs;uint32 Context=0;}ItemBonus;}item;uint32 count=1;bool freeforall=false,is_looted=false;std::set<uint32> GetAllowedLooters(){return {};}};
struct Loot{LootItem item;uint32 gold=0,unlootedCount=1;
#include "IsLooted.inc"
};
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct WorldLocation{WorldLocation(uint32 m,float a,float b,float c,float d):map(m),x(a),y(b),z(c),o(d){} uint32 map;float x,y,z,o;uint32 GetMapId()const{return map;}float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}};
struct Player:Unit{
 bool world=true,contact=true,alive=true,teleport=false,rewarded=false,owned=false,bank=false,room=true,storage=true,phase=true,visible=true,los=true,equipRoom=true,swapBlocked=false;
 bool synchronous=false,canDelay=false,m_bHasDelayedTeleport=false,scene=false,launched=false,record=true,hideSource=false;uint32 klass=5,status=3,instance=42,mapId=1489,casts=0,castResult=0,acquisitions=0,departures=0,explicitRewards=0,grants=0,errors=0,swaps=0,teleports=0,aborts=0;
 float distance=1;Map* map=&liveMap;QuestStatusData quest;Item parent,old{12345,{31},{},0xFF0F};Item* mainhand=&old;WorldLocation m_teleport_dest{0,0,0,0,0};uint32 m_teleport_options=0;
 std::vector<std::pair<uint32,bool>> castFlags;std::vector<uint32> castIds,otherEvents;std::vector<std::function<void()>> queued;std::function<void()> onCredit;
 Player* ToPlayer()override{return this;}uint32 GetTypeId()override{return 4;}bool IsInWorld(){return world;}bool CanContact(){return world&&contact;}bool IsAlive(){return alive;}
 bool IsBeingTeleported()const{return teleport;}uint32 getClass(){return klass;}uint32 GetMapId(){return mapId;}Map* GetMap()override{return map;}uint32 GetInstanceId(){return instance;}
#include "Pending.inc"
 bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}bool IsWithinLOSInMap(GameObject*){return los;}bool IsWithinDistInMap(GameObject*,float radius){return distance<=radius;}
 bool GetQuestRewardStatus(uint32 id){assert(id==41993);return rewarded;}uint32 GetQuestStatus(uint32 id){assert(id==41993);return status;}
 QuestStatusData* getQuestStatus(uint32 id){assert(id==41993);quest.Status=status;return record?&quest:nullptr;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 bool HasItemCount(uint32 id,uint32=1,bool withBank=false){assert(id==128825);return owned||(withBank&&bank);}Item* GetItemByEntry(uint32 id){assert(id==128825);return owned?&parent:nullptr;}
 Item* GetItemByPos(uint32 bag,uint32 slot){assert(bag==255&&slot==15);return mainhand;}bool HasSceneStatus(uint32 id,uint32 state){assert(id==1265&&state==3);return scene;}
 Item* AddItem(uint32 id,uint32 count){assert(id==128825&&count==1);++grants;if(!room||!storage)return nullptr;owned=true;return &parent;}
 void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32=0){if(type==CRITERIA_TYPE_SCRIPT_EVENT)return;if(type==CRITERIA_TYPE_COMPLETE_SCENARIO){++explicitRewards;return;}
  assert(type==CRITERIA_TYPE_SCRIPT_EVENT_2);if(asset==50679){++acquisitions;manager.scenario.progress[47679].Counter=1;}else if(asset==50702){assert(world&&map==&liveMap);++departures;manager.scenario.progress[47680].Counter=1;if(manager.scenario.progress[47679].Counter)manager.scenario.step=7;if(onCredit)onCredit();}else otherEvents.push_back(asset);}
 InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec& dest,uint32 id,uint32 count,void*,bool){assert(id==128825&&count==1);if(!room)return EQUIP_ERR_INVENTORY_FULL;dest.push_back(1);return EQUIP_ERR_OK;}
 Item* StoreNewItem(ItemPosCountVec const&,uint32,bool,uint32,std::set<uint32> const&,std::vector<uint32> const&,uint32){if(!storage)return nullptr;owned=true;return &parent;}
 void SendEquipError(InventoryResult,void*,void*,uint32){++errors;}void SendEquipError(InventoryResult,Item*){++errors;}
 InventoryResult CanEquipItem(uint32,uint16& dest,Item*,bool){dest=0xFF0F;return equipRoom?EQUIP_ERR_OK:EQUIP_ERR_INVENTORY_FULL;}InventoryResult CanEquipChildItem(Item*){assert(false);return EQUIP_ERR_OK;}
 void SwapItem(uint16 source,uint16 dest){++swaps;if(swapBlocked)return;old.position=source;parent.position=dest;mainhand=&parent;}
 void StoreLootItem(Loot* loot){LootItem* item=&loot->item;void* qitem=nullptr;
#include "StoreInventory.inc"
#include "ConsumeLoot.inc"
 }
 bool IsCanDelayTeleport(){return canDelay;}void SetCanDelayTeleport(bool value){canDelay=value;}bool IsHasDelayedTeleport(){return m_bHasDelayedTeleport;}
 void SetDelayedTeleportFlag(bool value){m_bHasDelayedTeleport=value;}void SetSemaphoreTeleportFar(bool value){teleport=value;}bool isWatchingMovie(){return false;}
#include "DelayScope.inc"
 WorldLocation& GetTeleportDest(){return m_teleport_dest;}
 bool TeleportToFixture(uint32 mapid,float x,float y,float z,float orientation,uint32 options){++teleports;
#include "TeleportDelay.inc"
  world=false;return true;}
 GameObject* FindNearestGameObject(uint32,float);
};
#include "QuestDelay.inc"
struct Creature{bool removed=false;uint32 delay=0;void DespawnOrUnsummon(uint32 value=0){removed=true;delay=value;}};
struct GameObjectAI;
struct GameObject:WorldObject{ObjectGuid guid{11};bool world=true,spawned=true,notSelectable=false;uint32 entry=249705,mapId=1489,instance=42,m_lootState=GO_READY;Map* map=&liveMap;GameObjectAI* ai=nullptr;Creature* visual=nullptr;
 ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool isSpawned(){return spawned;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapId;}Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}
 float GetInteractionDistance(){return 5;}bool HasFlag(uint32,uint32){return notSelectable;}uint32 getLootState(){return m_lootState;}GameObjectAI* AI(){return ai;}
 void SetLootState(LootState,Unit* =nullptr);uint32 GetGoType(){return 3;}Creature* FindNearestCreature(uint32 id,float radius){assert(id==106326&&radius==10);return visual;}};
std::vector<GameObject*> objects;std::vector<Player*> players;
GameObject* Player::FindNearestGameObject(uint32 id,float range){assert(id==249705&&range==5);for(auto* go:objects)if(!hideSource&&go->entry==id&&go->world&&go->map==map&&distance<=range)return go;return nullptr;}
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& result,Player* p,uint32 id,float range){for(auto* go:objects)if(!p->hideSource&&go->entry==id&&go->world&&go->map==p->map&&p->distance<=range)result.push_back(go);}
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* object):go(object){}virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual void OnStateChanged(uint32,Unit*){}
 virtual ObjectGuid GetGUID(int32){return {};}virtual void SetGUID(ObjectGuid const&,int32){}virtual uint32 GetData(uint32){return 0;}virtual void SetData(uint32,uint32){}virtual void UpdateAI(uint32){};};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual GameObjectAI* GetAI(GameObject* go)const{return new GameObjectAI(go);}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual void OnLootStateChanged(GameObject*,uint32,Unit*){}};
GameObjectScript* currentScript;
struct ScriptManager{void OnGameObjectLootStateChanged(GameObject* go,uint32 state,Unit* player){currentScript->OnLootStateChanged(go,state,player);}} scripts;auto sScriptMgr=&scripts;
// Deliberately model the native low-counter lookup; production must compare full identities.
struct ObjectAccessor{static Player* GetPlayer(GameObject& go,ObjectGuid guid){for(auto* p:players)if(p->guid.value==guid.value&&p->world&&p->map==go.map)return p;return nullptr;}
 static GameObject* GetGameObject(Player& p,ObjectGuid guid){for(auto* go:objects)if(go->guid.value==guid.value&&go->world&&go->map==p.map)return go;return nullptr;}};
struct SpellScene{uint32 MiscValue=1265,SceneScriptPackageID=1565;};
#include "Acquisition.inc"
#include "Callback.inc"
#include "SetLootState.inc"
#include "Presentation.inc"
struct Effect{int32 MiscValue=0;uint32 GetProvidedTargetMask()const{return 0;}};
struct SpellInfo{uint32 Id=0;Effect effects[5];Effect* GetEffect(uint32 index,uint32){return &effects[index];}};
struct Spell{Player* player;Unit* m_caster;Unit* unitTarget;GameObject* gameObjTarget=nullptr;WorldObject* focusObject=nullptr;uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;SpellInfo info;SpellInfo* m_spellInfo=&info;
 bool rejectTeleport=false;uint32 destination=1220;float destX=-828.85f;uint32 nativeTeleports=0;
 Spell(Player* p,uint32 id):player(p),m_caster(p),unitTarget(p){info.Id=id;info.effects[0].MiscValue=128825;info.effects[3].MiscValue=id==210733?50679:50737;info.effects[2].MiscValue=54073;}
 void EffectForceEquipItem(SpellEffIndex);void EffectSendEvent(SpellEffIndex);
 void EffectTeleportUnits(SpellEffIndex index){assert(index==0);++nativeTeleports;if(rejectTeleport){++player->aborts;return;}player->TeleportToFixture(destination,destX,4372,738.64f,1.86768f,0);}
 void Default(uint32 index){if(info.Id==210818){EffectTeleportUnits(index);return;}if(info.Id==210734&&index==0){effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;EffectForceEquipItem(index);}
  else if(info.Id==210733&&index==0){player->launched=true;player->scene=false;}
  else if((info.Id==210733&&index==3)||(info.Id==210734&&(index==2||index==3))){effectHandleMode=SPELL_EFFECT_HANDLE_HIT;EffectSendEvent(index);}}
};
#include "ForceEquip.inc"
#include "SendEvent.inc"
struct CheckHook{std::function<SpellCastResult(SpellScript*)> callback;CheckHook& operator+=(CheckHook value){callback=value.callback;return *this;}};
struct EffectHook{struct Entry{uint32 effect,type;std::function<void(SpellScript*,uint32)> callback;};std::vector<Entry> entries;EffectHook& operator+=(Entry value){entries.push_back(value);return *this;}};
struct SpellScript{Spell* spell=nullptr;CheckHook OnCheckCast;EffectHook OnEffectHit,OnEffectHitTarget;bool prevented[5]{};virtual ~SpellScript()=default;virtual bool Load(){return true;}virtual void Register(){}
 Unit* GetCaster(){return spell->m_caster;}Player* GetHitPlayer(){return spell->player;}Spell* GetSpell(){return spell;}SpellInfo* GetSpellInfo(){return &spell->info;}void PreventHitDefaultEffect(uint32 index){prevented[index]=true;}
 void Execute(){uint32 count=spell->info.Id==210818?1:spell->info.Id==210733?4:5;for(uint32 index=0;index<count;++index){auto& hook=((spell->info.Id==210733&&index==3)||(spell->info.Id==210734&&(index==2||index==3)))?OnEffectHit:OnEffectHitTarget;
   for(auto& entry:hook.entries)if(entry.effect==index||entry.effect==EFFECT_ALL)entry.callback(this,index);if(!prevented[index])spell->Default(index);}}
};
template<class T>CheckHook MakeCheck(SpellCastResult(T::*fn)()){return {[fn](SpellScript* script){return (static_cast<T*>(script)->*fn)();}};}
template<class T>EffectHook::Entry MakeEffect(void(T::*fn)(uint32),uint32 effect,uint32 type){return {effect,type,[fn](SpellScript* script,uint32 index){(static_cast<T*>(script)->*fn)(index);}};}
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(fn) MakeCheck(&fn)
#define SpellEffectFn(fn,index,type) MakeEffect(&fn,index,type)
#include "CastState.inc"
struct Request{std::shared_ptr<Spell> spell;std::shared_ptr<SpellScript> script;};std::vector<Request> requests;
SpellCastResult Unit::CastSpell(Unit* target,uint32 id,bool triggered){auto* p=ToPlayer();assert(p&&target==this);++p->casts;p->castIds.push_back(id);p->castFlags.push_back({id,triggered});if(p->castResult)return p->castResult;
 auto spell=std::make_shared<Spell>(p,id);std::shared_ptr<SpellScript> script;
#ifndef BASELINE
 if(id==210818)script=std::make_shared<spell_tuure_departure>();else script=std::make_shared<spell_tuure_pickup_cast>();
 script->spell=spell.get();script->Register();script->Load();auto checked=script->OnCheckCast.callback(script.get());if(checked!=SPELL_CAST_OK)return checked;
#endif
 requests.push_back({spell,script});auto run=[spell,script](){if(script)script->Execute();else{uint32 count=spell->info.Id==210818?1:spell->info.Id==210733?4:5;for(uint32 i=0;i<count;++i)spell->Default(i);}};if(p->synchronous)run();else p->queued.push_back(run);return SPELL_CAST_OK;}
bool CompletedLoot(Loot* loot,GameObject* go){
#include "CompleteLoot.inc"
}
struct Fixture{Player p;std::unique_ptr<GameObject> go=std::make_unique<GameObject>();go_art_tuure script;std::unique_ptr<GameObjectAI> ai;Creature visual;
 Fixture(){warnings=0;manager={};requests.clear();objects={go.get()};players={&p};currentScript=&script;ai.reset(script.GetAI(go.get()));go->ai=ai.get();go->visual=&visual;}
 ~Fixture(){objects.clear();players.clear();requests.clear();}
 void Flush(){auto q=std::move(p.queued);p.queued.clear();for(auto& fn:q)fn();}
 void LootStaff(){Loot loot;p.StoreLootItem(&loot);if(CompletedLoot(&loot,go.get()))go->SetLootState(GO_JUST_DEACTIVATED,&p);}
 void SceneComplete(){p.scene=true;TuurePresentation::Complete(&p,&Scene(),"complete");}
 static SpellScene& Scene();
};

SpellScene& Fixture::Scene(){static SpellScene scene;return scene;}
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef BASELINE
 Fixture f;f.p.room=false;f.go->SetLootState(GO_ACTIVATED,&f.p);assert(!f.p.owned&&f.p.acquisitions==1&&f.p.grants==1);
 f.p.functions.Update(14000);f.Flush();assert(!f.p.owned&&f.p.departures==1&&f.p.explicitRewards==1&&f.p.teleports==1);
 std::cout<<"PASS pinned d9137d5a counterfactual: full-bag activation gives50679/direct failed AddItem then14s50702/explicit1085 and return before actual storage/equip. Expected failing ordinary behavior.\n";
#else
 uint32 variants=0;
 for(uint32 status:{1u,3u})for(bool sync:{false,true})for(bool inherited:{false,true})for(bool missing:{false,true}){
  Fixture f;f.p.status=status;f.p.synchronous=sync;f.p.canDelay=inherited;if(missing)f.go->visual=nullptr;
  f.go->SetLootState(GO_ACTIVATED,&f.p);assert(!f.p.acquisitions&&!f.p.grants&&!f.p.casts);
  f.LootStaff();assert(f.p.owned&&f.go->getLootState()==GO_READY);f.Flush();assert(f.p.acquisitions==1&&!f.p.departures&&!f.p.grants&&!f.p.explicitRewards);
  f.go->SetLootState(GO_JUST_DEACTIVATED,&f.p);assert(std::count(f.p.castIds.begin(),f.p.castIds.end(),210733u)==1);
  f.SceneComplete();f.Flush();assert(f.p.mainhand==&f.p.parent&&f.p.otherEvents==std::vector<uint32>({54073,50737}));
  f.ai->UpdateAI(13999);assert(!f.p.departures&&!f.p.teleports);f.ai->UpdateAI(1);if(!sync)assert(!f.p.departures&&!f.p.teleports);
  f.Flush();assert(f.p.castFlags.back()==std::make_pair(210818u,false));assert(f.p.departures==1&&f.p.teleports==1&&f.p.world&&f.p.HasPendingTeleport()&&f.p.canDelay==inherited);
  assert(manager.scenario.step==7&&f.ai->GetData(TuureAcquisition::Finished)&&f.go->getLootState()==GO_JUST_DEACTIVATED&&!f.p.explicitRewards&&!f.p.grants);
  if(!missing)assert(f.visual.removed&&f.visual.delay==1000);f.ai->UpdateAI(15000);assert(f.p.departures==1);++variants;
 }
 for(uint32 bad=0;bad<28;++bad){Fixture f;switch(bad){case 0:f.p.world=false;break;case 1:f.p.contact=false;break;case 2:f.p.alive=false;break;case 3:f.p.teleport=true;break;case 4:f.p.klass=1;break;case 5:f.go->world=false;break;case 6:f.go->spawned=false;break;case 7:f.go->entry=1;break;case 8:f.go->mapId=1;break;case 9:f.p.map=&otherMap;break;case 10:f.p.instance=43;break;case 11:f.p.phase=false;break;case 12:f.p.visible=false;break;case 13:f.p.los=false;break;case 14:f.p.distance=6;break;case 15:f.go->notSelectable=true;break;case 16:f.p.rewarded=true;break;case 17:f.p.status=0;break;case 18:f.p.status=5;break;case 19:f.p.status=6;break;case 20:manager.present=false;break;case 21:manager.scenario.id=972;break;case 22:manager.scenario.step=5;break;case 23:manager.scenario.step=7;break;case 24:manager.scenario.progress.clear();break;case 25:manager.scenario.progress[47656].Counter=0;break;case 26:f.p.map=nullptr;break;case 27:f.p.m_bHasDelayedTeleport=true;break;}
  assert(f.script.OnGossipHello(&f.p,f.go.get())&&f.ai->GossipHello(&f.p,false));f.p.owned=true;f.go->SetLootState(GO_JUST_DEACTIVATED,&f.p);f.ai->UpdateAI(15000);f.Flush();assert(!f.p.casts&&!f.p.acquisitions&&!f.p.departures&&!f.p.grants&&!f.visual.removed);
 }
 for(bool allocation:{false,true}){Fixture f;Loot loot;if(allocation)f.p.storage=false;else f.p.room=false;f.go->SetLootState(GO_ACTIVATED,&f.p);f.p.StoreLootItem(&loot);assert(!loot.isLooted()&&!CompletedLoot(&loot,f.go.get())&&!f.p.owned&&!f.p.acquisitions&&!f.p.casts);f.p.room=f.p.storage=true;f.p.StoreLootItem(&loot);assert(CompletedLoot(&loot,f.go.get())&&f.p.owned);f.go->SetLootState(GO_JUST_DEACTIVATED,&f.p);f.Flush();assert(f.p.acquisitions==1&&!f.p.departures);}
 {Fixture f;GameObject foreign;foreign.guid={22};struct ForeignAI:GameObjectAI{using GameObjectAI::GameObjectAI;ObjectGuid GetGUID(int32)override{return {2};}} foreignAI(&foreign);foreign.ai=&foreignAI;objects.insert(objects.begin(),&foreign);f.LootStaff();f.Flush();assert(f.p.acquisitions==1&&std::static_pointer_cast<spell_tuure_pickup_cast>(requests.front().script)->state.source==f.go->guid);}
 {Fixture f;f.p.bank=true;assert(f.script.OnGossipHello(&f.p,f.go.get()));assert(!f.p.owned&&!f.p.casts&&!f.p.grants);}
 {Fixture f;f.p.owned=true;f.p.castResult=1;f.script.OnGossipHello(&f.p,f.go.get());assert(f.ai->GetGUID(0).IsEmpty()&&!f.p.acquisitions&&warnings==1);f.p.castResult=0;f.script.OnGossipHello(&f.p,f.go.get());f.Flush();assert(f.p.acquisitions==1&&!f.p.grants);}
 {Fixture f;f.LootStaff();f.Flush();f.ai->UpdateAI(30000);assert(f.ai->GetGUID(0).IsEmpty()&&!f.p.departures&&warnings==1&&f.p.owned);f.script.OnGossipHello(&f.p,f.go.get());f.Flush();f.SceneComplete();f.Flush();f.ai->UpdateAI(14000);f.Flush();assert(f.p.departures==1&&!f.p.grants);}
 {Fixture f;f.LootStaff();auto held=f.p.quest.ScriptLifetime;f.p.quest.ScriptLifetime.reset();f.p.AddQuestDelayedEvent(41993,0,[]{});f.Flush();assert(!f.p.acquisitions&&!f.p.launched&&!f.p.departures);f.ai->UpdateAI(1);assert(f.ai->GetGUID(0).IsEmpty());}
 {Fixture f;f.LootStaff();f.Flush();f.SceneComplete();f.p.quest.ScriptLifetime=std::make_shared<uint8>();f.Flush();assert(!f.p.swaps&&f.p.otherEvents.empty()&&!f.p.departures);}
 for(uint32 late=0;late<12;++late){Fixture f;f.LootStaff();f.Flush();f.SceneComplete();f.Flush();f.ai->UpdateAI(14000);assert(!f.p.departures&&!f.p.teleports);switch(late){case 0:f.p.quest.ScriptLifetime.reset();f.p.AddQuestDelayedEvent(41993,0,[]{});break;case 1:f.p.distance=6;break;case 2:f.p.map=&otherMap;break;case 3:f.p.status=0;break;case 4:f.p.owned=false;break;case 5:f.p.guid.high=1;break;case 6:f.go->guid.high=1;break;case 7:objects.clear();f.ai.reset();f.go.reset();break;case 8:f.p.mainhand=&f.p.old;break;case 9:f.p.instance=43;break;case 10:f.p.scene=false;break;case 11:manager.scenario.step=5;break;}f.Flush();assert(!f.p.departures&&!f.p.teleports&&!f.visual.removed);}
 {Fixture f;f.LootStaff();f.Flush();f.SceneComplete();f.Flush();f.p.castResult=SPELL_FAILED_BAD_TARGETS;f.ai->UpdateAI(14000);f.Flush();assert(!f.p.teleports&&!f.p.departures&&warnings==1&&!f.ai->GetGUID(0).IsEmpty());f.p.castResult=0;f.ai->UpdateAI(5000);f.Flush();assert(f.p.departures==1&&f.p.castFlags.back()==std::make_pair(210818u,false));}
 for(uint32 rejected=0;rejected<3;++rejected){Fixture f;f.LootStaff();f.Flush();f.SceneComplete();f.Flush();f.ai->UpdateAI(14000);if(rejected==0)requests.back().spell->rejectTeleport=true;if(rejected==1)requests.back().spell->destination=1;if(rejected==2)requests.back().spell->destX+=1;f.Flush();assert(!f.p.departures&&!f.ai->GetData(TuureAcquisition::Finished)&&!f.visual.removed&&warnings==1);if(rejected==0){f.ai->UpdateAI(5000);f.Flush();assert(f.p.departures==1&&f.p.aborts==1&&f.p.teleports==1);}}
 {Fixture f;f.LootStaff();f.Flush();f.SceneComplete();f.Flush();f.ai->UpdateAI(14000);auto request=requests.back();f.p.onCredit=[&](){request.script->Execute();};f.Flush();request.script->Execute();assert(f.p.departures==1&&request.spell->nativeTeleports==1);}
 for(uint32 context=0;context<2;++context){Fixture f;if(context==0)f.p.mapId=1712;else manager.scenario.id=972;assert(f.p.CastSpell(&f.p,210818,true)==SPELL_CAST_OK);f.Flush();assert(f.p.teleports==1&&!f.p.departures&&!f.visual.removed);}
 {Fixture f;f.LootStaff();f.Flush();f.SceneComplete();f.Flush();f.ai->UpdateAI(13999);assert(f.p.CastSpell(&f.p,210818,true)==SPELL_FAILED_BAD_TARGETS);f.Flush();assert(!f.p.teleports&&!f.p.departures);f.ai->UpdateAI(1);f.Flush();assert(f.p.departures==1);}
 {Fixture f;f.p.record=false;f.p.owned=true;f.script.OnGossipHello(&f.p,f.go.get());f.Flush();assert(f.ai->GetGUID(0).IsEmpty()&&!f.p.acquisitions&&!f.p.teleports);}
 {Fixture f;f.p.owned=true;assert(f.p.CastSpell(&f.p,210733,true)==SPELL_FAILED_BAD_TARGETS);assert(f.p.CastSpell(&f.p,210818,true)==SPELL_FAILED_BAD_TARGETS);f.Flush();assert(!f.p.launched&&!f.p.acquisitions&&!f.p.teleports);}
 {Fixture f;f.LootStaff();f.Flush();f.p.equipRoom=false;f.SceneComplete();f.Flush();assert(f.p.errors==1&&f.p.mainhand==&f.p.old);f.ai->UpdateAI(29999);assert(!f.p.departures);f.ai->UpdateAI(1);assert(f.ai->GetGUID(0).IsEmpty()&&f.p.owned&&!f.p.grants);}
 std::cout<<"PASS full extracted Tuure GO/claim and registered spell hooks, native storage/release/force-equip/send-event/quest-lease/teleport-deferral: "<<variants<<" status/synchronous/deferred/inherited-delay/optional-actor variants,28 admission denials, full bags/allocation failure, bank/owned replay, timeout/retry, queued same-tick abandon/scene/equip/exit races, freed source/full-GUID replacement, failed/wrong native transfer and repeated/reentrant exactly-once50702. Transfer prechecks/packet delivery/inventory placement are controlled adapters; actual client traversal remains pending.\n";
#endif
}

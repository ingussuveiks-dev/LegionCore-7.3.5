#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#ifdef ALUNETH
constexpr uint32 TestScenario=1101,TestQuest=42011,TestClass=8,TestOtherClass=5,TestOtherScenario=1065,TestItem=127857,TestChest=249814,TestSource=106756,TestSpell=211954,TestScene=1269,TestPackage=1717,TestOwned=47999,TestControl=47997,TestControl2=55130,TestControl3=55131,TestFinal=48000;
constexpr float TestOrientation=2.21201f;
#else
constexpr uint32 TestScenario=1065,TestQuest=41631,TestClass=5,TestOtherClass=8,TestOtherScenario=1101,TestItem=128868,TestChest=248572,TestSource=104985,TestSpell=208011,TestScene=1244,TestPackage=1600,TestOwned=47214,TestControl=47119,TestControl2=0,TestControl3=0,TestFinal=47172;
constexpr float TestOrientation=0.633369f;
#endif
using LootState=uint32;using SpellEffIndex=uint32;struct ItemPosCountVec:std::vector<uint32>{void emplace_back(std::uint16_t,uint32){assert(false);}};
#include "NativeEnums.inc"
constexpr uint32 CLASS_MAGE=8,CLASS_PRIEST=5,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
constexpr uint32 EFFECT_0=0,EFFECT_1=1,EFFECT_2=2,EFFECT_ALL=255,SPELL_EFFECT_ANY=0,SPELL_AURA_MOD_ROOT=26,SPELL_AURA_ACTIVATE_SCENE=430;
constexpr uint32 SPELL_EFFECT_FORCE_EQUIP_ITEM=249,SPELL_EFFECT_TELEPORT_L=252;
constexpr uint32 GO_READY=1,GO_JUST_DEACTIVATED=3,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_NOT_SELECTABLE=16,GO_FLAG_IN_USE=1;
constexpr uint32 INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15,NULL_SLOT=255,NULL_BAG=255,GAMEOBJECT_TYPE_FISHINGNODE=17;
constexpr uint32 SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,PHASE_UPDATE_FLAG_ZONE_UPDATE=1;
using CriteriaTypes=uint32;constexpr uint32 CRITERIA_TYPE_OWN_ITEM=36,CRITERIA_TYPE_SCRIPT_EVENT=46,CRITERIA_TYPE_SCRIPT_EVENT_2=92;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
enum InventoryResult{EQUIP_ERR_OK,EQUIP_ERR_INVENTORY_FULL};
constexpr InventoryResult EQUIP_ERR_INV_FULL=EQUIP_ERR_INVENTORY_FULL;

uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
#define PrepareSpellScript(x) public:
#define PrepareAuraScript(x) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0
#define AuraEffectApplyFn(...) 0
#define AuraEffectRemoveFn(...) 0
#define AuraEffectUpdateFn(...) 0
struct Hooks{template<class T>void operator+=(T){}};
struct ObjectGuid{uint64 high=1,low=0;uint32 entry=0,kind=0;static ObjectGuid Empty;
 bool IsEmpty()const{return !high&&!low;}bool IsCreatureOrVehicle()const{return kind==2;}bool IsGameObject()const{return kind==3;}uint32 GetEntry()const{return entry;}
 bool operator==(ObjectGuid const&o)const{return std::tie(high,low,entry,kind)==std::tie(o.high,o.low,o.entry,o.kind);}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}bool operator<(ObjectGuid const&o)const{return std::tie(high,low,entry,kind)<std::tie(o.high,o.low,o.entry,o.kind);}};ObjectGuid ObjectGuid::Empty{0,0,0,0};
struct Position{float x=1117.78f,y=1048.03f,z=310.663f,o=0;Position const& GetPosition()const{return *this;}};
struct WorldLocation:Position{uint32 map=0;WorldLocation()=default;WorldLocation(uint32 m,float a,float b,float c,float d){map=m;x=a;y=b;z=c;o=d;}uint32 GetMapId()const{return map;}float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}};
struct Player;struct Unit;struct GameObject;struct AuraRecord;struct AuraEffect;struct Spell;struct GameObjectAI;
struct ZoneScript;using InstanceScript=ZoneScript;
struct Reference{Player* player;Player* getSource()const{return player;}};
struct Map{uint32 id=42,mapId=1583;std::vector<Reference>players;std::vector<std::shared_ptr<GameObject>>objects;bool lowLookup=false;
 auto const& GetPlayers()const{return players;}GameObject* GetGameObject(ObjectGuid const&);template<class...T>void ScriptsStart(T...){} };
struct WorldObject:Position{Map* map=nullptr;ObjectGuid guid;uint32 entry=0,phase=6140;bool world=true,preDelete=false;
 virtual ~WorldObject()=default;virtual Player* ToPlayer(){return nullptr;}ObjectGuid GetGUID()const{return guid;}uint32 GetEntry()const{return entry;}Map* GetMap()const{return map;}
 uint32 GetMapId()const{return map?map->mapId:0;}uint32 GetInstanceId()const{return map?map->id:0;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}
 bool InSamePhase(WorldObject*o)const{return o&&o->phase==phase;}float GetExactDist(WorldObject*o)const{return std::sqrt((x-o->x)*(x-o->x)+(y-o->y)*(y-o->y)+(z-o->z)*(z-o->z));}
 bool IsWithinDist3d(float a,float b,float c,float r,bool=false)const{return (x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c)<=r*r;}
 void GetGameObjectListWithEntryInGrid(std::list<GameObject*>&,uint32,float);
};
struct Unit:WorldObject{std::map<std::pair<uint32,ObjectGuid>,std::shared_ptr<AuraRecord>>auras;bool alive=true;virtual ~Unit()=default;
 bool IsAlive()const{return alive;}bool HasAura(uint32,ObjectGuid=ObjectGuid::Empty)const;AuraRecord* GetAura(uint32,ObjectGuid=ObjectGuid::Empty);void RemoveAurasDueToSpell(uint32,ObjectGuid=ObjectGuid::Empty);
 virtual SpellCastResult CastSpell(Unit*,uint32,bool){assert(false);return SPELL_FAILED_BAD_TARGETS;}ZoneScript* GetZoneScript(){return nullptr;}InstanceScript* GetInstanceScript(){return nullptr;}};
struct ItemTemplate{uint32 GetArtifactID()const{return 1;}uint32 GetInventoryType()const{return 13;}};
constexpr uint32 INVTYPE_2HWEAPON=17;
struct Item{ItemTemplate const*GetTemplate()const{static ItemTemplate t;return &t;}uint32 entry=TestItem;uint16 pos=0xFF20;ObjectGuid child=ObjectGuid::Empty;bool IsEquipped()const{return pos==0xFF0F;}uint16 GetPos()const{return pos;}ObjectGuid GetChildItem()const{return child;}};
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8>ScriptLifetime;};
struct ProgressCounter{uint64 Counter=0;};
struct Scenario{uint32 id=TestScenario,step=6;std::map<uint32,ProgressCounter>counters{{TestControl,{1}},{TestControl2,{1}},{TestControl3,{1}}};uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}
 Scenario& GetAchievementMgr(){return *this;}auto const* GetCriteriaProgressMap()const{return &counters;}};
struct ScenarioMgr{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}}manager;auto sScenarioMgr=&manager;
struct SpellScene{uint32 MiscValue=TestScene,SceneScriptPackageID=TestPackage,PlaybackFlags=21,CustomDuration=0;};
struct SpellManager{SpellScene scene;bool present=true;SpellScene const* GetSpellScene(uint32 id){return present&&id==TestScene?&scene:nullptr;}}spellManager;auto sSpellMgr=&spellManager;
namespace WorldPackets{namespace Scene{struct PlayScene{uint32 SceneID=0,PlaybackFlags=0,SceneInstanceID=0,SceneScriptPackageID=0;Position Pos;void* Write(){return nullptr;}};struct CancelScene{explicit CancelScene(uint32){}void* Write(){return nullptr;}};struct SceneInstance{uint32 SceneInstanceID;};}}
struct PhaseMgr{void RemoveUpdateFlag(uint32){}};
struct LootItem{struct{uint32 ItemID=TestItem,RandomPropertiesID=0;struct{std::vector<uint32>BonusListIDs;uint32 Context=0;}ItemBonus;}item;uint32 count=1;bool freeforall=false,is_looted=false;std::set<uint32>GetAllowedLooters(){return {};}};
struct Loot{uint32 objGuid=0;LootItem item;uint32 gold=0,unlootedCount=1;
#include "IsLooted.inc"
};
struct GameObject;struct Unit;
struct ScriptMgr{ template<class... Args> bool CanAutoEquipLoot(Args&&...){return false;}void OnGameObjectLootStateChanged(GameObject*,uint32,Unit*);}scripts;auto sScriptMgr=&scripts;
struct Player:Unit{
 uint32 klass=TestClass,status=3,questCredit=0,ownEvents=0,finalEvents=0,other97377=0,pickupCasts=0,returnCasts=0,nativeEvents=0,errors=0,teleports=0,sceneInstanceID=0,legacyPortalEvents=0;
 bool contact=true,gm=false,teleport=false,m_bHasDelayedTeleport=false,canDelay=false,rewarded=false,owned=false,bank=false,record=true,visible=true,los=true,room=true,storage=true,equipRoom=true,swapBlocked=false,failCast=false,deferCast=false,rejectTeleport=false;
 QuestStatusData quest;Item staff,old{12345,0xFF0F,ObjectGuid::Empty};Item* mainhand=&old;WorldLocation m_teleport_dest;uint32 m_teleport_options=0;PhaseMgr phases;
 std::map<uint32,uint32>m_sceneInstanceID;std::map<uint32,SceneEventStatus>m_sceneStatus;std::vector<std::function<void()>>functions;std::function<void()>onCredit,onTeleport,onEquip;
 Player(){guid={1,7,0,1};}Player* ToPlayer()override{return this;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}uint32 getClass()const{return klass;}
 bool IsBeingTeleported()const{return teleport;}
#include "Pending.inc"
 bool GetQuestRewardStatus(uint32 q){assert(q==TestQuest);return rewarded;}uint32 GetQuestStatus(uint32 q){return q==TestQuest?status:0;}QuestStatusData*getQuestStatus(uint32 q){if(q!=TestQuest||!record||!status)return nullptr;quest.Status=status;return &quest;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 npc){assert(q==TestQuest&&npc==105109);return questCredit;}
 bool HasItemCount(uint32 id,uint32=1,bool b=false){assert(id==TestItem);return owned||(b&&bank);}Item* GetItemByEntry(uint32 id){assert(id==TestItem);return owned?&staff:nullptr;}
 Item* GetItemByPos(uint32 bag,uint32 slot){assert(bag==255&&slot==15);return mainhand;}
 bool canSeeOrDetect(WorldObject*)const{return visible;}bool IsWithinLOSInMap(WorldObject*)const{return los;}
 void AddDelayedEvent(uint64,std::function<void()>&&f){functions.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32=0){if(type==CRITERIA_TYPE_OWN_ITEM){assert(asset==TestItem);++ownEvents;manager.scenario.counters[TestOwned].Counter=1;}else if(type==CRITERIA_TYPE_SCRIPT_EVENT_2&&asset==49875){++finalEvents;assert(map&&map->mapId==1583&&m_bHasDelayedTeleport);manager.scenario.counters[TestFinal].Counter=1;if(manager.scenario.counters[TestOwned].Counter)manager.scenario.step=7;}else if(type==CRITERIA_TYPE_SCRIPT_EVENT_2&&asset==49926)++nativeEvents;if(onCredit){auto f=onCredit;f();}}
 InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec&dest,uint32 id,uint32 count,void*,bool){assert(id==TestItem&&count==1);if(!room)return EQUIP_ERR_INVENTORY_FULL;dest.push_back(1);return EQUIP_ERR_OK;}
 Item* StoreNewItem(ItemPosCountVec const&,uint32,bool,uint32,std::set<uint32>const&,std::vector<uint32>const&,uint32,bool refund=false,bool autoEquip=false) {assert(!refund&&!autoEquip);if(!storage)return nullptr;owned=true;return &staff;}
 void SendEquipError(InventoryResult,void*,void*,uint32){++errors;}void SendEquipError(InventoryResult,Item*){++errors;}
 InventoryResult CanEquipItem(uint32,uint16&dest,Item*,bool,bool notLoading,bool mailOffhand){assert(notLoading&&!mailOffhand);dest=0xFF0F;return equipRoom?EQUIP_ERR_OK:EQUIP_ERR_INVENTORY_FULL;}InventoryResult CanEquipChildItem(Item*){return EQUIP_ERR_OK;}
 void SwapItem(uint16 source,uint16 dest,bool mailOffhand){assert(!mailOffhand);if(!swapBlocked){old.pos=source;staff.pos=dest;mainhand=&staff;}if(onEquip){auto f=onEquip;f();}}
 void StoreLootItem(Loot*loot){LootItem*item=&loot->item;void*qitem=nullptr;
#include "StoreInventory.inc"
#include "ConsumeLoot.inc"
 }
 bool IsCanDelayTeleport(){return canDelay;}void SetCanDelayTeleport(bool b){canDelay=b;}bool IsHasDelayedTeleport(){return m_bHasDelayedTeleport;}void SetDelayedTeleportFlag(bool b){m_bHasDelayedTeleport=b;}
 void SetSemaphoreTeleportFar(bool b){teleport=b;}bool isWatchingMovie(){return false;}
#include "DelayScope.inc"
 WorldLocation&GetTeleportDest(){return m_teleport_dest;}
 bool TeleportToFixture(uint32 mapid,float x,float y,float z,float orientation,uint32 options){++teleports;if(onTeleport){auto f=onTeleport;f();}
#include "TeleportDelay.inc"
  world=false;return true;}
 SpellCastResult CastSpell(Unit*,uint32,bool)override;
 bool HasSceneStatus(uint32,SceneEventStatus const&)const;void SceneCompleted(uint32);void SendSpellScene(uint32,struct SpellInfo const*,bool,Position*);
 void SendDirectMessage(void*){}void TrigerScene(uint32,std::string const&){}PhaseMgr&GetPhaseMgr(){return phases;}
 std::vector<AuraEffect*>GetAuraEffectsByType(uint32);
};
#include "QuestDelay.inc"
struct GameObjectAI{GameObject*go;explicit GameObjectAI(GameObject*o):go(o){}virtual~GameObjectAI()=default;virtual void Reset(){}virtual void UpdateAI(uint32){}virtual bool GossipUse(Player*){return false;}virtual bool GossipHello(Player*,bool){return false;}virtual void OnStateChanged(uint32,Unit*){}};
struct SmartGameObjectAI:GameObjectAI{explicit SmartGameObjectAI(GameObject*o):GameObjectAI(o){}bool GossipUse(Player*p)override{return GossipHello(p,true);}bool GossipHello(Player*p,bool)override{++p->legacyPortalEvents;return false;}};
struct GameObject:WorldObject{GameObjectAI*ai=nullptr;ObjectGuid summoner{1,10,TestSource,2},owner=ObjectGuid::Empty;bool spawned=true,failSpawn=false;uint32 flags=0,m_lootState=GO_READY,spawnCalls=0;std::function<void()>onSpawn;
 GameObject(){entry=TestChest;guid={1,11,TestChest,3};}bool isSpawned()const{return spawned;}bool HasFlag(uint32,uint32 f)const{return flags&f;}ObjectGuid GetSummonerGUID()const{return summoner;}ObjectGuid GetOwnerGUID()const{return owner;}
 GameObjectAI*AI(){return ai;}void SetLootState(LootState,Unit* =nullptr);uint32 GetGoType(){return 3;}GameObject*SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32,ObjectGuid,void*,bool);
 uint32 getLootState()const{return m_lootState;}
};
GameObject*Map::GetGameObject(ObjectGuid const&guid){for(auto o:objects)if(o&&o->world&&o->map==this&&(lowLookup?o->guid.low==guid.low:o->guid==guid))return o.get();return nullptr;}
void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>&out,uint32 id,float radius){if(map)for(auto o:map->objects)if(o&&o->world&&o->entry==id&&GetExactDist(o.get())<=radius)out.push_back(o.get());}
void ScriptMgr::OnGameObjectLootStateChanged(GameObject*,uint32,Unit*){}
#include "SetLootState.inc"
struct EffectInfo{uint32 Effect=0,ApplyAuraName=0,MiscValue=0,TriggerSpell=0;uint32 GetProvidedTargetMask()const{return 1;}};
struct SpellInfo{uint32 Id;EffectInfo data[6];EffectInfo*Effects[6]={data,data+1,data+2,data+3,data+4,data+5};explicit SpellInfo(uint32 id=TestSpell):Id(id){if(id==TestSpell){data[0]={6,430,TestScene,0};data[1]={6,26,0,0};data[2]={249,0,TestItem,0};data[3]={90,0,105109,0};data[4]={61,0,49926,0};data[5]={64,0,0,225879};}else{data[0]={90,0,97377,0};data[1]={252,0,0,0};}}int32 GetDuration()const{return 9000;}EffectInfo*GetEffect(uint32 i,uint32){return &data[i];}};
struct Spell{Unit*m_caster;Unit*unitTarget;GameObject*gameObjTarget=nullptr;WorldObject*focusObject=nullptr;uint32 effectHandleMode=1,m_diffMode=0;SpellInfo info;SpellInfo*m_spellInfo=&info;uint32 destination=1220;float destX=-828.61f;
 Spell(Player*p,uint32 id):m_caster(p),unitTarget(p),info(id){}void EffectForceEquipItem(uint32);void EffectSendEvent(uint32);void EffectTeleportUnits(uint32 i){assert(i==1);auto*p=m_caster->ToPlayer();if(!p->rejectTeleport)p->TeleportToFixture(destination,destX,4371.63f,738.64f,1.89368f,0);}
};
#include "ForceEquip.inc"
struct ZoneScript{void ProcessEvent(WorldObject*,uint32){}};using InstanceScript=ZoneScript;uint32 sEventScripts=0;
// Unit's null script accessors are typed for the actual extracted SendEvent.
#include "SendEvent.inc"
struct SpellScript{Spell*spell=nullptr;Hooks OnCheckCast,OnEffectHitTarget,AfterHit;bool blocked[6]{};virtual~SpellScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}
 SpellInfo const* GetSpellInfo(){return &spell->info;}Unit*GetCaster(){return spell->m_caster;}Unit*GetHitUnit(){return spell->unitTarget;}Player*GetHitPlayer(){return spell->unitTarget->ToPlayer();}Spell*GetSpell(){return spell;}void PreventHitDefaultEffect(uint32 i){blocked[i]=true;}};
struct AuraEffect{uint32 index=1,misc=TestScene,id=TestSpell;uint32 GetEffIndex()const{return index;}uint32 GetMiscValue()const{return misc;}uint32 GetId()const{return id;}};
struct AuraApplication{uint32 mode=0;uint32 GetRemoveMode()const{return mode;}};
struct AuraScript{std::weak_ptr<AuraRecord>record;Hooks AfterEffectApply,OnEffectUpdate,AfterEffectRemove;virtual~AuraScript()=default;virtual bool Load(){return true;}virtual void Register(){}
 SpellInfo const*GetSpellInfo();Unit*GetUnitOwner();ObjectGuid GetCasterGUID();AuraApplication*GetTargetApplication();void Remove();int32 GetDuration();int32 GetMaxDuration();AuraRecord*GetAura();};
struct AuraRecord:std::enable_shared_from_this<AuraRecord>{Unit*owner=nullptr;ObjectGuid caster;uint32 spell=TestSpell;int32 m_duration=9000,m_allDuration=0,maxDuration=9000;bool removed=false;AuraEffect root,scene{0};AuraApplication application;std::shared_ptr<AuraScript>script;
 bool IsExpired()const{return !m_duration;}int32 GetAllDuration()const{return m_allDuration;}void Remove();void Advance(uint32 diff){
#include "NativeDuration.inc"
 }};
Unit*AuraScript::GetUnitOwner(){return record.lock()->owner;}ObjectGuid AuraScript::GetCasterGUID(){return record.lock()->caster;}AuraApplication*AuraScript::GetTargetApplication(){return &record.lock()->application;}
int32 AuraScript::GetDuration(){return record.lock()->m_duration;}int32 AuraScript::GetMaxDuration(){return record.lock()->maxDuration;}AuraRecord*AuraScript::GetAura(){return record.lock().get();}
SpellInfo const* AuraScript::GetSpellInfo(){static SpellInfo info(TestSpell);return &info;}
using Aura=AuraRecord;using AuraEffectList=std::vector<AuraEffect*>;
#include "Pickup.inc"
void RemoveAura(Unit*owner,std::shared_ptr<AuraRecord>a,uint32 mode){if(a->removed)return;owner->auras.erase({a->spell,a->caster});a->removed=true;a->application.mode=mode;if(auto*p=owner->ToPlayer())p->SendSpellScene(TestScene,nullptr,false,p);if(auto s=std::dynamic_pointer_cast<spell_lights_wrath_pickup_scene>(a->script))s->Removed(&a->root,AURA_EFFECT_HANDLE_REAL);}
void AuraRecord::Remove(){RemoveAura(owner,shared_from_this(),AURA_REMOVE_BY_DEFAULT);}void AuraScript::Remove(){if(auto a=record.lock())a->Remove();}
bool Unit::HasAura(uint32 id,ObjectGuid caster)const{for(auto const&a:auras)if(a.first.first==id&&(caster.IsEmpty()||a.first.second==caster))return true;return false;}
AuraRecord*Unit::GetAura(uint32 id,ObjectGuid caster){for(auto const&a:auras)if(a.first.first==id&&(caster.IsEmpty()||a.first.second==caster))return a.second.get();return nullptr;}
void Unit::RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){std::vector<std::shared_ptr<AuraRecord>>found;for(auto const&a:auras)if(a.first.first==id&&(caster.IsEmpty()||a.first.second==caster))found.push_back(a.second);for(auto a:found)a->Remove();}
std::vector<AuraEffect*>Player::GetAuraEffectsByType(uint32 type){assert(type==430);std::vector<AuraEffect*>out;for(auto const&a:auras)out.push_back(&a.second->scene);return out;}
#include "SceneSend.inc"
#include "SceneCompleted.inc"
#include "SceneStatus.inc"
struct WorldSession{Player*_player;void SendPacket(void*){}void HandleScenePlaybackCanceled(WorldPackets::Scene::SceneInstance&);void HandleScenePlaybackComplete(WorldPackets::Scene::SceneInstance&);};
#include "SceneCanceled.inc"
#include "SceneCompletePacket.inc"
using AuraMap=std::map<std::pair<uint32,ObjectGuid>,std::shared_ptr<AuraRecord>>;
void Advance(Player&p,uint32 diff){auto values=p.auras;for(auto const&a:values){a.second->Advance(diff);if(!a.second->removed)if(auto script=std::dynamic_pointer_cast<spell_lights_wrath_pickup_scene>(a.second->script))script->Tick(diff,&a.second->root);}auto&m_ownedAuras=p.auras;auto RemoveOwnedAura=[&](AuraMap::iterator&i,uint32 mode){auto a=(i++)->second;RemoveAura(&p,a,mode);};
#include "NativeExpiry.inc"
}
struct Request{std::shared_ptr<Spell>spell;std::shared_ptr<SpellScript>script;};std::vector<Request>requests;
void Deliver(Request r){if(auto s=std::dynamic_pointer_cast<spell_lights_wrath_pickup_cast>(r.script)){if(s->Check()!=SPELL_CAST_OK)return;for(uint32 i=0;i<6;++i){s->Hit(i);if(s->blocked[i])continue;auto*p=r.spell->m_caster->ToPlayer();if(i==0){auto a=std::make_shared<AuraRecord>();a->owner=p;a->caster=p->guid;p->auras[{TestSpell,p->guid}]=a;auto as=std::make_shared<spell_lights_wrath_pickup_scene>();as->record=a;if(as->Load())a->script=as;p->SendSpellScene(TestScene,&r.spell->info,true,p);if(a->script)as->Apply(&a->root,AURA_EFFECT_HANDLE_REAL);}if(i==2)r.spell->EffectForceEquipItem(i);if(i==3)p->questCredit=1;if(i==4)r.spell->EffectSendEvent(i);}s->After();}
 else if(auto s=std::dynamic_pointer_cast<spell_lights_wrath_return>(r.script)){if(s->Check()!=SPELL_CAST_OK)return;for(uint32 i=0;i<2;++i){s->Hit(i);if(i==1)s->Depart(i);if(!s->blocked[i]){if(i==0)++r.spell->m_caster->ToPlayer()->other97377;else r.spell->EffectTeleportUnits(i);}}}}
SpellCastResult Player::CastSpell(Unit*target,uint32 id,bool triggered){assert(target==this&&!triggered);if(id==TestSpell)++pickupCasts;else{assert(id==218172);++returnCasts;}auto spell=std::make_shared<Spell>(this,id);std::shared_ptr<SpellScript>script;if(id==TestSpell)script=std::make_shared<spell_lights_wrath_pickup_cast>();else script=std::make_shared<spell_lights_wrath_return>();script->spell=spell.get();assert(script->Load()&&script->Validate(&spell->info));auto check=id==TestSpell?static_cast<spell_lights_wrath_pickup_cast*>(script.get())->Check():static_cast<spell_lights_wrath_return*>(script.get())->Check();if(check!=SPELL_CAST_OK||failCast)return SPELL_FAILED_BAD_TARGETS;Request request{spell,script};requests.push_back(request);if(!deferCast)Deliver(request);return SPELL_CAST_OK;}
GameObject*GameObject::SummonGameObject(uint32 id,float a,float b,float c,float facing,float r0,float r1,float r2,float r3,uint32 lifetime,ObjectGuid viewer,void*list,bool creator){assert(id==248574&&a==1152.58f&&b==1002.02f&&c==309.289f&&facing==TestOrientation&&!r0&&!r1&&!r2&&!r3&&!lifetime&&viewer.IsEmpty()&&!list&&!creator);++spawnCalls;if(failSpawn)return nullptr;auto portal=std::make_shared<GameObject>();portal->entry=id;portal->guid={1,90,id,3};portal->summoner=guid;portal->map=map;portal->x=a;portal->y=b;portal->z=c;portal->ai=new go_lights_wrath_return(portal.get());map->objects.push_back(portal);if(onSpawn){auto f=onSpawn;f();}return portal.get();}
bool CompletedLoot(Loot*loot,GameObject*go){
#include "CompleteLoot.inc"
}
struct Run{Map map;Player player;std::shared_ptr<GameObject>source=std::make_shared<GameObject>();std::unique_ptr<go_lights_wrath_pickup>ai;
 Run(){warnings=0;manager={};requests.clear();source->map=&map;player.map=&map;map.players={{&player}};map.objects={source};ai=std::make_unique<go_lights_wrath_pickup>(source.get());source->ai=ai.get();}
 ~Run(){for(auto o:map.objects)if(o.get()!=source.get())delete o->ai;requests.clear();map.objects.clear();}
 void Tick(uint32 diff){if(ai)ai->UpdateAI(diff);}void Clear(){for(int i=0;i<4;++i)Tick(0);}void LootStaff(){Loot loot;player.StoreLootItem(&loot);if(CompletedLoot(&loot,source.get()))source->SetLootState(GO_JUST_DEACTIVATED,&player);}
 void Cast(){Tick(199);assert(!player.pickupCasts);Tick(1);assert(player.pickupCasts==1);}
 void Natural(){Advance(player,8999);Tick(8999);assert(!source->spawnCalls);Advance(player,1);Tick(1);assert(source->spawnCalls==1);}
 GameObject*Portal(){for(auto o:map.objects)if(o->entry==248574)return o.get();return nullptr;}
 void Exit(){auto*portal=Portal();assert(portal);player.x=portal->x;player.y=portal->y;player.z=portal->z;assert(!portal->AI()->GossipUse(&player));assert(player.CastSpell(&player,218172,false)==SPELL_CAST_OK);}
 void Finish(){LootStaff();Cast();Natural();}void Retake(){player.quest={};player.quest.ScriptLifetime=std::make_shared<uint8>();player.questCredit=0;}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);
 std::cout<<"Native route "<<TestScenario<<" quest "<<TestQuest<<" spell "<<TestSpell<<" scene "<<TestScene<<"\n";
 for(uint32 leaf:{TestControl,TestControl2,TestControl3})if(leaf){Run r;manager.scenario.counters.erase(leaf);assert(r.ai->GossipUse(&r.player)&&!r.ai->claim);}
 {Run r;r.player.deferCast=true;r.LootStaff();r.Cast();auto request=requests.back();request.spell->info.Id=TestSpell==208011?211954:208011;Deliver(request);assert(!r.player.questCredit&&!r.player.HasAura(TestSpell));}

 for(uint32 status:{1u,3u}){Run r;r.player.status=status;assert(!r.ai->GossipUse(&r.player));r.LootStaff();assert(r.ai->claim&&r.source->m_lootState==GO_READY&&r.player.owned);r.Cast();assert(r.player.questCredit==1&&LightsWrathPickup::Equipped(&r.player, TestItem));r.Natural();assert(r.source->m_lootState==GO_JUST_DEACTIVATED);r.Exit();assert(r.player.finalEvents==1&&manager.scenario.step==7);}
 for(uint32 bad=0;bad<23;++bad){Run r;
  if(bad==0)r.player.klass=TestOtherClass;if(bad==1)r.player.status=0;if(bad==2)r.player.rewarded=true;if(bad==3)r.player.alive=false;if(bad==4)r.player.world=false;if(bad==5)r.player.contact=false;if(bad==6)r.player.gm=true;if(bad==7)r.player.teleport=true;
  if(bad==8)r.player.phase=1;if(bad==9)r.player.visible=false;if(bad==10)r.player.los=false;if(bad==11)r.player.x+=6;if(bad==12)r.source->entry=1;if(bad==13)r.source->world=false;if(bad==14)r.source->preDelete=true;if(bad==15)r.source->spawned=false;
  if(bad==16)r.source->flags=GO_FLAG_NOT_SELECTABLE;if(bad==17)r.map.mapId=576;if(bad==18)manager.scenario.id=TestOtherScenario;if(bad==19)manager.present=false;if(bad==20)manager.scenario.step=5;if(bad==21)manager.scenario.counters.clear();if(bad==22)r.source->summoner.entry=1;
  assert(r.ai->GossipUse(&r.player)&&!r.ai->claim&&!r.player.ownEvents&&!r.player.pickupCasts);
 }
 for(uint32 step=0;step<10;++step)if(step!=6&&step!=7){Run r;manager.scenario.step=step;assert(r.ai->GossipUse(&r.player));}
 {Run r;r.player.room=false;r.LootStaff();assert(!r.player.owned&&!r.ai->claim&&r.player.errors==1);r.player.room=true;r.LootStaff();r.Cast();r.Natural();}
 {Run r;r.player.storage=false;r.LootStaff();assert(!r.player.owned&&!r.ai->claim);}
 {Run r;r.player.bank=true;assert(r.ai->GossipUse(&r.player)&&!r.ai->claim&&!r.player.owned);r.player.bank=false;r.player.owned=true;assert(r.ai->GossipUse(&r.player)&&r.ai->claim);r.Cast();r.Natural();}
 {Run r;manager.scenario.step=7;r.player.owned=true;r.ai->GossipUse(&r.player);r.Cast();r.Natural();r.Exit();assert(!r.player.finalEvents&&r.player.m_bHasDelayedTeleport);}
 for(bool canceled:{false,true}){Run r;r.LootStaff();r.Cast();Advance(r.player,1000);r.Tick(1000);WorldSession session{&r.player};WorldPackets::Scene::SceneInstance packet{r.player.sceneInstanceID};if(canceled)session.HandleScenePlaybackCanceled(packet);else session.HandleScenePlaybackComplete(packet);assert(!r.player.HasAura(TestSpell)&&r.ai->claim->phase==LightsWrathPickup::Phase::Finishing);r.Tick(7999);assert(!r.source->spawnCalls);r.Tick(1);assert(r.source->spawnCalls==1);r.Exit();assert(r.player.finalEvents==1);}
 {Run r;r.LootStaff();r.Cast();uint32 old=r.player.sceneInstanceID;r.player.SendSpellScene(TestScene,nullptr,true,&r.player);WorldSession session{&r.player};WorldPackets::Scene::SceneInstance packet{old};session.HandleScenePlaybackComplete(packet);assert(r.player.HasAura(TestSpell)&&!r.player.HasSceneStatus(TestScene,SCENE_COMPLETE));r.Natural();}
 for(uint32 mode:{uint32(AURA_REMOVE_BY_CANCEL),uint32(AURA_REMOVE_BY_ENEMY_SPELL),uint32(AURA_REMOVE_BY_DEATH),uint32(AURA_REMOVE_BY_DEFAULT),uint32(AURA_REMOVE_BY_EXPIRE)}){Run r;r.LootStaff();r.Cast();auto a=r.player.auras.begin()->second;RemoveAura(&r.player,a,mode);assert(r.ai->claim->phase==LightsWrathPickup::Phase::Cancelled&&!r.source->spawnCalls);r.Clear();assert(!r.ai->claim);assert(r.ai->GossipUse(&r.player)&&r.ai->claim);r.Tick(200);Advance(r.player,9000);r.Tick(0);assert(r.Portal());}
 {Run r;r.LootStaff();r.player.failCast=true;r.Tick(200);assert(!r.player.questCredit&&!r.source->spawnCalls&&warnings==1);r.player.failCast=false;r.Tick(4999);assert(r.player.pickupCasts==1);r.Tick(1);assert(r.player.pickupCasts==2);Advance(r.player,9000);r.Tick(0);assert(r.Portal());}
 for(bool swapBlocked:{false,true}){Run r;r.player.equipRoom=swapBlocked;r.player.swapBlocked=swapBlocked;r.LootStaff();r.Cast();assert(!LightsWrathPickup::Equipped(&r.player, TestItem));Advance(r.player,9000);r.Clear();assert(!r.Portal()&&!r.ai->claim&&r.player.owned);r.player.equipRoom=true;r.player.swapBlocked=false;r.ai->GossipUse(&r.player);r.Tick(200);Advance(r.player,9000);r.Tick(0);assert(r.Portal());}
 {Run r;r.source->failSpawn=true;r.LootStaff();r.Cast();Advance(r.player,9000);r.Tick(0);assert(!r.Portal()&&r.source->m_lootState==GO_READY&&r.source->spawnCalls==1);r.source->failSpawn=false;r.Tick(1999);assert(r.source->spawnCalls==1);r.Tick(1);assert(r.Portal()&&r.source->spawnCalls==2);}
 {Run r;r.LootStaff();r.Cast();Advance(r.player,9000);r.player.world=false;r.map.players.clear();r.Tick(0);assert(r.Portal());} // already earned exit world fact
 {Run r;r.player.deferCast=true;r.LootStaff();r.Cast();auto old=requests.back();auto held=r.player.quest.ScriptLifetime;r.Retake();Deliver(old);assert(!r.player.questCredit&&!r.player.HasAura(TestSpell)&&!r.Portal());r.Clear();assert(held!=r.player.quest.ScriptLifetime);r.player.deferCast=false;r.ai->GossipUse(&r.player);r.Tick(200);Advance(r.player,9000);r.Tick(0);assert(r.Portal());Deliver(old);assert(r.player.questCredit==1);}
 {Run r;r.player.deferCast=true;r.LootStaff();r.Cast();auto old=requests.back();r.ai.reset();r.source->ai=nullptr;r.ai=std::make_unique<go_lights_wrath_pickup>(r.source.get());r.source->ai=r.ai.get();Deliver(old);assert(!r.player.questCredit);r.player.deferCast=false;r.ai->GossipUse(&r.player);r.Tick(200);Advance(r.player,9000);r.Tick(0);assert(r.Portal());}
 {Run r;r.LootStaff();r.Cast();r.ai->Reset();Advance(r.player,1);assert(!r.player.HasAura(TestSpell));r.ai->GossipUse(&r.player);r.Tick(200);Advance(r.player,9000);r.Tick(0);assert(r.Portal());}
 {Run r;r.player.deferCast=true;r.LootStaff();r.Cast();auto old=requests.back();Map other;auto clone=std::make_shared<GameObject>();clone->map=&other;other.objects={clone};other.players={{&r.player}};r.player.map=&other;Deliver(old);assert(!r.player.questCredit&&!r.player.HasAura(TestSpell));}
 {Run r;r.player.deferCast=true;r.LootStaff();r.Cast();auto old=requests.back();r.source->guid.high+=1;r.map.lowLookup=true;Deliver(old);assert(!r.player.questCredit);}
 {Run r;r.player.onEquip=[&]{r.Retake();};r.LootStaff();r.Cast();assert(!r.player.questCredit&&!r.player.nativeEvents);Advance(r.player,9000);r.Clear();assert(!r.Portal());}
 {Run r;r.Finish();r.player.rejectTeleport=true;r.Exit();assert(!r.player.finalEvents&&warnings==1);r.player.rejectTeleport=false;r.Exit();assert(r.player.finalEvents==1);}
 {Run r;r.Finish();r.player.deferCast=true;r.Exit();auto old=requests.back();r.Retake();Deliver(old);assert(!r.player.finalEvents&&!r.player.m_bHasDelayedTeleport);}
 {Run r;r.Finish();r.player.onTeleport=[&]{r.Retake();};r.Exit();assert(!r.player.finalEvents&&r.player.m_bHasDelayedTeleport);}
 {Run r;r.Finish();r.player.onTeleport=[&]{r.player.gm=true;};r.Exit();assert(!r.player.finalEvents);}
 {Run r;r.Finish();r.player.status=0;r.player.quest={};r.Exit();assert(!r.player.finalEvents&&r.player.m_bHasDelayedTeleport);}
 {Run r;r.Finish();manager.scenario.step=7;r.Exit();assert(!r.player.finalEvents&&r.player.m_bHasDelayedTeleport);}
 {Run r;r.Finish();r.player.deferCast=true;r.Exit();auto old=requests.back();old.spell->destination=1;Deliver(old);assert(!r.player.finalEvents&&warnings==1);}
 {Run r;r.Finish();r.player.deferCast=true;r.Exit();auto old=requests.back();r.Portal()->AI()->Reset();Deliver(old);assert(!r.player.finalEvents&&!r.player.m_bHasDelayedTeleport);}
 {Run r;r.Finish();r.player.deferCast=true;r.Exit();auto old=requests.back();r.Portal()->AI()->UpdateAI(5000);Deliver(old);assert(!r.player.finalEvents&&!r.player.m_bHasDelayedTeleport);}
 {Run r;r.Finish();auto extra=std::make_shared<GameObject>();extra->map=&r.map;extra->entry=248574;extra->guid={1,91,248574,3};extra->summoner=r.source->guid;extra->x=r.Portal()->x;extra->y=r.Portal()->y;extra->z=r.Portal()->z;extra->ai=new go_lights_wrath_return(extra.get());r.map.objects.push_back(extra);r.player.x=extra->x;r.player.y=extra->y;r.player.z=extra->z;assert(!r.Portal()->AI()->GossipUse(&r.player)&&!extra->AI()->GossipUse(&r.player));assert(r.player.CastSpell(&r.player,218172,false)==SPELL_FAILED_BAD_TARGETS);assert(!r.player.finalEvents&&!r.player.other97377);}
 {Run r;r.player.onCredit=[&]{r.player.onCredit={};r.ai.reset();r.source->ai=nullptr;r.ai=std::make_unique<go_lights_wrath_pickup>(r.source.get());r.source->ai=r.ai.get();};r.LootStaff();assert(r.player.owned&&r.source->m_lootState==GO_READY&&!r.ai->claim);r.ai->GossipUse(&r.player);r.Tick(200);Advance(r.player,9000);r.Tick(0);assert(r.Portal());}
 {Run r;r.player.failCast=true;r.LootStaff();r.Tick(200);for(int i=0;i<5;++i)r.Tick(5000);r.Tick(4800);assert(r.ai->claim->phase==LightsWrathPickup::Phase::Cancelled&&r.source->m_lootState==GO_READY&&!r.Portal());r.Clear();assert(!r.ai->claim);}
 {Run r;r.LootStaff();r.Cast();Advance(r.player,9000);auto earned=r.ai->claim;r.ai->Reset();r.player.world=false;r.map.players.clear();r.Tick(0);assert(r.Portal()&&r.ai->claim==earned);r.ai->Reset();assert(r.ai->GossipUse(&r.player));}
 {Run r;r.player.deferCast=true;r.LootStaff();r.Cast();auto old=requests.back();r.Tick(5000);auto fresh=requests.back();assert(requests.size()==2);Deliver(old);Deliver(fresh);assert(r.player.questCredit==1&&r.player.nativeEvents==1);Advance(r.player,9000);r.Tick(0);assert(r.Portal());Deliver(fresh);assert(r.player.nativeEvents==1);}
 {Run r;Unit pet;pet.map=&r.map;r.player.owned=true;r.source->SetLootState(GO_JUST_DEACTIVATED,&pet);assert(!r.ai->claim&&!r.player.ownEvents);}
 {Run r;r.LootStaff();r.Cast();WorldSession session{&r.player};WorldPackets::Scene::SceneInstance packet{r.player.sceneInstanceID};session.HandleScenePlaybackComplete(packet);r.player.world=false;r.map.players.clear();r.Tick(8999);assert(!r.Portal());r.Tick(1);assert(r.Portal());}
 for(uint32 bad=0;bad<3;++bad){Run r;r.LootStaff();r.Cast();auto a=r.player.auras.begin()->second;if(bad==0)a->m_duration=0;if(bad==1){a->m_duration=0;a->m_allDuration=9000;a->maxDuration=10000;}if(bad==2){auto script=std::dynamic_pointer_cast<spell_lights_wrath_pickup_scene>(a->script);script->Apply(&a->root,AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);}else RemoveAura(&r.player,a,AURA_REMOVE_BY_EXPIRE);assert(r.ai->claim->phase==LightsWrathPickup::Phase::Cancelled&&!r.Portal());}
 {Run r;manager.scenario.id=9999;assert(r.player.CastSpell(&r.player,TestSpell,false)==SPELL_CAST_OK);assert(r.player.questCredit==1&&r.player.HasAura(TestSpell)&&!r.ai->claim);r.player.CastSpell(&r.player,218172,false);assert(r.player.other97377==1&&!r.player.finalEvents);}
 {Run r;r.Finish();auto*portal=r.Portal();manager.scenario.id=9999;portal->summoner={1,100,106756,2};portal->owner=portal->summoner;assert(!portal->AI()->GossipUse(&r.player));assert(r.player.legacyPortalEvents==1&&!static_cast<go_lights_wrath_return*>(portal->AI())->receipt);r.player.CastSpell(&r.player,218172,false);assert(r.player.other97377==1&&!r.player.finalEvents);}
 {Run r;assert(r.player.CastSpell(&r.player,TestSpell,false)==SPELL_FAILED_BAD_TARGETS);assert(!r.player.questCredit&&!r.player.HasAura(TestSpell));}
 assert(LightsWrathPickup::castRequest.expired());
 std::cout<<"PASS full actual pickup/return handlers and native loot/force-equip/scene packet+completion/duration/expiry/deferred teleport: both statuses,23 admission denials/stage bounds, real9s and legitimate visual skip, bags/bank, cast/equip/creation retry, quest/AI reset/retake/crossMap/fullGUID/deferred effect rejection, actual accepted-only49875 and completed/abandoned escape. Native cast/effect/Map admission endpoints controlled; client route/VFX pending.\n";
}

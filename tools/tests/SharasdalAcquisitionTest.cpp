#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <cstdlib>
#include <atomic>
#include <mutex>
#include <cmath>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
using SpellEffIndex=uint32;using LootState=uint32;using AuraEffectHandleModes=uint32;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_NOT_HERE,SPELL_FAILED_ERROR};
enum{CLASS_SHAMAN=7,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6,
 GO_READY=1,GO_ACTIVATED=2,GO_JUST_DEACTIVATED=3,GAMEOBJECT_FIELD_FLAGS=17,GO_FLAG_NOT_SELECTABLE=16,
 INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15,EQUIPMENT_SLOT_OFFHAND=16,NULL_SLOT=255,NULL_BAG=255,
 INVTYPE_2HWEAPON=17,EFFECT_0=0,EFFECT_1=1,EFFECT_2=2,EFFECT_3=3,SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,
 SPELL_EFFECT_SEND_EVENT=61,SPELL_EFFECT_FORCE_EQUIP_ITEM=249,SPELL_EFFECT_KILL_CREDIT2=134,SPELL_EFFECT_TRIGGER_SPELL=64,
 SPELL_AURA_ACTIVATE_SCENE=430,AURA_EFFECT_HANDLE_REAL=1,
 TARGET_FLAG_UNIT_MASK=2,TARGET_FLAG_GAMEOBJECT_MASK=8,GAMEOBJECT_TYPE_FISHINGNODE=17,GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51,TYPEID_PLAYER=4,RATE_MINING_AMOUNT=0,RATE_MINING_NEXT=1,SKILL_MINING=186};
#include "CriteriaTypes.inc"
#include "NativeEnums.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid{static ObjectGuid const Empty;uint64 value=0;void Clear(){value=0;}bool IsEmpty()const{return !value;}bool operator==(ObjectGuid const& b)const{return value==b.value;}bool operator<(ObjectGuid const& b)const{return value<b.value;}};ObjectGuid const ObjectGuid::Empty{};
#include "QuestObjectScope.inc"
struct Position{};struct Map{template<class...T>void ScriptsStart(T...){}}liveMap,otherMap;int sEventScripts=0;
struct Player;struct GameObject;struct Creature;struct Spell;struct SpellInfo;struct GameObjectAI;
struct ZoneScript{void ProcessEvent(void*,uint32){}};struct InstanceScript:ZoneScript{GameObject* GetGameObjectByEntry(uint32);};InstanceScript nativeInstance;
struct Unit:Position{virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual bool IsPlayer(){return ToPlayer()!=nullptr;}virtual uint32 GetTypeId(){return 3;}ObjectGuid guid{1};ObjectGuid GetGUID()const{return guid;}Map* GetMap(){return &liveMap;}ZoneScript* GetZoneScript(){return nullptr;}InstanceScript* GetInstanceScript(){return &nativeInstance;}SpellCastResult CastSpell(Unit*,uint32,bool);bool _questObjectBound=false;std::weak_ptr<QuestObjectScope>_questObjectScope;void BindQuestObjectScope(std::shared_ptr<QuestObjectScope>const&);bool IsQuestObjectExpired()const;void AddPlayerInPersonnalVisibilityList(ObjectGuid){} };
using WorldObject=Unit;
struct ItemTemplate{uint32 GetArtifactID()const{return 42;}uint32 GetInventoryType()const{return 21;}};
struct Item{uint32 entry=128911;ObjectGuid guid{21},child{22};bool equipped=false;uint16 pos=0xff17;ItemTemplate proto;
 uint32 GetEntry()const{return entry;}ObjectGuid GetGUID()const{return guid;}ObjectGuid GetChildItem()const{return child;}bool IsEquipped()const{return equipped;}uint16 GetPos()const{return pos;}ItemTemplate const* GetTemplate()const{return &proto;}};
struct CriteriaProgress{uint32 Counter=0;};struct Scenario{uint32 id=1066,step=5;bool accept=true;std::map<uint32,CriteriaProgress> progress{{47380,{1}},{47133,{1}}};uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Scenario& GetAchievementMgr(){return *this;}auto const* GetCriteriaProgressMap(){return &progress;}};
struct ScenarioMgr{bool present=true;Scenario scenario;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}}manager;auto* sScenarioMgr=&manager;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime=std::make_shared<uint8>();std::shared_ptr<QuestObjectScope>ScriptObjects;};
enum InventoryResult{EQUIP_ERR_OK,EQUIP_ERR_INV_FULL,EQUIP_ERR_CANT_EQUIP};
struct ItemPosCountVec:std::vector<uint32>{void emplace_back(uint16,uint32){assert(false);}};
struct LootItem{struct{uint32 ItemID=128911,RandomPropertiesID=0;struct{std::vector<uint32> BonusListIDs;uint32 Context=0;}ItemBonus;}item;uint32 count=1;bool freeforall=false,is_looted=false;std::set<uint32> GetAllowedLooters(){return {};}};
struct Loot{uint32 objGuid=0;LootItem item;uint32 gold=0,unlootedCount=1;
#include "IsLooted.inc"
void AutoStoreItems(bool){assert(false);}
};
struct World{float getRate(uint32){assert(false);return 1;}}worldService;auto*sWorld=&worldService;struct LockEntry{uint32 Skill[1]{};};struct LockStore{LockEntry const*LookupEntry(uint32){assert(false);return nullptr;}}lockStore;auto&sLockStore=lockStore;bool roll_chance_f(float){assert(false);return false;}
struct ScriptMgr{template<class...T>bool CanAutoEquipLoot(T...){return false;}void OnGameObjectLootStateChanged(GameObject*,uint32,Unit*);}scripts;auto* sScriptMgr=&scripts;
struct Creature:Unit{uint32 entry=102826;uint64 spawn=362093;bool alive=true;bool IsAlive(){return alive;}uint64 GetDBTableGUIDLow(){return spawn;}void DespawnOrUnsummon(uint32){}};
struct Player:Unit{
 bool world=true,contact=true,alive=true,gm=false,teleport=false,rewarded=false,phase=true,visible=true,los=true,owned=false,bank=false,paired=false,
 room=true,storage=true,canEquip=true,canChild=true,swap=true,aura=false,complete=false,jump=false,makeAura=true,quest=true;
 uint32 klass=7,status=3,mapId=1600,instance=42,failSpell=0,casts=0,events=0,event1=0,explicitRewards=0,grants=0,errors=0,swaps=0,equipChecks=0,childChecks=0,objective=0,credits=0,ability=0;
 float distance=1;Map* map=&liveMap;Item parent,child{128934,{22},{},false,0xff18,{}};QuestStatusData questStatus;GameObject* object=nullptr;
 std::vector<uint32> trace;Creature allies[3];std::function<void()> onSwap;
 Player(){allies[1].entry=105298;allies[1].spawn=362092;allies[2].entry=104853;allies[2].spawn=362094;}
 Player* ToPlayer()override{return this;}uint32 GetTypeId()override{return 4;}bool IsInWorld(){return world;}bool CanContact(){return contact;}bool IsAlive(){return alive;}bool isGameMaster(){return gm;}bool IsBeingTeleported(){return teleport;}uint32 getClass(){return klass;}
 Map* GetMap(){return map;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}bool IsWithinLOSInMap(GameObject*){return los;}bool IsWithinDistInMap(GameObject*,float r){return distance<=r;}float GetExactDist(GameObject*){return distance;}
 bool GetQuestRewardStatus(uint32 q){assert(q==40341);return rewarded;}uint32 GetQuestStatus(uint32 q){assert(q==40341);return status;}QuestStatusData* getQuestStatus(uint32 q){assert(q==40341);return quest?&questStatus:nullptr;}
 Item* GetItemByEntry(uint32 id){assert(id==128911);return owned?&parent:nullptr;}Item* GetItemByGuid(ObjectGuid g){return owned&&g==parent.guid?&parent:nullptr;}
 Item* GetItemByPos(uint32 bag,uint32 slot){assert(bag==255);if(!owned)return nullptr;return slot==15&&parent.equipped?&parent:slot==16&&paired?&child:nullptr;}
 bool HasItemCount(uint32 id,uint32,bool withBank=false){assert(id==128911);return owned||(withBank&&bank);}bool HasAura(uint32 id){assert(id==209684);return aura;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==209684);aura=false;}bool HasSceneStatus(uint32 id,uint32){assert(id==1255);return complete;}
 GameObject* FindNearestGameObject(uint32,float);Creature* FindNearestCreature(uint32 e,float){for(auto& a:allies)if(a.entry==e&&a.alive)return &a;return nullptr;}float GetVisibilityRange(){return 100;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 id){assert(q==40341&&id==105787);return objective;}
 uint32 GetSkillValue(uint32){assert(false);return 0;}
 void BindQuestObject(uint32,WorldObject*);
 std::vector<std::function<void()>>actions;void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 delay,std::function<void()>&& action){assert(delay==0);actions.push_back(std::move(action));}
 void UpdateAchievementCriteria(uint32 type,uint32 event,uint32=0){assert(event==49820||type==CRITERIA_TYPE_COMPLETE_SCENARIO);if(type==CRITERIA_TYPE_SCRIPT_EVENT_2){
#ifndef BASELINE
 assert(parent.equipped&&paired);
#endif
 trace.push_back(61);++events;if(manager.scenario.accept&&manager.scenario.step==5){manager.scenario.progress[47134].Counter=1;manager.scenario.step=6;phase=visible=false;}}else if(type==CRITERIA_TYPE_SCRIPT_EVENT)++event1;else ++explicitRewards;}
 void RewardPlayerAndGroupAtEvent(uint32 id,Unit* target){assert(id==105787&&target==this);assert(parent.equipped&&paired);trace.push_back(134);++credits;objective=1;}
 void KilledMonsterCredit(uint32){++credits;objective=1;}void AddItem(uint32,uint32){++grants;if(room)owned=true;}
 InventoryResult CanEquipItem(uint32,uint16& dst,Item* i,bool,bool,bool){assert(i==&parent);++equipChecks;dst=0xff0f;return canEquip?EQUIP_ERR_OK:EQUIP_ERR_CANT_EQUIP;}
 InventoryResult CanEquipChildItem(Item* i){assert(i==&parent);++childChecks;return canChild?EQUIP_ERR_OK:EQUIP_ERR_CANT_EQUIP;}
 void SendEquipError(InventoryResult,Item*){++errors;}void SwapItem(uint16 from,uint16 dst,bool){assert(from==parent.pos&&dst==0xff0f);++swaps;trace.push_back(249);if(swap){parent.equipped=true;parent.pos=dst;paired=!parent.child.IsEmpty();}if(onSwap)onSwap();}
 InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec& dst,uint32 id,uint32 count,void*,bool){assert(id==128911&&count==1);if(!room)return EQUIP_ERR_INV_FULL;dst.push_back(1);return EQUIP_ERR_OK;}
 Item* StoreNewItem(ItemPosCountVec const&,uint32,bool,uint32,std::set<uint32>const&,std::vector<uint32>const&,uint32,bool refund=false,bool autoEquip=false){assert(!refund&&!autoEquip);if(!storage)return nullptr;owned=true;return &parent;}
 void SendEquipError(InventoryResult,void*,void*,uint32){++errors;}
 void StoreLootItem(Loot* loot){LootItem* item=&loot->item;void* qitem=nullptr;
#include "StoreInventory.inc"
#include "ConsumeLoot.inc"
 }
};
struct GameObjectTemplate{uint32 type=3;struct{uint32 minRestock=0,maxRestock=0,chestPersonalLoot=0;}chest;bool IsOploteChest()const;uint32 GetLockId()const{return 1691;}};
struct GameObject:Unit{bool world=true,spawned=true,notSelectable=false,deleted=false;uint32 entry=249342,mapId=1600,instance=42,m_lootState=GO_READY;uint64 spawn=134299;Map* map=&liveMap;GameObjectAI* ai=nullptr;GameObjectTemplate info;
 bool IsInWorld(){return world;}bool isSpawned(){return spawned;}uint32 GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return spawn;}uint32 GetMapId(){return mapId;}Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}float GetInteractionDistance(){return 5;}bool HasFlag(uint32,uint32){return notSelectable;}uint32 getLootState(){return m_lootState;}GameObjectAI* AI(){return ai;}void SetLootState(LootState,Unit* =nullptr);Creature* FindNearestCreature(uint32,float){return nullptr;}void Delete(){deleted=true;}uint32 GetGoType()const{return info.type;}GameObjectTemplate const*GetGOInfo()const{return &info;}bool IsPersonal()const;void AddUse(){assert(false);}uint32 GetUseCount(){assert(false);return 0;}void DestroyForPlayer(Player*){assert(false);}void UpdateObjectVisibility(){assert(false);}};
struct GameObjectAI{GameObject* const go;explicit GameObjectAI(GameObject*g):go(g){}virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual void UpdateAI(uint32){}virtual void OnStateChanged(uint32,Unit*){}virtual void Reset(){}};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual GameObjectAI* GetAI(GameObject*g)const{return new GameObjectAI(g);}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual void OnLootStateChanged(GameObject*,uint32,Unit*){}};
GameObjectScript* currentScript=nullptr;Player* currentPlayer=nullptr;
void ScriptMgr::OnGameObjectLootStateChanged(GameObject*g,uint32 s,Unit*u){currentScript->OnLootStateChanged(g,s,u);}
struct ObjectAccessor{static Player* GetPlayer(GameObject&,ObjectGuid g){return currentPlayer&&currentPlayer->guid==g?currentPlayer:nullptr;}static GameObject* GetGameObject(Player& p,ObjectGuid g){return p.object&&p.object->GetGUID()==g?p.object:nullptr;}};
GameObject* InstanceScript::GetGameObjectByEntry(uint32 id){assert(id==249342);return currentPlayer?currentPlayer->object:nullptr;}
GameObject* Player::FindNearestGameObject(uint32 e,float r){return object&&object->entry==e&&distance<=r&&object->world?object:nullptr;}
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 71;}}generator;struct ObjectMgr{template<HighGuid>Generator* GetGenerator(){return &generator;}}objects;auto* sObjectMgr=&objects;
struct Effect{int32 MiscValue=0;uint32 GetProvidedTargetMask(){return TARGET_FLAG_UNIT_MASK;}};
struct SpellInfo{uint32 Id=209685;Effect effects[4];Effect* GetEffect(uint32 i,uint32){return &effects[i];}};
struct SpellMgr{SpellInfo info;SpellInfo const* GetSpellInfo(uint32 id){assert(id==209743);info.Id=id;return &info;}}spellMgr;auto* sSpellMgr=&spellMgr;
bool conversationSuccess=true;uint32 conversations=0;
struct Conversation;static std::vector<Conversation*>mapConversations;
struct Conversation:Unit{Conversation(){guid={71};}bool CreateConversation(uint32,uint32 id,Unit*u,SpellInfo const*info,Position const&){assert(id==1923&&info&&info->Id==209743&&u==currentPlayer);++conversations;if(conversationSuccess)mapConversations.push_back(this);return conversationSuccess;}};
struct SpellScript;
struct EffectBinding{std::function<void(SpellScript*,uint32)> fn;uint32 index,type;};struct CheckBinding{std::function<SpellCastResult(SpellScript*)> fn;};struct HitBinding{std::function<void(SpellScript*)>fn;};
template<class T>struct Hooks:std::vector<T>{void operator+=(T value){this->push_back(value);}};
template<class C>EffectBinding BindEffect(void(C::*f)(uint32),uint32 i,uint32 t){return {[f](SpellScript*s,uint32 e){(static_cast<C*>(s)->*f)(e);},i,t};}
template<class C>CheckBinding BindCheck(SpellCastResult(C::*f)()){return {[f](SpellScript*s){return (static_cast<C*>(s)->*f)();}};}
template<class C>HitBinding BindHit(void(C::*f)()){return {[f](SpellScript*s){(static_cast<C*>(s)->*f)();}};}
#define PrepareSpellScript(C) public:
#define SpellEffectFn(F,I,T) BindEffect(&F,I,T)
#define SpellCheckCastFn(F) BindCheck(&F)
#define SpellHitFn(F) BindHit(&F)
struct Spell{Player*m_caster;Unit*unitTarget;Unit*gameObjTarget=nullptr,*focusObject=nullptr;uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;SpellInfo info;SpellInfo*m_spellInfo=&info;
 explicit Spell(Player*p):m_caster(p),unitTarget(p){info.effects[0].MiscValue=49820;info.effects[1].MiscValue=128911;info.effects[2].MiscValue=105787;}void EffectForceEquipItem(SpellEffIndex);void EffectSendEvent(SpellEffIndex);void EffectKillCredit(SpellEffIndex);};
struct SpellScript{Spell* spell=nullptr;uint32 m_scriptSpellId=0;bool prevented[4]{};Hooks<EffectBinding>OnEffectHitTarget;Hooks<CheckBinding>OnCheckCast;Hooks<HitBinding>AfterHit;virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return spell->m_caster;}Spell*GetSpell(){return spell;}SpellInfo const*GetSpellInfo(){return &spell->info;}void PreventHitDefaultEffect(uint32 i){prevented[i]=true;}};
struct AuraEffect{};struct AuraApplication{uint32 mode=0;uint32 GetRemoveMode(){return mode;}};
struct AuraScript{Player* player=nullptr;AuraApplication app;struct Sink{template<class T>void operator+=(T){}}AfterEffectRemove;virtual ~AuraScript()=default;virtual void Register(){}virtual bool Load(){return true;}Unit*GetTarget(){return player;}Unit*GetUnitOwner(){return player;}AuraApplication*GetTargetApplication(){return &app;}};
#define PrepareAuraScript(C) public:
#define AuraEffectRemoveFn(F,I,T,M) 0
#include "Controller.inc"
#include "SetLootState.inc"
#include "IsPersonal.inc"
#include "ForceEquipItem.inc"
#include "SendEvent.inc"
#include "KillCredit.inc"
#include "BindQuestObject.inc"
#include "QuestLease.inc"
#include "BindScope.inc"
static std::unique_ptr<AuraScript> liveAura;
void ReleaseChest(Loot* loot,GameObject*go,Player*player){Loot*lootPesonal=nullptr;if(!loot->isLooted())return;
#include "ReleaseChest.inc"
}
SpellCastResult Unit::CastSpell(Unit* target,uint32 id,bool triggered){assert(target==this&&triggered);Player*p=ToPlayer();assert(p);++p->casts;if(p->failSpell==id)return SPELL_FAILED_ERROR;
#ifdef BASELINE
 assert(id==209684);p->aura=true;return SPELL_CAST_OK;
#else
 Spell spell(p);spell.info.Id=id;spell_sharasdal_acquisition script;script.spell=&spell;script.m_scriptSpellId=id;script.Register();
 for(auto const& h:script.OnCheckCast){auto result=h.fn(&script);if(result!=SPELL_CAST_OK)return result;}
 if(id==209683){p->jump=true;return SPELL_CAST_OK;}
 uint32 types[4]={61,249,134,64};if(id==209684){types[0]=6;types[1]=6;types[2]=61;}
 for(uint32 i=0;i<(id==209684?3u:4u);++i){for(auto const& h:script.OnEffectHitTarget)if(h.index==i){assert(h.type==types[i]);h.fn(&script,i);}if(script.prevented[i])continue;
  if(id==209684){if(i==0&&p->makeAura){auto aura=std::make_unique<aura_sharasdal_acquisition>();aura->player=p;assert(aura->Load());liveAura=std::move(aura);p->aura=true;p->complete=false;}if(i==2)spell.EffectSendEvent(i);}else{if(i==0)spell.EffectSendEvent(i);if(i==1)spell.EffectForceEquipItem(i);if(i==2)spell.EffectKillCredit(i);if(i==3){assert(p->parent.equipped&&p->paired);++p->ability;p->trace.push_back(64);}}
 }
 for(auto const& h:script.AfterHit)h.fn(&script);return SPELL_CAST_OK;
#endif
}
struct Fixture{Player p;GameObject go;go_art_sharasdal script;std::unique_ptr<GameObjectAI>ai;Fixture(){manager={};warnings=conversations=0;conversationSuccess=true;currentPlayer=&p;currentScript=&script;p.object=&go;ai.reset(script.GetAI(&go));go.ai=ai.get();}~Fixture(){for(auto c:mapConversations)delete c;mapConversations.clear();liveAura.reset();currentPlayer=nullptr;}void LootWeapon(){go.SetLootState(GO_ACTIVATED,&p);Loot loot;p.StoreLootItem(&loot);ReleaseChest(&loot,&go,&p);}void Arrive(){assert(p.jump);p.CastSpell(&p,209684,true);}void End(uint32 mode,bool complete=false){p.aura=false;p.complete=complete;
#ifndef BASELINE
 if(liveAura){auto aura=dynamic_cast<aura_sharasdal_acquisition*>(liveAura.get());assert(aura);aura->app.mode=mode;aura->Removed(nullptr,AURA_EFFECT_HANDLE_REAL);}
#endif
 }void Finish(){Arrive();End(AURA_REMOVE_BY_EXPIRE);ai->UpdateAI(1);}};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef BASELINE
 for(bool full:{false,true}){Fixture f;f.p.room=!full;f.go.SetLootState(GO_ACTIVATED,&f.p);assert(f.p.events==1&&f.p.explicitRewards==1&&f.p.credits==1&&f.go.deleted&&!f.p.parent.equipped&&f.p.owned==!full);}
 std::cout<<"PASS COUNTERFACTUALab9a5b3d9: GO_ACTIVATED grants49820/manual complete/105787 and deletes source before real loot/equip, including native full-bag grant failure. No repaired success claim.\n";
#else
 unsigned cases=0;
 {Fixture f;f.go.SetLootState(GO_ACTIVATED,&f.p);assert(!f.p.owned&&!f.p.events&&!f.p.casts);f.LootWeapon();assert(f.p.owned&&!f.p.events&&f.go.getLootState()==GO_READY);f.Finish();assert(f.p.events==1&&f.p.objective==1&&f.p.ability==1&&f.p.swaps==1&&!f.p.explicitRewards&&!f.p.grants&&conversations==1&&f.go.getLootState()==GO_JUST_DEACTIVATED);assert((f.p.trace==std::vector<uint32>{249,61,134,64}));f.ai->UpdateAI(100000);assert(f.p.events==1);++cases;}
 for(int failure=0;failure<8;++failure){Fixture f;if(failure==0)f.p.room=false;if(failure==1)f.p.storage=false;f.LootWeapon();if(failure<2){assert(!f.p.events&&!f.p.owned&&!f.p.casts);++cases;continue;}if(failure==2)f.p.canEquip=false;if(failure==3)f.p.canChild=false;if(failure==4)f.p.swap=false;if(failure==5)f.p.parent.child.Clear();if(failure==6)f.p.failSpell=209685;if(failure==7){f.p.parent.equipped=true;f.p.paired=false;}f.Finish();assert(!f.p.events&&!f.p.credits&&!f.p.ability&&f.go.getLootState()==GO_READY&&f.p.owned);assert(dynamic_cast<go_art_sharasdal::AI*>(f.ai.get())->stage==0);++cases;}
 for(int denial=0;denial<24;++denial){Fixture f;switch(denial){case 0:f.p.world=false;break;case 1:f.p.contact=false;break;case 2:f.p.alive=false;break;case 3:f.p.gm=true;break;case 4:f.p.teleport=true;break;case 5:f.p.klass=8;break;case 6:f.go.world=false;break;case 7:f.go.spawned=false;break;case 8:f.go.entry=249343;break;case 9:f.go.spawn=134300;break;case 10:f.go.mapId=1;break;case 11:f.p.map=&otherMap;break;case 12:f.p.instance=43;break;case 13:f.p.phase=false;break;case 14:f.p.visible=false;break;case 15:f.p.los=false;break;case 16:f.p.distance=6;break;case 17:f.go.notSelectable=true;break;case 18:f.p.rewarded=true;break;case 19:f.p.status=QUEST_STATUS_NONE;break;case 20:manager.present=false;break;case 21:manager.scenario.id=1;break;case 22:manager.scenario.step=4;break;case 23:manager.scenario.progress[47380].Counter=0;break;}assert(f.script.OnGossipHello(&f.p,&f.go));f.p.owned=true;f.go.SetLootState(GO_JUST_DEACTIVATED,&f.p);assert(!f.p.casts&&!f.p.events);++cases;}
 for(int loss=0;loss<14;++loss){Fixture f;f.LootWeapon();f.Arrive();switch(loss){case 0:f.p.alive=false;break;case 1:f.p.world=false;break;case 2:f.p.teleport=true;break;case 3:f.p.rewarded=true;break;case 4:f.p.status=QUEST_STATUS_NONE;break;case 5:f.p.questStatus.ScriptLifetime.reset();break;case 6:f.p.questStatus.ScriptLifetime=std::make_shared<uint8>();break;case 7:f.p.parent.guid.value=99;break;case 8:f.p.guid.value=2;break;case 9:currentPlayer=nullptr;break;case 10:f.p.map=&otherMap;break;case 11:f.p.distance=11;break;case 12:f.p.owned=false;break;case 13:f.p.quest=false;break;}f.End(AURA_REMOVE_BY_EXPIRE);f.ai->UpdateAI(1000);assert(!f.p.events&&!f.p.credits&&dynamic_cast<go_art_sharasdal::AI*>(f.ai.get())->stage==0);++cases;}
 {Fixture f;f.p.bank=true;assert(f.script.OnGossipHello(&f.p,&f.go));assert(!f.p.owned&&!f.p.casts);++cases;}
 {Fixture f;f.p.owned=true;assert(f.script.OnGossipHello(&f.p,&f.go));f.Finish();assert(f.p.events==1&&!f.p.grants);++cases;}
 {Fixture f;f.p.owned=true;f.p.distance=0;f.script.OnGossipHello(&f.p,&f.go);assert(!f.p.jump&&f.p.aura);f.End(AURA_REMOVE_BY_DEFAULT,true);f.ai->UpdateAI(1);assert(f.p.events==1);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.End(AURA_REMOVE_BY_DEFAULT,false);f.ai->UpdateAI(16000);assert(!f.p.events&&f.p.owned);++cases;}
 {Fixture f;f.LootWeapon();f.ai->UpdateAI(5000);assert(!f.p.events&&warnings==1);f.p.CastSpell(&f.p,209684,true);assert(!f.p.aura);++cases;}
 {Fixture f;f.p.makeAura=false;f.LootWeapon();f.Arrive();f.ai->UpdateAI(5000);assert(!f.p.events&&warnings==1);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.ai->Reset();assert(!f.p.aura&&!f.p.events&&f.p.owned);++cases;}
 {Fixture f;f.LootWeapon();assert(f.p.CastSpell(&f.p,209685,true)==SPELL_FAILED_NOT_HERE);f.Finish();assert(f.p.events==1);++cases;}
 {Fixture f;f.LootWeapon();Player spectator;spectator.object=&f.go;spectator.owned=true;spectator.guid.value=999;assert(spectator.CastSpell(&spectator,209684,true)==SPELL_FAILED_NOT_HERE);assert(!spectator.events&&!spectator.aura);f.Finish();++cases;}
 {Fixture f;f.LootWeapon();manager.scenario.accept=false;f.Arrive();f.End(AURA_REMOVE_BY_EXPIRE);for(int i=0;i<30;++i)f.ai->UpdateAI(1000);assert(f.p.events==30&&!f.p.objective&&!f.p.credits&&!f.p.ability&&f.p.swaps==1&&warnings==1&&f.p.owned);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.End(AURA_REMOVE_BY_EXPIRE);f.p.allies[0].alive=false;for(int i=0;i<30;++i)f.ai->UpdateAI(1000);assert(f.p.events==1&&f.p.objective==1&&warnings==1&&!conversations&&f.go.getLootState()==GO_JUST_DEACTIVATED);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.End(AURA_REMOVE_BY_EXPIRE);conversationSuccess=false;f.ai->UpdateAI(1);assert(f.p.events==1&&conversations==1);conversationSuccess=true;f.ai->UpdateAI(1000);assert(f.p.events==1&&conversations==2&&f.go.getLootState()==GO_JUST_DEACTIVATED);++cases;}
 {Fixture f;manager.scenario.progress[47133].Counter=0;assert(f.script.OnGossipHello(&f.p,&f.go));f.p.owned=true;f.go.SetLootState(GO_JUST_DEACTIVATED,&f.p);assert(!f.p.casts);++cases;}
 {Fixture f;manager.scenario.step=6;assert(f.script.OnGossipHello(&f.p,&f.go));++cases;}
 {Fixture f;Unit pet;f.go.SetLootState(GO_ACTIVATED,&pet);f.go.SetLootState(GO_JUST_DEACTIVATED,&pet);assert(!f.p.casts&&!f.p.events);++cases;}
 {Fixture f;f.p.failSpell=209683;f.LootWeapon();assert(!f.p.events&&f.p.owned&&warnings==1);f.p.failSpell=0;f.script.OnGossipHello(&f.p,&f.go);f.Finish();assert(f.p.events==1);++cases;}
 {Fixture f;f.p.canChild=false;f.LootWeapon();f.Finish();assert(!f.p.events&&f.p.errors==1);f.p.canChild=true;f.script.OnGossipHello(&f.p,&f.go);f.Finish();assert(f.p.events==1&&f.p.swaps==1);++cases;}
 {Fixture f;f.LootWeapon();f.go.SetLootState(GO_JUST_DEACTIVATED,&f.p);assert(f.p.casts==1);f.Arrive();f.End(AURA_REMOVE_BY_EXPIRE);f.End(AURA_REMOVE_BY_EXPIRE);f.ai->UpdateAI(1);f.ai->UpdateAI(100000);assert(f.p.events==1&&conversations==1);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.p.teleport=true;f.ai->UpdateAI(1);assert(!f.p.aura&&!f.p.events&&f.p.owned);++cases;}
 for(int change=0;change<4;++change){Fixture f;f.LootWeapon();f.Arrive();f.End(AURA_REMOVE_BY_EXPIRE);f.p.onSwap=[&]{if(change==0)f.p.questStatus.ScriptLifetime=std::make_shared<uint8>();if(change==1)f.p.parent.guid.value=44;if(change==2)f.go.world=false;if(change==3)f.p.status=0;};f.ai->UpdateAI(1);assert(!f.p.events&&!f.p.credits&&!f.p.ability&&f.p.owned);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.End(AURA_REMOVE_BY_EXPIRE);f.ai->UpdateAI(1);assert(!f.p.phase&&!f.p.visible&&f.p.events==1&&conversations==1);++cases;}
 {Fixture f;f.LootWeapon();f.Finish();assert(f.p.questStatus.ScriptObjects&&mapConversations.size()==1);auto scope=f.p.questStatus.ScriptObjects;assert(!mapConversations[0]->IsQuestObjectExpired()&&scope->Owner==f.p.guid);auto cancelled=scope->Cancel();assert(cancelled.size()==1&&cancelled[0].value==71&&mapConversations[0]->IsQuestObjectExpired());++cases;}
 {Fixture f;f.p.questStatus.ScriptLifetime.reset();f.p.owned=true;assert(f.script.OnGossipHello(&f.p,&f.go));assert(f.p.questStatus.ScriptLifetime&&f.p.actions.size()==1);f.p.actions[0]();assert(!f.p.events);f.Finish();assert(f.p.events==1&&f.p.swaps==1);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();auto held=f.p.questStatus.ScriptLifetime;f.p.questStatus.ScriptLifetime=std::make_shared<uint8>();assert(!dynamic_cast<go_art_sharasdal::AI*>(f.ai.get())->attempt.expired());f.End(AURA_REMOVE_BY_EXPIRE);f.ai->UpdateAI(1);assert(!f.p.events&&!f.p.credits&&f.p.owned);++cases;}
 {Fixture f;f.LootWeapon();f.Arrive();f.p.guid.value|=uint64(1)<<32;f.End(AURA_REMOVE_BY_EXPIRE);f.ai->UpdateAI(1);assert(!f.p.events&&!f.p.credits);++cases;}
 std::cout<<"PASS "<<cases<<" actual whole acquisition controller/registered SpellScript/AuraScript and native SetLootState, loot-storage/consumption,249 equip,61 event,134 eligible-group dispatch fragments: actual owned parent/child before native progression, full bags/equip/cast/scene failures, original quest/item/player identity, stale callbacks/retry/cleanup/optional outro and foreign admission. Registry/collision/spline/scene/network/group/criteria services controlled; no client/complete scenario traversal claim.\n";
#endif
}

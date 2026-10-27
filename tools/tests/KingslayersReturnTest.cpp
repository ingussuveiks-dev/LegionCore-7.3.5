#define _USE_MATH_DEFINES
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
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using SpellEffIndex=uint32;
constexpr uint32 QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,CLASS_ROGUE=4;
constexpr uint32 EFFECT_0=0,EFFECT_1=1,EFFECT_ALL=255,SPELL_EFFECT_ANY=0,SPELL_EFFECT_SEND_EVENT=61,SPELL_EFFECT_TELEPORT_L=252,SPELL_EFFECT_TELEPORT=5;
constexpr uint32 SPELL_EFFECT_HANDLE_HIT=2,SPELL_EFFECT_HANDLE_HIT_TARGET=1,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,TARGET_UNIT_CASTER=1,TARGET_NONE=0,TARGET_DEST_TARGET_FRONT=64,MAPID_INVALID=uint32(-1);
constexpr uint32 GO_READY=1,GO_ACTIVATED=2,GO_STATE_READY=1,GO_STATE_ACTIVE=0,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,GAMEOBJECT_TYPE_GOOBER=10,GAMEOBJECT_TYPE_TRAP=6;
constexpr uint32 SPELL_AURA_MOD_STEALTH=16,TELE_TO_SPELL=1,TELE_TO_NOT_LEAVE_COMBAT=2;
#include "CriteriaTypes.inc"
constexpr uint32 MAX_SPELL_EFFECTS=2,TARGET_DEST_DYNOBJ_ENEMY=28;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_ERROR};
uint32 warnings=0;std::vector<std::string> order;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++warnings)
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
struct Hooks{template<class T>void operator+=(T){}};
struct ObjectGuid{uint64 value=0;bool operator==(ObjectGuid const&o)const{return value==o.value;}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}bool operator<(ObjectGuid const&o)const{return value<o.value;}};
struct Player;struct Unit;struct GameObject;struct GameObjectAI;struct Spell;struct SpellInfo;struct Battleground;
struct WorldLocation{uint32 map=0;float x=0,y=0,z=0,o=0;WorldLocation()=default;WorldLocation(uint32 m,float a,float b,float c,float d){WorldRelocate(m,a,b,c,d);}void WorldRelocate(uint32 m,float a,float b,float c,float d){map=m;x=a;y=b;z=c;o=d;}uint32 GetMapId()const{return map;}float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}void GetPosition(float&a,float&b,float&c,float&d){a=x;b=y;c=z;d=o;}};
struct Map{uint32 id=42,mapId=1620;GameObject* portal=nullptr;uint32 scriptCalls=0;template<class...T>void ScriptsStart(T...){++scriptCalls;}};
struct WorldObject{Map*map=nullptr;ObjectGuid guid;uint32 entry=0,phase=1;bool world=true;virtual~WorldObject()=default;virtual Player*ToPlayer(){return nullptr;}
 Map*GetMap()const{return map;}uint32 GetMapId()const{return map?map->mapId:0;}uint32 GetInstanceId()const{return map?map->id:0;}bool IsInWorld()const{return world;}ObjectGuid GetGUID()const{return guid;}uint32 GetGUIDLow()const{return uint32(guid.value);}uint32 GetEntry()const{return entry;}bool InSamePhase(WorldObject*o)const{return o&&phase==o->phase;}float GetOrientation()const{return 1;}};
struct ZoneScript{void ProcessEvent(WorldObject*,uint32){}};using InstanceScript=ZoneScript;
struct Unit:WorldObject{bool alive=true,flight=false;virtual~Unit()=default;bool IsAlive()const{return alive;}bool IsPlayer(){return ToPlayer()!=nullptr;}bool isInFlight(){return flight;}bool HasAuraType(uint32){return false;}virtual SpellCastResult CastSpell(Unit*,SpellInfo const*,bool){return SPELL_FAILED_ERROR;}ZoneScript*GetZoneScript(){return nullptr;}InstanceScript*GetInstanceScript(){return nullptr;}void NearTeleportTo(float,float,float,float,bool){}};
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8>ScriptLifetime;};
struct ProgressCounter{uint64 Counter=0;};
struct Scenario{uint32 id=1123,step=9;std::map<uint32,ProgressCounter>counters{{48591,{1}}};uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Scenario&GetAchievementMgr(){return*this;}auto const*GetCriteriaProgressMap(){return &counters;}};
struct Manager{Scenario scenario;bool present=true;Scenario*GetScenario(uint32 i){return present&&i==42?&scenario:nullptr;}}manager;auto*sScenarioMgr=&manager;
struct Player:Unit{uint32 questId=42504,status=3,klass=4,events=0,event73=0,teleports=0,interactions=0,casts=0,errors=0;bool rewarded=false,contact=true,gm=false,item=true,visible=true,los=true,near=true,record=true,teleport=false,canDelay=false,m_bHasDelayedTeleport=false,rejectTeleport=false;
 QuestStatusData quest;WorldLocation m_teleport_dest;uint32 m_teleport_options=0;std::function<void()>onTeleport,onEvent;std::vector<std::function<void()>>functions;
 Player(){guid={7};}Player*ToPlayer()override{return this;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}uint32 getClass(){return klass;}bool IsSpectator(){return false;}
 bool IsBeingTeleported()const{return teleport;}
#include "Pending.inc"
 bool GetQuestRewardStatus(uint32 id){return id==questId&&rewarded;}uint32 GetQuestStatus(uint32 id){return id==questId?status:0;}QuestStatusData*getQuestStatus(uint32 id){if(id!=questId||!record||!status)return nullptr;quest.Status=status;return &quest;}
 bool HasItemCount(uint32 id,uint32,bool){assert(id==128870);return item;}bool canSeeOrDetect(WorldObject*)const{return visible;}bool IsWithinLOSInMap(WorldObject*)const{return los;}bool IsWithinDistInMap(WorldObject*,float)const{return near;}
 void AddDelayedEvent(uint64,std::function<void()>&&f){functions.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32=0){assert(asset==51617);if(type==CRITERIA_TYPE_SCRIPT_EVENT_2){++events;order.push_back("event");manager.scenario.counters[48593].Counter=1;manager.scenario.step=10;status=1;}else if(type==CRITERIA_TYPE_SCRIPT_EVENT)++event73;if(onEvent)onEvent();}
 void KillCreditGO(uint32,ObjectGuid){++interactions;}void PrepareGossipMenu(WorldObject*,uint32){}void SendPreparedGossip(WorldObject*){}void SendDirectMessage(void*){}Battleground*GetBattleground(){return nullptr;}
 bool IsCanDelayTeleport(){return canDelay;}void SetCanDelayTeleport(bool b){canDelay=b;}bool IsHasDelayedTeleport(){return m_bHasDelayedTeleport;}void SetDelayedTeleportFlag(bool b){m_bHasDelayedTeleport=b;}void SetSemaphoreTeleportFar(bool b){teleport=b;}bool isWatchingMovie(){return false;}
#include "DelayScope.inc"
 WorldLocation const&GetTeleportDest()const{return m_teleport_dest;}
 bool TeleportTo(uint32 mapid,float x,float y,float z,float orientation,uint32 options,uint32){++teleports;order.push_back("teleport");if(onTeleport)onTeleport();if(rejectTeleport)return false;
#include "TeleportDelay.inc"
  world=false;return true;}
 void CancelQuestObjects(uint32){}void GetGameObjectListWithEntryInGrid(std::list<GameObject*>&,uint32,float);SpellCastResult CastSpell(Unit*,SpellInfo const*,bool)override;
};
#include "QuestDelay.inc"
struct Target{uint32 value;uint32 GetTarget()const{return value;}};
struct EffectInfo{uint32 Effect=0,MiscValue=0;Target TargetA{0},TargetB{0};uint32 GetProvidedTargetMask()const{return TargetA.value==1?1:0;}};
struct SpellInfo{uint32 Id=215151,EffectMask=3;EffectInfo data[2]={{61,51617,{0},{0}},{252,0,{1},{17}}};EffectInfo*Effects[2]={data,data+1};EffectInfo*GetEffect(uint32 i,uint32)const{return Effects[i];}bool HasEffect(uint32 v)const{return v==252;}};
struct SpellTargetPosition{uint32 target_mapId=1220;float target_X=-854.53f,target_Y=4597.28f,target_Z=748.81f,target_Orientation=4.5759f;};
struct SpellManager{SpellInfo info;SpellTargetPosition dest;SpellInfo const*GetSpellInfo(uint32 id){return id==215151?&info:nullptr;}SpellTargetPosition const*GetSpellTargetPosition(uint32){return &dest;}}spellManager;auto*sSpellMgr=&spellManager;
namespace WorldPackets{namespace GameObject{struct PageText{explicit PageText(ObjectGuid){}void*Write(){return nullptr;}};}namespace Misc{struct CustomLoadScreen{CustomLoadScreen(uint32,uint32){}void*Write(){return nullptr;}};}}
struct GameObjectTemplate{uint32 entry=251123,type=10;struct{uint32 pageID=0,gossipID=0,eventID=0,questID=0,linkedTrap=0,customAnim=0,spell=215151;}goober;uint32 GetCooldown()const{return 0;}uint32 GetAutoCloseTime()const{return 0;}};
struct GameObjectAI{GameObject*go;explicit GameObjectAI(GameObject*o):go(o){}virtual~GameObjectAI()=default;virtual void Reset(){}virtual void UpdateAI(uint32){}virtual bool GossipUse(Player*){return false;}};
struct GameObject:WorldObject{GameObjectAI*ai=nullptr;GameObjectTemplate info;uint32 dbGuid=134046,flags=32,loot=1,state=1,m_cooldownTime=0;bool spawned=true;
 GameObject(){guid={11};entry=251123;}GameObjectAI*AI(){return ai;}bool isSpawned()const{return spawned;}uint32 GetDBTableGUIDLow()const{return dbGuid;}bool HasFlag(uint32,uint32 f)const{return flags&f;}void SetFlag(uint32,uint32 f){flags|=f;}void RemoveFlag(uint32,uint32 f){flags&=~f;}
 uint32 getLootState()const{return loot;}void SetLootState(uint32 v,Unit* =nullptr){loot=v;}uint32 GetGoState()const{return state;}void SetGoState(uint32 v){state=v;}float GetInteractionDistance()const{return 5;}
 Unit*GetOwner(){return nullptr;}GameObjectTemplate const*GetGOInfo()const{return &info;}uint32 GetGoType(){return 10;}void EventInform(uint32){}void TriggeringLinkedGameObject(uint32,Unit*){}uint32 GetGoAnimProgress(){return 255;}void SendCustomAnim(uint32){}void Use(Unit*);SpellCastResult CastSpell(Unit*,uint32);
};
void Player::GetGameObjectListWithEntryInGrid(std::list<GameObject*>&o,uint32 id,float){if(map&&map->portal&&map->portal->GetEntry()==id)o.push_back(map->portal);}
namespace ObjectAccessor{GameObject*GetGameObject(Player&p,ObjectGuid g){return p.map&&p.map->portal&&p.map->portal->guid==g?p.map->portal:nullptr;}}
struct Scripts{bool OnGossipHello(Player*,GameObject*){return false;}}scripts;auto*sScriptMgr=&scripts;
struct Conditions{bool IsPlayerMeetingCondition(Player*,uint32){return true;}}conditions;auto*sConditionMgr=&conditions;
struct QuestStore{bool GetQuestTemplate(uint32){return false;}}quests;auto*sQuestDataStore=&quests;
struct Battleground{void EventPlayerUsedGO(Player*,GameObject*){}};
struct Outdoor{bool HandleCustomSpell(Player*,uint32,GameObject*){return false;}}outdoor;auto*sOutdoorPvPMgr=&outdoor;
namespace GameTime{uint32 GetGameTime(){return 100;}}int sEventScripts=0,sGameObjectScripts=0;
bool destinationAvailable=true;
struct Spell{Unit*m_caster;Unit*unitTarget;GameObject*gameObjTarget=nullptr;WorldObject*focusObject=nullptr;uint32 effectHandleMode=2,m_diffMode=0;SpellInfo const*m_spellInfo;WorldLocation target;WorldLocation*destTarget=&target;struct{bool HasDst(){return destinationAvailable;}Unit*GetUnitTarget(){return nullptr;}}m_targets;
 explicit Spell(Player*p):m_caster(p),unitTarget(p),m_spellInfo(&spellManager.info){auto&d=spellManager.dest;target.WorldRelocate(d.target_mapId,d.target_X,d.target_Y,d.target_Z,d.target_Orientation);}void EffectSendEvent(uint32);void EffectTeleportUnits(uint32);
};
struct SpellScript{virtual~SpellScript()=default;Spell*spell=nullptr;bool prevented=false;Hooks OnCheckCast,OnEffectHit,OnEffectHitTarget;virtual bool Validate(SpellInfo const*){return true;}virtual bool Load(){return true;}virtual void Register(){}Unit*GetCaster(){return spell->m_caster;}Player*GetHitPlayer(){return spell->unitTarget?spell->unitTarget->ToPlayer():nullptr;}Spell*GetSpell(){return spell;}void PreventHitDefaultEffect(uint32){prevented=true;}};
#include "Acquisition.inc"
#include "Return.inc"
#include "SendEvent.inc"
#include "Teleport.inc"
#include "Use.inc"
#include "GoCast.inc"
bool baseline=false,failCast=false;std::function<void(spell_kingslayers_return&)>beforeHit;
SpellCastResult Player::CastSpell(Unit*,SpellInfo const*,bool triggered){assert(triggered);++casts;if(failCast)return SPELL_FAILED_ERROR;Spell native(this);spell_kingslayers_return script;script.spell=&native;
 if(!baseline){assert(script.Validate(native.m_spellInfo));script.Load();auto result=script.Check();if(result!=SPELL_CAST_OK)return result;if(beforeHit)beforeHit(script);script.Event(0);}
 if(baseline||!script.prevented)native.EffectSendEvent(0);script.prevented=false;native.effectHandleMode=1;
 if(!baseline){script.Hit(1);script.Depart(1);}if(baseline||!script.prevented)native.EffectTeleportUnits(1);return SPELL_CAST_OK;}
struct Run{Map map;Player player,other;GameObject portal;go_kingslayers_return ai;Run():ai(&portal){manager=Manager{};spellManager.dest=SpellTargetPosition{};destinationAvailable=true;warnings=0;order.clear();baseline=failCast=false;beforeHit={};player.map=other.map=portal.map=&map;other.guid={8};map.portal=&portal;portal.ai=&ai;}
 void Use(){portal.Use(&player);}void ResetFlight(){player.teleport=player.m_bHasDelayedTeleport=false;player.world=true;}
};
int main(){
 {Run r;baseline=true;r.player.rejectTeleport=true;r.Use();assert(r.player.events==1&&r.player.teleports==1);assert(r.portal.flags&1);assert(r.portal.m_cooldownTime==160);std::cout<<"COUNTERFACTUAL native goober/SendEvent credits51617 before refused teleport and retains60s IN_USE.\n";}
 for(uint32 q:{42504u,42627u}){Run r;r.player.questId=q;r.Use();assert(r.player.teleports==1&&r.player.events==1&&r.player.event73==1);assert(r.player.m_bHasDelayedTeleport&&r.player.map==&r.map);assert((order==std::vector<std::string>{"teleport","event"}));assert(r.portal.flags&1);r.ai.UpdateAI(1);assert(r.portal.flags==32&&r.portal.loot==GO_READY&&r.portal.state==GO_STATE_READY&&!r.ai.receipt);}
 {Run r;r.player.rejectTeleport=true;r.Use();assert(!r.player.events&&warnings==1);r.ai.UpdateAI(1);r.player.rejectTeleport=false;r.Use();assert(r.player.events==1&&r.player.teleports==2);}
 {Run r;failCast=true;r.Use();r.ai.UpdateAI(4999);assert(r.portal.flags&1);r.ai.UpdateAI(1);assert(r.portal.flags==32);failCast=false;r.Use();assert(r.player.events==1);}
 for(uint32 kind=0;kind<16;++kind){Run r;switch(kind){case 0:r.player.alive=false;break;case 1:r.player.contact=false;break;case 2:r.player.visible=false;break;case 3:r.player.los=false;break;case 4:r.player.near=false;break;case 5:r.player.teleport=true;break;case 6:r.player.m_bHasDelayedTeleport=true;break;case 7:r.player.phase=2;break;case 8:r.portal.world=false;break;case 9:r.portal.spawned=false;break;case 10:r.portal.flags|=16;break;case 11:r.portal.flags|=1;break;case 12:r.portal.loot=GO_ACTIVATED;break;case 13:r.player.world=false;break;case 14:r.portal.state=GO_STATE_ACTIVE;r.portal.loot=GO_ACTIVATED;break;case 15:r.player.map=nullptr;break;}r.Use();assert(!r.player.events&&!r.player.teleports);}
 for(uint32 kind=0;kind<8;++kind){Run r;switch(kind){case 0:r.player.status=0;break;case 1:r.player.status=1;break;case 2:r.player.rewarded=true;r.player.status=1;break;case 3:r.player.item=false;break;case 4:r.player.klass=1;break;case 5:r.player.gm=true;break;case 6:manager.scenario.step=8;break;case 7:manager.scenario.counters[48591].Counter=0;break;}r.Use();assert(r.player.teleports==1&&!r.player.events);}
 {Run r;Spell n(&r.player);spell_kingslayers_return s;s.spell=&n;s.Load();assert(s.Check()==SPELL_FAILED_BAD_TARGETS);s.Event(0);assert(s.prevented);s.Depart(1);assert(!r.player.teleports&&!r.player.events);}
 for(uint32 kind=0;kind<8;++kind){Run r;beforeHit=[&](spell_kingslayers_return&){switch(kind){case 0:r.player.status=0;break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.portal.guid={12};break;case 3:r.portal.phase=2;break;case 4:r.portal.spawned=false;break;case 5:r.ai.Reset();break;case 6:r.player.alive=false;break;case 7:r.player.teleport=true;break;}};r.Use();assert(!r.player.teleports&&!r.player.events);}
 {Run r;r.player.onTeleport=[&]{r.player.quest.ScriptLifetime=std::make_shared<uint8>();};r.Use();assert(r.player.teleports==1&&!r.player.events&&warnings==1);}
 {Run r;bool nested=false;r.player.onTeleport=[&]{if(!nested){nested=true;assert(r.player.CastSpell(&r.player,&spellManager.info,true)==SPELL_FAILED_BAD_TARGETS);}};r.Use();assert(r.player.teleports==1&&r.player.events==1);}
 {Run r;r.player.onEvent=[&]{r.portal.Use(&r.other);};r.Use();assert(r.player.events==1&&!r.other.casts);r.ai.UpdateAI(1);r.portal.Use(&r.other);assert(r.other.casts==1&&!r.other.events);}
 {Run r;assert(!r.ai.GossipUse(&r.player));r.portal.loot=GO_ACTIVATED;r.portal.state=GO_STATE_ACTIVE;r.portal.flags|=1;Spell n(&r.player);spell_kingslayers_return s;s.spell=&n;s.Load();assert(s.Check()==SPELL_CAST_OK);n.effectHandleMode=1;s.Depart(1);s.Depart(1);assert(r.player.events==1&&r.player.teleports==1);}
 {Run r;r.player.status=0;r.Use();assert(!r.player.events);r.ai.UpdateAI(1);r.ResetFlight();r.player.status=3;r.Use();assert(r.player.events==1);}
 {Run r;manager.present=false;r.Use();assert(r.player.teleports==1&&!r.player.events);}
 {Run r;r.player.flight=true;r.Use();assert(!r.player.events&&!r.player.teleports&&warnings==1);r.ai.UpdateAI(1);assert(r.portal.flags==32);}
 {Run r;destinationAvailable=false;r.Use();assert(!r.player.events&&!r.player.teleports&&warnings==2);}
 {Run r;spellManager.dest.target_mapId=1;r.Use();assert(r.player.teleports==1&&!r.player.events&&warnings==1);}
 {Run r;spellManager.dest.target_X=0;r.Use();assert(r.player.teleports==1&&!r.player.events&&warnings==1);}
 {Run r;r.player.onTeleport=[&]{r.player.alive=false;};r.Use();assert(r.player.teleports==1&&!r.player.events&&warnings==1);}
 {Run r;r.player.canDelay=true;r.Use();assert(r.player.events==1&&r.player.canDelay);}
 {Run r;r.player.rejectTeleport=true;r.Use();r.portal.flags|=16;r.ai.UpdateAI(1);assert(r.portal.flags==48&&r.portal.loot==GO_READY);}
 {Run r;failCast=true;r.Use();r.portal.state=2;r.ai.UpdateAI(5000);assert(r.portal.state==2&&(r.portal.flags&1));}
 std::cout<<"PASS actual Kingslayers return hooks, native goober activation/self-cast/SendEvent/teleport route/quest delay: accepted-before-credit, two quests, refusals/retry, bounded release, duplicate/reentrant, abandonment/reaccept/stale receipts, escape and shared scope. Engine map/criteria endpoints controlled; client traversal separate.\n";
}

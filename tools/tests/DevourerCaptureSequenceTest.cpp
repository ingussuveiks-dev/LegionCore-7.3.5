#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
using DamageEffectType=uint32;using Milliseconds=std::chrono::milliseconds;
enum {TYPEID_PLAYER=4,CLASS_ROGUE=4,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=2,
 UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_FLAG_NOT_ATTACKABLE_1=8,REACT_PASSIVE=0,REACT_AGGRESSIVE=1,UNIT_STATE_CASTING=1,UNIT_STATE_STUNNED=2,TEXT_GENERIC_3=3,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
#include "FunctionProcessor.inc"
struct ObjectGuid{uint64 high=0,low=0;uint32 GetGUIDLow()const{return uint32(low);}bool IsEmpty()const{return !high&&!low;}bool IsPlayer()const{return high==1;}
 void Clear(){high=low=0;}bool operator!=(ObjectGuid b)const{return high!=b.high||low!=b.low;}bool operator==(ObjectGuid b)const{return !(*this!=b);}static const ObjectGuid Empty;};
const ObjectGuid ObjectGuid::Empty{};
struct Map{};
struct WorldObject{Map* map=nullptr;ObjectGuid guid;bool world=true,preDelete=false,alive=true;uint32 entry=105536,phase=1,mapId=1607;float distance=1;
 Map* GetMap()const{return map;}uint32 GetMapId()const{return mapId;}ObjectGuid GetGUID()const{return guid;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}
 uint32 GetEntry()const{return entry;}bool InSamePhase(WorldObject* o)const{return phase==o->phase;}bool IsWithinDistInMap(WorldObject* o,float range)const{assert(range==40);return o->map==map&&o->distance<=range;}};
struct Player;struct Creature;
struct Unit:WorldObject{ObjectGuid owner,charmer;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Player const* ToPlayer()const{return nullptr;}
 virtual uint32 GetTypeId()const{return 3;}ObjectGuid GetOwnerGUID()const{return owner;}ObjectGuid GetCharmerGUID()const{return charmer;}
#include "OwnerGuid.inc"
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;};
struct Destination{uint32 mapId=0;float x=0,y=0,z=0;uint32 GetMapId()const{return mapId;}float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}};
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit{QuestStatusData quest;bool record=true;uint32 classId=4,status=3,credits=0,teleports=0,teleportMode=0;bool contact=true,teleporting=false,m_bHasDelayedTeleport=false;Destination dest;
 FunctionProcessor functions;std::vector<uint32> casts;std::vector<std::pair<uint32,ObjectGuid>> removals;std::function<void()> onTeleport,onCredit;
 QuestStatusData* getQuestStatus(uint32 id){assert(id==41924);quest.Status=status;return record?&quest:nullptr;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 Player* ToPlayer()override{return this;}Player const* ToPlayer()const override{return this;}uint32 GetTypeId()const override{return 4;}
 bool CanContact(){return contact&&world&&!preDelete;}uint32 getClass()const{return classId;}uint32 GetQuestStatus(uint32 id)const{assert(id==41924);return status;}
 bool IsBeingTeleported()const{return teleporting;}
#include "PendingTeleport.inc"
Destination& GetTeleportDest(){return dest;}
 bool TeleportTo(uint32 id,float x,float y,float z,float o){assert(id==1607&&x==1847.77f&&y==1259.92f&&z==57.09f&&!o);++teleports;if(onTeleport)onTeleport();
  if(teleportMode==1)return false;if(teleportMode==2)return true;teleporting=true;dest={id,x,y,z};if(teleportMode==3)dest.mapId=1;if(teleportMode==4)dest.x+=1;return true;}
 void CastSpell(Player* target,uint32 id){assert(target==this&&(id==211686||id==211684));casts.push_back(id);}
 void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92&&id==50261);++credits;if(onCredit)onCredit();}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){removals.push_back({id,caster});}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& fn){functions.AddDelayedEvent(delay,std::move(fn));}};
#include "QuestDelay.inc"
struct Creature:Unit{Unit* victim=nullptr;Creature* helper=nullptr;Player* nearest=nullptr;uint32 flags=0,react=1,state=0,despawns=0;bool failSpawn=false;FunctionProcessor functions;std::vector<uint32> casts;std::function<void()> onCast;
 uint32 GetInstanceId()const{return 7;}uint64 GetHealth(Unit*)const{return 1000;}Unit* getVictim(){return victim;}
 float GetPositionX()const{return 10;}float GetPositionY()const{return 20;}float GetPositionZ()const{return 30;}float GetExactDist2d(Unit*)const{return 1;}
 void SetFlag(uint32 field,uint32 value){assert(field==1);flags|=value;}void RemoveFlag(uint32 field,uint32 value){assert(field==1);flags&=~value;}void SetReactState(uint32 value){react=value;}
 bool HasUnitState(uint32 mask)const{return (state&mask)!=0;}Creature* SummonCreature(uint32 id,float x,float y,float z){assert(id==105540&&x==13&&y==23&&z==30);return failSpawn?nullptr:helper;}
 void CastSpell(Unit*,uint32 id){assert(id==189170||id==209264);casts.push_back(id);if(onCast)onCast();}void DespawnOrUnsummon(uint32 delay=0){++despawns;if(!delay)world=false;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& fn){functions.AddDelayedEvent(delay,std::move(fn));}Player* FindNearestPlayer(float range){assert(range==40);return nearest;}};
template<class T>struct HashMapHolder{inline static bool _checkLock=false;inline static std::shared_mutex i_lockVector;inline static uint32 _size=32;inline static std::vector<T*> _objectVector=std::vector<T*>(32,nullptr);
#include "Find.inc"
};
struct ObjectAccessor{
#include "Lookup.inc"
};
#include "LookupMethods.inc"
#include "Owner.inc"
struct Scenario{uint32 id=1078,step=0;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 instance){return present&&instance==7?&scenario:nullptr;}} manager;
auto sScenarioMgr=&manager;
enum class HighGuid{Conversation};struct Generator{uint64 Generate(){return 1;}};
struct ObjectManager{Generator gen;template<HighGuid>Generator* GetGenerator(){return &gen;}} objectManager;
auto sObjectMgr=&objectManager;
struct Conversation{bool CreateConversation(uint64,uint32,Unit*,void*,Unit&){return false;}};
struct ScriptedAI{Creature* me;uint32 evades=0;std::vector<uint32> talks,casts;explicit ScriptedAI(Creature* actor):me(actor){}virtual ~ScriptedAI()=default;
 virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}virtual void MoveInLineOfSight(Unit*){}virtual void UpdateAI(uint32){}
 void EnterEvadeMode(){++evades;me->victim=nullptr;Reset();}void Talk(uint32 id){talks.push_back(id);}void SetCanSeeEvenInPassiveMode(bool){}bool UpdateVictim(){return me->victim!=nullptr;}
 void DoCast(uint32 id){assert(id==209231||id==209238);casts.push_back(id);}void DoMeleeAttackIfReady(){}};
struct TextManager{ScriptedAI* ai=nullptr;void SendChat(Creature*,uint32 group){ai->Talk(group);}} textManager;
auto sCreatureTextMgr=&textManager;
#include "Actor.inc"
struct Fixture{Map map,other;Player player,stranger;Unit pet,guardian,charmed;Creature actor;std::unique_ptr<Creature> helper=std::make_unique<Creature>();std::unique_ptr<npc_akaari_shadowgoreAI> ai;
 Fixture(){warnings=0;manager={};HashMapHolder<Player>::_objectVector.assign(32,nullptr);HashMapHolder<Creature>::_objectVector.assign(32,nullptr);
  player.map=&map;player.guid={1,1};stranger.map=&map;stranger.guid={1,2};actor.map=&map;actor.guid={2,3};actor.nearest=&stranger;actor.helper=helper.get();helper->map=&map;helper->guid={2,4};helper->entry=105540;
  pet.map=&map;pet.owner=player.guid;guardian=pet;charmed=pet;charmed.owner=stranger.guid;charmed.charmer=player.guid;
  HashMapHolder<Player>::_objectVector[1]=&player;HashMapHolder<Player>::_objectVector[2]=&stranger;HashMapHolder<Creature>::_objectVector[4]=helper.get();ai=std::make_unique<npc_akaari_shadowgoreAI>(&actor);textManager.ai=ai.get();ai->Reset();}
 void Start(Unit* who=nullptr){actor.victim=who?who:&player;ai->EnterCombat(actor.victim);Tick(12000);Tick(11000);}
 void Tick(uint32 ms){actor.functions.Update(ms);player.functions.Update(ms);stranger.functions.Update(ms);if(ai&&actor.world)ai->UpdateAI(ms);}
 void DropHelper(){HashMapHolder<Creature>::_objectVector[4]=nullptr;actor.helper=nullptr;helper.reset();}
};
int main(){
#ifdef BASELINE
 {Fixture f;f.actor.nearest=&f.player;f.Start();f.player.map=&f.other;f.player.status=0;f.Tick(10000);assert(f.player.teleports==1&&f.player.credits==1);}
 {Fixture f;f.actor.failSpawn=true;f.Start();f.Tick(20000);assert(f.actor.flags==14&&f.actor.react==0&&f.actor.despawns==0&&!f.player.credits);}
 std::cout<<"PASS pinnede4e4fe2f counterfactuals: abandoned/moved participant is forcibly transferred/credited; missing105540 leaves immune actor without timer. Expected failing ordinary behavior.\n";
#else
 uint32 variants=0;
 for(int owner=0;owner<4;++owner)for(int missing=0;missing<2;++missing)for(int blocked=0;blocked<2;++blocked){Fixture f;f.actor.failSpawn=missing!=0;Unit* who=owner==0?static_cast<Unit*>(&f.player):owner==1?&f.pet:owner==2?&f.guardian:&f.charmed;
  f.Start(who);assert(f.ai->capturePending&&f.actor.flags==14);f.ai->EnterCombat(&f.stranger);f.actor.victim=nullptr;if(blocked)f.actor.state=3;
  f.Tick(4999);assert(!f.player.teleports&&f.helper->casts.size()==(missing?0:1));f.Tick(1);assert(!f.player.teleports&&f.ai->talks.back()==3&&f.helper->casts.size()==(missing?0:2));
  if(!missing)f.DropHelper();f.Tick(4999);assert(!f.player.teleports);f.Tick(1);assert(f.player.teleports==1&&f.player.credits==1&&f.player.casts==std::vector<uint32>({211686,211684})&&!f.stranger.teleports&&!f.stranger.credits&&f.actor.despawns==1);
  f.ai->UpdateAI(10000);f.ai->EnterCombat(&f.stranger);assert(f.player.credits==1&&f.ai->captureComplete);++variants;}
 for(int invalid=0;invalid<16;++invalid){Fixture f;f.Start();f.Tick(5000);switch(invalid){case 0:f.player.map=&f.other;break;case 1:f.player.status=0;break;case 2:f.player.status=1;break;case 3:f.player.contact=false;break;case 4:f.player.alive=false;break;case 5:f.player.teleporting=true;break;case 6:f.player.m_bHasDelayedTeleport=true;break;case 7:f.player.classId=1;break;case 8:f.player.phase=2;break;case 9:f.player.distance=41;break;case 10:manager.scenario.id=1;break;case 11:manager.scenario.step=1;break;case 12:manager.present=false;break;case 13:f.player.preDelete=true;break;case 14:f.player.guid.high=9;break;case 15:f.player.status=6;break;}
  f.Tick(1);if(f.ai->evades!=1)std::cerr<<"invalid index "<<invalid<<" not cancelled\n";assert(f.ai->evades==1&&!f.ai->capturePending&&!f.actor.flags&&f.actor.react==1&&!f.player.teleports&&!f.player.credits);f.player.map=&f.map;f.player.status=3;f.Tick(10000);assert(!f.player.credits);}
 for(int failure=1;failure<=4;++failure){Fixture f;f.player.teleportMode=failure;f.Start();f.Tick(10000);assert(f.player.teleports==1&&!f.player.credits&&f.player.casts.empty()&&!f.actor.flags&&f.ai->evades==1);}
 {Fixture f;f.Start();f.Tick(5000);ObjectGuid old=f.helper->guid;f.ai->Reset();assert(f.helper->despawns==2&&f.player.removals.back().first==209264&&f.player.removals.back().second==old);f.Tick(10000);assert(!f.player.teleports);f.helper->world=true;f.Start(&f.pet);f.Tick(10000);assert(f.player.credits==1);}
 for(int missing=0;missing<6;++missing){Fixture f;f.Start();switch(missing){case 0:f.DropHelper();break;case 1:f.helper->preDelete=true;break;case 2:f.helper->alive=false;break;case 3:f.helper->phase=2;break;case 4:f.helper->entry=1;break;case 5:f.helper->guid.high=8;break;}f.Tick(5000);if(f.helper)assert(f.helper->casts.size()==1);f.Tick(5000);assert(f.player.credits==1);}
 {Fixture f;f.Start();auto weak=f.ai->captureLifetime;f.player.quest.ScriptLifetime.reset();f.player.status=3;f.player.AddQuestDelayedEvent(41924,0,[]{});assert(weak.expired());f.Tick(10000);assert(f.ai->evades==1&&!f.player.credits&&!f.player.teleports);}
 {Fixture f;f.Start();auto old=f.player.quest.ScriptLifetime;f.player.quest.ScriptLifetime=std::make_shared<uint8>();f.Tick(10000);assert(f.ai->evades==1&&!f.player.credits&&!f.player.teleports);}
 {Fixture f;f.Start();f.helper->onCast=[&](){f.player.status=0;};f.Tick(10000);assert(f.ai->evades==1&&!f.player.teleports&&!f.player.credits);}
 {Fixture f;f.actor.victim=&f.pet;f.ai->EnterCombat(&f.pet);f.Tick(6000);f.ai->EnterCombat(&f.stranger);f.Tick(6000);assert(f.ai->casts==std::vector<uint32>({209231}));f.Tick(11000);f.Tick(10000);assert(f.player.credits==1&&!f.stranger.credits);}
 {Fixture f;f.Start();f.player.onTeleport=[&](){f.ai->UpdateAI(10000);f.ai->EnterCombat(&f.stranger);};f.player.onCredit=[&](){f.ai->UpdateAI(10000);};f.Tick(10000);assert(f.player.credits==1&&f.player.teleports==1);}
 {Fixture f;f.Start();f.DropHelper();f.ai.reset();f.Tick(20000);assert(!f.player.credits&&!f.player.teleports);}
 {Fixture f;f.Start();f.player.world=false;f.Tick(1);f.player.world=true;f.Tick(10000);assert(!f.player.credits&&!f.player.teleports);}
 {Fixture f;f.actor.nearest=&f.stranger;f.player.classId=1;f.Start();assert(f.ai->evades==1&&!f.player.credits&&!f.stranger.credits);}
 std::cout<<"PASS full extracted opening AI/native EventMap and full-GUID player/creature registry/owner resolution: "<<variants<<" owner/helper/victim/cast-state variants,23s combat+5s visual+10s transfer, invalid context cancellation, reset/retry, actual freed helper/AI, legacy true-but-rejected teleport, reentrancy and original-recipient-only credit. Client arrival/visual playback pending.\n";
#endif
}

// Complete production death/admission methods with controlled external endpoints.
// Native unit ownership and actual data/linked code are checked independently.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
static constexpr uint32 TYPEID_PLAYER=4,TYPEID_UNIT=3,TEAM_ALLIANCE=0,TEAM_HORDE=1,QUEST_STATUS_INCOMPLETE=1,SIZE_OF_GRIDS=533;
namespace TimeConstants{static constexpr uint32 IN_MILLISECONDS=1000;}
struct Player;struct Creature;struct Instance;struct Map;struct Unit;
struct ObjectGuid{uint64 value=0;static ObjectGuid Empty;uint64 GetCounter()const{return value;}bool operator==(ObjectGuid const&x)const{return value==x.value;}bool operator<(ObjectGuid const&x)const{return value<x.value;}};ObjectGuid ObjectGuid::Empty;
using GuidSet=std::set<ObjectGuid>;
struct Position{void Relocate(float,float,float,float){}};
struct Record{uint32 talks=0,casts=0,auras=0,kills=0,despawns=0,credits=0,actions=0,restarts=0,flags=0,fall=999,honor=0,cleanup=0,resurrections=0;Unit*recipient=nullptr;Unit*source=nullptr;ObjectGuid target{};}rec;
struct Endpoint{bool bOpFerok=false,bOpFerokFail=false;uint64 starter=1;void DoAction(uint32){++rec.actions;}uint32 GetData(uint32){return uint32(starter);}template<class...T>void Talk(T...){++rec.talks;}void SetGUID(ObjectGuid,uint32){}}endpoint;
#define CAST_AI(type,value) (value)
struct Motion{void MoveTargetedHome(){}void MoveFall(uint32 id=0){rec.fall=id;}}motion;
struct SpellInfo{uint32 Id=0;};
struct Unit{Player*self=nullptr;Unit*owner=nullptr;Unit*charmer=nullptr;Instance*instance=nullptr;ObjectGuid guid{1};uint32 entry=1;bool alive=true,aura=true,vehicle=true,hostile=true;
 virtual~Unit()=default;Player*ToPlayer(){return self;}virtual Creature*ToCreature(){return nullptr;}bool IsCreature(){return ToCreature()!=nullptr;}Unit*GetOwner(){return owner;}Unit*GetAnyOwner(){return owner;}Endpoint*GetAI(){return &endpoint;}uint32 GetTypeId(){return self?TYPEID_PLAYER:TYPEID_UNIT;}uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}bool HasAura(uint32){return aura;}Instance*GetInstanceScript(){return instance;}float GetPositionZ(){return 42.5f;}
 Player*GetAffectingPlayer(){Unit*u=this;for(uint32 i=0;u&&i<32;++i){if(u->self)return u->self;u=u->charmer?u->charmer:u->owner;}return nullptr;}
 void AddAura(uint32,Unit*u){++rec.auras;rec.recipient=u;}template<class...T>void CastSpell(Unit*u,uint32,T...){++rec.casts;rec.recipient=u;rec.source=this;}void Kill(Unit*u){rec.target=GetGUID();if(u->alive){++rec.kills;rec.recipient=u;rec.source=this;u->alive=false;}}void RemoveAurasDueToSpell(uint32){++rec.cleanup;rec.recipient=this;}
 void SetFlag(uint32,uint32){++rec.flags;}void RemoveFlag(uint32,uint32){++rec.flags;}void setFaction(uint32){++rec.flags;}void SetReactState(uint32){}void SetInCombatWithZone(){}bool IsVehicle(){return vehicle;}
 static Creature*GetCreature(Unit&,ObjectGuid);
};
struct Player:Unit{uint32 quest=QUEST_STATUS_INCOMPLETE;bool movie=false;Player(){self=this;}uint32 GetQuestStatus(uint32){return quest;}bool isWatchingMovie(){return movie;}template<class...T>void UpdateAchievementCriteria(T...){++rec.credits;rec.recipient=this;}void KilledMonsterCredit(uint32){++rec.credits;rec.recipient=this;}template<class...T>void KilledMonster(T...){++rec.credits;rec.recipient=this;}void RewardHonor(Player*,uint32,uint32){++rec.honor;}void ResurrectPlayer(float){alive=true;++rec.resurrections;}void SpawnCorpseBones(){++rec.cleanup;}};
Creature*peer=nullptr;Player*selected=nullptr;
struct Group{bool member=true;bool IsMember(ObjectGuid){return member;}}group;
struct Map{Creature*SummonCreature(uint32,Position);void UpdateEncounterState(uint32,uint32,Unit*,Unit*){++rec.credits;}}map;
struct Creature:Unit{bool hasGroup=true;Creature*ToCreature()override{return this;}Map*GetMap(){return &map;}Endpoint*AI(){return &endpoint;}Group*GetLootRecipientGroup(){return hasGroup?&group:nullptr;}Creature*FindNearestCreature(uint32,uint32){return peer;}void SetObjectScale(float){}void SetCanFly(bool){}void RemoveUnitMovementFlag(uint32){}void DespawnOrUnsummon(uint32){++rec.despawns;}uint64 GetMaxHealth(){return 100;}bool IsDamageEnoughForLootingAndReward(){return true;}void LowerPlayerDamageReq(uint64){}void InterruptNonMeleeSpells(bool){}void*GetCreatureTemplate(){return this;}Motion*GetMotionMaster(){return &motion;}bool IsValidAttackTarget(Player*p){return p->hostile;}void GetPlayerListInGrid(std::list<Player*>&out,float){if(selected)out.push_back(selected);}void AddPlayerInPersonnalVisibilityList(ObjectGuid){}void DisappearAndDie(){++rec.despawns;}void setDeathState(uint32){++rec.despawns;}uint32 GetPowerType(){return 0;}void SetPower(uint32,int32){}
};using TempSummon=Creature;
Creature*Map::SummonCreature(uint32,Position){return peer;}Creature*Unit::GetCreature(Unit&,ObjectGuid){return peer;}
struct Instance{void SendEncounterUnit(uint32,Unit*,uint32=0,uint32=0){}void SetData(uint32,uint32){}ObjectGuid GetGuidData(uint32){return {};}void SetGuidData(uint32,ObjectGuid g){rec.target=g;}void SetBossState(uint32,uint32){}void SaveToDB(){}void DoRemoveAurasDueToSpellOnPlayers(uint32){++rec.cleanup;}template<class...T>void DoUpdateAchievementCriteria(T...){++rec.credits;}};
using InstanceScript=Instance;
struct OutdoorPvPAshran{ObjectGuid GetFactionGenericMoP(uint32){return {};}void HandleFactionBossDeath(uint32){++rec.actions;}};
struct Accessor{static Creature*GetCreature(Unit&,ObjectGuid){return peer;}static Creature*FindCreature(ObjectGuid){return peer;}};using ObjectAccessor=Accessor;Accessor accessor;auto sObjectAccessor=&accessor;
namespace MS{namespace ScriptUtils{Player*SelectRandomPlayerIncludedTank(Unit*){return selected;}}}
struct Text{void SendChat(Creature*,uint32){++rec.talks;}}text;auto sCreatureTextMgr=&text;
struct Events{void Reset(){}};
struct Summons{void DespawnAll(){}};
struct Invocation{ObjectGuid guid;};
struct Base{Creature*me;Instance*instance=nullptr,*_instance=nullptr;OutdoorPvPAshran*m_ZoneScript=nullptr;Events events,_events;Summons summons;bool bWalking=true;uint32 _invocationStage=0;Invocation _invocationOrder[3];uint32 cooldown=0;float tickCount=0,powerCount=100,playersCount=10;ObjectGuid RageclawGUID;GuidSet m_player_for_event;
 explicit Base(Creature*c):me(c){}virtual~Base()=default;virtual void JustDied(Unit*){}virtual void OnSpellClick(Unit*){}virtual void SpellHit(Unit*,SpellInfo const*){}virtual void SpellHitTarget(Unit*,SpellInfo const*){}virtual void MoveInLineOfSight(Unit*){}template<class...T>void Talk(uint32,T...args){++rec.talks;SetTalkTarget(args...);}void SetTalkTarget(){}void SetTalkTarget(ObjectGuid g){rec.target=g;}void _JustDied(){}void DoCast(uint32){++rec.casts;}void DoCast(Unit*u,uint32,bool){++rec.casts;rec.recipient=u;}Creature*DoSummon(uint32,Creature*,uint32){return peer;}void UnlockRageclaw(Unit*u){rec.recipient=u;}
 struct eGromkarboomerSpells{enum{SpellBlackrockMortarShellsAura=160702};};struct eGromkarGrenadierSpells{enum{SpellBlackrockGrenadeAura=160703};};
};
#include "Selected.inc"
static void Require(bool ok,char const*message){if(!ok)throw std::runtime_error(message);}
template<class F>static void Run(uint32 which){
 Instance instance,foreign;Creature victim,other,owner;victim.guid={10};victim.instance=&instance;victim.owner=&owner;owner.guid={90};other.guid={2};peer=&other;
 Player player;player.guid={1};player.instance=&foreign;selected=&player;Unit pet,guardian,charmed,unowned;pet.owner=&player;guardian.owner=&pet;charmed.charmer=&player;pet.guid={3};guardian.guid={4};charmed.guid={5};unowned.guid={6};
 int index=nativeDeathIndex[which];
 if(index<100){
 for(Unit*killer:{static_cast<Unit*>(&player),&pet,&guardian,&charmed,&unowned,static_cast<Unit*>(nullptr)}){
  rec={};other.alive=true;owner.alive=true;F frame(&victim);frame.instance=frame._instance=&instance;victim.entry=index==14?NPC_HEALTHY_MUSHROOM:999;frame.JustDied(killer);bool eligible=killer&&killer!=&unowned;
  if(index==0)Require(rec.actions==unsigned(eligible),"Sironas resolves the actual starter/group owner");
  if(index==1||index==2){Require(rec.auras==unsigned(eligible),"Ammunition belongs to an eligible player owner");if(eligible)Require(rec.recipient==&player,"Ammo targets the actual owner");Require(rec.despawns==unsigned(killer!=nullptr),"Original independent two-second corpse cleanup retained");}
  if(index==3||index==4||index==12||index==16||index==17){Require(rec.kills>=1,"Native peer death must not be skipped by null killer");if(index!=17)Require(rec.source==(killer?killer:&victim),"Original killer identity or explicit victim source retained");}
  if(index==5)Require(rec.casts==unsigned(killer!=nullptr),"Pure Energy skips only unavailable feedback recipient");
  if(index==6)Require(rec.flags==3*unsigned(eligible),"Dark Rider unlocks its real nearby vehicle for player/pet kills");
  if(index==7)Require(rec.casts==1&&rec.source==(killer?killer:&victim),"Valroth native remains cast survives null attribution");
  if(index==10)Require(rec.talks==1,"Medivh preserves death dialogue without a killer");
  if(index==11)Require(rec.actions==1&&rec.credits==3*unsigned(killer!=nullptr),"Paraxis cleanup cannot turn unattributed death into victory");
  if(index==13)Require(rec.kills==1,"Council cleanup uses native Kill for available princes");
  if(index==14)Require(rec.casts==unsigned(eligible)&&(!eligible||rec.recipient==&player),"Healthy mushroom targets actual eligible owner");
  if(index==15)Require(rec.actions==unsigned(eligible)&&rec.target.value==(eligible?1:0),"Jedoga initiand retains sacrifice cleanup and player owner GUID");
  if(index==18)Require(rec.talks==1&&rec.target.value==(killer?killer->guid.value:0),"Fizzlebang preserves target and independent encounter transition");
  if(index==19)Require(rec.fall==0&&rec.casts==1,"Pyrite falls with native default movement ID,independent of killer Z");
  if(index==21)Require(rec.honor==1&&rec.casts==unsigned(killer!=nullptr),"Ashran nearby honor survives absent strongbox caster");
  if(index==22)Require(rec.despawns==1&&rec.cleanup==unsigned(killer!=nullptr),"Nightmare Beast cleanup is independent of missing killer");
  if(index==23)Require(rec.credits==1&&rec.casts==unsigned(eligible),"Broken Shore instance event survives unavailable visual recipient");
  if(index>=24)Require(rec.credits==unsigned(index==24?3:1),"Vengeance progress is dispatched by the victim's instance");
 }
 // Actor lookup and authored admission remain prerequisites.
 if(index==0||index==6||index==14||index==19){rec={};peer=nullptr;F frame(&victim);frame.instance=frame._instance=&instance;victim.entry=999;frame.JustDied(&guardian);Require(!rec.actions&&!rec.flags&&!rec.casts,"Missing actor/wrong mushroom does not fabricate progress");peer=&other;}
 if(index==0){rec={};F f(&victim);f.JustDied(&other);Require(rec.actions==1,"Legoso's own final blow remains valid");group.member=false;endpoint.starter=77;rec={};f.JustDied(&guardian);Require(!rec.actions,"Unrelated group/starter cannot advance Legoso");endpoint.starter=1;f.JustDied(&guardian);Require(rec.actions==1,"Actual starter guardian remains eligible without group");group.member=true;}
 if(index==5){F f(&victim);owner.alive=false;rec={};f.JustDied(&player);Require(!rec.casts,"Dead Vexallus owner cannot create feedback");owner.alive=true;victim.owner=nullptr;f.JustDied(&player);Require(!rec.casts,"Missing Vexallus owner is safe");victim.owner=&owner;}
 if(index==6){F f(&victim);other.vehicle=false;rec={};f.JustDied(&guardian);Require(!rec.flags,"A nearby non-vehicle cannot be unlocked");other.vehicle=true;}
 if(index==10){F f(&victim);victim.entry=player.entry;rec={};f.JustDied(&player);Require(!rec.talks,"Original same-entry Medivh death suppression retained");}
 if(index==11){F f(&victim);f.instance=&instance;rec={};f.JustDied(&victim);Require(rec.actions==1&&!rec.credits,"Original Paraxis self-kill requests restart only");victim.owner=nullptr;rec={};f.JustDied(nullptr);Require(!rec.actions&&!rec.credits,"Missing Eonar owner cannot invent victory");victim.owner=&owner;}
 if(index==15){F f(&victim);f.instance=&instance;f.bWalking=false;rec={};f.JustDied(&guardian);Require(!rec.actions&&rec.target.value==player.guid.value,"Non-walking initiand retains only original player target handoff");}
 if(index==21){F f(&victim);selected->hostile=false;rec={};f.JustDied(nullptr);Require(!rec.honor&&!rec.casts,"Friendly nearby player cannot receive hostile kill rewards");selected->hostile=true;}
 if(index==23||index>=24){victim.instance=nullptr;F f(&victim);rec={};f.JustDied(nullptr);Require(!rec.credits,"Missing victim instance cannot fabricate scenario progress");victim.instance=&instance;}
 }else{
  for(Unit*clicker:{static_cast<Unit*>(&player),&pet,&guardian,&unowned,static_cast<Unit*>(nullptr)}){
   rec={};F frame(&victim);frame.instance=frame._instance=&instance;SpellInfo spell;bool eligible=clicker==&player;
   if(index==100){frame.OnSpellClick(clicker);Require(rec.credits==unsigned(eligible)&&frame.cooldown==(eligible?5000u:0u),"Player-only click consumes cooldown only on actual player");if(eligible){frame.OnSpellClick(clicker);Require(rec.credits==1,"Click cooldown prevents duplicate credit");}}
   if(index==101){spell.Id=SPELL_UNLOCK_SHACKLE;frame.SpellHit(clicker,&spell);Require(rec.credits==unsigned(eligible),"Unlock requires actual active-quest player");}
   if(index==102){spell.Id=SPELL_GIFT_OF_THE_LIFEBINDER_SCRIPT;if(clicker)clicker->alive=false;frame.SpellHitTarget(clicker,&spell);Require(rec.resurrections==unsigned(eligible),"Life gift resurrects players only");}
   if(index==104){frame.MoveInLineOfSight(clicker);Require(frame.m_player_for_event.size()==unsigned(eligible),"Gizmo private event admits actual player only");if(eligible){frame.MoveInLineOfSight(clicker);Require(frame.m_player_for_event.size()==1,"Gizmo repeat LOS stays one-shot");}}
   if(index==106){victim.entry=119459;frame.OnSpellClick(clicker);Require(rec.credits==unsigned(eligible),"Invasion progress is player-only");rec={};victim.entry=119460;frame.OnSpellClick(clicker);Require(rec.casts==unsigned(clicker!=nullptr),"Other dragon retains native NPC/player boarding cast");}
  }
  F frame(&victim);frame.instance=frame._instance=&instance;SpellInfo spell;
  if(index==101){spell.Id=SPELL_UNLOCK_SHACKLE;player.quest=0;rec={};frame.SpellHit(&player,&spell);Require(!rec.credits&&!rec.despawns,"Inactive shackle quest retains original admission");player.quest=QUEST_STATUS_INCOMPLETE;peer=nullptr;frame.SpellHit(&player,&spell);Require(!rec.credits&&rec.despawns==1,"Missing Rageclaw follows original failure cleanup without credit");peer=&other;}
  if(index==102){spell.Id=SPELL_GIFT_OF_THE_LIFEBINDER_SCRIPT;player.alive=true;rec={};frame.SpellHitTarget(&player,&spell);Require(!rec.resurrections&&!rec.casts,"Living player cannot consume gift resurrection");player.alive=false;frame.powerCount=0;frame.SpellHitTarget(&player,&spell);Require(rec.resurrections==1&&rec.casts==1,"Exhausted gift retains authored Withering Roots cast");}
  if(index==104){player.movie=true;rec={};frame.MoveInLineOfSight(&player);Require(frame.m_player_for_event.empty(),"Movie still playing cannot consume Gizmo intro");player.movie=false;peer=nullptr;frame.MoveInLineOfSight(&player);Require(frame.m_player_for_event.empty(),"Failed Doc summon retains Gizmo retry");peer=&other;frame.MoveInLineOfSight(&player);Require(frame.m_player_for_event.size()==1,"Successful retry consumes Gizmo intro once");}
 }
 std::cout<<"PASS complete production method "<<which<<" original admission/cleanup,player/pet/guardian/charmer/null/unowned; external endpoints controlled\n";
}
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("case index required");uint32 which=uint32(std::stoul(argv[1]));switch(which){CASE_DISPATCH default:throw std::runtime_error("Unknown case");}return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

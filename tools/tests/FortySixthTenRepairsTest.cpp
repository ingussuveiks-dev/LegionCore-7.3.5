// Full extracted production JustDied methods; external instance/quest endpoints
// are controlled. Real owner lookup is checked separately by the native probe.
#include <cstdint>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
struct Player;struct Creature;struct Instance;struct Map;
struct ObjectGuid{uint64 value=0;static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct Record{uint32 credits=0,binds=0,actions=0,hatches=0,doors=0,states=0,clears=0,talks=0,deaths=0,casts=0;Player*recipient=nullptr;uint32 action=0;}rec;
static constexpr uint32 TYPEID_PLAYER=4,TYPEID_UNIT=3,TEAM_ALLIANCE=0,TEAM_HORDE=1,QUEST_STATUS_INCOMPLETE=1;
struct Unit{Player*self=nullptr;Unit*owner=nullptr;Unit*charmer=nullptr;Instance*instance=nullptr;bool aura=true;
 Player*ToPlayer(){return self;}Unit*GetOwner(){return owner;}uint32 GetTypeId(){return self?TYPEID_PLAYER:TYPEID_UNIT;}bool HasAura(uint32){return aura;}Instance*GetInstanceScript(){return instance;}
 Player*GetAffectingPlayer(){Unit*u=this;for(uint32 i=0;u&&i<32;++i){if(u->self)return u->self;u=u->charmer?u->charmer:u->owner;}return nullptr;}};
struct Player:Unit{uint32 team=TEAM_ALLIANCE,quest=QUEST_STATUS_INCOMPLETE;Player(){self=this;}uint32 GetTeamId(){return team;}uint32 GetQuestStatus(uint32){return quest;}
 template<class...T>void UpdateAchievementCriteria(T...){++rec.credits;rec.recipient=this;}void KilledMonsterCredit(uint32,ObjectGuid){++rec.credits;rec.recipient=this;}};
struct Endpoint{void DoAction(uint32 action){++rec.actions;rec.action=action;}void Talk(uint32){++rec.talks;}}endpoint;
struct Aura{ObjectGuid GetCasterGUID(){return {};}};
struct Map{Creature*sibling=nullptr;Map*ToInstanceMap(){return this;}void PermBindAllPlayers(Player*p){++rec.binds;rec.recipient=p;}Creature*GetCreature(ObjectGuid){return sibling;}};
struct Creature:Unit{Map*map=nullptr;uint32 entry=0;bool IsAIEnabled=true,alive=true;Endpoint*AI(){return &endpoint;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}Map*GetMap(){return map;}
 void SetLootRecipient(void*){}Aura*GetAura(uint32){return nullptr;}void CastSpell(Unit*,uint32,bool){++rec.casts;}void GetCreatureListWithEntryInGrid(std::list<Creature*>&list,uint32,float){list.push_back(this);}};
struct Instance{Map*instance=nullptr;bool active=true;ObjectGuid GetGuidData(uint32){return {};}void SendEncounterUnit(uint32,Creature*){}void SetData(uint32,uint32){++rec.states;}bool GetData(uint32){return active;}void SetBossState(uint32,uint32){++rec.states;}void DoRemoveAurasDueToSpellOnPlayers(uint32){++rec.clears;}void HandleGameObject(ObjectGuid,bool){++rec.doors;}};
struct Outdoor{ObjectGuid GetCreature(uint32){return {};}};
Creature*korlok=nullptr;struct Accessor{Creature*FindCreature(ObjectGuid){return korlok;}}accessor;auto sObjectAccessor=&accessor;
uint32 aliveProtectors=0;uint32 ProtectorsAlive(Instance*,Creature*){return aliveProtectors;}
struct Base{Creature*me;Instance*instance=nullptr,*pInstance=nullptr;Outdoor*m_OutdoorPvP=nullptr;bool notSpellHited=true,passengerIn=true;uint32 damaged=3;
 explicit Base(Creature*c):me(c){}virtual~Base()=default;virtual void JustDied(Unit*){}void Talk(uint32){++rec.talks;}void _JustDied(){++rec.deaths;}
 void Hatchiling(std::list<Creature*>&,Unit*u){++rec.hatches;rec.recipient=u->ToPlayer();}};
#include "Selected.inc"
static void Require(bool ok,char const*message){if(!ok)throw std::runtime_error(message);}
static bool nullCounterfactual=false;
template<class F>static void Run(uint32 which)
{
 Map map;Instance instance;instance.instance=&map;Outdoor outdoor;Creature victim,champion;victim.map=&map;victim.instance=&instance;korlok=&champion;
 Player player;player.instance=&instance;Unit pet,guardian,charmed,unowned;pet.owner=&player;guardian.owner=&pet;charmed.charmer=&player;
 auto invoke=[&](Unit*killer,bool eligible=true,bool context=true,uint32 phase=0){
  rec={};aliveProtectors=phase;F frame(&victim);frame.instance=context?&instance:nullptr;frame.pInstance=context?&instance:nullptr;frame.m_OutdoorPvP=context?&outdoor:nullptr;
  frame.notSpellHited=eligible;frame.passengerIn=eligible;frame.damaged=eligible?3:2;player.aura=eligible;player.quest=eligible?QUEST_STATUS_INCOMPLETE:0;instance.active=eligible;victim.instance=context?&instance:nullptr;
  victim.entry=SH_AELMAR;frame.JustDied(killer);
 };
 auto progress=[&](){return which<4?rec.credits:which==4?rec.hatches:which==5?rec.actions:which<9?rec.binds:rec.doors;};
#ifdef COUNTERFACTUAL
 if(which==9||((which==1||which==3)&&nullCounterfactual)){invoke(nullptr);throw std::runtime_error("Expected old null killer failure missing");}
 invoke(&guardian);Require(progress()==0,"Pinned owner-chain counterfactual missing");
 std::cout<<"PASS expected-broken full-method counterfactual:eligible guardian owner loses original progression\n";
#else
 for(uint32 faction:{TEAM_ALLIANCE,TEAM_HORDE}){
  player.team=faction;
  for(Unit*killer:{static_cast<Unit*>(&player),&pet,&guardian,&charmed,&unowned,static_cast<Unit*>(nullptr)}){
   invoke(killer);bool eligible=killer&&killer!=&unowned;uint32 expected=which==9?1:eligible?(which==4?4:1):0;
   Require(progress()==expected,"Eligible player/pet/guardian/charmer/null/unowned endpoint count");
   if(which==3)Require(rec.states==1,"Silver Hand death state transition survives null/unowned killers");
   if(eligible&&which!=5&&which!=9)Require(rec.recipient==&player,"Progression must target the eligible player");
   if(which>=6&&which<9)Require(rec.deaths==1&&rec.clears==F::cleanupAuraCount,"Final protector cleanup independent of killer ownership");
   if(which==5&&eligible)Require(rec.action==(faction==TEAM_ALLIANCE?ActionAllianceRecruit:ActionHordeRecruit),"Recruit actual killer owner's faction");
  }
 }
 if(which<5){invoke(&guardian,false);Require(progress()==0,"Original quest/aura/encounter admission retained");}
 if(which>=3&&which!=4){invoke(&guardian,true,false);Require(progress()==0,"Missing original context is safe");}
 if(which==4){invoke(&guardian,true,false);Require(progress()==4,"Outdoor Siren quest does not require an instance");}
 if(which>=6&&which<9){for(uint32 phase:{1u,2u}){invoke(&guardian,true,true,phase);Require(!rec.binds&&!rec.deaths,"Earlier protector deaths cannot consume the final binding");}}
 if(which==5){korlok=nullptr;invoke(&guardian);Require(!rec.actions,"Missing Korlok is safe");korlok=&champion;champion.IsAIEnabled=false;invoke(&guardian);Require(!rec.actions,"Disabled Korlok AI is safe");}
 std::cout<<"PASS full extracted death method:player/pet/guardian/charmer/null/unowned,both factions,original admission/cleanup and context boundaries;controlled external endpoints\n";
#endif
}
int main(int argc,char**argv){try{if(argc!=2&&argc!=3)throw std::runtime_error("case index required");auto parsed=std::stoul(argv[1]);if(parsed>9)throw std::runtime_error("invalid case");nullCounterfactual=argc==3&&std::string(argv[2])=="null";uint32 which=static_cast<uint32>(parsed);switch(which){CASE_DISPATCH default:throw std::runtime_error("invalid case");}return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

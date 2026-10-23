#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
using uint32=uint32_t; using CriteriaTypes=uint32; using SpellCastResult=int;
constexpr int SMART_ACTION_CAST=11,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205,SMART_EVENT_LINK=61;
constexpr int SMART_TARGET_POSITION=8,SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_SPELL_IN_PROGRESS=2;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#define ENSURE_AI(type,ai) (ai)
#include "CastFlags.inc"
struct Player;
struct AI {void SetCombatMove(bool){};};
struct Unit {
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}
 Player* owner=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return owner;}
 bool castWorks=true;AI ai;int casts=0;bool IsUnit(){return true;}bool HasAura(uint32){return false;}
 AI* AI(){return &ai;}void InterruptNonMeleeSpells(bool){}void GetPosition(float& x,float& y,float& z){x=y=z=0;}
 SpellCastResult CastSpell(Unit*,uint32 spell,bool){assert(spell==210494||spell==187207);++casts;return castWorks?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 SpellCastResult CastSpell(float,float,float,uint32,bool){return SPELL_FAILED_BAD_TARGETS;}
};
struct Player:Unit {int credits=0;Player* ToPlayer()override{return this;}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit*,bool){assert(type==92&&asset==49879);++credits;}
};
using WorldObject=Unit;
#include "CreditPlayer.inc"
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
 uint32 entryOrGuid=0,event_id=0,link=0,event=0,flags=0,type=0,targetType=0,t1=0;
 struct {struct {uint32 spell=0,flags=0;}cast;struct {uint32 type=0,misc1=0,misc2=0,misc3=0;}achievementCriteria;}action;
 struct {float x=0,y=0,z=0;}target;
 uint32 GetActionType(){return type;}uint32 GetEventType(){return event;}uint32 GetScriptType(){return 0;}uint32 GetTargetType(){return targetType;}
};
struct GO {SpellCastResult CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};
struct SmartScript {
 Unit* me;Player* participant;Unit* ghost;GO* go=nullptr;std::map<uint32,SmartScriptHolder> rows;
 int lists=0,paths=0,follows=0,data=0,flags=0,retries=0;bool playerNearby=true;
 Unit* GetBaseObject(){return me;}bool IsUnit(Unit* u){return u!=nullptr;}
 // Spatial selection is a fixture boundary. CAST, credit ownership and LINK
 // dispatch below are extracted from production, not a second implementation.
 ObjectList* GetTargets(SmartScriptHolder& e,Unit* invoker){
  auto selected=new ObjectList;
  if(e.targetType==1)selected->push_back(me);
  else if(e.targetType==7&&invoker)selected->push_back(invoker);
  else if(e.targetType==21&&playerNearby)selected->push_back(participant);
  else if(e.targetType==19&&ghost)selected->push_back(ghost);
  return selected;
 }
 void RecalcTimer(SmartScriptHolder&,uint32 a,uint32 b){assert(a==500&&b==500);++retries;}
 SmartScriptHolder FindLinkedEvent(uint32 id){assert(rows.count(id));return rows.at(id);}
 void ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32,uint32,bool,void*,void*){ProcessAction(e,unit);}
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr){
  uint32 var0=0,var1=0;bool bvar=false;void* spell=nullptr;void* gob=nullptr;
  switch(e.type){
#include "CastAction.inc"
#include "CriteriaAction.inc"
   // These counters check ordering only, not native motion/timed-list execution.
   case 80:++lists;break;case 53:++paths;break;case 29:++follows;break;
   case 45:++data;break;case 81:++flags;break;
   default:assert(false);
  }
#include "LinkTail.inc"
};
using Rows=std::map<uint32,std::map<uint32,SmartScriptHolder>>;
int main(int argc,char** argv){
 assert(argc==2);std::ifstream file(argv[1]);assert(file);std::map<std::string,Rows> versions;std::string line;
 while(std::getline(file,line)){
  std::istringstream in(line);std::string version;SmartScriptHolder e;uint32 a1=0,a2=0;
  if(!(in>>version>>e.entryOrGuid>>e.event_id>>e.link>>e.event>>e.flags>>e.type>>a1>>a2>>e.targetType>>e.t1))continue;
  e.action.cast.spell=a1;e.action.cast.flags=a2;e.action.achievementCriteria.type=a1;e.action.achievementCriteria.misc1=a2;
  versions[version][e.entryOrGuid][e.event_id]=e;
 }
 Player player;Unit actor,ghost,pet;pet.owner=&player;
 for(auto version: {std::string("BEFORE"),std::string("AFTER")})for(bool castWorks:{false,true})for(bool ghostPresent:{false,true}){
  actor.castWorks=castWorks;player.credits=0;
  SmartScript grif{&actor,&player,ghostPresent?&ghost:nullptr};grif.rows=versions.at(version).at(104904);
  grif.ProcessAction(grif.rows.at(0),&player);
  assert(grif.lists==int(castWorks||version=="AFTER"));
  assert(grif.paths==grif.lists&&grif.flags==grif.lists);
  for(Unit* invoker:{static_cast<Unit*>(&player),&pet,static_cast<Unit*>(nullptr)}){
   player.credits=0;SmartScript prustaga{&actor,&player,ghostPresent?&ghost:nullptr};prustaga.rows=versions.at(version).at(104949);
   prustaga.ProcessAction(prustaga.rows.at(1),invoker);
   bool completed=version=="AFTER"||castWorks||!ghostPresent;
   assert(player.credits==int(completed));assert(prustaga.follows==int(completed)&&prustaga.data==int(completed));
  }
 }
 // A missing player is never replaced by pet/NPC credit.
 player.credits=0;SmartScript absent{&actor,&player,&ghost};absent.rows=versions.at("AFTER").at(104949);absent.playerNearby=false;
 absent.ProcessAction(absent.rows.at(1),&pet);assert(player.credits==0);
 std::cout<<"PASS: actual SQL rows through native CAST/criterion205/LINK; both old failure paths reproduced, failed/missing optional casts cannot block new introduction or meeting; player/pet/null invokers and absent player. Movement, timing and one-shot event admission remain integration boundaries.\n";
}

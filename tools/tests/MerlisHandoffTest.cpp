// Complete production handoff helpers/death/Reset/summon hooks under ASAN.
// Native map, AI dispatch and owner-credit services controlled; real native
// source/receipt/factory/core update coverage is separate.
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;using uint64=uint64_t;
enum{CRITERIA_TYPE_SCRIPT_EVENT_2=92};enum class HighGuid{Conversation};
struct ObjectGuid{uint64 lo=0,hi=0;ObjectGuid(uint64 v=0):lo(v){}void Clear(){lo=hi=0;}bool IsEmpty()const{return !lo&&!hi;}bool operator==(ObjectGuid const& r)const{return lo==r.lo&&hi==r.hi;}bool operator!=(ObjectGuid const& r)const{return !(*this==r);}bool operator<(ObjectGuid const& r)const{return hi<r.hi||(hi==r.hi&&lo<r.lo);}};
struct Creature;struct Player;
struct Unit{Player* owner=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return owner;}};
struct Player:Unit{unsigned credits=0;Player(){owner=this;}void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92&&id==51615);++credits;}};
struct Map{uint32 instance=1;uint32 GetId()const{return 1620;}uint32 GetInstanceId()const{return instance;}Creature* GetCreature(ObjectGuid);};
struct Maps{std::map<uint32,Map*> values;Map* FindMap(uint32 id,uint32 instance){assert(id==1620);auto p=values.find(instance);return p==values.end()?nullptr:p->second;}}mapManager;auto sMapMgr=&mapManager;
struct CreatureAI{virtual ~CreatureAI()=default;virtual void DoAction(int32_t){}virtual uint32 GetData(uint32)const{return 0;}virtual void Reset(){}virtual void JustDied(Unit*){}virtual void JustSummoned(Creature*){}virtual void UpdateLifecycle(uint32){};};
struct ReceiptAI:CreatureAI{bool introDone=false,accept=true;unsigned actions=0;std::function<void()> hook;void DoAction(int32_t value)override{assert(value==1);++actions;if(hook)hook();if(accept)introDone=true;}
#include "GaronaReceipt.inc"
};
static std::map<ObjectGuid,Creature*> actors;static std::vector<std::unique_ptr<Creature>> ownedActors;
struct Creature:Unit{ObjectGuid guid,summoner;bool summoned=false;uint32 entry=108222,mapid=1620,instance=1,scope=1;Map* map=nullptr;bool world=true,pre=false,alive=true,expired=false,phase=true,IsAIEnabled=true;CreatureAI* ai=nullptr;Creature* previous=nullptr;std::list<Creature*> adds;std::function<Creature*()> summon;unsigned despawns=0,zoneCalls=0;bool visible=false;ObjectGuid GetGUID()const{return guid;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return mapid;}uint32 GetInstanceId()const{return instance;}Map* GetMap()const{return map;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return pre;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool InSamePhase(Creature const* other)const{return phase==other->phase;}bool CanTargetQuestObject(Creature const* other)const{return scope==other->scope;}Creature* ToTempSummon(){return summoned?this:nullptr;}ObjectGuid GetSummonerGUID()const{return summoner;}CreatureAI* AI(){return ai;}Creature* FindNearestCreature(uint32 id,float range,bool yes){assert(id==108222&&range==60&&yes);return previous;}Creature* SummonCreature(uint32 id,float x,float y,float z){assert(id==108222&&x==-8310.60f&&y==284.40f&&z==156.83f);return summon?summon():nullptr;}void SetVisible(bool value){visible=value;}void DespawnOrUnsummon(){++despawns;alive=false;}};
Creature* Map::GetCreature(ObjectGuid guid){auto p=actors.find(guid);return p!=actors.end()&&p->second->map==this?p->second:nullptr;}
namespace ObjectAccessor{Creature* GetCreature(Creature const&,ObjectGuid guid){auto found=actors.find(guid);return found==actors.end()?nullptr:found->second;}}
struct ScriptedAI:CreatureAI{Creature* me;explicit ScriptedAI(Creature* p):me(p){}void DoZoneInCombat(Creature* actor,float range){assert(range==150);++actor->zoneCalls;}};
struct EventMap{void Reset(){}};
struct SummonList{std::set<Creature*> list;void Summon(Creature* p){list.insert(p);}void DespawnAll(){auto snapshot=list;list.clear();for(auto p:snapshot)p->DespawnOrUnsummon();}};
static unsigned warnings=0,scenes=0;
#define TC_LOG_WARN(...) (++warnings)
struct Generator{uint64 Generate(){return 1;}};struct Mgr{template<HighGuid>Generator* GetGenerator(){static Generator result;return &result;}}manager;auto sObjectMgr=&manager;
struct Conversation{bool CreateConversation(uint64,uint32 id,Player* p,void*,Player& anchor){assert(id==2448&&p==&anchor);++scenes;return false;}};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& rows,Creature* source,uint32 id,float range){assert(id==108397&&range==70);rows=source->adds;}
struct Merlis:ScriptedAI{using ScriptedAI::ScriptedAI;SummonList summons;EventMap events;
#include "Handoff.inc"
#include "Reset.inc"
#include "Summoned.inc"
#include "Death.inc"
};
int main(){unsigned cases=0;
 for(unsigned killerKind=0;killerKind<4;++killerKind)for(unsigned mode=0;mode<31;++mode){
  actors.clear();ownedActors.clear();warnings=scenes=0;Map map,otherMap;otherMap.instance=2;mapManager.values={{1,&map},{2,&otherMap}};Creature boss,old,add,extra;boss.guid=1;boss.entry=107831;boss.map=old.map=add.map=extra.map=&map;old.guid=2;add.guid=3;actors[boss.guid]=&boss;actors[old.guid]=&old;boss.previous=mode==1?nullptr:&old;boss.adds={&extra};Merlis ai(&boss);boss.ai=&ai;Player owner;Unit proxy;proxy.owner=killerKind<3?&owner:nullptr;Unit* killer=killerKind==0?static_cast<Unit*>(&owner):killerKind<3?&proxy:nullptr;
  ai.JustSummoned(&add);assert(add.zoneCalls==1);boss.alive=false;ReceiptAI receipt;unsigned attempts=0;bool fail=mode>=2&&(mode<=14||mode==22||mode==23);bool retired=false;
  boss.summon=[&]()->Creature*{++attempts;if(fail)return nullptr;auto actor=std::make_unique<Creature>();actor->guid=100+attempts;actor->summoned=true;actor->summoner=boss.guid;actor->map=&map;actor->ai=&receipt;actor->scope=boss.scope;actor->phase=boss.phase;if(mode==15||(mode>=24&&mode<=29))actor->IsAIEnabled=false;if(mode==16)actor->ai=nullptr;auto result=actor.get();actors[result->guid]=result;ownedActors.push_back(std::move(actor));ai.JustSummoned(result);if(mode==17){ai.StartOutro();ai.UpdateLifecycle(1000);ai.Reset();}return result;};
  if(mode==18)receipt.accept=false;
  if(mode==19)receipt.hook=[&]{ai.StartOutro();ai.UpdateLifecycle(1000);ai.Reset();};
  if(mode==30)receipt.hook=[&]{boss.alive=true;};
  if(mode==20)old.phase=false;if(mode==21)old.scope=2;
  ai.JustDied(killer);assert(add.despawns==1&&extra.despawns==1&&owner.credits==unsigned(killerKind<3)&&scenes==unsigned(killerKind<3));
  if(mode>=24&&mode<=29){
   assert(attempts==1&&old.despawns==0&&ownedActors.size()==1);
   if(mode==24)boss.alive=true;if(mode==25)boss.world=false;if(mode==26)boss.pre=true;if(mode==27)boss.expired=true;if(mode==28){boss.map=&otherMap;boss.instance=2;}if(mode==29)boss.guid.hi=99;
   ai.UpdateLifecycle(1);assert(!ai.outroPending&&!ai.outroComplete&&ownedActors.front()->despawns==1&&old.despawns==0);++cases;continue;
  }
  if(mode==30){assert(ai.outroComplete&&!ai.outroPending&&ownedActors.size()==1&&ownedActors.front()->alive&&old.despawns==0);ai.Reset();ai.StartOutro();assert(attempts==1);++cases;continue;}
  if(fail){assert(old.despawns==0&&attempts==1&&!ai.outroComplete);ai.Reset();ai.UpdateLifecycle(1999);assert(attempts==1);ai.UpdateLifecycle(1);assert(attempts==2&&warnings==1);
   if(mode==3)old.phase=false;if(mode==4)old.scope=2;if(mode==5){actors.erase(old.guid);retired=true;}if(mode==6){actors.erase(old.guid);old.guid.hi=99;actors[old.guid]=&old;}
   if(mode==7)boss.alive=true;if(mode==8)boss.world=false;if(mode==9)boss.pre=true;if(mode==10)boss.expired=true;if(mode==11)boss.mapid=1220;if(mode==12)boss.entry=108222;
   if(mode==22)boss.instance=2;if(mode==23)boss.guid.hi=99;
   if((mode>=7&&mode<=12)||mode>=22){ai.UpdateLifecycle(2000);assert(!ai.outroPending&&attempts==2&&old.despawns==0);++cases;continue;}
   fail=false;ai.UpdateLifecycle(2000);
  }
  if(mode==15||mode==16){assert(attempts==1&&old.despawns==0);ownedActors.front()->IsAIEnabled=true;ownedActors.front()->ai=&receipt;ai.UpdateLifecycle(2000);assert(attempts==1);}
  if(mode==18){assert(attempts==1&&old.despawns==0&&warnings==1);receipt.accept=true;ai.UpdateLifecycle(2000);assert(attempts==1);}
  assert(ai.outroComplete&&!ai.outroPending&&ownedActors.size()==1&&ownedActors.front()->zoneCalls==0&&ownedActors.front()->visible);
  bool keepOld=mode==1||mode==3||mode==4||mode==5||mode==6||mode==20||mode==21;assert(old.despawns==unsigned(!keepOld));(void)retired;
  if(mode==13){auto retiredGuid=ownedActors.front()->guid;actors.erase(retiredGuid);ownedActors.clear();} // Accepted reveal lifetime belongs to the actor/map, never recreate a consumed handoff.
  ai.Reset();ai.StartOutro();for(unsigned i=0;i<60;++i)ai.UpdateLifecycle(1000);assert(owner.credits==unsigned(killerKind<3)&&scenes==unsigned(killerKind<3)&&ownedActors.size()==unsigned(mode!=13));++cases;
 }
 actors.clear();ownedActors.clear();std::cout<<"PASS "<<cases<<" complete production handoff/death/Reset/summon hook ASAN contexts: refused factory, old full-GUID/phase/scope cleanup, accepted/rejected/absent AI receipt, reentrant callbacks, corpse cancellation/retry, accepted/consumed actor, owner/proxy/null unchanged single progress. Native engine/client separate\n";
}

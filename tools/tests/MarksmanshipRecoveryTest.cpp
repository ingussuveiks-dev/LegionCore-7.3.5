// Extracted production ownership, context, controller, condition and return
// state; actor cast/native transfer endpoints controlled. No character writes.
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <list>
using uint32=uint32_t;using uint8=uint8_t;using ObjectGuid=uint64_t;
constexpr int CLASS_HUNTER=3,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SMART_EVENT_OOC_LOS=10;
enum SpellCastResult {SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=12};using SpellEffIndex=int;
unsigned warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Player;
struct WorldObject {virtual Player* ToPlayer(){return nullptr;}virtual ~WorldObject()=default;};
struct Creature;
struct Unit:WorldObject {virtual Creature* ToCreature(){return nullptr;}bool IsCreature(){return ToCreature()!=nullptr;}virtual uint32 GetEntry(){return 0;}virtual uint32 GetMapId()const{return 0;}virtual uint32 GetInstanceId(){return 0;}virtual bool IsInWorld(){return false;}virtual bool IsAlive(){return false;}};
struct Item {ObjectGuid owner=1;uint32 count=1;ObjectGuid GetOwnerGUID()const{return owner;}uint32 GetCount()const{return count;}};
struct QuestStatusData {std::shared_ptr<uint8> ScriptLifetime;};
struct Session {bool loading=false;bool PlayerLoading(){return loading;}};
struct SmartScript {bool consumed=false;bool HasRunEvent(uint32 id,int type){assert(id==0&&type==10);return consumed;}};
struct SmartAI {SmartScript script;SmartScript*GetScript(){return &script;}};
struct Creature:Unit {
 uint32 entry=100397,db=370758,scriptId=0,map=1489,instance=42,casts=0;bool world=true,alive=true,phase=true,range=true,combat=false,busy=false,IsAIEnabled=true;
 struct {std::string AIName="SmartAI";} info;SmartAI ai;SpellCastResult result=SPELL_CAST_OK;std::function<void()> callback;
 Creature*ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return db;}uint32 GetScriptId(){return scriptId;}auto GetCreatureTemplate(){return &info;}
 bool IsPreDelete(){return false;}
 SmartAI*AI(){return &ai;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetMapId()const{return map;}uint32 GetInstanceId(){return instance;}
 bool InSamePhase(Player*){return phase;}bool IsWithinDistInMap(Player*,float d){assert(d==20);return range;}bool isInCombat(){return combat;}bool IsNonMeleeSpellCast(bool){return busy;}
 SpellCastResult CastSpell(Player*,uint32 id,bool triggered){assert(id==205040&&!triggered);++casts;if(callback)callback();return result;}
};
struct Player:Unit {
 uint32 map=1489,cls=3,instance=42;int status=3;bool gm=false,rewarded=false,world=true,alive=true,pending=false,transport=false,vehicle=false,combat=false,haveSession=true,haveItem=true;
 ObjectGuid guid=1;Item artifact;Session session;QuestStatusData quest;Creature* actor=nullptr;std::map<uint32,int> goals{{102128,1},{103481,1},{102129,1},{102131,1},{103490,1},{102548,0}};
 unsigned removes=0;bool canDelay=false;
 Player*ToPlayer()override{return this;}uint32 GetMapId()const{return map;}uint32 getClass()const{return cls;}bool isGameMaster()const{return gm;}
 bool GetQuestRewardStatus(uint32 id)const{assert(id==40419);return rewarded;}int GetQuestStatus(uint32 id)const{assert(id==40419);return status;}
 int GetQuestObjectiveData(uint32 id,uint32 goal)const{assert(id==40419);auto p=goals.find(goal);return p==goals.end()?0:p->second;}
 Item* GetItemByEntry(uint32 id,bool bank)const{assert(id==128826&&bank);return haveItem?const_cast<Item*>(&artifact):nullptr;}
 ObjectGuid GetGUID()const{return guid;}uint32 GetGUIDLow(){return uint32(guid);}bool HasEarnedMarksmanshipReturn()const;
 bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetInstanceId(){return instance;}bool IsBeingTeleported(){return pending;}bool IsBeingTeleportedFar(){return pending;}
 Session* GetSession(){return haveSession?&session:nullptr;}void*GetTransport(){return transport?this:nullptr;}void*GetVehicle(){return vehicle?this:nullptr;}bool isInCombat(){return combat;}
 QuestStatusData*getQuestStatus(uint32 id){assert(id==40419);return &quest;}void AddQuestDelayedEvent(uint32 id,uint32 delay,std::function<void()>){assert(id==40419&&!delay);if(!quest.ScriptLifetime)quest.ScriptLifetime=std::make_shared<uint8>();}
 Creature*decoy=nullptr;void GetCreatureListWithEntryInGrid(std::list<Creature*>&out,uint32 id,float radius){assert(id==100397&&radius==50);if(decoy)out.push_back(decoy);if(actor)out.push_back(actor);}
 bool InSamePhase(Unit* u){return static_cast<Creature*>(u)->phase;}bool HasItemCount(uint32 id,uint32 count,bool bank){assert(id==128826&&count==1&&bank);return haveItem;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==200279);++removes;}
 struct ScopedTeleportDelay {Player&p;bool old;explicit ScopedTeleportDelay(Player&v):p(v),old(v.canDelay){p.canDelay=true;}~ScopedTeleportDelay(){p.canDelay=old;}bool IsPending(){return p.pending;}};
};
struct Scenario {uint32 id=972,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {Scenario value;bool present=true;Scenario*GetScenario(uint32){return present?&value:nullptr;}} manager;auto sScenarioMgr=&manager;
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual~PlayerScript()=default;};
struct Condition {};struct ConditionSourceInfo {WorldObject*mConditionTargets[3]{};};struct ConditionScript {explicit ConditionScript(char const*){}virtual bool OnConditionCheck(Condition*,ConditionSourceInfo&){return true;}};
namespace ArtifactAcquisition {bool Active(Player*p,uint32 q){return !p->GetQuestRewardStatus(q)&&(p->GetQuestStatus(q)==3||p->GetQuestStatus(q)==1);}}
struct Spell {Player*p;bool accepted=true;unsigned calls=0;void EffectTeleportUnits(int){assert(p->canDelay);++calls;if(accepted)p->pending=true;}};
#include "Earned.inc"
#include "Recovery.inc"
#include "Return.inc"
int main(){unsigned cases=0;
 for(int status:{3,1})for(unsigned mode=0;mode<29;++mode){manager={};Player p;Creature actor;p.actor=&actor;p.status=status;
  if(mode<5)p.goals[std::vector<uint32>{102128,103481,102129,102131,103490}[mode]]=0;
  if(mode==5)p.haveItem=false;if(mode==6)p.artifact.owner=2;if(mode==7)p.artifact.count=0;if(mode==8)p.status=0;if(mode==9)p.rewarded=true;if(mode==10)p.gm=true;if(mode==11)p.cls=1;if(mode==12)p.map=1220;
  bool earned=mode>=13;assert(p.HasEarnedMarksmanshipReturn()==earned);
  if(mode==13)p.world=false;if(mode==14)p.alive=false;if(mode==15)manager.present=false;if(mode==16)manager.value.id=973;if(mode==17)manager.value.step=3;if(mode==18)actor.db=1;if(mode==19)actor.scriptId=1;if(mode==20)actor.info.AIName="custom";if(mode==21)actor.IsAIEnabled=false;if(mode==22)actor.world=false;if(mode==23)actor.alive=false;if(mode==24)actor.map=1220;if(mode==25)actor.instance=43;if(mode==26)actor.phase=false;if(mode==27)actor.range=false;if(mode==28)actor.ai.script.consumed=true;
  assert(!MarksmanshipRecovery::Ready(&p,&actor));condition_marksmanship_recovered_arrival gate;ConditionSourceInfo source;source.mConditionTargets[0]=&p;assert(gate.OnConditionCheck(nullptr,source)==!earned);++cases;
 }
 for(int status:{3,1})for(bool accepted:{false,true}){manager={};Player p;Creature actor;p.actor=&actor;p.status=status;assert(p.HasEarnedMarksmanshipReturn()&&MarksmanshipRecovery::Ready(&p,&actor));auto before=p.goals;player_marksmanship_return_recovery controller;controller.OnUpdate(&p,999);assert(!actor.casts);controller.OnUpdate(&p,1);assert(actor.casts==1&&!p.pending&&p.goals==before);actor.busy=true;controller.OnUpdate(&p,5000);assert(actor.casts==1);actor.busy=false;
  Spell spell{&p,accepted};MarksmanshipReturn::State state;state.Teleport(&p,&actor,&spell,0);assert(spell.calls==1&&p.pending==accepted&&p.removes==unsigned(accepted)&&state.CreditAllowed(&p,&actor)==accepted&&p.goals==before&&!p.canDelay);++cases;
 }
 for(bool missing:{false,true}){manager={};Player p;Creature actor;p.actor=missing?nullptr:&actor;actor.result=SPELL_FAILED_BAD_TARGETS;auto count=warnings;player_marksmanship_return_recovery controller;for(unsigned i=0;i<30;++i)controller.OnUpdate(&p,5000);assert(warnings==count+1&&actor.casts==(missing?0u:12u)&&!p.pending&&!p.removes&&p.goals[102548]==0);p.session.loading=true;controller.OnUpdate(&p,5000);p.session.loading=false;p.actor=&actor;actor.result=SPELL_CAST_OK;auto casts=actor.casts;controller.OnUpdate(&p,1000);assert(actor.casts==casts+1);++cases;}
 {manager={};Player p;Creature actor;p.actor=&actor;player_marksmanship_return_recovery controller;actor.callback=[&]{controller.OnLogout(&p);p.goals[103490]=0;p.quest.ScriptLifetime.reset();};controller.OnUpdate(&p,1000);controller.OnUpdate(&p,5000);assert(actor.casts==1&&!p.pending&&p.goals[102548]==0);++cases;}
 {manager={};Player p;Creature actor;p.actor=&actor;player_marksmanship_return_recovery controller;actor.ai.script.consumed=true;for(unsigned i=0;i<30;++i)controller.OnUpdate(&p,5000);assert(!actor.casts);actor.ai.script.consumed=false;controller.OnUpdate(&p,5000);assert(actor.casts==1);++cases;}
 {manager={};Player p;Creature actor,decoy;p.actor=&actor;p.decoy=&decoy;decoy.db=0;player_marksmanship_return_recovery controller;controller.OnUpdate(&p,1000);assert(actor.casts==1&&!decoy.casts);++cases;}
 assert(cases==67);std::cout<<"PASS recovery67 ASAN:actual earned/context/condition/controller/return state;incomplete+complete,all five receipts and ownership refusals,real-caster/nearer-decoy guards,normal cast request,busy/consumed intro deferral,accepted transfer-only cleanup/credit eligibility,finite retries/reentry/reentrant logout;actor cast/transfer endpoints controlled,no direct item/quest/return grant\n";
}

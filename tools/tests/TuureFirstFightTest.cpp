#include <algorithm>
#include "NativeHost.inc"
enum {UNIT_FIELD_FLAGS=2,UNIT_FLAG_NOT_SELECTABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_FLAG_NOT_ATTACKABLE_1=8};
inline uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
Creature* Map::GetCreature(ObjectGuid const& guid){for(auto c:actors)if(c->guid==guid&&c->map==this)return c;return nullptr;}
Creature* Unit::FindNearestCreature(uint32 entry,float range,bool living){
 assert(range==40||range==100);if(!map)return nullptr;
 for(auto c:map->actors)if(c->entry==entry&&c->world&&c->alive==living&&InSamePhase(c))return c;return nullptr;
}
// Only the exact native GM/contact guard is used here. Criteria application and
// step advance below are adapters, checked against the typed catalog separately.
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){
 assert(type==92&&(asset==50562||asset==50563));++attempts;
 struct Cache{Player* player;};Cache value{this};Cache* cachePtr=&value;
#include "NativeCreditGate.inc"
 if(rejectCredit)return;
 if(asset==50562&&scenario.step==1){++firstCredits;++credits;scenario.achievement.progress[47519].Counter++;scenario.step=2;}
 else if(asset==50563&&scenario.step==0){++healCredits;++credits;scenario.achievement.progress[47498].Counter++;scenario.step=1;}
 if(onCredit)onCredit();
}
#include "Policy.inc"
#include "Instance.inc"
void Unit::Kill(Creature* creature){
 if(!creature||!creature->alive)return;
 creature->alive=false;creature->health=0;
 if(auto script=creature->GetInstanceScript())script->OnUnitDeath(creature);
 // Actual Unit::Kill -> instance dispatch, after the modeled death endpoint.
#include "NativeKillBridge.inc"
}
struct EventMap{void Reset(){}};
#include "Boros.inc"
struct EvadeBoros:CreatureAI{
 Creature* me;bool firstfighting=false;FunctionProcessor _functions,_entryFunctions;uint64 _entryGeneration=0;
 explicit EvadeBoros(Creature* c):me(c){c->primary=this;}
#include "Evade.inc"
};
struct Run{
 Map map,other;Player player,observer;Unit pet,guardian,charmed,unknown;
 std::unique_ptr<Creature> boros,foe,target;Creature jace,mage;
 instance_tuure_InstanceMapScript script;
 Run():script(&map){
  warnings=0;scenario={};scenario.step=1;manager.active=&scenario;map.script=&script;
  player.map=observer.map=pet.map=guardian.map=charmed.map=unknown.map=&map;observer.guid={1,2};observer.classId=4;
  pet.owner=guardian.owner=player.guid;charmed.charmer=player.guid;ObjectAccessor::players={&player,&observer};map.players={{&observer},{&player}};
  Make(boros,106134,361419,10);Make(foe,106201,361428,20);Make(target,106221,361429,30);
  jace.map=mage.map=&map;jace.entry=106137;mage.entry=106253;jace.guid={2,40};mage.guid={2,50};map.actors.push_back(&jace);map.actors.push_back(&mage);script.OnCreatureCreate(&jace);script.OnCreatureCreate(&mage);
 }
 void Make(std::unique_ptr<Creature>& c,uint32 entry,uint64 db,uint64 low){c=std::make_unique<Creature>();c->map=&map;c->entry=entry;c->dbGuid=db;c->guid={2,low};c->health=1000;map.actors.push_back(c.get());script.OnCreatureCreate(c.get());}
 void Remove(std::unique_ptr<Creature>& c){map.actors.erase(std::remove(map.actors.begin(),map.actors.end(),c.get()),map.actors.end());c.reset();}
 void Kill(Unit& killer){killer.Kill(foe.get());}
};
int main(){
#ifdef BASELINE
 {Run r;EvadeBoros ai(r.boros.get());ai.firstfighting=true;r.boros->nearest=&r.player;ai.EnterEvadeMode();ai._functions.Update(1000);assert(r.foe->alive&&r.player.firstCredits==1&&r.boros->conversations==1);}
 {Run r;EvadeBoros ai(r.boros.get());r.Kill(r.pet);ai.EnterEvadeMode();ai._functions.Update(1000);r.script.Update(20000);assert(!r.foe->alive&&!r.player.firstCredits&&!r.boros->conversations&&r.target->flags==0x800f);}
 {Run r;EvadeBoros ai(r.boros.get());ai.firstfighting=true;r.boros->nearest=&r.player;ai.EnterEvadeMode();ai._functions.Update(1000);assert(r.player.firstCredits==1);++ai._entryGeneration;ai._entryFunctions.Update(16000);assert(r.target->flags==0x800f);}
 std::cout<<"PASS labelled8c946eb5 full-evade/instance counterfactual: evade awards50562 while opponent alive; real early pet kill before Boros latch is lost; AI Reset cancels already-earned16s continuation.\n";
#else
 uint32 variants=0;
 for(uint32 status:{1u,3u})for(bool ownerAlive:{false,true})for(int killer=0;killer<7;++killer){
  Run r;r.player.quest.Status=status;r.player.alive=ownerAlive;Unit* attackers[]={&r.player,&r.pet,&r.guardian,&r.charmed,r.boros.get(),&r.jace,&r.mage};
  assert(r.script.creates==5);r.Kill(*attackers[killer]);r.script.CreatureDies(r.foe.get(),attackers[killer]);
  assert(r.script.firstFight==decltype(r.script)::FirstFight::Pending&&!r.player.firstCredits);
  r.script.Update(999);assert(!r.player.firstCredits&&!r.boros->conversations);r.script.Update(1);
  assert(r.player.firstCredits==1&&!r.observer.firstCredits&&scenario.step==2&&r.boros->conversations==1);
  r.script.Update(15999);assert(r.target->flags==0x800f);r.target->onRemove=[&]{r.script.Update(1000);};r.script.Update(1);
  assert(r.target->flags==0x8000&&r.target->flagRemovals==1);r.script.Update(20000);assert(r.player.firstCredits==1&&r.target->flagRemovals==1);++variants;
 }
 {Run r;EvadeBoros ai(r.boros.get());ai.firstfighting=true;ai.EnterEvadeMode();ai._functions.Update(1000);r.script.Update(20000);assert(r.foe->alive&&!r.player.firstCredits&&!r.boros->conversations);}
 {Run r;r.foe->alive=false;r.script.OnUnitDeath(r.foe.get());r.script.Update(20000);assert(!r.player.firstCredits&&r.script.firstFight==decltype(r.script)::FirstFight::Waiting);}
 {Run r;r.script.CreatureDies(r.foe.get(),&r.player);r.script.Update(20000);assert(!r.player.firstCredits);}
 for(int invalid=0;invalid<21;++invalid){Run r;Unit* killer=&r.pet;switch(invalid){
  case 0:r.foe->entry=106221;break;case 1:r.foe->dbGuid=0;break;case 2:r.foe->map=&r.other;break;case 3:scenario.id=972;break;case 4:scenario.id=940;break;case 5:scenario.id=1145;break;case 6:scenario.id=1146;break;case 7:scenario.step=3;break;case 8:scenario.step=2;break;
  case 9:r.player.classId=4;break;case 10:r.player.quest.Status=0;break;case 11:r.player.rewarded=true;break;case 12:r.player.gm=true;break;case 13:r.player.contact=false;break;case 14:r.player.phase=false;break;case 15:r.player.near=false;break;case 16:r.player.pending=true;break;case 17:r.pet.owner={1,99};break;case 18:killer=&r.unknown;break;case 19:killer=nullptr;break;case 20:r.player.record=false;break;}
  r.foe->alive=false;r.script.CreatureDies(r.foe.get(),killer);r.script.Update(30000);assert(!r.player.firstCredits&&!r.boros->conversations&&r.target->flags==0x800f);
 }
 {Run r;r.observer.classId=5;r.Kill(r.jace);r.script.Update(20000);assert(!r.player.firstCredits&&!r.observer.firstCredits);}
 {Run r;r.observer.classId=5;r.observer.gm=true;r.player.alive=false;r.Kill(r.jace);r.script.Update(1000);assert(r.player.firstCredits==1&&!r.observer.firstCredits);}
 {Run r;r.boros->states=UNIT_STATE_STUNNED;r.Kill(r.pet);r.script.Update(20000);assert(!r.player.firstCredits);r.boros->states=0;r.script.Update(1000);assert(r.player.firstCredits==1);}
 {Run r;scenario.step=0;r.boros->health=500;r.Kill(r.jace);r.script.Update(20000);assert(!r.player.firstCredits);Boros ai(r.boros.get());assert(r.player.DealHeal(r.boros.get(),450)==450);r.boros->Tick();assert(r.player.healCredits==1&&scenario.step==1);r.script.Update(999);assert(!r.player.firstCredits);r.script.Update(1);assert(r.player.firstCredits==1);}
 {Run r;r.Kill(r.pet);r.Remove(r.foe);r.script.Update(1000);assert(r.player.firstCredits==1);}
 {Run r;r.Kill(r.pet);r.Remove(r.boros);r.script.Update(20000);assert(!r.player.firstCredits);r.Make(r.boros,106134,361419,11);r.script.Update(1000);assert(r.player.firstCredits==1);}
 {Run r;r.Kill(r.pet);r.player.onCredit=[&]{r.script.Update(1000);r.Remove(r.boros);};r.script.Update(1000);assert(r.player.firstCredits==1&&r.script.firstFight==decltype(r.script)::FirstFight::Unlocking&&warnings==1);r.script.Update(16000);assert(r.target->flags==0x800f);r.Make(r.boros,106134,361419,11);r.script.Update(0);assert(r.target->flags==0x8000);}
 {Run r;r.Kill(r.pet);r.script.Update(1000);Boros ai(r.boros.get());ai.Reset();r.script.Update(16000);assert(r.target->flags==0x8000&&r.player.firstCredits==1);}
 {Run r;r.boros->failConversation=true;r.Kill(r.pet);r.script.Update(1000);assert(r.player.firstCredits==1&&!r.boros->conversations&&r.boros->conversationAttempts==1);r.script.Update(16000);assert(r.target->flags==0x8000);}
 {Run r;r.Kill(r.pet);r.player.rejectCredit=true;r.script.Update(1000);assert(!r.player.firstCredits&&warnings==1&&!r.boros->conversations);r.script.Update(4999);assert(r.player.attempts==1);r.player.rejectCredit=false;r.script.Update(1);assert(r.player.firstCredits==1&&r.player.attempts==2&&r.boros->conversations==1);r.script.Update(16000);assert(r.target->flags==0x8000);}
 {Run r;r.Kill(r.pet);auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.script.Update(1000);assert(!r.player.firstCredits&&warnings==1);r.foe->alive=true;r.script.OnCreatureCreate(r.foe.get());r.Kill(r.pet);r.script.Update(1000);assert(r.player.firstCredits==1&&r.player.quest.ScriptLifetime!=held);}
 {Run r;r.Kill(r.pet);auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.foe->alive=true;r.Kill(r.pet);r.script.Update(1000);assert(r.player.firstCredits==1&&r.player.quest.ScriptLifetime!=held);}
 {Run r;r.Kill(r.pet);r.player.contact=false;r.script.Update(1000);assert(!r.player.firstCredits);r.player.contact=true;r.script.Update(1000);assert(r.player.firstCredits==1);}
 {Run r;r.Kill(r.pet);r.player.gm=true;r.script.Update(1000);assert(!r.player.firstCredits);r.player.gm=false;r.script.Update(1000);assert(r.player.firstCredits==1);}
 {Run r;r.Kill(r.pet);r.foe->alive=true;r.script.Update(1000);assert(!r.player.firstCredits&&r.script.firstFight==decltype(r.script)::FirstFight::Waiting);}
 {Run r;r.Kill(r.pet);r.script.Update(1000);r.Remove(r.target);r.script.Update(16000);r.Make(r.target,106221,361429,31);r.boros->routeNear=false;r.script.Update(0);assert(r.target->flags==0x800f);r.boros->routeNear=true;r.script.Update(0);assert(r.target->flags==0x8000);}
 {Run r;scenario.step=2;scenario.achievement.progress[47519].Counter=1;r.Kill(r.pet);r.script.Update(1000);assert(!r.player.firstCredits&&!r.player.attempts&&r.boros->conversations==1);r.script.Update(16000);assert(r.target->flags==0x8000);}
 {Run r;r.Kill(r.pet);scenario.step=2;scenario.achievement.progress[47519].Counter=1;r.player.quest={};r.script.Update(1000);assert(!r.player.attempts&&r.boros->conversations==1);r.script.Update(16000);assert(r.target->flags==0x8000);}
 {Run r;r.Kill(r.pet);r.script.Update(1000);r.target->entry=1;r.script.Update(16000);assert(r.target->flags==0x800f);r.target->entry=106221;r.script.Update(0);assert(r.target->flags==0x8000);}
 {Run r;r.Kill(r.pet);r.script.Update(1000);r.Remove(r.target);Creature foreign;foreign.map=&r.map;foreign.entry=106221;foreign.dbGuid=361429;foreign.guid={3,30};r.map.actors.push_back(&foreign);r.script.Update(16000);assert(foreign.flags==0x800f);}
 std::cout<<"PASS actual full instance/first-fight policy, actual native Kill-instance bridge/owner/quest-lifetime/FunctionProcessor/heal application and initial Boros heal lifecycle: "<<variants<<" live/dead-owner/status/player-pet-guardian-charmer-three-ally variants; no evade/state-only/spawn victory, wrong shared scenarios/metadata/owners, early kill before arrival/heal,1s/16s/native-rejected5s retry, exact accepted counter, corpse/freed-replaced actor/AI/reset/missing actor/range/fullGUID/held-lease retake/reentrant/optional conversation paths. Criteria application, health/death/grid/motion endpoints are controlled adapters; actual client route remains pending.\n";
#endif
}

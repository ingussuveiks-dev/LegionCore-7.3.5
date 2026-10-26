#include "IntroHost.inc"
enum {TYPEID_PLAYER=4};
// Place actual policy at namespace scope, actual complete LOS override in AI.
#include "EntryPolicy.inc"
struct Greeting:CreatureAI {
 Creature* me;bool _introDone=false;FunctionProcessor _entryFunctions;uint64 _entryGeneration=0;std::vector<uint32> speech;ObjectGuid greeted;
 explicit Greeting(Creature* c):me(c){c->primary=this;}
 void Talk(uint32 id,ObjectGuid recipient={}){speech.push_back(id);if(!id)greeted=recipient;}
 void UpdateLifecycle(uint32 ms)override{_entryFunctions.Update(ms);}
#include "Greeting.inc"
};
int main(){
#ifdef BASELINE
 {Map map;Creature actor;actor.map=&map;Player visitor,eligible;visitor.map=eligible.map=&map;visitor.classId=4;visitor.quest.Status=0;Greeting ai(&actor);ai.MoveInLineOfSight(&visitor);assert(ai._introDone&&ai.greeted==visitor.guid);eligible.guid={1,2};ai.MoveInLineOfSight(&eligible);assert(ai.greeted==visitor.guid);actor.Tick(4000);assert(ai.speech==(std::vector<uint32>{0,1}));}
 std::cout<<"PASS labelled4fab41ce counterfactual: unrelated questless non-Priest consumes native one-time Boros greeting before eligible player. Unsafe null baseline deliberately not invoked.\n";
#else
 for(uint32 status:{3u,1u}){Map map;Creature actor;actor.map=&map;Player p;p.map=&map;p.quest.Status=status;Greeting ai(&actor);
  ai.MoveInLineOfSight(&p);ai.MoveInLineOfSight(&p);assert(ai._introDone&&ai.speech==std::vector<uint32>{0}&&ai.greeted==p.guid);actor.Tick(3999);assert(ai.speech.size()==1);scenario.step=1;actor.Tick(1);assert(ai.speech==(std::vector<uint32>{0,1}));actor.Tick(10000);assert(ai.speech.size()==2);scenario={};}
 for(int invalid=0;invalid<24;++invalid){Map map,other;Creature actor;actor.map=&map;Player p;p.map=&map;Greeting ai(&actor);Unit pet;pet.owner=p.guid;pet.map=&map;
  Unit* input=&p;switch(invalid){case 0:input=nullptr;break;case 1:input=&pet;break;case 2:p.classId=4;break;case 3:p.quest.Status=0;break;case 4:p.rewarded=true;break;case 5:p.contact=false;break;case 6:p.alive=false;break;case 7:p.world=false;break;case 8:p.pending=true;break;case 9:p.phase=false;break;case 10:p.near=false;break;case 11:p.entryNear=false;break;case 12:p.los=false;break;case 13:p.detect=false;break;case 14:p.map=&other;break;case 15:actor.alive=false;break;case 16:actor.world=false;break;case 17:actor.preDelete=true;break;case 18:actor.entry=106137;break;case 19:actor.mapId=1220;break;case 20:scenario.id=972;break;case 21:scenario.step=1;break;case 22:manager.active=nullptr;break;case 23:p.record=false;break;}
  ai.MoveInLineOfSight(input);actor.Tick(4000);assert(!ai._introDone&&ai.speech.empty()&&ai._entryFunctions.Empty());scenario={};manager.active=&scenario;
 }
 {Map map;Creature actor;actor.map=&map;Player p;p.map=&map;Greeting ai(&actor);ai.MoveInLineOfSight(nullptr);Unit pet;ai.MoveInLineOfSight(&pet);p.classId=4;ai.MoveInLineOfSight(&p);p.classId=5;ai.MoveInLineOfSight(&p);assert(ai._introDone&&ai.speech==std::vector<uint32>{0});++ai._entryGeneration;actor.Tick(4000);assert(ai.speech.size()==1);}
 std::cout<<"PASS complete actual Boros greeting/initial policy via native FunctionProcessor/locked lifecycle: two quest statuses,24 denied/null/pet/map/scenario/visibility/LOS/range contexts leave latch/queue untouched; later valid admission, repeated LOS, original4000ms story continuation after real heal stage advance, generation cancellation. World/visibility/packet endpoints are adapters; actual voice/client remains pending.\n";
#endif
}

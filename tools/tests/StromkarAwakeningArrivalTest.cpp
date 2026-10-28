#include "ArrivalHost.inc"
int main(int argc,char**argv){
 assert(argc==2);std::ifstream in(argv[1]);std::vector<Condition>rows(6);ConditionList list;
 for(auto&r:rows){assert(in>>r.ElseGroup>>r.ConditionType>>r.ConditionTarget>>r.ConditionValue1>>r.ConditionValue2>>r.ConditionValue3>>r.NegativeCondition);list.push_back(&r);}
 conditionManager.ConditionStore[17][206361]=list;
 EffectMovementGenerator landing;
 for(uint32 stage=0;stage<12;++stage)for(uint32 status:{0u,1u,3u,5u,6u}){
  Reset();scenarioManager.scenario.step=stage;Player p;p.status=status;landing._arrived=true;landing.Finalize(p);assert(p.effects==uint32(stage==7&&status==3));assert(scenarioManager.scenario.step==stage);
 }
 for(uint32 invalid=0;invalid<10;++invalid){Reset();scenarioManager.scenario.step=7;Player p;if(invalid==0)p.map=1;if(invalid==1)p.alive=false;if(invalid==2)p.classMask=16;if(invalid==3)p.quest=1;if(invalid==4)scenarioManager.present=false;if(invalid==5)scenarioManager.scenario.identity=991;if(invalid==6)world.instanced=false;if(invalid==7)instance.identity=43;if(invalid==8)p.status=0;landing._arrived=invalid!=9;landing.Finalize(p);assert(!p.effects);if(invalid==9)assert(!p.casts);}
 Reset();scenarioManager.scenario.step=7;Unit pet;landing._arrived=true;landing.Finalize(pet);assert(!pet.effects);
 Reset();scenarioManager.scenario.step=7;Player p;landing._arrived=false;landing.Finalize(p);assert(!p.casts);landing._arrived=true;landing.Finalize(p);assert(p.effects==1);
 std::cout<<"PASS production arrival Finalize/MovementInform and actual native source17 cast/conditions: only real living Warrior active41105/scenario1037/stage7, rejects interruption/death/map transfer/abandonment/late-stage/pet. Controlled cast-effect sink, no movement/rendering certification.\n";
}

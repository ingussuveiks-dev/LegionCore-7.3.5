#define main PreservedLootMain
#include "TuurePickupConditionsTest.cpp"
#undef main
ConditionList Load(std::string path,std::vector<Condition>& rows){
 std::ifstream f(path);assert(f);rows.resize(28);ConditionList result;
 for(auto& c:rows){uint32 target;assert(f>>c.ElseGroup>>c.ConditionType>>target>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition);assert(!target);result.push_back(&c);}return result;
}
int main(int argc,char** argv){
 assert(argc==4);expectedQuest=41631;expectedMap=1583;expectedScenario=1065;expectedItem=128868;expectedLoot=248572;
 char* args[]={argv[0],argv[1]};assert(!PreservedLootMain(2,args));
 for(int source:{2,3}){
  std::vector<Condition> rows;auto conditions=Load(argv[source],rows);
  for(uint32 stage=0;stage<9;++stage)for(uint32 status:{0u,1u,3u,6u})for(uint32 ownership=0;ownership<3;++ownership){
   scenarioManager={};scenarioManager.scenario.step=stage;Player p;p.status=status;p.owned=ownership==1;p.bank=ownership==2;
   bool valid=(stage==6||stage==7)&&(status==1||status==3)&&ownership!=0;
   assert(conditionManager.IsObjectMeetToConditions(&p,conditions)==valid);
  }
  for(uint32 wrong=0;wrong<8;++wrong){
   scenarioManager={};Player p;p.owned=true;
   if(wrong==0)p.alive=false;if(wrong==1){p.rewarded=true;p.status=1;}if(wrong==2)p.classMask=128;if(wrong==3)p.mapId=576;
   if(wrong==4)scenarioManager.scenario.id=1101;if(wrong==5)scenarioManager.present=false;if(wrong==6)p.quest=42011;if(wrong==7)p.status=0;
   assert(!conditionManager.IsObjectMeetToConditions(&p,conditions));
  }
 }
 std::cout<<"PASS actual native root/cast ElseGroup predicates: native1065/6..7/41631 owned bank-or-inventory state70/cast208011; wrong shared1101/class/history, stale-map/stage and failed loot boundaries retained. Consumer dispatch verified separately; full spell/route/client pending.\n";
}

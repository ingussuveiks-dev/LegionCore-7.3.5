#define main PreservedLootMain
#include "TuurePickupConditionsTest.cpp"
#undef main
int main(int argc,char** argv)
{
    assert(argc==3);
    expectedQuest=42011;expectedMap=1583;expectedScenario=1101;
    expectedItem=127857;expectedLoot=249814;expectedClassMask=128;
    char* lootArgs[]={argv[0],argv[1]};
    assert(!PreservedLootMain(2,lootArgs));
    std::ifstream file(argv[2]);assert(file);
    std::vector<Condition> rows(28);ConditionList conditions;
    for(auto& c:rows)
    {
        uint32 target;
        assert(file>>c.ElseGroup>>c.ConditionType>>target>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition);
        assert(!target);conditions.push_back(&c);
    }
    for(uint32 stage=0;stage<9;++stage)
        for(uint32 status:{0u,1u,3u,6u})
            for(uint32 ownership=0;ownership<3;++ownership)
            {
                scenarioManager={};scenarioManager.scenario.step=stage;
                Player player;player.status=status;player.owned=ownership==1;player.bank=ownership==2;
                bool valid=(stage==6||stage==7)&&(status==1||status==3)&&ownership!=0;
                assert(conditionManager.IsObjectMeetToConditions(&player,conditions)==valid);
            }
    for(uint32 wrong=0;wrong<8;++wrong)
    {
        scenarioManager={};Player player;player.owned=true;
        if(wrong==0)player.alive=false;
        if(wrong==1){player.rewarded=true;player.status=1;}
        if(wrong==2)player.classMask=16;
        if(wrong==3)player.mapId=576;
        if(wrong==4)scenarioManager.scenario.id=1065;
        if(wrong==5)scenarioManager.present=false;
        if(wrong==6)player.quest=41631;
        if(wrong==7)player.status=0;
        assert(!conditionManager.IsObjectMeetToConditions(&player,conditions));
    }
    std::cout<<"PASS actual Aluneth native source4 cached-slot and source17 cast predicates:1101/6..7/42011/127857, wrong Priest class/quest/scenario, bank/history/death/stale-stage boundaries. Native pickup handler tests own all three control leaves and real equip/scene/return.\n";
}

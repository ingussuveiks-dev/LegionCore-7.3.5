#define ORESTES_FAREWELL_TEST
#include "MarksmanshipOrestesTest.cpp"

SmartAIEventList farewellDefinitions;
SmartScriptHolder timerDefinition;
struct FarewellFixture : Fixture
{
    GameObject door;
    FarewellFixture()
    {
        scenarioManager.scenario.step=3;
        script.mEvents=farewellDefinitions;
        actor.nearbyDoor=&door;
        door.source=&actor;
    }
    void Start()
    {
        auto timer=timerDefinition;
        timer.type=SMART_EVENT_UPDATE;
        timer.enableTimed=true;
        script.mTimedActionList={timer};
    }
    void Tick(uint32 elapsed)
    {
        for(auto& row:script.mTimedActionList)
            if(row.enableTimed)
                script.UpdateTimer(row,elapsed);
    }
};

int main(int argc,char** argv)
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    assert(argc==3);
    std::vector<Condition> conditions;
    std::ifstream gates(argv[2]);
    Condition condition;
    while(gates>>condition.ElseGroup>>condition.ConditionType>>condition.ConditionTarget>>condition.ConditionValue1>>condition.ConditionValue2>>condition.ConditionValue3>>condition.NegativeCondition)
        conditions.push_back(condition);
    assert(conditions.size()==8||conditions.size()==10);
    for(auto& row:conditions)
        conditionManager.SmartEventConditionStore[{100398,0}][6].push_back(&row);
    std::ifstream events(argv[1]);
    SmartScriptHolder row;
    uint32 minimum,maximum,first,second,third,fourth;
    std::string eventText;
    while(std::getline(events,eventText))
    {
        std::istringstream record(eventText);
        assert(record>>row.source_type>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>minimum>>maximum>>row.actionType>>first>>second>>third>>fourth>>row.targetType>>row.targetRange);
        row.targetExtra=0;
        record>>row.targetExtra;
        row.event.los.noHostile=minimum;
        row.event.los.maxDist=maximum;
        row.action.achievementCriteria={first,second,third,fourth};
        row.action.setEventPhase.phase=first;
        row.timer=minimum;
        if(row.source_type==9)
            timerDefinition=row;
        else
            farewellDefinitions.push_back(row);
    }
    bool deferredDoor=farewellDefinitions.size()==3;
    assert((farewellDefinitions.size()==2||deferredDoor)&&timerDefinition.actionType==22&&(timerDefinition.timer==10000||timerDefinition.timer==50500));
    assert(!deferredDoor||(farewellDefinitions[1].link==7&&farewellDefinitions[2].event_id==7&&farewellDefinitions[2].actionType==9&&conditions.size()==10));
    uint32 waitMs=timerDefinition.timer;
    assert(farewellDefinitions[0].event_id==5&&farewellDefinitions[0].link==6&&farewellDefinitions[0].targetType==7&&farewellDefinitions[0].event.event_phase_mask==1);
    assert(farewellDefinitions[1].event_id==6&&farewellDefinitions[1].actionType==37);
    if(deferredDoor)
    {
        FarewellFixture early;
        Creature torvis;
        torvis.nearbyDoor=&early.door;
        early.script.me=&torvis;
        auto premature=farewellDefinitions[2];
        premature.entryOrGuid=100749;premature.event_id=3;
        early.script.ProcessAction(premature,&early.finder);
        assert(early.door.opens==1&&early.door.stepAtOpen==3&&!early.door.deadAtOpen&&!early.finder.criteria);
    }
    {
        FarewellFixture baseline;
        auto credit=timerDefinition;
        credit.type=SMART_EVENT_UPDATE;
        credit.enableTimed=true;
        credit.actionType=205;
        credit.timer=10000;
        credit.action.achievementCriteria={92,48071,0,0};
        credit.targetType=21;
        credit.targetRange=50;
        auto death=farewellDefinitions[1];
        death.source_type=9;
        death.event_id=1;
        death.type=SMART_EVENT_UPDATE;
        death.link=0;
        death.enableTimed=false;
        baseline.peer.distance=51;
        baseline.finder.distance=51;
        baseline.script.mTimedActionList={credit,death};
        baseline.Tick(10001);
        assert(!baseline.finder.criteria&&!baseline.peer.criteria&&baseline.actor.dead&&scenarioManager.scenario.step==3);
    }
    for(uint32 faction=0;faction<2;++faction)
    {
        FarewellFixture repaired;
        repaired.finder.faction=faction;
        repaired.See(&repaired.finder);
        assert(!repaired.finder.criteria&&!repaired.actor.dead);
        assert(repaired.door.state==GO_STATE_READY&&!repaired.door.opens);
        repaired.Start();
        if(waitMs>10000)
        {
            repaired.Tick(10001);
            repaired.See(&repaired.finder);
            assert(!repaired.finder.criteria&&!repaired.script.mEventPhase&&!repaired.actor.dead);
            repaired.Tick(waitMs-10001);
        }
        else
            repaired.Tick(waitMs);
        repaired.See(&repaired.finder);
        assert(!repaired.finder.criteria&&!repaired.script.mEventPhase);
        repaired.Tick(1);
        assert(repaired.script.mEventPhase==1);
        repaired.finder.distance=51;
        for(Unit* nonplayer:{static_cast<Unit*>(&repaired.pet),static_cast<Unit*>(&repaired.guardian),static_cast<Unit*>(&repaired.npc),static_cast<Unit*>(nullptr)})
            repaired.See(nonplayer);
        repaired.See(&repaired.finder);
        repaired.peer.quest=QUEST_STATUS_NONE;
        repaired.See(&repaired.peer);
        repaired.Tick(60000);
        assert(!repaired.actor.dead&&!repaired.finder.criteria&&!repaired.peer.criteria&&!repaired.script.mEvents[0].runOnce);
        assert(repaired.door.state==GO_STATE_READY&&!repaired.door.opens);
        repaired.finder.distance=50;
        repaired.See(&repaired.finder);
        assert(repaired.finder.accepted==1&&repaired.finder.actualInvoker==&repaired.finder&&!repaired.peer.criteria);
        assert(scenarioManager.scenario.step==4&&repaired.actor.dead&&repaired.actor.deaths==1);
        if(deferredDoor)assert(repaired.door.state==GO_STATE_ACTIVE&&repaired.door.opens==1&&repaired.door.deadAtOpen&&repaired.door.stepAtOpen==4&&repaired.door.recipient==&repaired.finder&&(repaired.door.flags&16));
        for(uint32 repeat=0;repeat<20;++repeat)
        {
            repaired.See(&repaired.finder);
            repaired.Tick(10000);
        }
        assert(repaired.finder.criteria==1&&repaired.actor.deaths==1);
        if(deferredDoor)assert(repaired.door.opens==1&&repaired.door.state==GO_STATE_ACTIVE);
    }
    if(deferredDoor)
    {
        FarewellFixture missingDoor;
        missingDoor.actor.nearbyDoor=nullptr;
        missingDoor.Start();missingDoor.Tick(waitMs+1);missingDoor.See(&missingDoor.finder);
        assert(!missingDoor.finder.criteria&&!missingDoor.actor.dead&&!missingDoor.script.mEvents[0].runOnce&&!missingDoor.door.opens);
        missingDoor.actor.nearbyDoor=&missingDoor.door;
        missingDoor.See(&missingDoor.finder);
        assert(missingDoor.finder.accepted==1&&missingDoor.actor.dead&&missingDoor.door.opens==1);
    }
    for(uint32 quest:{QUEST_STATUS_NONE,QUEST_STATUS_REWARDED,QUEST_STATUS_COMPLETE,QUEST_STATUS_INCOMPLETE})
    {
        FarewellFixture retry;
        retry.Start();
        retry.Tick(waitMs+1);
        retry.finder.quest=quest;
        retry.See(&retry.finder);
        if(quest==QUEST_STATUS_COMPLETE||quest==QUEST_STATUS_INCOMPLETE)
            assert(retry.finder.accepted==1&&retry.actor.deaths==1);
        else
        {
            assert(!retry.finder.criteria&&!retry.actor.dead&&!retry.script.mEvents[0].runOnce);
            retry.finder.quest=QUEST_STATUS_INCOMPLETE;
            retry.See(&retry.finder);
            assert(retry.finder.accepted==1&&retry.actor.deaths==1);
        }
    }
    for(uint32 stage=0;stage<15;++stage)
    {
        if(stage==3)
            continue;
        FarewellFixture retry;
        retry.Start();
        retry.Tick(waitMs+1);
        scenarioManager.scenario.step=stage;
        retry.See(&retry.finder);
        assert(!retry.finder.criteria&&!retry.actor.dead&&!retry.script.mEvents[0].runOnce);
        scenarioManager.scenario.step=3;
        retry.See(&retry.finder);
        assert(retry.finder.accepted==1);
    }
    for(uint32 blocked=0;blocked<11;++blocked)
    {
        FarewellFixture retry;
        retry.Start();
        retry.Tick(waitMs+1);
        switch(blocked)
        {
            case 0:retry.finder.visible=false;break;
            case 1:retry.finder.hostile=true;break;
            case 2:retry.actor.combat=true;break;
            case 3:scenarioManager.present=false;break;
            case 4:scenarioManager.scenario.id=973;break;
            case 5:retry.actor.instanced=false;break;
            case 6:retry.actor.expired=true;break;
            case 7:retry.script.me=nullptr;break;
            case 8:retry.finder.alive=false;break;
            case 9:retry.finder.sameMap=false;break;
            case 10:retry.finder.samePhase=false;break;
        }
        retry.See(&retry.finder);
        assert(!retry.finder.criteria&&!retry.actor.dead&&!retry.script.mEvents[0].runOnce);
        retry.finder.visible=true;
        retry.finder.hostile=false;
        retry.finder.alive=true;
        retry.finder.sameMap=true;
        retry.finder.samePhase=true;
        retry.actor.combat=false;
        retry.actor.instanced=true;
        retry.actor.expired=false;
        retry.script.me=&retry.actor;
        scenarioManager={};
        scenarioManager.scenario.step=3;
        retry.See(&retry.finder);
        assert(retry.finder.accepted==1&&retry.actor.deaths==1);
    }
    std::cout<<"PASS: actual native timer advance reproduces creditless Orestes death; repaired phase/conditions/LOS/LINK wait for the configured delay and a present eligible quest Player, then emit48071 once before death. Player/pet/guardian proximity, 50yd boundary, both faction hosts, abandonment/retry, wrong stages, missing scenario/actor and duplicate events covered. Scenario/group world hosts are not client traversal.\n";
}

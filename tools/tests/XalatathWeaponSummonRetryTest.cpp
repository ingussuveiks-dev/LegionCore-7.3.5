#define XALATATH_WEAPON_RETRY_TEST
constexpr unsigned CONDITION_MAPID=22,SMART_ACTION_DIE=37;
#include "RetryHost.inc"
SmartScriptHolder arm;
struct Finale {
    Creature actor;
    SmartScript script;
    Finale() {
        scenarioManager={};scenarioManager.scenario.id=991;scenarioManager.scenario.step=8;
        actor.identity=101148;script.me=&actor;actor.ai.script=&script;script.mEvents=definitions;
    }
    void Arm() {script.ProcessAction(arm);}
    void Tick(uint32 elapsed=501) {script.UpdateTimer(script.mEvents[0],elapsed);}
};
int main(int argc,char** argv) {
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<3;++index) {
        SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond,actionThird;
        assert(events>>row.entryOrGuid>>row.source_type>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>actionThird>>row.targetType>>row.target.x>>row.target.y>>row.target.z>>row.target.o);
        row.event.minMaxRepeat={first,second,third,fourth};row.action.setEventPhase={actionFirst};row.action.summonGO={actionFirst,actionSecond,actionThird};
        if(row.source_type==9)arm=row;else definitions.push_back(row);
    }
    assert(arm.actionType==22&&arm.action.setEventPhase.phase==1&&definitions.size()==2);
    assert(definitions[0].event_id==30&&definitions[0].link==31&&definitions[0].event.event_phase_mask==1&&definitions[0].action.summonGO.flags==2);
    assert(definitions[1].event_id==31&&definitions[1].type==61&&definitions[1].actionType==37&&ValidateSummon(definitions[0]));
    std::vector<Condition> gates(3);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){uint32 group;assert(conditions>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);conditionManager.SmartEventConditionStore[{101148,0}][group].push_back(&gate);}
    expected={247690,3600,1868.87f,2198.75f,37.3797f,0.141143f};
    {
        Finale legacy;SmartScriptHolder summon=definitions[0],death=definitions[1];
        summon.source_type=death.source_type=9;summon.event_id=1;death.event_id=2;summon.link=0;summon.event.event_phase_mask=0;summon.action.summonGO.flags=0;
        summon.type=death.type=60;summon.enableTimed=true;death.enableTimed=false;
        legacy.script.mTimedActionList={summon,death};legacy.actor.summonAvailable=false;
        legacy.script.UpdateTimer(legacy.script.mTimedActionList[0],1);
        assert(!legacy.script.mTimedActionList[0].enableTimed&&legacy.script.mTimedActionList[1].enableTimed);
        legacy.script.UpdateTimer(legacy.script.mTimedActionList[1],1);
        assert(legacy.actor.summonAttempts==1&&!legacy.actor.summons&&!legacy.actor.alive&&legacy.actor.deaths==1);
        std::cout<<"BASELINE: actual timed-list timer advances from failed legacy creation to native DIE; no weapon exists\n";
    }
    {
        Finale pending;pending.actor.summonAvailable=false;
        for(uint32 repeat=0;repeat<4;++repeat)pending.Tick();assert(!pending.actor.summonAttempts&&pending.actor.alive);
        pending.Arm();pending.Tick();assert(pending.actor.summonAttempts==1&&!pending.actor.summons&&pending.actor.alive&&!pending.script.mEvents[0].runOnce);
        pending.Tick(500);assert(pending.actor.summonAttempts==1);
        for(uint32 repeat=0;repeat<20;++repeat)pending.Tick();
        assert(pending.actor.summonAttempts==21&&pending.actor.alive&&!pending.actor.summons&&!pending.actor.deaths);
        pending.actor.summonAvailable=true;pending.Tick();assert(pending.actor.summons==1&&!pending.actor.alive&&pending.actor.deaths==1&&pending.script.mEvents[0].runOnce);
        for(uint32 repeat=0;repeat<20;++repeat)pending.Tick();assert(pending.actor.summons==1&&pending.actor.deaths==1);
    }
    for(uint32 invalid=0;invalid<7;++invalid) {
        Finale pending;pending.Arm();
        if(invalid==0)scenarioManager.scenario.step=7;
        if(invalid==1)scenarioManager.scenario.step=9;
        if(invalid==2)scenarioManager.scenario.id=1037;
        if(invalid==3)scenarioManager.present=false;
        if(invalid==4)pending.actor.mapId=1;
        if(invalid==5)pending.actor.instanced=false;
        if(invalid==6)pending.script.me=nullptr;
        pending.Tick();assert(!pending.actor.summonAttempts&&pending.actor.alive&&!pending.script.mEvents[0].runOnce);
        pending.script.me=&pending.actor;pending.actor.instanced=true;pending.actor.mapId=1539;scenarioManager={};scenarioManager.scenario.id=991;scenarioManager.scenario.step=8;
        pending.Tick();assert(pending.actor.summons==1&&pending.actor.deaths==1);
    }
    {Finale reset;reset.Arm();reset.script.SetPhase(0);reset.Tick();assert(!reset.actor.summons&&reset.actor.alive);}
    {Finale happy;happy.Arm();happy.Tick(1);assert(happy.actor.summonAttempts==1&&happy.actor.summons==1&&happy.actor.deaths==1);}
    std::cout<<"PASS: actual native phase/conditions/timer/summon-pointer/repeat/LINK/DIE dispatch retains failed creation, retries at500ms and dies once only after success; full20s credit/list entry and inventory/visuals are separate\n";
}

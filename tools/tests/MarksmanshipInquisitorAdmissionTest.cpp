#define MARKSMANSHIP_INQUISITOR_TEST
#include "MarksmanshipHeraldChallengeTest.cpp"
bool SmartAI::LoadPath(uint32 identity){
    assert(identity==10039703);++loadAttempts;
    if(!pathAvailable)return false;
    mWayPoints=&points;script->SetPathId(identity);return true;
}
#include "StartPath.inc"
struct InquisitorFixture {
    Creature vereesa,inquisitor;SmartScript sender,receiver;
    InquisitorFixture(){scenarioManager={};scenarioManager.scenario.step=5;vereesa.identity=100397;inquisitor.identity=101269;
        inquisitor.nearby={&vereesa};vereesa.nearby={&inquisitor};sender.me=&inquisitor;receiver.me=&vereesa;
        inquisitor.ai.me=&inquisitor;vereesa.ai.me=&vereesa;inquisitor.ai.script=&sender;vereesa.ai.script=&receiver;
        for(auto const& row:definitions)(row.entryOrGuid==101269?sender.mEvents:receiver.mEvents).push_back(row);
    }
    void Tick(){sender.UpdateTimer(sender.mEvents[0],1001);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> gates(2);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);assert(gate.ConditionTarget==1&&gate.ConditionType==44&&gate.ConditionValue1==972&&gate.ConditionValue2==5);conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<2;++index){SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>row.targetType>>row.target.closest.entry>>row.target.closest.dist>>row.target.closest.dead);
        row.event.minMaxRepeat={first,second,third,fourth};row.event.dataSet={first,second,third,fourth};row.action.setData={actionFirst,actionSecond};row.action.wpStart.run=actionFirst;row.action.wpStart.pathID=actionSecond;definitions.push_back(row);
    }
    assert(definitions[0].entryOrGuid==100397&&definitions[0].event_id==30&&definitions[0].link==31&&definitions[0].actionType==53);
    assert(definitions[1].entryOrGuid==101269&&definitions[1].event_id==0&&!definitions[1].event.event_flags&&definitions[1].action.setData.data==9);
    auto saved=conditionManager.SmartEventConditionStore;conditionManager.SmartEventConditionStore.clear();
    {InquisitorFixture baseline;scenarioManager.scenario.step=4;baseline.Tick();assert(baseline.vereesa.motion.moves==1&&baseline.receiver.mEvents[0].runOnce);
        scenarioManager.scenario.step=5;baseline.Tick();assert(baseline.vereesa.motion.moves==1);}
    conditionManager.SmartEventConditionStore=saved;
    for(uint32 stage=0;stage<12;++stage){if(stage==5)continue;InquisitorFixture retry;scenarioManager.scenario.step=stage;
        for(uint32 tick=0;tick<10;++tick)retry.Tick();assert(!retry.vereesa.motion.moves&&!retry.receiver.mEvents[0].runOnce);
        retry.vereesa.AI()->SetData(9,9);assert(!retry.vereesa.motion.moves&&!retry.receiver.mEvents[0].runOnce);
        scenarioManager.scenario.step=5;retry.Tick();assert(retry.vereesa.motion.moves==1&&retry.receiver.mEvents[0].runOnce&&retry.receiver.pathId==10039703&&retry.vereesa.ai.running);
        assert(!retry.vereesa.ai.mCanRepeatPath&&!retry.vereesa.ai.mEscortQuestID&&!retry.vereesa.ai.despawnTime);
        for(uint32 tick=0;tick<10;++tick)retry.Tick();assert(retry.vereesa.motion.moves==1);
    }
    for(uint32 blocked=0;blocked<9;++blocked){InquisitorFixture retry;
        switch(blocked){case 0:retry.inquisitor.nearby.clear();break;case 1:retry.vereesa.distance=60.01f;break;case 2:retry.vereesa.sameMap=false;break;case 3:retry.vereesa.samePhase=false;break;case 4:retry.vereesa.alive=false;break;case 5:scenarioManager.present=false;break;case 6:scenarioManager.scenario.id=1010;break;case 7:retry.inquisitor.instanced=false;break;case 8:retry.inquisitor.expired=true;break;}
        retry.Tick();assert(!retry.vereesa.motion.moves&&!retry.receiver.mEvents[0].runOnce);
        retry.inquisitor.nearby={&retry.vereesa};retry.vereesa.distance=60;retry.vereesa.sameMap=true;retry.vereesa.samePhase=true;retry.vereesa.alive=true;retry.inquisitor.instanced=true;retry.inquisitor.expired=false;scenarioManager={};scenarioManager.scenario.step=5;retry.Tick();assert(retry.vereesa.motion.moves==1);
    }
    {InquisitorFixture invalid;invalid.vereesa.AI()->SetData(9,8);invalid.vereesa.AI()->SetData(8,9);assert(!invalid.receiver.mEvents[0].runOnce);invalid.Tick();assert(invalid.vereesa.motion.moves==1);}
    {InquisitorFixture invokers;scenarioManager.scenario.step=4;Player player;Unit pet,guardian;
        for(Unit* source:{static_cast<Unit*>(&player),&pet,&guardian,static_cast<Unit*>(nullptr)})invokers.receiver.ProcessEventsFor(SMART_EVENT_DATA_SET,source,9,9);
        assert(!invokers.vereesa.motion.moves&&!invokers.receiver.mEvents[0].runOnce);scenarioManager.scenario.step=5;invokers.Tick();assert(invokers.vereesa.motion.moves==1&&!invokers.receiver.mLastInvoker.unit);}
    {InquisitorFixture missing;missing.sender.me=nullptr;missing.Tick();assert(!missing.vereesa.motion.moves);}
    {InquisitorFixture combat;combat.vereesa.combat=true;combat.Tick();assert(combat.receiver.mEvents[0].runOnce&&!combat.vereesa.motion.moves&&!combat.vereesa.ai.loadAttempts);}
    {InquisitorFixture missingPath;missingPath.vereesa.ai.pathAvailable=false;missingPath.Tick();assert(missingPath.receiver.mEvents[0].runOnce&&!missingPath.vereesa.motion.moves&&missingPath.vereesa.ai.loadAttempts==1);}
    std::cout<<"PASS: native conditions/timer/nearest/SET_DATA/DATA_SET/WP_START/SmartAI StartPath reject premature interrogation and preserve stage5 approach, retries and one-shot path dispatch. Later LINK/casts/scenes and world/path/motion primitives are not exercised here; combat/missing-path retry remains unresolved.\n";
}

#define MARKSMANSHIP_INQUISITOR_TEST
#define MARKSMANSHIP_POST_INQUISITOR_TEST
constexpr unsigned CONDITION_ALIVE=36;
#include "MarksmanshipHeraldChallengeTest.cpp"
bool SmartAI::LoadPath(uint32 identity){
    assert(identity==10039705);++loadAttempts;if(!pathAvailable)return false;
    mWayPoints=&points;script->SetPathId(identity);return true;
}
#include "StartPath.inc"
struct PostInquisitorFixture {
    Creature vereesa;SmartScript script;
    PostInquisitorFixture(){scenarioManager={};scenarioManager.scenario.step=6;vereesa.identity=100397;
        script.me=&vereesa;vereesa.ai.me=&vereesa;vereesa.ai.script=&script;vereesa.ai.points={{1,187.494f,1295.63f,-59.0451f}};script.mEvents=definitions;
    }
    void Tick(){if(script.mEvents.size()>1)script.UpdateTimer(script.mEvents[1],1001);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> gates(4);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);assert(gate.ConditionTarget==1&&(gate.ConditionType==44||gate.ConditionType==36));conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<2;++index){SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>row.targetType>>row.target.closest.entry>>row.target.closest.dist>>row.target.closest.dead);
        row.event.minMaxRepeat={first,second,third,fourth};row.event.dataSet={first,second,third,fourth};row.action.setData={actionFirst,actionSecond};row.action.wpStart.run=actionFirst;row.action.wpStart.pathID=actionSecond;definitions.push_back(row);
    }
    assert(definitions[0].event_id==39&&definitions[0].link==40&&definitions[0].actionType==53&&definitions[0].action.wpStart.pathID==10039705);
    assert(definitions[1].event_id==60&&definitions[1].type==60&&!definitions[1].event.event_flags&&definitions[1].actionType==45&&definitions[1].action.setData.field==10&&definitions[1].targetType==1);
    auto saved=conditionManager.SmartEventConditionStore;conditionManager.SmartEventConditionStore.clear();
    {PostInquisitorFixture baseline;baseline.script.mEvents.pop_back();baseline.vereesa.combat=true;baseline.vereesa.AI()->SetData(10,10);baseline.vereesa.combat=false;
        for(uint32 tick=0;tick<20;++tick)baseline.Tick();assert(!baseline.vereesa.motion.moves&&!baseline.script.mEvents[0].runOnce);}
    {PostInquisitorFixture missing;missing.script.mEvents.pop_back();for(uint32 tick=0;tick<20;++tick)missing.Tick();assert(!missing.vereesa.motion.moves);}
    {PostInquisitorFixture early;scenarioManager.scenario.step=5;early.vereesa.AI()->SetData(10,10);assert(early.vereesa.motion.moves==1);}
    conditionManager.SmartEventConditionStore=saved;
    for(uint32 stage=0;stage<12;++stage){if(stage==6)continue;PostInquisitorFixture retry;scenarioManager.scenario.step=stage;
        for(uint32 tick=0;tick<10;++tick){retry.Tick();retry.vereesa.AI()->SetData(10,10);}assert(!retry.vereesa.motion.moves&&!retry.script.mEvents[0].runOnce);
        scenarioManager.scenario.step=6;retry.Tick();assert(retry.vereesa.motion.moves==1&&retry.script.mEvents[0].runOnce&&retry.script.pathId==10039705);
        for(uint32 tick=0;tick<10;++tick){retry.Tick();retry.vereesa.AI()->SetData(10,10);}assert(retry.vereesa.motion.moves==1);
    }
    for(uint32 blocked=0;blocked<7;++blocked){PostInquisitorFixture retry;
        switch(blocked){case 0:retry.vereesa.combat=true;break;case 1:retry.vereesa.ai.pathAvailable=false;break;case 2:retry.vereesa.alive=false;break;case 3:scenarioManager.present=false;break;case 4:scenarioManager.scenario.id=1010;break;case 5:retry.vereesa.instanced=false;break;case 6:retry.vereesa.expired=true;break;}
        for(uint32 tick=0;tick<10;++tick)retry.Tick();assert(!retry.vereesa.motion.moves&&!retry.script.mEvents[0].runOnce);
        retry.vereesa.combat=false;retry.vereesa.ai.pathAvailable=true;retry.vereesa.alive=true;retry.vereesa.instanced=true;retry.vereesa.expired=false;scenarioManager={};scenarioManager.scenario.step=6;retry.Tick();assert(retry.vereesa.motion.moves==1);
    }
    {PostInquisitorFixture fresh;fresh.Tick();assert(fresh.vereesa.motion.moves==1&&!fresh.script.mLastInvoker.unit);}
    {PostInquisitorFixture competition;competition.vereesa.AI()->SetData(10,10);competition.Tick();assert(competition.vereesa.motion.moves==1);}
    {PostInquisitorFixture absent;absent.script.me=nullptr;absent.Tick();assert(!absent.vereesa.motion.moves);absent.script.me=&absent.vereesa;absent.Tick();assert(absent.vereesa.motion.moves==1);}
    {PostInquisitorFixture direct;scenarioManager.scenario.step=5;Unit pet,guardian;Player player;
        for(Unit* source:{&pet,&guardian,static_cast<Unit*>(&player),static_cast<Unit*>(nullptr)})direct.script.ProcessEventsFor(SMART_EVENT_DATA_SET,source,10,10);
        assert(!direct.vereesa.motion.moves&&!direct.script.mEvents[0].runOnce);scenarioManager.scenario.step=6;direct.Tick();assert(direct.vereesa.motion.moves==1);}
    std::cout<<"PASS: native periodic UPDATE/SELF/SET_DATA/SmartAI DATA_SET/alive+972-stage6 conditions/WP_START recover a lost death signal and combat/path refusal exactly once, rejecting early/dead/wrong-scenario/scope contexts. No death credit is replaced; later LINK40 speech/cleanup, actual map loading and quest-abandon persistence are not executed by this host.\n";
}

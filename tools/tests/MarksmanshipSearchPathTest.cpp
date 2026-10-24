#define MARKSMANSHIP_INQUISITOR_TEST
#define MARKSMANSHIP_POST_INQUISITOR_TEST
#define MARKSMANSHIP_SEARCH_PATH_TEST
constexpr unsigned CONDITION_ALIVE=36,SMART_ACTION_SET_EVENT_PHASE=22,SMART_EVENT_PHASE_COUNT=12;
#include "MarksmanshipHeraldChallengeTest.cpp"
bool SmartAI::LoadPath(uint32 identity){
    assert(identity==10039704);++loadAttempts;if(!pathAvailable)return false;
    mWayPoints=&points;script->SetPathId(identity);return true;
}
#include "StartPath.inc"
#include "PhaseMethods.inc"
#include "FindLink.inc"
struct SearchFixture {
    Creature vereesa;SmartScript script;
    SearchFixture(){scenarioManager={};scenarioManager.scenario.step=6;vereesa.identity=100397;
        script.me=&vereesa;vereesa.ai.me=&vereesa;vereesa.ai.script=&script;vereesa.ai.points={{1,192.083f,1282.94f,-59.0106f}};script.mEvents=definitions;
    }
    void Tick(){script.UpdateTimer(script.mEvents[2],1001);}
    void TextOver(){script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,16,100397);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> gates(6);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<3;++index){SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>row.targetType>>row.target.closest.entry>>row.target.closest.dist>>row.target.closest.dead);
        row.event.minMaxRepeat={first,second,third,fourth};row.event.textOver={first,second};row.action.talk={actionFirst,actionSecond};row.action.wpStart.run=actionFirst;row.action.wpStart.pathID=actionSecond;row.action.setEventPhase.phase=actionFirst;definitions.push_back(row);
    }
    assert(definitions[0].event_id==42&&definitions[0].link==43&&definitions[0].actionType==1&&definitions[0].action.talk.textGroupID==17&&definitions[0].action.talk.duration==7000);
    assert(definitions[1].event_id==43&&definitions[1].actionType==22&&definitions[1].action.setEventPhase.phase==1);
    assert(definitions[2].event_id==61&&definitions[2].type==60&&definitions[2].event.event_phase_mask==1&&definitions[2].actionType==53);
    auto saved=conditionManager.SmartEventConditionStore;conditionManager.SmartEventConditionStore.clear();
    {SearchFixture baseline;baseline.script.mEvents[1].actionType=53;baseline.script.mEvents[1].action.wpStart={1,10039704,0,0,0,0};baseline.vereesa.combat=true;baseline.TextOver();assert(baseline.script.mEvents[0].runOnce&&baseline.vereesa.speeches==1&&!baseline.vereesa.motion.moves);
        baseline.vereesa.combat=false;for(uint32 tick=0;tick<10;++tick){baseline.Tick();baseline.TextOver();}assert(!baseline.vereesa.motion.moves&&baseline.vereesa.speeches==1);}
    conditionManager.SmartEventConditionStore=saved;
    {SearchFixture before;for(uint32 tick=0;tick<10;++tick)before.Tick();assert(!before.vereesa.motion.moves&&!before.script.mEvents[2].runOnce&&!before.vereesa.speeches);}
    for(uint32 blocker=0;blocker<2;++blocker){SearchFixture retry;if(blocker==0)retry.vereesa.combat=true;else retry.vereesa.ai.pathAvailable=false;
        retry.TextOver();assert(retry.vereesa.speeches==1&&retry.vereesa.lastSpeech==17&&retry.script.mEventPhase==1&&retry.script.mTextTimer==7000&&!retry.vereesa.motion.moves);
        for(uint32 tick=0;tick<10;++tick){retry.Tick();retry.TextOver();}assert(!retry.vereesa.motion.moves&&!retry.script.mEvents[2].runOnce&&retry.vereesa.speeches==1);
        retry.vereesa.combat=false;retry.vereesa.ai.pathAvailable=true;retry.Tick();assert(retry.vereesa.motion.moves==1&&retry.script.mEvents[2].runOnce&&retry.script.pathId==10039704&&retry.script.mTextTimer==7000);
        for(uint32 tick=0;tick<10;++tick)retry.Tick();assert(retry.vereesa.motion.moves==1&&retry.vereesa.speeches==1);}
    {SearchFixture happy;happy.TextOver();happy.script.UpdateTimer(happy.script.mEvents[2],1);assert(happy.vereesa.speeches==1&&happy.vereesa.motion.moves==1&&happy.script.mTextTimer==7000);}
    {SearchFixture timing;timing.script.mUseTextTimer=true;timing.script.mLastTextID=16;timing.script.mTalkerEntry=100397;timing.script.mTextTimer=8000;
        timing.script.TextTick(8000);timing.Tick();assert(!timing.vereesa.motion.moves&&!timing.vereesa.speeches);timing.script.TextTick(1);timing.Tick();assert(timing.vereesa.motion.moves==1&&timing.vereesa.speeches==1&&timing.script.mTextTimer==7000);}
    for(uint32 stage=0;stage<12;++stage){if(stage==6)continue;SearchFixture reject;scenarioManager.scenario.step=stage;reject.TextOver();for(uint32 tick=0;tick<10;++tick)reject.Tick();assert(!reject.script.mEventPhase&&!reject.vereesa.motion.moves&&!reject.vereesa.speeches&&!reject.script.mEvents[0].runOnce);
        scenarioManager.scenario.step=6;reject.TextOver();reject.Tick();assert(reject.vereesa.motion.moves==1&&reject.vereesa.speeches==1);
        SearchFixture delayed;delayed.TextOver();scenarioManager.scenario.step=stage;delayed.Tick();assert(!delayed.vereesa.motion.moves&&!delayed.script.mEvents[2].runOnce);}
    for(uint32 blocker=0;blocker<6;++blocker){SearchFixture retry;retry.TextOver();switch(blocker){case 0:retry.vereesa.alive=false;break;case 1:scenarioManager.present=false;break;case 2:scenarioManager.scenario.id=1010;break;case 3:retry.vereesa.instanced=false;break;case 4:retry.vereesa.expired=true;break;case 5:retry.script.me=nullptr;break;}
        retry.Tick();assert(!retry.vereesa.motion.moves&&!retry.script.mEvents[2].runOnce);retry.script.me=&retry.vereesa;retry.vereesa.alive=true;retry.vereesa.instanced=true;retry.vereesa.expired=false;scenarioManager={};scenarioManager.scenario.step=6;retry.Tick();assert(retry.vereesa.motion.moves==1);}
    std::cout<<"PASS: native TEXT_OVER/TALK/phase action and phase-mask/UPDATE/conditions/WP_START retain armed search retry after combat/path refusal, start once while Enough speech is still pending, and reject premature/late/dead/missing/scenario/scope contexts. Real motion/traversal, reset persistence and later route visuals/credits are not modeled.\n";
}

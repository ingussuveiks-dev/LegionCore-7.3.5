#define MARKSMANSHIP_INQUISITOR_TEST
#define MARKSMANSHIP_POST_INQUISITOR_TEST
#define MARKSMANSHIP_SEARCH_PATH_TEST
#define MARKSMANSHIP_BOW_REVEAL_TEST
constexpr unsigned CONDITION_ALIVE=36,SMART_ACTION_SET_EVENT_PHASE=22,SMART_EVENT_PHASE_COUNT=12;
constexpr unsigned SMART_ACTION_SUMMON_GO=50,SMART_TARGET_POSITION=8,SMART_TARGET_RANDOM_POSITION=128;
constexpr unsigned SMART_SUMMON_GAMEOBJECT_RETRY_ON_FAILURE=2;
constexpr unsigned SMART_SUMMON_GAMEOBJECT_NO_CREATOR=4;
constexpr unsigned SMART_EVENT_WAYPOINT_PAUSED=55,SMART_EVENT_WAYPOINT_RESUMED=56,SMART_EVENT_WAYPOINT_STOPPED=57,SMART_EVENT_WAYPOINT_ENDED=58;
constexpr double M_PI=3.14159265358979323846;
float rand_norm(){return 0;}
#include "MarksmanshipHeraldChallengeTest.cpp"
#include "FindLink.inc"
#include "PhaseMethods.inc"
bool SmartAI::StartPath(bool,unsigned,bool,Unit*){return false;}
GameObject* WorldObject::SummonGameObject(uint32 entry,float positionX,float positionY,float positionZ,float orientation,float,float,float,float,uint32 lifetime,ObjectGuid,void*,bool creator){
    assert(creator);
    assert(entry==248419&&std::abs(positionX-62.6788f)<0.001f&&std::abs(positionY-1215.55f)<0.001f&&std::abs(positionZ+38.6585f)<0.001f&&std::abs(orientation-0.705752f)<0.001f&&lifetime==3600);
    if(!summonAvailable)return nullptr;
    ++summons;static GameObject object;return &object;
}
struct RevealFixture {
    Creature vereesa;SmartScript script;
    RevealFixture(){scenarioManager={};scenarioManager.scenario.step=6;vereesa.identity=100397;script.me=&vereesa;vereesa.ai.script=&script;script.mEvents=definitions;script.SetPathId(10039704);}
    void Reach(uint32 point){script.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,point);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> gates(4);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<2;++index){SmartScriptHolder row;uint32 point,path,first,second;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>point>>path>>row.actionType>>first>>second>>row.targetType>>row.target.x>>row.target.y>>row.target.z>>row.target.o);
        row.event.waypoint={point,path};row.action.talk={first,second};row.action.summonGO={first,second};definitions.push_back(row);
    }
    assert(definitions[0].event_id==45&&definitions[0].link==50&&definitions[0].action.talk.textGroupID==19&&definitions[0].action.talk.duration==5000);
    assert(definitions[1].event_id==50&&!definitions[1].link&&definitions[1].actionType==50);
    auto saved=conditionManager.SmartEventConditionStore;conditionManager.SmartEventConditionStore.clear();
    {RevealFixture baseline;baseline.script.mEvents[0].link=0;baseline.Reach(14);assert(baseline.vereesa.speeches==1&&baseline.vereesa.lastSpeech==19&&!baseline.vereesa.summons);
        scenarioManager.scenario.step=7;baseline.script.ProcessEvent(baseline.script.mEvents[1]);assert(baseline.vereesa.summons==1);}
    conditionManager.SmartEventConditionStore=saved;
    {RevealFixture current;current.Reach(13);assert(!current.vereesa.speeches&&!current.vereesa.summons);current.script.SetPathId(10039705);current.Reach(14);assert(!current.vereesa.summons&&!current.vereesa.speeches);
        current.script.SetPathId(10039704);current.Reach(14);assert(current.vereesa.summons==1&&current.vereesa.speeches==1&&current.vereesa.lastSpeech==19&&current.script.mTextTimer==5000&&scenarioManager.scenario.step==6);
        for(unsigned repeat=0;repeat<20;++repeat)current.Reach(14);assert(current.vereesa.summons==1&&current.vereesa.speeches==1);}
    for(unsigned stage=0;stage<12;++stage){if(stage==6)continue;RevealFixture blocked;scenarioManager.scenario.step=stage;blocked.Reach(14);assert(!blocked.vereesa.summons&&!blocked.vereesa.speeches&&!blocked.script.mEvents[0].runOnce);
        scenarioManager.scenario.step=6;blocked.Reach(14);assert(blocked.vereesa.summons==1);}
    for(unsigned blocker=0;blocker<6;++blocker){RevealFixture blocked;switch(blocker){case 0:blocked.vereesa.alive=false;break;case 1:scenarioManager.present=false;break;case 2:scenarioManager.scenario.id=1010;break;case 3:blocked.vereesa.instanced=false;break;case 4:blocked.vereesa.expired=true;break;case 5:blocked.script.me=nullptr;break;}
        blocked.Reach(14);assert(!blocked.vereesa.summons&&!blocked.vereesa.speeches&&!blocked.script.mEvents[0].runOnce);}
    {RevealFixture failed;failed.vereesa.summonAvailable=false;failed.Reach(14);failed.vereesa.summonAvailable=true;failed.Reach(14);assert(failed.vereesa.speeches==1&&!failed.vereesa.summons&&failed.script.mEvents[0].runOnce);}
    std::cout<<"PASS: historical migration12 fixture reveals the original bow once with speech19 at stage6 without advancing it; native waypoint/condition/TALK/LINK/SUMMON_GO reject wrong point/path/stage/dead/expired/missing context before discovery. Its unflagged summon failure still needs the separate migration13 retry fixture. No map rendering/loot/equip traversal is modeled.\n";
}

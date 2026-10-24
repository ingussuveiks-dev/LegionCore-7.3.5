#define MARKSMANSHIP_INQUISITOR_TEST
#define MARKSMANSHIP_POST_INQUISITOR_TEST
#define MARKSMANSHIP_SEARCH_PATH_TEST
#define MARKSMANSHIP_FINAL_STAGE_TEST
constexpr unsigned CONDITION_ALIVE=36,SMART_ACTION_SET_EVENT_PHASE=22,SMART_EVENT_PHASE_COUNT=12;
constexpr unsigned SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205,SMART_ACTION_CALL_KILLEDMONSTER=33;
constexpr unsigned SMART_TARGET_CLOSEST_PLAYER=21;
using CriteriaTypes=unsigned;
#include "MarksmanshipHeraldChallengeTest.cpp"
#include "FindLink.inc"
#include "PhaseMethods.inc"
bool SmartAI::StartPath(bool,unsigned,bool,Unit*){return false;}
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset,uint32 amount,uint32 extra,Unit* invoker,bool scripted){
    assert(type==92&&asset==47846&&!amount&&!extra&&!invoker&&scripted);++criteria;
    if(allowProgress)scenarioManager.scenario.step=7;
}
struct FinalFixture {
    Creature vereesa;Player player;SmartScript script;
    FinalFixture(){scenarioManager={};scenarioManager.scenario.step=6;vereesa.identity=100397;vereesa.selectedPlayer=&player;script.me=&vereesa;vereesa.ai.script=&script;script.mEvents=definitions;script.SetPhase(2);}
    void Begin(){script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,21,100397);}
    void Tick(){script.UpdateTimer(script.mEvents[3],1001);}
    void Expire(){script.TextTick(15000);script.TextTick(1);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> gates(5);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(unsigned index=0;index<4;++index){SmartScriptHolder row;unsigned first,second,third,fourth,actionFirst,actionSecond,range;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>row.targetType>>range);
        row.event.textOver={first,second};row.event.minMaxRepeat={first,second,third,fourth};row.action.talk={actionFirst,actionSecond};row.action.setEventPhase={actionFirst};row.action.achievementCriteria={actionFirst,actionSecond,0,0};row.target.playerDistance={range};definitions.push_back(row);
    }
    assert(definitions[0].event_id==48&&!definitions[0].link&&definitions[0].action.talk.textGroupID==22&&definitions[0].action.talk.duration==15000);
    assert(definitions[1].event_id==49&&definitions[1].type==52&&definitions[1].action.setEventPhase.phase==3);
    assert(definitions[2].event_id==51&&definitions[2].actionType==33&&definitions[3].event_id==63&&definitions[3].link==51&&definitions[3].event.event_phase_mask==4&&!definitions[3].event.event_flags);
    auto saved=conditionManager.SmartEventConditionStore;conditionManager.SmartEventConditionStore.clear();
    {FinalFixture baseline;baseline.script.mEvents[0].link=49;baseline.script.mEvents[1]=baseline.script.mEvents[3];baseline.script.mEvents[1].event_id=49;baseline.script.mEvents[1].type=61;baseline.script.mEvents[1].event.event_phase_mask=0;
        baseline.Begin();assert(baseline.player.criteria==1&&baseline.player.creditHandoffs==1&&scenarioManager.scenario.step==7&&baseline.script.mTextTimer==15000);}
    conditionManager.SmartEventConditionStore=saved;
    {FinalFixture unrelated;unrelated.Begin();unrelated.script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,23,100397);unrelated.script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,22,100398);unrelated.Tick();assert(unrelated.script.mEventPhase==2&&!unrelated.player.criteria&&unrelated.script.mTextTimer==15000);}
    {FinalFixture happy;happy.Begin();assert(happy.vereesa.speeches==1&&happy.vereesa.lastSpeech==22&&happy.script.mTextTimer==15000);for(unsigned repeat=0;repeat<10;++repeat)happy.Tick();assert(!happy.player.criteria&&!happy.player.creditHandoffs&&scenarioManager.scenario.step==6);
        happy.script.TextTick(15000);happy.Tick();assert(!happy.player.criteria&&happy.script.mEventPhase==2&&!happy.script.mTextTimer);happy.script.TextTick(1);assert(happy.script.mEventPhase==3);happy.Tick();assert(happy.player.criteria==1&&happy.player.creditHandoffs==1&&scenarioManager.scenario.step==7);
        for(unsigned repeat=0;repeat<20;++repeat){happy.Tick();happy.script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,22,100397);}assert(happy.player.criteria==1&&happy.player.creditHandoffs==1&&happy.vereesa.speeches==1);}
    {FinalFixture absent;absent.Begin();absent.Expire();absent.vereesa.selectedPlayer=nullptr;for(unsigned repeat=0;repeat<20;++repeat)absent.Tick();assert(!absent.player.criteria&&!absent.player.creditHandoffs&&scenarioManager.scenario.step==6);absent.vereesa.selectedPlayer=&absent.player;absent.vereesa.playerDistance=50.01f;absent.Tick();assert(!absent.player.criteria);absent.vereesa.playerDistance=50;absent.Tick();assert(absent.player.criteria==1&&absent.player.creditHandoffs==1);}
    {FinalFixture rejected;rejected.Begin();rejected.Expire();rejected.player.allowProgress=false;for(unsigned repeat=0;repeat<5;++repeat)rejected.Tick();assert(rejected.player.criteria==5&&!rejected.player.creditHandoffs&&scenarioManager.scenario.step==6);rejected.player.allowProgress=true;rejected.Tick();assert(rejected.player.criteria==6&&rejected.player.creditHandoffs==1);}
    {FinalFixture deadPlayer;deadPlayer.Begin();deadPlayer.Expire();deadPlayer.player.alive=false;deadPlayer.Tick();assert(!deadPlayer.player.criteria&&!deadPlayer.player.creditHandoffs);deadPlayer.player.alive=true;deadPlayer.Tick();assert(deadPlayer.player.criteria==1&&deadPlayer.player.creditHandoffs==1);}
    {FinalFixture dead;dead.Begin();dead.vereesa.alive=false;dead.Expire();assert(dead.script.mEventPhase==3);dead.Tick();assert(!dead.player.criteria);dead.vereesa.alive=true;dead.Tick();assert(dead.player.criteria==1);}
    for(unsigned stage=0;stage<12;++stage){if(stage==6)continue;FinalFixture reject;reject.Begin();scenarioManager.scenario.step=stage;reject.Expire();reject.Tick();assert(reject.script.mEventPhase==2&&!reject.player.criteria&&!reject.player.creditHandoffs);}
    for(unsigned blocker=0;blocker<6;++blocker){FinalFixture reject;reject.Begin();reject.Expire();switch(blocker){case 0:reject.vereesa.alive=false;break;case 1:scenarioManager.present=false;break;case 2:scenarioManager.scenario.id=1010;break;case 3:reject.vereesa.instanced=false;break;case 4:reject.vereesa.expired=true;break;case 5:reject.script.me=nullptr;break;}
        reject.Tick();assert(!reject.player.criteria&&!reject.player.creditHandoffs);}
    {FinalFixture claim;scenarioManager.scenario.step=8;claim.script.ProcessEventsFor(SMART_EVENT_TEXT_OVER,&claim.player,22,100397);claim.Tick();assert(claim.script.mEventPhase==2&&!claim.player.criteria&&!claim.player.creditHandoffs);}
    std::cout<<"PASS: native TALK/text-expiry/phase/UPDATE/conditions/LINK/criteria dispatch retains15000ms Take speech, waits actual timer expiry and recovers missing/out-of-range player without repeated speech. No search credit before actual modeled6->7 advancement; retries stop at7/8. Grid nearest-player and scenario update are controlled hosts; credit51 handoff is counted, not a real quest/group/item grant.\n";
}

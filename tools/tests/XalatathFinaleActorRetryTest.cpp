#define XALATATH_ACTOR_RETRY_TEST
#define XALATATH_WEAPON_RETRY_TEST
using TempSummonType=unsigned;
constexpr unsigned CONDITION_MAPID=22,SMART_ACTION_DIE=37,SMART_EVENT_SPELLHIT_TARGET=31,SMART_EVENT_FLAG_DONT_RESET=0x100;
#include "RetryHost.inc"
#include "NativeJustSummoned.inc"
#include "NativeSpellHit.inc"
#include "NativeReset.inc"
struct TempSummon:Creature {
    Unit* summoner=nullptr;
    Unit* GetSummoner(){return summoner;}
    void InitSummon();
};
#include "NativeInitSummon.inc"
TempSummon* spawnSlot=nullptr;
bool deliverSoulHit=true;
Creature* WorldObject::SummonCreature(uint32 entry,float positionX,float positionY,float positionZ,float orientation,TempSummonType type,uint32 duration){
    assert(entry==102247&&type==8&&duration==0);
    assert(std::abs(positionX-1868.87f)<0.001f&&std::abs(positionY-2198.75f)<0.001f&&std::abs(positionZ-38.171f)<0.001f&&std::abs(orientation-0.141143f)<0.001f);
    ++summonAttempts;if(!summonAvailable)return nullptr;
    ++summons;assert(spawnSlot);spawnSlot->identity=entry;spawnSlot->summoner=ToUnit();spawnSlot->ai.me=spawnSlot;
    spawnSlot->InitSummon();return spawnSlot;
}
void SmartAI::IsSummonedBy(Unit* owner){
    assert(me&&me->identity==102247&&owner&&owner->ToCreature());
    if(deliverSoulHit){SpellInfo spell;owner->ToCreature()->AI()->SpellHit(me,&spell);}
}
struct Finale {
    Creature owner;TempSummon soul;SmartScript script;
    Finale(){scenarioManager={};scenarioManager.scenario.id=991;scenarioManager.scenario.step=7;deliverSoulHit=true;
        owner.identity=101148;owner.ai.me=&owner;owner.ai.script=&script;script.me=&owner;script.mEvents=definitions;spawnSlot=&soul;}
    SmartScriptHolder& Row(uint32 identity){for(auto& row:script.mEvents)if(row.event_id==identity)return row;std::abort();}
    void Arm(){script.ProcessEvent(Row(26));}
    void Tick(uint32 elapsed=501){script.UpdateTimer(Row(32),elapsed);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<6;++index){SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond,actionThird;
        assert(events>>row.entryOrGuid>>row.source_type>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags
            >>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>actionThird>>row.targetType>>row.target.x>>row.target.y>>row.target.z>>row.target.o);
        row.event.minMaxRepeat={first,second,third,fourth};row.event.summoned={first,second,third};row.event.spellHit={first,second,third,fourth};
        row.action.setEventPhase={actionFirst};row.action.summonCreature={actionFirst,actionSecond,actionThird,0,0,0};row.action.spell=actionFirst;
        definitions.push_back(row);
    }
    std::vector<Condition> gates(6);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){uint32 group;assert(conditions>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);
        conditionManager.SmartEventConditionStore[{101148,0}][group].push_back(&gate);}
    {
        Finale baseline;SmartScriptHolder legacy=baseline.Row(32);
        legacy.event_id=26;legacy.type=61;legacy.event.event_phase_mask=0;legacy.event.event_flags=1;legacy.link=27;
        baseline.owner.summonAvailable=false;baseline.script.ProcessEvent(legacy);
        assert(baseline.owner.summonAttempts==1&&!baseline.owner.summons&&baseline.script.casts==std::vector<uint32>({41062,201791})&&!baseline.script.lists);
        std::cout<<"BASELINE: actual native fixed creature summon returns null but LINK27/28 still requests disarm and conversation\n";
    }
    {
        Finale pending;pending.owner.summonAvailable=false;
        pending.Tick();assert(!pending.owner.summonAttempts);
        pending.Arm();assert(pending.script.mEventPhase==2&&pending.script.casts.empty());
        pending.Tick(1);assert(pending.owner.summonAttempts==1&&!pending.owner.summons&&pending.script.casts.empty()&&!pending.script.lists);
        pending.Tick(500);assert(pending.owner.summonAttempts==1);
        for(uint32 retry=0;retry<20;++retry)pending.Tick();
        assert(pending.owner.summonAttempts==21&&pending.script.mEventPhase==2&&pending.script.casts.empty()&&!pending.script.lists);
        pending.owner.summonAvailable=true;pending.Tick();
        assert(pending.owner.summons==1&&pending.script.mEventPhase==3&&pending.script.lists==1&&pending.script.casts==std::vector<uint32>({41062,201791}));
        SpellInfo repeated;pending.owner.AI()->SpellHit(&pending.soul,&repeated);
        for(uint32 repeat=0;repeat<20;++repeat)pending.Tick();
        assert(pending.owner.summons==1&&pending.script.lists==1&&pending.script.casts.size()==2);
    }
    {
        Finale waiting;deliverSoulHit=false;waiting.Arm();waiting.Tick(1);
        assert(waiting.owner.summons==1&&waiting.script.mEventPhase==3&&!waiting.script.lists&&waiting.script.casts.empty());
        for(uint32 repeat=0;repeat<20;++repeat)waiting.Tick();
        assert(waiting.owner.summons==1&&waiting.script.casts.empty());
        SpellInfo actual;waiting.owner.AI()->SpellHit(&waiting.soul,&actual);
        assert(waiting.script.lists==1&&waiting.script.casts==std::vector<uint32>({41062,201791}));
    }
    for(uint32 invalid=0;invalid<7;++invalid){Finale pending;pending.Arm();
        if(invalid==0)scenarioManager.scenario.step=6;
        if(invalid==1)scenarioManager.scenario.step=8;
        if(invalid==2)scenarioManager.scenario.id=1037;
        if(invalid==3)scenarioManager.present=false;
        if(invalid==4)pending.owner.mapId=1;
        if(invalid==5)pending.owner.alive=false;
        if(invalid==6)pending.owner.instanced=false;
        pending.Tick(1);assert(!pending.owner.summonAttempts&&pending.script.casts.empty()&&!pending.script.lists);
    }
    {
        Finale pending;pending.owner.summonAvailable=false;pending.Arm();pending.Tick(1);
        Creature unrelated;unrelated.identity=1;pending.owner.AI()->JustSummoned(&unrelated);
        assert(pending.script.mEventPhase==2);
        pending.script.OnReset();pending.Tick();assert(pending.script.mEventPhase==0&&pending.owner.summonAttempts==1);
        pending.owner.summonAvailable=true;pending.Arm();pending.Tick(1);
        assert(pending.owner.summons==1&&pending.script.lists==1);
    }
    std::cout<<"PASS: actual native phase/timer/conditions/fixed summon, TempSummon owner-before-child callbacks, summoned-unit/spell-hit admission, repeat/LINK and reset branches; null creation retries and dialogue waits for actual hit. Cast/effect/map allocation and list execution are controlled or separate regressions; not client traversal.\n";
}

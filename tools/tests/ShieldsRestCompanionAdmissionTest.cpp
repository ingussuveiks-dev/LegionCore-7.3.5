#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
enum { DATA_STAGE_2=1, SPELL_SUMMON_ORIK=210036, SPELL_SUMMON_CATO=210043 };
struct Scenario
{
    uint32 identity=1082,step=1;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instance) { assert(instance==42); return present?&scenario:nullptr; }
} manager;
auto* sScenarioMgr=&manager;
struct Map { uint32 GetInstanceId() const { return 42; } } map;
struct Base { virtual ~Base()=default; virtual void onScenarionNextStep(uint32) { } };
struct Instance : Base
{
    Map* instance=&map;
#ifdef BASELINE
    uint32 scenarioid=0;
#endif
    std::vector<uint32> casts;
    void DoCastSpellOnPlayers(uint32 spell) { casts.push_back(spell); }
#include "Step.inc"
};
int main()
{
    for(uint32 identity : {909u,1068u,1082u,0u,962u})
        for(uint32 actual=0;actual<7;++actual)
            for(uint32 notified=0;notified<7;++notified)
                for(bool present : {false,true})
                {
                    manager.scenario={identity,actual}; manager.present=present; Instance script;
                    Base* dispatch=&script; dispatch->onScenarionNextStep(notified);
#ifdef BASELINE
                    bool admitted=notified==1;
#else
                    bool admitted=present && identity==1082 && actual==1 && notified==1;
#endif
                    assert(script.casts==(admitted?std::vector<uint32>{210036,210043}:std::vector<uint32>{}));
                }
#ifdef BASELINE
    std::cout << "PASS pinneddd021ddb: zero cache admits both Paladin companion casts at numerical step1 for Warrior/Hunter/unrelated/missing scenarios. Controlled dispatcher, not rendered actors.\n";
#else
    manager.present=false; Instance script; script.onScenarionNextStep(1); assert(script.casts.empty());
    manager.present=true; manager.scenario={1068,1}; script.onScenarionNextStep(1); assert(script.casts.empty());
    manager.scenario={1082,1}; script.onScenarionNextStep(1);
    assert((script.casts==std::vector<uint32>{210036,210043}));
    manager.scenario={909,1}; script.onScenarionNextStep(1); assert(script.casts.size()==2);
    std::cout << "PASS:490 live identity/current-step/notification/presence cases through extracted virtual handler; missing/replaced scenarios fail closed, native Paladin spell order retained. Recipient/group/cast/rendering services not instantiated.\n";
#endif
}

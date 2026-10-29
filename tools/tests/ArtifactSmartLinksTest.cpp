#include <cstdint>
#include <cstdio>
#include <vector>
using uint32 = uint32_t;
constexpr uint32 SMART_EVENT_LINK = 61;
struct SmartScriptHolder
{
    uint32 event_id = 0, link = 0, action = 0, event = 0;
    uint32 GetActionType() const { return action; }
    uint32 GetEventType() const { return event; }
};
struct SmartScript
{
    std::vector<SmartScriptHolder> mEvents;
    unsigned errors = 0, executed = 0;
    SmartScriptHolder FindLinkedEvent(uint32 link);
    void ProcessAction(SmartScriptHolder const& e);
    void ProcessEvent(SmartScriptHolder const&, void*, uint32, uint32, bool, void*, void*) { ++executed; }
};
#define TC_LOG_ERROR(...) ++errors
#include "FindLinkedEvent.inc"
void SmartScript::ProcessAction(SmartScriptHolder const& e)
{
    void* unit = nullptr;
    uint32 var0 = 0, var1 = 0;
    bool bvar = false;
    void* spell = nullptr;
    void* gob = nullptr;
#include "LinkTail.inc"

int main()
{
    struct Case { char const* name; uint32 targetEvent, targetAction, link; bool target, invalid; };
    Case cases[] = {
        {"valid",61,33,1,true,false}, {"missing",61,33,1,false,true},
        {"hello",64,80,1,true,true}, {"ordinary",62,33,1,true,true},
        {"empty",61,0,1,true,true}, {"none",64,80,0,true,false},
        {"self",64,80,2,true,false}
    };
    for (auto const& c : cases)
    {
        SmartScript script;
        script.mEvents.push_back({2,c.link,33,62});
        if (c.target) script.mEvents.push_back({1,0,c.targetAction,c.targetEvent});
        script.ProcessAction(script.mEvents.front());
        bool invalid = script.errors != 0;
        if (invalid != c.invalid || script.errors > 1 || script.executed != unsigned(c.link == 1 && !invalid))
            return 1;
        std::printf("%s\t%u\t%u\t%u\t%u\t%u\n",c.name,c.targetEvent,c.targetAction,c.link,unsigned(c.target),unsigned(invalid));
    }
    return 0;
}

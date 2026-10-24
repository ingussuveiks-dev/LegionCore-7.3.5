#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>
using uint32=uint32_t;
using int32=int32_t;
enum { LINK_ACTION_DEFAULT=0 };
struct Request { void* target;uint32 spell;bool triggered;uint32 delay; };
struct Unit {
    std::vector<Request> requests;
    void CastSpell(Unit* target,uint32 spell,bool triggered,int item) { assert(item==0);requests.push_back({target,spell,triggered,0}); }
    void CastSpellDelay(Unit* target,uint32 spell,bool triggered,uint32 delay,int item) { assert(item==0);requests.push_back({target,spell,triggered,delay}); }
};
struct Link { int32 effect=0,duration=0;uint32 actiontype=0; };
void Dispatch(Unit* _caster,Unit* _target,Link const& link) {
    Link const& i=link;
    int32 spell_trigger=i.effect;
    int m_CastItem=0;
    std::set<uint32> spellBanList;
    switch(i.actiontype) {
#include "LinkedDefault.inc"
        default: assert(false);
    }
    assert(spellBanList==std::set<uint32>{201904});
}
int main(int argc,char** argv) {
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    Link actual;std::ifstream input(argv[1]);assert(input>>actual.effect>>actual.duration>>actual.actiontype);
    assert(actual.effect==201904&&actual.duration==0&&actual.actiontype==0);
    Unit player,explicitTarget;
    Dispatch(&player,nullptr,actual);Dispatch(&player,&player,actual);
    assert(player.requests.size()==2);
    for(auto const& request:player.requests)assert(request.target==&player&&request.spell==201904&&request.triggered&&!request.delay);
    Dispatch(&player,&explicitTarget,actual);
    assert(player.requests.back().target==&explicitTarget);
    Link delayed=actual;delayed.duration=1500;Dispatch(&player,nullptr,delayed);
    assert(player.requests.back().target==&player&&player.requests.back().delay==1500);
    std::cout<<"PASS: existing206456 type0/default link requests triggered201904 through the actual native positive default action, using explicit target or caster fallback. Requests are controlled; target selection, child cast success/SpellHit, Smart timed list and native credit are not simulated as completed.\n";
}

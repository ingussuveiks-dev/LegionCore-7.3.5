#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
#include "PhaseConstants.inc"
struct SmartScript {
    uint32 mEventPhase=0;
    bool IsInPhase(uint32)const;
    void SetPhase(uint32);
};
#include "PhaseMethods.inc"
int main(){
    SmartScript script;
#ifdef PHASE_BASELINE
    script.SetPhase(33);
    bool wraps=script.IsInPhase(1);
    std::cout<<"BASELINE: actual MSVC/x64 phase33 aliases phase1="<<wraps<<"; inherited zero/oversized shifts have no portable C++ guarantee. No client failure reproduced.\n";
    assert(wraps);
#else
    for(uint32 phase=0;phase<=SMART_EVENT_PHASE_COUNT;++phase){script.SetPhase(phase);
        for(uint32 mask=0;mask<(1u<<SMART_EVENT_PHASE_COUNT);++mask){
            bool expected=phase!=0&&(mask&(uint32_t(1)<<(phase?phase-1:0)))!=0;
            assert(script.IsInPhase(mask)==expected);
        }
    }
    for(uint32 phase:{13u,31u,32u,33u,63u,64u,0xffffffffu}){
        script.SetPhase(phase);assert(!script.IsInPhase(1)&&!script.IsInPhase(4095)&&!script.IsInPhase(0xffffffffu));
    }
    script.SetPhase(1);assert(script.IsInPhase(1)&&!script.IsInPhase(2));
    script.SetPhase(0);assert(!script.IsInPhase(1));
    std::cout<<"PASS: extracted native phase setter/predicate cover phase0/reset, all12 phases against all4096 native masks, and invalid/wrapping phases without undefined shifts.\n";
#endif
}

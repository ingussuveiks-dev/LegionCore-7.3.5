#define MONK_NINE_ARRIVAL_TEST
#include "MonkChenEarlyPresentationTest.cpp"
#include "VoiceBaseline.inc"
#include "VoiceMs.inc"

int main(){
 auto retained=RunEarlyPresentationCases();uint32 cases=0;
 {Run r;r.Death();MonkChenPresentationBeforeVoice::State old(&r.state());clockNow+=4000;old.UpdateRescue(4000);assert(Count(r.chen,4)==1&&4000<NativeVoiceMs);++cases;}
 std::cout<<"COUNTERFACTUAL pinned8b9ea7da4 actual controller requests next Chen line443ms before fully decoded native99965/62031/1384167 voice ends. No client clipping/listening claim.\n";
 for(uint32 mode=0;mode<4;++mode){Run r;r.Death();if(mode){assert(r.chen.AIM_Destroy());if(mode==2)r.chen.i_AI=new Chen(&r.chen);if(mode==3)r.chen.i_disabledAI=new Chen(&r.chen);}r.Tick(4000);NoEarly(r);r.Tick(NativeVoiceMs-4001);NoEarly(r);r.Tick(1);assert(Count(r.chen,4)==1);r.Tick(5000-NativeVoiceMs);Early(r);r.Tick(3000);assert(r.state().routeIssued&&Count(r.chen,5)==1);++cases;}
 // The existing shared/unscoped actor also retains its complete native queue,
 // including FunctionProcessor's inclusive deadline, with only this voice bound.
 {Run r;r.chen.spawn=362134;r.chen.i_AI->SetData(1,1);r.chen.Lifecycle(4000);assert(Count(r.chen,4)==0);r.chen.Lifecycle(NativeVoiceMs-4001);assert(Count(r.chen,4)==0);r.chen.Lifecycle(1);assert(Count(r.chen,4)==1);r.chen.Lifecycle(5000-NativeVoiceMs);assert(r.li.motion.follows==1);++cases;}
 std::cout<<"PASS "<<cases<<" independent voice-bound contexts plus retained "<<retained<<" early cases: actual fully decoded first voice completes before ordinary next-line issue; native5s unvoiced student reply and8s route unchanged. Late/coalesced frames/full client audio remain separate.\n";
}

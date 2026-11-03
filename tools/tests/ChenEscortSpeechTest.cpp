#define MONK_NINE_ARRIVAL_TEST
#include "MonkChenEarlyPresentationTest.cpp"
#include "EscortBeforeRescue.inc"
#include "EscortBeforePresentation.inc"
#include "EscortBeforeJorvinax.inc"
#include "EscortBeforeNineArrival.inc"
#include "EscortBeforeNineRescue.inc"
#include "EscortBeforeInstance.inc"
#include "EscortVoiceMs.inc"

static_assert(sizeof(MonkChenPresentation::State)==24, "Existing instance layout retained");
static_assert(offsetof(MonkChenPresentation::State,rescueSpokenAt)==20, "Actual clock uses original padding");

int main()
{
 uint32 cases=0;
 {Run r;instance_monk_intro_before_escort old(&r.map);r.map.script=&old;r.map.i_data=&old;old.OnCreatureCreate(&r.chen);old.OnCreatureCreate(&r.source);old.OnCreatureCreate(&r.hait);r.Death(&r.guardian);uint32 first=0,next=0;r.chen.onTalk=[&]{if(r.chen.talks.back().first==4)first=getMSTime();if(r.chen.talks.back().first==5)next=getMSTime();};clockNow+=9000;r.map.Update(9000);assert(old.chenRescue.routeIssued&&first&&next==first);++cases;}
 std::cout<<"COUNTERFACTUAL full pinnedf5122dd06 instance/controllers/native map: late9s issues Talk4 and Talk5 together, below decoded2125ms. Not client playback evidence.\n";
 for(uint32 late:{6000u,8000u,9000u,30000u})for(uint32 mode=0;mode<4;++mode)
 {Run r;r.Death(mode%2?&r.pet:&r.guardian);if(mode){assert(r.chen.AIM_Destroy());if(mode==2)r.chen.i_AI=new Chen(&r.chen);if(mode==3)r.chen.i_disabledAI=new Chen(&r.chen);}r.Tick(late);assert(Count(r.chen,4)==1&&Count(r.chen,5)==0);assert(r.state().routeIssued==(late>=8000));r.Tick(EscortVoiceMs-1);assert(Count(r.chen,5)==0);assert(r.state().routeIssued);r.Tick(1);assert(Count(r.chen,5)==1&&r.chen.talks.back().second==r.player.guid);assert(r.chen.motion.paths==std::vector<uint32>{9859005});for(uint32 n=0;n<100;++n)r.instance.Update(0);r.Tick(50000);assert(Count(r.chen,4)==1&&Count(r.chen,5)==1&&r.player.credits==std::vector<uint32>{46956}&&r.other.credits.empty());++cases;}
 {Run r;r.Death();r.Tick(4442);assert(!Count(r.chen,4));r.Tick(1);assert(Count(r.chen,4)==1);r.Tick(3556);assert(!r.state().routeIssued&&!Count(r.chen,5));r.Tick(1);assert(r.state().routeIssued&&Count(r.chen,5)==1);++cases;}
 for(uint32 gate=0;gate<5;++gate)
 {Run r;r.Death();if(gate==0)r.chen.world=false;if(gate==1)r.chen.predelete=true;if(gate==2)r.chen.spawn=362134;if(gate==3)r.chen.hidden=true;if(gate==4)r.map.creatures.erase(r.chen.guid);r.Tick(8000);assert(!Count(r.chen,4)&&!Count(r.chen,5)&&r.state().routeIssued==(gate==3));r.chen.world=true;r.chen.predelete=false;r.chen.spawn=362201;r.chen.hidden=false;r.map.creatures[r.chen.guid]=&r.chen;r.Tick(2000);if(gate==3){assert(!Count(r.chen,4)&&Count(r.chen,5)==1&&r.state().routeIssued);}else{assert(Count(r.chen,4)==1&&r.state().routeIssued&&!Count(r.chen,5));r.Tick(EscortVoiceMs-1);assert(!Count(r.chen,5));r.Tick(1);assert(Count(r.chen,5)==1);}assert(r.chen.motion.paths.size()==1);++cases;}
 // Deferred speech retains the original weak attempt, current actor and player
 // admission, without delaying or restarting the already installed route.
 for(uint32 gate=0;gate<11;++gate)
 {Run r;r.Death();r.Tick(9000);switch(gate){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.map.unloading=true;break;case 4:r.scenario.step=5;break;case 5:r.scenario.id=1;break;case 6:r.chen.hidden=true;break;case 7:r.chen.world=false;break;case 8:r.chen.predelete=true;break;case 9:r.chen.spawn=362134;break;case 10:r.map.creatures.erase(r.chen.guid);break;}r.Tick(EscortVoiceMs);assert(!Count(r.chen,5)&&r.chen.motion.paths.size()==1);++cases;}
 for(uint32 gate=0;gate<4;++gate)
 {Run r;r.Death();r.Tick(9000);if(gate==0)r.player.contact=false;if(gate==1)r.player.phase=false;if(gate==2)r.player.distance=101;if(gate==3)r.chen.world=false;r.Tick(EscortVoiceMs);assert(!Count(r.chen,5));r.player.contact=true;r.player.phase=true;r.player.distance=0;r.chen.world=true;r.Tick(1);assert(Count(r.chen,5)==1&&r.chen.motion.paths.size()==1);++cases;}
 for(uint32 mutation=0;mutation<4;++mutation)
 {Run r;r.Death();r.Tick(9000);r.chen.onTalk=[&]{if(r.chen.talks.back().first!=5)return;if(mutation==0)r.instance.Update(0);if(mutation==1)r.state().Cancel();if(mutation==2)r.player.quest.ScriptLifetime=std::make_shared<uint8>();if(mutation==3)r.map.creatures.erase(r.chen.guid);};r.Tick(EscortVoiceMs);r.chen.onTalk={};assert(Count(r.chen,5)==1&&!r.instance.chenPresentation.rescueUpdating&&r.chen.motion.paths.size()==1);++cases;}
 {Escort r;r.Death();r.Tick(9000);r.Arrive();r.Finished();r.Tick(EscortVoiceMs);assert(!Count(r.chen,5));++cases;}
 {Run r;r.Death();clockNow+=9000;r.state().Update(9000);assert(r.state().routeIssued&&!Count(r.chen,4));r.instance.Update(0);assert(!Count(r.chen,4)&&Count(r.chen,5)==1);++cases;}
 {Run r;r.Death();waypointMgr.available=false;r.Tick(9000);assert(!r.state().routeIssued&&!Count(r.chen,5));r.Tick(EscortVoiceMs);assert(!Count(r.chen,5));waypointMgr.available=true;r.Tick(2000);assert(r.state().routeIssued&&Count(r.chen,5)==1);++cases;}
 {Run r;r.Death();r.chen.motion.install=false;r.Tick(9000);assert(!r.state().routeIssued&&!Count(r.chen,5));r.Tick(EscortVoiceMs);assert(Count(r.chen,5)==1&&!r.state().routeIssued);r.chen.motion.install=true;r.Tick(2000);assert(r.state().routeIssued&&Count(r.chen,5)==1&&r.chen.motion.paths.size()==1);++cases;}
 {Run r;r.map.players.push_back({&r.other});r.Death(&r.guardian);r.Tick(9000);r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(EscortVoiceMs);assert(Count(r.chen,5)==1&&r.chen.talks.back().second==r.other.guid&&r.other.credits.empty()&&r.chen.motion.paths.size()==1);++cases;}
 {Run r;r.Death();r.Tick(9000);r.map.players.push_back({&r.other});r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(EscortVoiceMs);assert(!Count(r.chen,5)&&r.other.credits.empty());++cases;}
 {Run r;r.Death();r.Tick(9000);r.state().Cancel();r.Tick(50000);assert(!Count(r.chen,5)&&r.chen.motion.paths.size()==1);++cases;}
 {Run fresh;fresh.Death(&fresh.pet);fresh.Tick(4443);fresh.Tick(3557);assert(Count(fresh.chen,4)==1&&Count(fresh.chen,5)==1&&fresh.state().routeIssued);++cases;}
 {Run r;clockNow=0xFFFFFE00;r.Death();r.Tick(9000);assert(Count(r.chen,4)==1&&!Count(r.chen,5));r.Tick(EscortVoiceMs-1);assert(!Count(r.chen,5));r.Tick(1);assert(Count(r.chen,5)==1);++cases;}
 std::cout<<"PASS "<<cases<<" actual optional escort-speech contexts: full predecessor, ordinary/native2125 spacing, late frames, AI retirement, original pet/guardian owner/quest/actor, once/reentry/retry/arrival/cancel/rollover and unchanged8s movement. Injected missing route warning is paired with recovery. Shared unscoped late queue and client playback remain separate.\n";
 auto retained=RunEarlyPresentationCases();std::cout<<"PASS retained "<<retained<<" early plus outro/routes/arrival/rescue contexts.\n";
}

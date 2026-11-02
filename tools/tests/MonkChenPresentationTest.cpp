// Actual production actor/context/instance, native AI retirement, waypoint and
// map dispatch. Text, motion and world endpoints remain controlled services.
#define MONK_JORVINAX_TEST
#include "MonkChenRoutesTest.cpp"
#include "PresentationBaselineActor.inc"
#include "PresentationBaselineInstance.inc"

using PriorChen=npc_monk_chen_porter_presentation_baseline::npc_monk_chen_porter_presentation_baselineAI;

void NoOutro(Escort const& r)
{
    assert(Count(r.hait,10)==0 && Count(r.chen,8)==0 && Count(r.hait,11)==0 && r.chen.motion.points==0);
}

void Outro(Escort const& r)
{
    assert(Count(r.hait,10)==1 && Count(r.chen,8)==1 && Count(r.hait,11)==1 && r.chen.motion.points==1);
    assert(r.player.credits==(std::vector<uint32>{46956,46947}));
    for(auto const& line:r.chen.talks) if(line.first==8) assert(line.second==r.player.guid);
    for(auto const& line:r.hait.talks) if(line.first==11) assert(line.second==r.player.guid);
}

uint32 RunPresentationCases()
{
    std::cout<<std::unitbuf;
    uint32 retained=RunRoutesCases(),cases=0;
    // Full pinned predecessor, including the old instance, not a surrogate
    // timer model: the essential route survives, all four optional actions die.
    for(bool destroy:{false,true})
    {
        Escort r;instance_monk_intro_presentation_baseline previous(&r.map);
        r.map.script=&previous;delete r.chen.i_AI;r.chen.i_AI=new PriorChen(&r.chen);
        previous.OnCreatureCreate(&r.chen);previous.OnCreatureCreate(&r.source);previous.OnCreatureCreate(&r.hait);
        auto tick=[&](uint32 diff){clockNow+=diff;r.map.i_data=&previous;r.map.onUpdate=[&](uint32 elapsed){r.chen.Lifecycle(elapsed);};r.map.Update(diff);};
        r.Death(&r.pet);assert(previous.chenRescue.started);tick(8000);
        assert(previous.chenRescue.routeIssued);r.Endpoint();r.chen.motion.waypoint.OnArrived(r.chen);
        assert(previous.chenRescue.arrivalDone);
        if(destroy)assert(r.chen.AIM_Destroy());
        tick(3000);tick(4000);tick(4000);tick(4000);tick(1000);
        assert(r.hait.motion.paths==std::vector<uint32>{9859006});
        if(destroy)NoOutro(r);else Outro(r);
        std::cout<<(destroy?"COUNTERFACTUAL":"CONTROL")<<" full pinned6cb25d055 actor/instance, native AI destruction: optional3/7/11/16s lost="<<destroy<<", genuine arrival and original15s route retained; controlled endpoints, not client traversal.\n";
    }
    for(uint32 mode=0;mode<5;++mode)
    {
        Escort r;r.Begin();r.Arrive();r.Finished();
        if(mode==1||mode==2){assert(r.chen.AIM_Destroy());if(mode==2)r.chen.i_AI=new Chen(&r.chen);}
        if(mode==3){r.chen.i_disabledAI=r.chen.i_AI;r.chen.i_AI=nullptr;}
        if(mode==4)r.hait.i_AI=nullptr;
        r.Tick(2999);NoOutro(r);r.Tick(1);assert(Count(r.hait,10)==1&&Count(r.chen,8)==0);
        r.Tick(3999);assert(Count(r.chen,8)==0);r.Tick(1);assert(Count(r.chen,8)==1&&Count(r.hait,11)==0);
        r.Tick(3999);assert(Count(r.hait,11)==0);r.Tick(1);assert(Count(r.hait,11)==1);
        r.Tick(4000);Handoff(r);assert(r.chen.motion.points==0);r.Tick(999);assert(r.chen.motion.points==0);r.Tick(1);Outro(r);
        for(uint32 i=0;i<100;++i)r.instance.Update(100000);
        r.Tick(100000);Outro(r);Handoff(r);++cases;
    }
    // Neither the deadline nor raw phase/pose alone can supply an arrival.
    for(uint32 gate=0;gate<4;++gate)
    {
        Escort r;r.Begin();if(gate==1)r.state().arrivalDone=true;
        if(gate==2)r.state().handoffPlayer=r.player.guid;
        if(gate==3){r.scenario.step=5;r.scenario.progress[29070].Counter=1;}
        r.Tick(100000);NoOutro(r);++cases;
    }
    for(uint32 gate=0;gate<13;++gate)
    {
        Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());
        switch(gate)
        {
            case 0:r.player.quest.ScriptLifetime.reset();break;
            case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;
            case 2:r.player.map=nullptr;break;
            case 3:r.map.unloading=true;break;
            case 4:r.player.status=0;break;
            case 5:r.player.reward=true;break;
            case 6:r.player.gm=true;break;
            case 7:r.player.world=false;break;
            case 8:r.player.contact=false;break;
            case 9:r.player.teleport=true;break;
            case 10:r.player.phase=false;break;
            case 11:r.player.distance=101;break;
            case 12:r.chen.hidden=true;break;
        }
        r.Tick(20000);NoOutro(r);++cases;
    }
    for(uint32 gate=0;gate<5;++gate)
    {
        Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());
        switch(gate){case 0:r.map.creatures.erase(r.chen.guid);break;case 1:r.chen.world=false;break;case 2:r.chen.predelete=true;break;case 3:r.chen.deathState=CORPSE;break;case 4:r.chen.spawn=362134;break;}
        r.Tick(2999);NoOutro(r);
        r.map.creatures[r.chen.guid]=&r.chen;r.chen.world=true;r.chen.predelete=false;r.chen.deathState=ALIVE;r.chen.spawn=362201;r.scenario.step=5;
        r.Tick(1);assert(Count(r.hait,10)==1);r.Tick(4000);r.Tick(4000);r.Tick(5000);Outro(r);++cases;
    }
    {Escort r;r.Begin();r.Arrive();r.scenario.step=4;r.Tick(2999);r.scenario.step=5;r.Tick(20000);NoOutro(r);assert(r.state().members.empty());++cases;}
    for(uint32 gate=0;gate<6;++gate)
    {
        Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());
        switch(gate){case 0:r.map.creatures.erase(r.hait.guid);break;case 1:r.hait.world=false;break;case 2:r.hait.predelete=true;break;case 3:r.hait.deathState=CORPSE;break;case 4:r.hait.spawn=362105;break;case 5:r.chen.hait=nullptr;break;}
        r.Tick(3000);assert(Count(r.hait,10)==0);
        r.map.creatures[r.hait.guid]=&r.hait;r.hait.world=true;r.hait.predelete=false;r.hait.deathState=ALIVE;r.hait.spawn=362271;r.chen.hait=&r.hait;
        r.Tick(4000);r.Tick(4000);r.Tick(5000);
        // The native optional3s attempt is not replayed against a late actor.
        assert(Count(r.hait,10)==0&&Count(r.chen,8)==1&&Count(r.hait,11)==1&&r.chen.motion.points==1);++cases;
    }
    for(uint32 reentry=0;reentry<7;++reentry)
    {
        Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());
        r.hait.onTalk=[&]{if(r.hait.talks.back().first!=10)return;switch(reentry){case 0:r.instance.Update(100000);break;case 1:r.state().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:r.map.unloading=true;break;case 4:r.map.creatures.erase(r.chen.guid);break;case 5:r.scenario.step=7;break;case 6:r.map.creatures.erase(r.hait.guid);break;}};
        r.Tick(3000);r.hait.onTalk={};assert(Count(r.hait,10)==1&&!r.instance.chenPresentation.updating);
        r.Tick(13000);assert(Count(r.hait,10)==1);
        if(reentry==0)Outro(r);
        else if(reentry!=6){assert(Count(r.chen,8)==0&&Count(r.hait,11)==0&&r.chen.motion.points==0);}
        ++cases;
    }
    {Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());r.other.gm=true;r.chen.nearest=&r.other;r.Tick(16000);Outro(r);assert(r.other.credits.empty());++cases;}
    {Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());r.scenario.step=6;r.scenario.progress[29071].Counter=1;r.state().genuineBossDefeat=true;r.Tick(16000);Outro(r);++cases;}
    {Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());r.scenario.step=6;r.scenario.progress[29071].Counter=1;r.Tick(16000);NoOutro(r);++cases;}
    {Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());r.player.contact=false;r.Tick(16000);NoOutro(r);assert(!r.state().handoffPending);r.player.contact=true;r.Tick(1);assert(Count(r.hait,10)==0&&Count(r.chen,8)==0&&Count(r.hait,11)==0&&r.chen.motion.points==1);++cases;}
    {Escort r;r.Begin();r.map.i_data=&r.instance;r.map.onUpdate=[&](uint32){r.Arrive();};clockNow+=100000;r.map.Update(100000);NoOutro(r);assert(r.chen.AIM_Destroy());r.Tick(2999);NoOutro(r);r.Tick(1);assert(Count(r.hait,10)==1);++cases;}
    // All overdue actions retain the native3/7/11/15/16 ordering even when
    // the old AI is still installed and the next frame arrives very late.
    for(bool noAI:{false,true})
    {
        Escort r;r.Begin();r.Arrive();if(noAI)assert(r.chen.AIM_Destroy());
        std::vector<uint32> order;
        r.hait.onTalk=[&]{order.push_back(r.hait.talks.back().first);};
        r.chen.onTalk=[&]{order.push_back(r.chen.talks.back().first);};
        r.chen.motion.onPoint=[&]{order.push_back(99);};
        r.Tick(16000);assert(order==(std::vector<uint32>{10,8,11,12,13,99}));Outro(r);Handoff(r);++cases;
    }
    for(uint32 line:{8u,11u})for(uint32 reentry=0;reentry<6;++reentry)
    {
        Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());
        auto callback=[&]{switch(reentry){case 0:r.instance.Update(100000);break;case 1:r.state().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:r.map.unloading=true;break;case 4:r.map.creatures.erase(r.chen.guid);break;case 5:r.map.creatures.erase(r.hait.guid);break;}};
        r.chen.onTalk=[&]{if(r.chen.talks.back().first==line)callback();};
        r.hait.onTalk=[&]{if(r.hait.talks.back().first==line)callback();};
        r.Tick(3000);r.Tick(4000);r.Tick(4000);
        r.chen.onTalk={};r.hait.onTalk={};r.Tick(5000);
        assert(Count(r.hait,10)==1&&Count(r.chen,8)==1&&!r.instance.chenPresentation.updating);
        if(reentry==0)Outro(r);
        else if(reentry!=5)assert(r.chen.motion.points==0&&Count(r.hait,11)==(line==11?1u:0u));
        else assert(r.chen.motion.points==1&&Count(r.hait,11)==(line==11?1u:0u));
        ++cases;
    }
    for(uint32 reentry=0;reentry<4;++reentry)
    {
        Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());r.Tick(15000);
        r.chen.motion.onPoint=[&]{switch(reentry){case 0:r.instance.Update(100000);break;case 1:r.state().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:r.map.creatures.erase(r.chen.guid);break;}};
        r.Tick(1000);r.chen.motion.onPoint={};for(uint32 i=0;i<100;++i)r.instance.Update(100000);
        assert(r.chen.motion.points==1&&!r.instance.chenPresentation.updating);++cases;
    }
    {Escort r;r.map.players.push_back({&r.other});r.Begin();r.Arrive();assert(r.state().members.size()==2);r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.chen.nearest=&r.other;r.Tick(20000);NoOutro(r);assert(r.other.credits.empty());++cases;}
    {Escort r;r.Begin();clockNow=0xFFFFFE00;r.Arrive();assert(r.chen.AIM_Destroy());r.Tick(3000);NoOutro(r);r.Tick(1);assert(Count(r.hait,10)==1);r.Tick(4000);r.Tick(4000);r.Tick(5000);Outro(r);++cases;}
    {Escort r;r.Begin();r.Arrive();assert(r.chen.AIM_Destroy());r.chen.i_AI=new Chen(&r.chen);for(uint32 i=0;i<100;++i)r.actor()->MovementInform(WAYPOINT_MOTION_TYPE,5);r.Tick(16000);Outro(r);Handoff(r);assert(r.actor()->_presentationFunctions.SizeQueue()==0);++cases;}
    std::cout<<"PASS "<<cases<<" actual Chen outro ASAN contexts plus retained "<<retained<<" routes/50 arrival/70 rescue: full pinned AI-loss counterfactual, real3/7/11/16s clocks and before-route dispatch, original weak owner/full actors, AI recreation/no AI, native stage/phase/visibility/cancellation/reentry, missing optional guide once-only attempt and no stale greeting after original handoff. Point0 is a native command, not an arrival/progression receipt; ordinary mmap geometry separate. Client/full943/all36 remain OPEN.\n";
    return cases;
}
#ifndef MONK_CHEN_EARLY_PRESENTATION_TEST
int main(){RunPresentationCases();}
#endif

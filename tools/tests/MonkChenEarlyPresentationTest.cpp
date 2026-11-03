// Extracted production context, actors and instance; actual native AI/map
// dispatch. World/text/motion endpoints are controlled, not client traversal.
#define MONK_CHEN_EARLY_PRESENTATION_TEST
#include "MonkChenPresentationTest.cpp"
#include "EarlyBaselinePresentation.inc"
#include "EarlyBaselineActor.inc"
#include "EarlyBaselineInstance.inc"

using EarlyPriorChen=npc_monk_chen_porter_early_baseline::npc_monk_chen_porter_early_baselineAI;

std::vector<Creature*> Kids(Run& r){return {&r.li,&r.add,&r.kid3,&r.kid4};}
void NoEarly(Run& r)
{
    assert(Count(r.chen,4)==0);
    for(auto kid:Kids(r))assert(kid->motion.follows==0&&Count(*kid,0)==0);
}
void Early(Run& r)
{
    assert(Count(r.chen,4)==1);
    for(auto kid:Kids(r))
    {
        assert(kid->motion.follows==1&&Count(*kid,0)==1);
        assert(kid->motion.followTarget==&r.chen&&kid->motion.followDistance>=1&&kid->motion.followDistance<=4);
        assert(kid->motion.followAngle==-PET_FOLLOW_ANGLE&&kid->motion.followSlot==MOTION_SLOT_IDLE);
        assert(kid->talks.back().second.IsEmpty());
    }
    assert(r.player.credits==std::vector<uint32>{46956});
}

uint32 RunEarlyPresentationCases()
{
    auto retained=RunPresentationCases();uint32 cases=0;
    // Complete pinned predecessor plus actual AIM_Destroy: the essential8s
    // route survives while optional4/5s callbacks disappear. A surviving and
    // suspended old AI are controls. No failed ordinary run is called passing.
    for(uint32 mode=0;mode<4;++mode)
    {
        Run r;instance_monk_intro_early_baseline old(&r.map);r.map.script=&old;r.map.i_data=&old;
        delete r.chen.i_AI;r.chen.i_AI=new EarlyPriorChen(&r.chen);
        old.OnCreatureCreate(&r.chen);old.OnCreatureCreate(&r.source);old.OnCreatureCreate(&r.hait);
        r.Death(&r.pet);assert(old.chenRescue.started);
        assert(static_cast<EarlyPriorChen*>(r.chen.i_AI)->_presentationFunctions.SizeQueue()==3);
        if(mode==1||mode==2){assert(r.chen.AIM_Destroy());if(mode==2)r.chen.i_AI=new EarlyPriorChen(&r.chen);}
        if(mode==3){r.chen.i_disabledAI=r.chen.i_AI;r.chen.i_AI=nullptr;}
        r.map.onUpdate=[&](uint32 diff){r.chen.Lifecycle(diff);};
        auto tick=[&](uint32 diff){clockNow+=diff;r.map.Update(diff);};
        tick(4000);tick(1000);bool lost=mode==1||mode==2;
        if(lost)NoEarly(r);else Early(r);
        tick(3000);assert(old.chenRescue.routeIssued&&r.chen.motion.paths==std::vector<uint32>{9859005});
        assert(r.li.motion.follows==(lost?1u:2u)&&r.player.credits==std::vector<uint32>{46956});++cases;
    }
    std::cout<<"COUNTERFACTUAL pinned8d66b5e09 actual4/5 callback loss on AI retirement, surviving/suspended controls and native8s route retained.\n";
    for(uint32 mode=0;mode<6;++mode)
    {
        Run r;r.Death(mode%2?&r.pet:&r.guardian);r.Started();
        if(mode>0){assert(r.chen.AIM_Destroy());if(mode==2)r.chen.i_AI=new Chen(&r.chen);if(mode==3)r.chen.i_disabledAI=new Chen(&r.chen);if(mode==4){r.chen.i_AI=new Chen(&r.chen);r.chen.i_disabledAI=r.chen.i_AI;r.chen.i_AI=nullptr;}if(mode==5){r.chen.i_AI=new Chen(&r.chen);r.chen.IsAIEnabled=false;}}
        for(auto kid:Kids(r))kid->IsAIEnabled=false;
        r.Tick(4442);NoEarly(r);r.Tick(1);assert(Count(r.chen,4)==1);
        r.Tick(556);for(auto kid:Kids(r))assert(!kid->motion.follows);r.Tick(1);Early(r);
        for(uint32 i=0;i<100;++i){r.instance.Update(0);if(r.chen.i_AI)r.chen.i_AI->SetData(1,1);}Early(r);
        r.Tick(2999);assert(!r.state().routeIssued);r.Tick(1);
        assert(r.state().routeIssued&&r.chen.motion.paths==std::vector<uint32>{9859005}&&Count(r.chen,5)==1);
        assert(r.li.motion.follows==2&&r.add.motion.follows==1&&r.kid3.motion.follows==1&&r.kid4.motion.follows==1);
        r.Tick(100000);assert(r.li.motion.follows==2&&Count(r.chen,4)==1&&r.player.credits==std::vector<uint32>{46956});++cases;
    }
    for(uint32 gate=0;gate<12;++gate)
    {
        Run r;r.Death();assert(r.chen.AIM_Destroy());
        switch(gate){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.player.status=0;break;case 4:r.player.reward=true;break;case 5:r.player.slot=25;break;case 6:r.player.type=1;break;case 7:r.map.unloading=true;break;case 8:r.scenario.step=3;break;case 9:r.scenario.id=1;break;case 10:r.scenario.progress[29047].Counter=0;break;case 11:r.state().Cancel();break;}
        r.Tick(5000);NoEarly(r);assert(r.player.credits==std::vector<uint32>{46956});++cases;
    }
    // These NPC-only original actions remain independent of a temporary
    // player's contact/visibility gate after the real accepted death receipt.
    for(uint32 gate=0;gate<7;++gate)
    {
        Run r;r.Death();assert(r.chen.AIM_Destroy());
        switch(gate){case 0:r.player.world=false;break;case 1:r.player.contact=false;break;case 2:r.player.phase=false;break;case 3:r.player.teleport=true;break;case 4:r.player.gm=true;break;case 5:r.player.distance=1000;break;case 6:r.chen.nearest=&r.other;r.other.gm=true;break;}
        r.Tick(5000);Early(r);assert(r.other.credits.empty());++cases;
    }
    for(uint32 gate=0;gate<6;++gate)
    {
        Run r;r.Death();assert(r.chen.AIM_Destroy());
        switch(gate){case 0:r.map.creatures.erase(r.chen.guid);break;case 1:r.chen.world=false;break;case 2:r.chen.predelete=true;break;case 3:r.chen.deathState=CORPSE;break;case 4:r.chen.spawn=362134;break;case 5:r.chen.hidden=true;break;}
        r.Tick(5000);NoEarly(r);
        r.map.creatures[r.chen.guid]=&r.chen;r.chen.world=true;r.chen.predelete=false;r.chen.deathState=ALIVE;r.chen.spawn=362201;r.chen.hidden=false;
        r.Tick(1);Early(r);++cases;
    }
    for(uint32 gate=0;gate<13;++gate)
    {
        Run r;r.Death();assert(r.chen.AIM_Destroy());Map elsewhere;
        switch(gate){case 0:r.add.entry=1;break;case 1:r.add.spawn=1;break;case 2:r.add.map=&elsewhere;break;case 3:r.add.world=false;break;case 4:r.add.predelete=true;break;case 5:r.add.deathState=CORPSE;break;case 6:r.add.hidden=true;break;case 7:r.add.phaseVisible=false;break;case 8:r.add.distance=70.01f;break;case 9:r.add.owner=r.player.guid;break;case 10:r.add.owner={999,99};break;case 11:r.map.creatures.erase(r.add.guid);break;case 12:r.chen.adds.erase(r.chen.adds.begin()+1);break;}
        r.Tick(5000);assert(Count(r.chen,4)==1&&!r.add.motion.follows&&!Count(r.add,0));
        for(auto kid:{&r.li,&r.kid3,&r.kid4})assert(kid->motion.follows==1&&Count(*kid,0)==1);
        r.Tick(1);assert(!r.add.motion.follows&&!Count(r.add,0));++cases;
    }
    {Run r;r.Death();r.chen.adds.push_back(&r.add);r.chen.adds.push_back(&r.li);r.add.distance=70;r.Tick(5000);Early(r);++cases;}
    // The snapshot contains GUIDs only. Deleting a later student from an
    // earlier MoveFollow callback must not leave an actionable dangling ptr.
    {Run r;r.Death();auto kid=new Creature;kid->map=&r.map;kid->entry=98074;kid->spawn=362263;kid->guid={801,2};auto guid=kid->guid;r.chen.adds[1]=kid;r.map.creatures[guid]=kid;
     r.li.motion.onFollow=[&]{r.map.creatures.erase(guid);delete kid;kid=nullptr;};r.Tick(5000);r.li.motion.onFollow={};assert(!kid&&r.li.motion.follows==1&&r.kid3.motion.follows==1&&r.kid4.motion.follows==1);++cases;}
    for(uint32 source=0;source<3;++source)for(uint32 effect=0;effect<7;++effect)
    {
        Run r;r.Death();assert(r.chen.AIM_Destroy());
        auto callback=[&]{switch(effect){case 0:r.instance.Update(100000);break;case 1:r.state().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:r.map.unloading=true;break;case 4:r.map.creatures.erase(r.chen.guid);break;case 5:r.scenario.step=5;break;case 6:r.chen.hidden=true;break;}};
        if(source==0)r.chen.onTalk=[&]{if(r.chen.talks.back().first==4)callback();};
        if(source==1)r.li.motion.onFollow=callback;
        if(source==2)r.li.onTalk=callback;
        r.Tick(5000);r.chen.onTalk={};r.li.motion.onFollow={};r.li.onTalk={};
        assert(!r.instance.chenPresentation.rescueUpdating&&Count(r.chen,4)==1);
        if(effect==0)Early(r);
        else{assert(!r.add.motion.follows&&!r.kid3.motion.follows&&!r.kid4.motion.follows);assert(r.li.motion.follows==(source?1u:0u));assert(Count(r.li,0)==(source==2?1u:0u));}
        assert(r.player.credits==std::vector<uint32>{46956});++cases;
    }
    for(uint32 effect=0;effect<5;++effect)
    {
        Run r;r.Death();r.add.motion.onFollow=[&]{switch(effect){case 0:r.map.creatures.erase(r.add.guid);break;case 1:r.add.predelete=true;break;case 2:r.add.owner={999,99};break;case 3:r.add.phaseVisible=false;break;case 4:r.add.distance=71;break;}};
        r.Tick(5000);assert(r.add.motion.follows==1&&!Count(r.add,0)&&r.kid3.motion.follows==1&&r.kid4.motion.follows==1);++cases;
    }
    for(bool deadAI:{false,true})
    {
        Run r;r.Death();if(deadAI)assert(r.chen.AIM_Destroy());std::vector<uint32> order;
        r.chen.onTalk=[&]{order.push_back(r.chen.talks.back().first);};for(auto kid:Kids(r)){kid->motion.onFollow=[&]{order.push_back(100);};kid->onTalk=[&]{order.push_back(200);};}
        r.Tick(8000);assert(order==(std::vector<uint32>{4,100,200,100,200,100,200,100,200,100}));assert(r.state().routeIssued&&Count(r.chen,5)==0);r.Tick(2124);assert(Count(r.chen,5)==0);r.Tick(1);assert(Count(r.chen,5)==1);++cases;
    }
    // An already-issued route retires an overdue scold/follow rather than
    // ordering students back after the original8s transition.
    {Run r;r.Death();clockNow+=8000;r.state().Update(8000);assert(r.state().routeIssued);r.instance.Update(0);assert(Count(r.chen,4)==0&&r.add.motion.follows==0&&r.li.motion.follows==1);++cases;}
    {Run r;r.map.i_data=&r.instance;r.map.onUpdate=[&](uint32){r.Death(&r.guardian);};clockNow+=100000;r.map.Update(100000);NoEarly(r);r.Tick(4442);NoEarly(r);r.Tick(1);assert(Count(r.chen,4)==1);r.Tick(557);Early(r);++cases;}
    {Run r;clockNow=0xFFFFFE00;r.Death();assert(r.chen.AIM_Destroy());r.Tick(4443);NoEarly(r);r.Tick(1);assert(Count(r.chen,4)==1);r.Tick(557);Early(r);++cases;}
    {Run r;r.chen.i_AI->SetData(1,1);r.Tick(5000);NoEarly(r);assert(r.player.credits.empty());++cases;}
    // Only participants captured by the genuine native receipt can retain
    // these shared NPC effects. A later nearby player cannot inherit them.
    for(uint32 mode=0;mode<4;++mode)
    {
        Run r;if(mode!=3)r.map.players.push_back({&r.other});r.Death(&r.pet);
        assert(r.state().members.size()==(mode==3?1u:2u));
        r.player.quest.ScriptLifetime=std::make_shared<uint8>();
        if(mode==1)r.other.quest.ScriptLifetime=std::make_shared<uint8>();
        if(mode==2)r.player.map=nullptr;
        r.chen.nearest=&r.other;assert(r.chen.AIM_Destroy());r.Tick(5000);
        if(mode==0||mode==2)Early(r);else NoEarly(r);
        assert(r.other.credits.empty());++cases;
    }
    std::cout<<"PASS "<<cases<<" early Chen ASAN contexts plus retained "<<retained<<" outro/81 routes/50 arrival/70 rescue: actual4.443/5/8 ordering, AI destruction/replacement/disabled aliases, original accepted weak attempt, NPC phase/private/owner/distance/full GUID/student reread, deletion/reentry/cancellation, clock birth/rollover and exactly-once effects. No client/full943/all36 traversal claimed.\n";
    return cases;
}
#ifndef MONK_NINE_ARRIVAL_TEST
int main(){RunEarlyPresentationCases();}
#endif

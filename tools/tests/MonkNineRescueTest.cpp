// Production initial/late actor, instance/controllers and actual native
// owner, criteria gate, AI lifetime, map clock, point finalize/AI dispatcher.
// World lookup, spline/path construction and packet delivery are endpoints.
#define MONK_NINE_RESCUE_TEST
#include "MonkNineArrivalTest.cpp"
#include "InitialBaselineActor.inc"
using InitialOld=npc_nine_czi_rescue_baseline::npc_nine_czi_rescue_baselineAI;
struct Rescue:Visit
{
    Creature vizznak;CreatureAI vizznakAI{&vizznak};
    Rescue(){nine.spawn=362181;nine.y=2007.45f;scenario.step=1;scenario.progress.clear();vizznak.map=&map;vizznak.entry=97968;vizznak.spawn=362187;vizznak.guid={91,2};vizznak.i_AI=&vizznakAI;map.creatures[vizznak.guid]=&vizznak;instance.OnCreatureCreate(&nine);}
    auto& state(){return *instance.nineRescue;}
    void Die(Unit* killer=nullptr){vizznak.deathState=CORPSE;if(killer)killer->DispatchDeath(&vizznak);else instance.CreatureDies(&vizznak,nullptr);}
    void Tick(uint32 ms){TickNine(ms);}
    void FinishPoint(bool correct=true){assert(nine.motion.pointOwner);nine.x=3848.98f;nine.y=1945.78f;nine.z=correct?931.85f:940.f;nine.spline.done=true;nine.motion.pointOwner->Finalize(nine);nine.motion.current=nullptr;Tick(0);}
    void Ready(){if(!(state().ready&&state().arrived&&state().credited&&state().moved))std::cerr<<"Ready failure time="<<clockNow<<" started="<<state().started<<" credited="<<state().credited<<" moved="<<state().moved<<" arrived="<<state().arrived<<" ready="<<state().ready<<" stage="<<scenario.step<<" spline="<<nine.spline.GetId()<<" issued="<<state().splineId<<" arrivalTime="<<state().arrivedAt<<"\n";assert(state().ready&&state().arrived&&state().credited&&state().moved);assert(scenario.progress[29046].Counter==1&&scenario.step==2);assert(player.credits==std::vector<uint32>{46734}&&other.credits.empty());assert(nine.motion.jumps==1&&nine.motion.points==1&&(nine.flags&UNIT_NPC_FLAG_SPELLCLICK));assert(nine.casts==(std::vector<uint32>{193248,193721}));assert(Count(nine,3)==1&&Count(nine,4)==1&&Count(nine,5)==1&&Count(nine,6)==1);}
    void Complete(){Tick(3000);Tick(4000);FinishPoint();Tick(4000);Ready();}
};
int main()
{
    uint32 retained=RunNineArrivalCases(),cases=0;
    for(uint32 mode=0;mode<4;++mode)
    {
        Rescue r;delete r.nine.i_AI;r.nine.i_AI=new InitialOld(&r.nine);auto old=static_cast<InitialOld*>(r.nine.i_AI);
        if(mode==1)r.scenario.step=0;
        if(mode==2){r.other.status=0;r.nine.nearest=&r.other;}
        old->SetData(1,1);
        if(mode==3){assert(r.nine.AIM_Destroy());r.Tick(10000);assert(r.player.credits.empty()&&!r.nine.motion.points);}
        else{r.Tick(3000);if(mode==1)assert(!r.scenario.progress[29046].Counter);if(mode==2)assert(r.other.credits==std::vector<uint32>{46734}&&r.player.credits.empty());if(mode==0)assert(r.scenario.progress[29046].Counter==1);}
        ++cases;
    }
    std::cout<<"COUNTERFACTUAL pinnedcfe6cd302 complete initial actor: early future-tree event consumed, unrelated nearest recipient, actual AI destruction loses3s/7s; ordinary control passes.\n";
    for(uint32 killer=0;killer<5;++killer)for(uint32 ai=0;ai<5;++ai)
    {
        Rescue r;r.nine.nearest=&r.other;r.other.status=0;
        if(ai){assert(r.nine.AIM_Destroy());if(ai==2)r.nine.i_AI=new Nine(&r.nine);if(ai==3)r.nine.i_disabledAI=new Nine(&r.nine);if(ai==4){r.nine.i_AI=new Nine(&r.nine);r.nine.IsAIEnabled=false;}}
        Unit* source=killer==0?static_cast<Unit*>(&r.player):killer==1?&r.pet:killer==2?&r.guardian:killer==3?static_cast<Unit*>(&r.chen):nullptr;
        r.Die(source);assert(r.state().started&&r.player.credits.empty()&&Count(r.nine,3)==1&&r.nine.motion.jumps==1);
        r.Tick(2999);assert(r.player.credits.empty());r.Tick(1);assert(r.state().credited&&r.scenario.step==2&&!r.nine.motion.points);
        r.Tick(3999);assert(!r.nine.motion.points);r.Tick(1);assert(r.nine.motion.points==1&&!r.state().arrived&&!(r.nine.flags&1));
        auto point=dynamic_cast<PointMovementGenerator<Creature>*>(r.nine.motion.pointOwner.get());assert(point&&point->id==1&&point->path&&point->x==3848.98f&&point->y==1945.78f&&point->z==931.85f);
        r.FinishPoint();assert(r.state().arrived&&!r.state().ready&&Count(r.nine,5)==1);r.Tick(3999);assert(!(r.nine.flags&1));r.Tick(1);r.Ready();
        for(uint32 i=0;i<100;++i){r.Die(source);if(r.nine.i_AI){r.nine.i_AI->SetData(1,1);r.nine.i_AI->MovementInform(POINT_MOTION_TYPE,1);}r.Tick(10000);}r.Ready();++cases;
    }
    {Rescue r;r.scenario.step=0;r.Die(&r.pet);assert(r.state().pending&&!r.state().started);r.Tick(100000);assert(r.player.credits.empty()&&r.nine.casts.empty());r.scenario.step=1;r.Tick(0);assert(r.state().started);r.Complete();++cases;}
    for(uint32 gate=0;gate<19;++gate)
    {
        Rescue r;Map other;
        switch(gate){case 0:r.vizznak.spawn=1;break;case 1:r.vizznak.entry=1;break;case 2:r.vizznak.world=false;break;case 3:r.vizznak.predelete=true;break;case 4:r.vizznak.map=&other;break;case 5:r.player.status=0;break;case 6:r.player.reward=true;break;case 7:r.player.type=1;break;case 8:r.player.slot=25;break;case 9:r.player.alive=false;break;case 10:r.player.world=false;break;case 11:r.player.contact=false;break;case 12:r.player.gm=true;break;case 13:r.player.teleport=true;break;case 14:r.player.phase=false;break;case 15:r.player.distance=100.01f;break;case 16:r.vizznak.hidden=true;break;case 17:r.map.unloading=true;break;case 18:r.scenario.id=1;break;}
        r.Die(&r.player);r.Tick(20000);assert(!r.state().pending&&!r.state().started&&r.player.credits.empty()&&r.nine.casts.empty());++cases;
    }
    {Rescue r;r.instance.CreatureDies(&r.vizznak,&r.player);assert(!r.state().pending&&!r.state().started);r.ai()->SetData(1,1);r.ai()->MovementInform(POINT_MOTION_TYPE,1);r.Tick(20000);assert(r.nine.casts.empty()&&r.player.credits.empty()&&!(r.nine.flags&1));r.Die(&r.player);r.Complete();++cases;}
    for(uint32 mode=0;mode<4;++mode)
    {
        Rescue r;r.Die(&r.player);r.Tick(7000);assert(r.state().moved&&!r.state().arrived);
        if(mode==0){r.ai()->MovementInform(POINT_MOTION_TYPE,0);r.ai()->MovementInform(POINT_MOTION_TYPE,2);r.Tick(10000);assert(!r.state().arrived);r.FinishPoint();}
        if(mode==1){r.FinishPoint(false);r.Tick(10000);assert(!r.state().arrived);r.FinishPoint();}
        if(mode==2){r.nine.spline.done=false;r.nine.motion.pointOwner->Finalize(r.nine);r.Tick(10000);assert(!r.state().arrived);r.FinishPoint();}
        if(mode==3){assert(r.nine.AIM_Destroy());r.FinishPoint();}
        r.Tick(4000);r.Ready();++cases;
    }
    for(uint32 stage=0;stage<4;++stage)for(uint32 mode=0;mode<7;++mode)
    {
        Rescue r;r.Die(&r.player);if(stage>=1)r.Tick(3000);if(stage>=2)r.Tick(4000);if(stage>=3)r.FinishPoint();auto credits=r.player.credits;auto talks=r.nine.talks.size();auto casts=r.nine.casts.size();Scenario replacement=r.scenario;
        switch(mode){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.status=0;break;case 3:r.player.map=nullptr;break;case 4:r.scenario.step=0;break;case 5:manager.current=&replacement;break;case 6:r.map.unloading=true;break;}
        r.Tick(10000);assert(!r.state().started&&!r.state().pending&&!r.state().ready);assert(r.player.credits==credits&&r.nine.talks.size()==talks&&r.nine.casts.size()==casts&&!(r.nine.flags&1));++cases;
    }
    {Rescue r;r.player.accept=false;auto before=warnings;r.Die(&r.player);r.Tick(3000);assert(!r.state().credited&&warnings==before+1);r.Tick(1999);assert(r.player.credits.size()==1);r.player.accept=true;r.Tick(1);assert(r.state().credited&&r.player.credits.size()==2&&Count(r.nine,4)==1);r.Tick(2000);r.FinishPoint();r.Tick(4000);assert(r.state().ready&&r.nine.motion.points==1);++cases;}
    {Rescue r;r.Die(&r.player);r.Tick(3000);r.nine.transport=true;r.Tick(4000);assert(!r.state().moved);r.nine.transport=false;r.Tick(0);auto point=dynamic_cast<PointMovementGenerator<Creature>*>(r.nine.motion.pointOwner.get());assert(point);r.FinishPoint();r.Tick(4000);r.Ready();++cases;}
    {Rescue r;r.Die(&r.player);auto original=r.nine.guid;r.map.creatures.erase(original);Creature other;other.map=&r.map;other.entry=97774;other.spawn=362181;other.guid={92,2};r.map.creatures[other.guid]=&other;r.instance.OnCreatureCreate(&other);r.Tick(10000);assert(r.state().source==original&&!r.nine.motion.points&&other.casts.empty());r.map.creatures[original]=&r.nine;r.Tick(0);r.FinishPoint();r.Tick(4000);r.Ready();++cases;}
    {Rescue r;r.Die(&r.player);r.Complete();assert(r.nine.AIM_Destroy());r.nine.i_AI=new Nine(&r.nine);assert(!(r.nine.flags&1));r.Tick(0);r.Ready();++cases;}
    for(uint32 source=0;source<4;++source)for(uint32 effect=0;effect<4;++effect)
    {
        Rescue r;auto callback=[&]{switch(effect){case 0:r.instance.Update(100000);break;case 1:r.state().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:if(r.nine.i_AI)assert(r.nine.AIM_Destroy());break;}};
        if(source==0)r.nine.onCast=callback;
        if(source==1)r.nine.onTalk=callback;
        if(source==2)r.player.onCredit=callback;
        if(source==3)r.nine.motion.onPoint=callback;
        r.Die(&r.player);r.Tick(3000);r.Tick(4000);if(r.nine.motion.pointOwner)r.FinishPoint();r.Tick(4000);assert(!r.state().busy);if(effect==0||effect==3)r.Ready();else assert(!r.state().ready);++cases;
    }
    {Rescue r;r.player.status=0;r.ai()->MoveInLineOfSight(&r.player);assert(!Count(r.nine,1));r.player.status=3;r.ai()->MoveInLineOfSight(&r.pet);assert(!Count(r.nine,1));r.ai()->MoveInLineOfSight(&r.player);assert(Count(r.nine,1)==1);assert(r.nine.AIM_Destroy());r.nine.i_AI=new Nine(&r.nine);r.ai()->MoveInLineOfSight(&r.player);assert(Count(r.nine,1)==1);r.Die(&r.player);r.Complete();++cases;}
    {Rescue r;r.state().source.Clear();r.map.creatures.erase(r.nine.guid);r.Die(&r.guardian);assert(r.state().pending&&!r.state().started);r.Tick(100000);r.map.creatures[r.nine.guid]=&r.nine;r.instance.OnCreatureCreate(&r.nine);r.Tick(0);assert(r.state().started);r.Complete();++cases;}
    {Rescue r;r.map.players={{&r.other},{&r.player}};r.Die(&r.pet);assert(r.state().members.front().player==r.player.guid);r.Complete();++cases;}
    {Rescue r;r.Die(&r.player);r.Tick(7000);auto issued=r.state().splineId;r.nine.x=3848.98f;r.nine.y=1945.78f;r.nine.z=931.85f;r.nine.spline.done=true;++r.nine.spline.id;r.ai()->MovementInform(POINT_MOTION_TYPE,1);r.Tick(10000);assert(r.state().moved&&r.state().splineId==issued&&!r.state().arrived&&!r.state().ready&&!(r.nine.flags&1));++cases;}
    {Rescue r;r.nine.motion.install=false;auto before=warnings;r.Die(&r.player);r.Tick(7000);assert(!r.state().moved&&warnings==before+1);auto attempts=r.nine.motion.points;r.nine.motion.install=true;r.Tick(1999);assert(r.nine.motion.points==attempts);r.Tick(1);assert(r.state().moved&&r.nine.motion.points==attempts+1);r.FinishPoint();r.Tick(4000);assert(r.state().ready&&Count(r.nine,3)==1&&r.player.credits==std::vector<uint32>{46734});++cases;}
    {Rescue r;r.Die(&r.player);r.Tick(3000);r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(0);assert(!r.state().started);r.scenario.step=1;r.scenario.progress.clear();r.vizznak.deathState=ALIVE;r.instance.OnCreatureCreate(&r.vizznak);r.Die(&r.player);r.Tick(3000);assert(r.state().credited&&r.player.credits==(std::vector<uint32>{46734,46734})&&r.nine.motion.jumps==2);++cases;}
    {Rescue r;clockNow=0xFFFFFE00;r.Die(&r.player);r.Tick(3000);r.Tick(1);assert(r.state().credited);r.Tick(4000);assert(r.state().moved);r.FinishPoint();r.Tick(4000);assert(r.state().ready);++cases;}
    {Rescue r;r.player.status=0;r.Die(&r.player);assert(!r.state().pending&&!r.state().started);r.player.status=3;r.Die(&r.player);r.Tick(10000);assert(!r.state().started&&r.player.credits.empty());r.vizznak.deathState=ALIVE;r.instance.OnCreatureCreate(&r.vizznak);r.Die(&r.player);r.Complete();++cases;}
    std::cout<<"PASS "<<cases<<" initial Nine contexts plus retained "<<retained<<" destination/Chen contexts: genuine native death/player-pet-guardian ownership, weak attempt and Scenario identity, early prerequisite, actual46734 receipt, original3/7s and4s after native completed point1, AI absence/recreation, exactly-once/reentry/cancellation/retry/visibility/native point finalize. World/path/spline/packets controlled; actual client/full943/all36 remain OPEN.\n";
}

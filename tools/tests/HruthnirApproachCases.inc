    uint32 cases=0;
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u})for(uint32 fault=0;fault<14;++fault){
        Fixture f;f.player.faction=faction;f.player.status=status;f.Select();auto token=f.ai.magnarStartToken;
        f.actor.arrivalDistanceSq=100;f.ai.Arrive();f.actor.motion.type=IDLE_MOTION_TYPE;CreatureAI paused;
        switch(fault){
        case 0:f.actor.spline.finalized=false;break;case 1:f.actor.combat=true;break;
        case 2:f.actor.state=UNIT_STATE_NOT_MOVE;break;case 3:f.actor.prevented=true;break;
        case 4:f.actor.motion.type=5;break;case 5:f.player.world=false;break;case 6:f.player.teleport=true;break;
        case 7:f.actor.phase=false;break;case 8:f.player.startDistance=101;break;case 9:f.player.alive=false;break;
        case 10:f.actor.IsAIEnabled=false;break;case 11:f.actor.ai=&paused;f.actor.disabledAI=&f.ai;break;
        case 12:f.player.map=&otherMap;break;case 13:f.player.contact=false;break;
        }
        f.Tick(1000);assert(f.actor.motion.moves==1&&!f.actor.conversations&&!f.ai.magnarStartReached&&!*token);
        f.actor.spline.finalized=true;f.actor.combat=f.actor.prevented=false;f.actor.state=0;f.actor.phase=f.actor.IsAIEnabled=true;
        f.actor.motion.type=IDLE_MOTION_TYPE;f.actor.ai=&f.ai;f.actor.disabledAI=nullptr;
        f.player.world=f.player.alive=f.player.contact=true;f.player.teleport=false;f.player.map=&instanceMap;f.player.startDistance=1;
        f.Tick(999);assert(f.actor.motion.moves==1);f.Tick(1);assert(f.actor.motion.moves==2&&!f.actor.conversations&&!f.ai.magnarStartReached);
        f.Tick(1000);assert(f.actor.motion.moves==2); // The active request is never restarted.
        f.actor.arrivalDistanceSq=0;f.ai.Arrive();assert(f.actor.conversations==1&&f.ai.magnarStartReached);
        f.Tick(5999);assert(!f.bossAI.starts);f.Tick(1);assert(f.bossAI.starts==1&&*token);
        f.Tick(60000);assert(f.actor.motion.moves==2&&f.actor.conversations==1&&f.bossAI.requests==1);++cases;
    }
    for(uint32 fault=0;fault<7;++fault){
        Fixture f;f.Select();f.actor.motion.type=IDLE_MOTION_TYPE;
        if(fault==0)f.player.CancelAttempt();if(fault==1)++f.actor.request.generation;
        if(fault==2)scenarioManager.scenario.step=4;if(fault==3)f.player.rewarded=true;
        if(fault==4)f.player.status=0;if(fault==5)f.ai.Reset();if(fault==6)f.actor.alive=false;
        f.Tick(1000);assert(!f.ai.magnarStartToken&&!f.actor.conversations&&f.actor.motion.moves==1);
        f.Tick(60000);assert(!f.bossAI.starts&&!f.actor.m_Functions.SizeQueue());++cases;
    }
    for(uint32 remove=0;remove<2;++remove){
        Fixture f;auto live=std::make_unique<HruthnirAI>(&f.actor);f.actor.ai=live.get();
        live->magnarChamberStarted=true;live->magnarIntroInstanceGeneration=f.actor.request.generation;
        live->magnarOfferReady=true;live->magnarOfferToken=std::make_shared<bool>(true);f.Select(false);
        f.actor.motion.type=IDLE_MOTION_TYPE;
        f.actor.motion.reenter=[&]{if(remove){f.actor.ai=nullptr;live.reset();}else live->Reset();};
        f.Tick(1000);assert(f.actor.motion.moves==2);f.Tick(60000);
        assert(!f.actor.m_Functions.SizeQueue()&&!f.actor.conversations&&!f.bossAI.starts);++cases;
    }
    {
        Fixture f;f.Select();auto old=f.ai.magnarStartToken;f.ai.Reset();
        f.ai.magnarChamberStarted=true;f.ai.magnarIntroInstanceGeneration=f.actor.request.generation;
        f.ai.magnarOfferReady=true;f.ai.magnarOfferToken=std::make_shared<bool>(true);
        f.actor.flags|=UNIT_NPC_FLAG_GOSSIP;f.player.talk.menu._menuItems[0]={};f.Select(false);
        f.actor.motion.type=IDLE_MOTION_TYPE;f.Tick(1000);
        assert(f.actor.motion.moves==3&&f.ai.magnarStartToken!=old&&!f.actor.conversations);++cases;
    }
    for(uint32 fault=0;fault<3;++fault){
        Fixture f;f.Select();WaypointMovementGenerator<Creature> route;f.actor.motion.current=&route;f.actor.motion.type=WAYPOINT_MOTION_TYPE;
        if(fault==1)route.complete=false;if(fault==2)route.id=1;
        f.Tick(1000);assert(f.actor.motion.moves==uint32(fault?1:2)&&!f.actor.conversations);
        if(fault){route.complete=true;route.id=9859004;f.Tick(1000);assert(f.actor.motion.moves==2);}
        ++cases;
    }
    std::cout<<"PASS "<<cases<<" ASAN actual approach/selection/Unit queue cases:1s idle-or-finished-original-route native request, no active/root/cast/combat override, original owner/receipt/current-suspended identity, terminal cancellation/epoch and actual AI deletion during motion, old request rejection, native arrival required/full6s/no duplicated497 or boss action. Movement/world endpoints controlled, not client traversal.\n";


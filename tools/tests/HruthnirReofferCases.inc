    uint32 cases=0;
    auto prepare=[](Fixture&f){instanceMap.players={{&f.player}};otherMap.players.clear();f.Select();f.ai.Reset();f.actor.motion.type=IDLE_MOTION_TYPE;};
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u})for(uint32 fault=0;fault<27;++fault){
        Fixture f;prepare(f);f.player.faction=faction;f.player.status=status;CreatureAI paused;
        switch(fault){
        case 0:f.ai.magnarChamberStarted=false;break;case 1:f.actor.request.credit=false;break;case 2:++f.actor.request.generation;break;
        case 3:f.actor.request.pillikCompanion={99,1};break;case 4:scenarioManager.scenario.step=2;break;case 5:scenarioManager.scenario.identity=1068;break;
        case 6:f.actor.world=false;break;case 7:f.actor.alive=false;break;case 8:f.actor.IsAIEnabled=false;break;case 9:f.actor.ai=&paused;break;
        case 10:f.actor.combat=true;break;case 11:f.actor.state=UNIT_STATE_NOT_MOVE;break;case 12:f.actor.prevented=true;break;
        case 13:f.actor.spline.finalized=false;break;case 14:f.actor.motion.type=5;break;case 15:f.actor.boss=nullptr;break;
        case 16:f.boss.IsAIEnabled=false;break;case 17:f.bossAI.starts=1;break;case 18:f.player.alive=false;break;
        case 19:f.player.gm=true;break;case 20:f.player.status=0;break;case 21:f.player.rewarded=true;break;
        case 22:f.player.playerClass=2;break;case 23:f.player.map=&otherMap;break;case 24:f.actor.phase=false;break;
        case 25:f.player.teleport=true;break;case 26:f.player.startDistance=151;break;
        }
        f.ai.UpdateMagnarReoffer(0);assert(f.actor.motion.moves==1&&!f.ai.magnarOfferToken&&!f.actor.conversations&&!f.bossAI.requests);
        f.ai.magnarChamberStarted=true;f.actor.request.credit=true;f.actor.request.generation=1;f.actor.request.pillikCompanion=f.actor.GetGUID();
        scenarioManager={};f.actor.world=f.actor.alive=f.actor.IsAIEnabled=f.actor.phase=true;f.actor.ai=&f.ai;f.actor.combat=f.actor.prevented=false;f.actor.state=0;f.actor.spline.finalized=true;f.actor.motion.type=IDLE_MOTION_TYPE;f.actor.boss=&f.boss;f.boss.IsAIEnabled=true;f.bossAI.starts=0;
        f.player.alive=true;f.player.gm=f.player.rewarded=f.player.teleport=false;f.player.status=status;f.player.playerClass=1;f.player.map=&instanceMap;f.player.startDistance=1;
        f.ai.UpdateMagnarReoffer(1000);assert(f.actor.motion.moves==2&&!f.ai.magnarOfferToken&&!f.actor.conversations&&!f.bossAI.requests);++cases;
    }
    for(uint32 claim=0;claim<4;++claim){Fixture f;prepare(f);if(claim==0)f.ai.magnarOfferToken=std::make_shared<bool>(false);if(claim==1)f.ai.magnarOfferToken=std::make_shared<bool>(true);if(claim==2)f.ai.magnarArrivalPending=true;if(claim==3)f.ai.magnarStartToken=std::make_shared<bool>(false);f.ai.UpdateMagnarReoffer(60000);assert(f.actor.motion.moves==1);++cases;}
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u}){
        Fixture f;prepare(f);f.player.faction=faction;f.player.status=status;
        f.ai.UpdateAI(0);assert(f.actor.motion.moves==2&&!f.ai.magnarOfferToken);
        f.ai.UpdateAI(999);assert(f.actor.motion.moves==2);f.ai.UpdateAI(1);assert(f.actor.motion.moves==2);
        f.ai.OfferArrive();auto offer=f.ai.magnarOfferToken;assert(offer&&!f.ai.magnarOfferReady&&f.ai.texts==std::vector<uint32>{15});
        f.Tick(12999);assert(!f.ai.magnarOfferReady);f.Tick(1);assert(f.ai.GetData(1061)&&*offer&&f.ai.texts==std::vector<uint32>({15,16}));
        f.player.talk.menu._menuItems[0]={};f.Select(false);assert(f.ai.magnarStartToken&&f.ai.magnarArrivalPending&&f.actor.motion.moves==3);
        f.ai.Arrive();f.Tick(5999);assert(!f.bossAI.starts);f.Tick(1);assert(f.bossAI.starts==1&&f.actor.conversations==1);++cases;
    }
    {Fixture f;prepare(f);f.actor.motion.reenter=[&]{f.ai.Reset();};f.ai.UpdateAI(0);assert(f.actor.motion.moves==2&&!f.actor.conversations);++cases;}
    {Fixture f;prepare(f);auto live=std::make_unique<HruthnirAI>(&f.actor);f.actor.ai=live.get();live->magnarChamberStarted=true;live->magnarIntroInstanceGeneration=f.actor.request.generation;f.actor.motion.reenter=[&]{f.actor.ai=nullptr;live.reset();};live->UpdateAI(0);assert(!live&&f.actor.motion.moves==2);++cases;}
    for(uint32 fault=0;fault<3;++fault){Fixture f;prepare(f);WaypointMovementGenerator<Creature> route;route.complete=fault!=1;route.id=fault==2?1:9859004;f.actor.motion.type=WAYPOINT_MOTION_TYPE;f.actor.motion.current=&route;f.ai.UpdateMagnarReoffer(0);assert(f.actor.motion.moves==uint32(fault?1:2));++cases;}
    {Fixture f;prepare(f);f.ai.magnarOfferToken=std::make_shared<bool>(true);f.ai.magnarOfferReady=true;f.ai.UpdateMagnarReoffer(0);assert(f.actor.motion.moves==1&&f.ai.magnarOfferReady);++cases;}
    {Fixture f;prepare(f);f.ai.magnarOfferToken=std::make_shared<bool>(true);auto old=f.ai.magnarOfferToken;f.ai.magnarOfferReady=true;f.actor.flags|=UNIT_NPC_FLAG_GOSSIP;f.actor.offerDistanceSq=100;f.ai.UpdateMagnarReoffer(0);assert(f.actor.motion.moves==2&&!f.ai.magnarOfferToken&&!f.ai.magnarOfferReady&&!(f.actor.flags&UNIT_NPC_FLAG_GOSSIP));f.actor.offerDistanceSq=0;f.ai.OfferArrive();assert(f.ai.magnarOfferToken!=old);f.Tick(12999);assert(!f.ai.GetData(1061));f.Tick(1);assert(f.ai.GetData(1061));++cases;}
    {Fixture f;prepare(f);f.ai.magnarStartToken=std::make_shared<bool>(true);auto token=f.ai.magnarStartToken;f.ai.magnarStartedBoss=f.boss.GetGUID();recreationReceipt=false;f.ai.UpdateMagnarReoffer(0);assert(f.ai.magnarStartToken==token&&f.actor.motion.moves==1);recreationReceipt=true;f.ai.UpdateMagnarReoffer(999);assert(f.ai.magnarStartToken==token);f.ai.UpdateMagnarReoffer(1);assert(!f.ai.magnarStartToken&&!f.ai.magnarOfferToken&&f.actor.motion.moves==1);f.ai.UpdateMagnarReoffer(0);assert(f.actor.motion.moves==2&&!f.ai.magnarOfferToken);recreationReceipt=false;++cases;}
    {Fixture f;prepare(f);auto live=std::make_unique<HruthnirAI>(&f.actor);f.actor.ai=live.get();live->magnarChamberStarted=true;live->magnarIntroInstanceGeneration=f.actor.request.generation;live->magnarStartInstance=42;live->magnarStartToken=std::make_shared<bool>(false);assert(live->StartMagnarConversation());auto old=conversationPool.objects.back();*live->magnarStartToken=true;live->magnarStartedBoss=f.boss.GetGUID();old->onRemove=[&]{f.actor.ai=nullptr;live.reset();};recreationReceipt=true;live->UpdateAI(0);assert(!live&&!old->world&&f.actor.motion.moves==1);recreationReceipt=false;++cases;}
    std::cout<<"PASS "<<cases<<" ASAN actual reoffer/packet/FunctionProcessor cases: native chamber/epoch/original actor/eligible participant/boss admission,1s retry/active movement and claims retained,published drift retires only old offer,original point6/12999+1ms then real cached option/5999+1ms start,actual AI deletion/Reset during movement; service/world endpoints controlled,not client UI/full chain.\n";

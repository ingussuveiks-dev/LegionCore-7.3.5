    uint32 cases=0;
    auto fresh=[](Fixture&f){f.ai.magnarChamberStarted=false;f.ai.magnarIntroInstanceGeneration=0;f.ai._introDone=false;f.actor.request.chamberDelivered=true;instanceMap.players={{&f.player}};};
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u})for(uint32 fault=0;fault<25;++fault){
        Fixture f;fresh(f);f.player.faction=faction;f.player.status=status;CreatureAI foreign;
        switch(fault){
        case 0:f.ai.magnarChamberStarted=true;break;case 1:f.ai.magnarIntroPending=true;break;case 2:f.ai.magnarIntroFinished=true;break;
        case 3:f.ai.magnarArrivalPending=true;break;case 4:f.ai.magnarStartToken=std::make_shared<bool>(false);break;case 5:f.ai.magnarOfferToken=std::make_shared<bool>(false);break;case 6:f.ai.magnarIntroInstanceGeneration=7;break;
        case 7:f.actor.world=false;break;case 8:f.actor.alive=false;break;case 9:f.actor.despawn=true;break;case 10:f.actor.entry=1;break;case 11:f.actor.mapId=1;break;case 12:f.actor.instance=43;break;case 13:f.actor.map=nullptr;break;
        case 14:f.actor.IsAIEnabled=false;break;case 15:f.actor.ai=&foreign;break;case 16:f.actor.instanceScript=nullptr;break;
        case 17:f.actor.request.chamberDelivered=false;break;case 18:f.actor.request.credit=false;break;case 19:f.actor.request.pillikCompanion={99,1};break;
        case 20:scenarioManager.available=false;break;case 21:scenarioManager.scenario.identity=1068;break;case 22:scenarioManager.scenario.step=2;break;case 23:scenarioManager.scenario.step=4;break;
        case 24:f.ai.magnarStartToken=std::make_shared<bool>(true);break;
        }
        auto epoch=f.ai.magnarIntroInstanceGeneration;bool chamber=f.ai.magnarChamberStarted;auto start=f.ai.magnarStartToken,offer=f.ai.magnarOfferToken;
        f.ai.RestoreMagnarChamber();assert(f.ai.magnarChamberStarted==chamber&&f.ai.magnarIntroInstanceGeneration==epoch&&!f.ai._introDone&&f.ai.magnarStartToken==start&&f.ai.magnarOfferToken==offer&&!f.actor.motion.moves&&!f.actor.conversations&&!f.bossAI.requests);++cases;
    }
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u}){
        Fixture f;fresh(f);f.player.faction=faction;f.player.status=status;f.ai.RestoreMagnarChamber();assert(f.ai.magnarChamberStarted&&f.ai._introDone&&f.ai.HasMagnarChamberRequest()&&!f.actor.motion.moves&&!f.actor.conversations&&!f.bossAI.requests);
        f.ai.RestoreMagnarChamber();++f.actor.request.generation;f.ai.RestoreMagnarChamber();assert(!f.ai.HasMagnarChamberRequest()&&f.ai.magnarIntroInstanceGeneration==1);--f.actor.request.generation;
        f.ai.UpdateAI(0);assert(f.actor.motion.moves==1&&!f.ai.magnarOfferToken);f.ai.OfferArrive();f.Tick(12999);assert(!f.ai.GetData(1061));f.Tick(1);assert(f.ai.GetData(1061));
        f.player.talk.menu._menuItems[0]={};f.Select(false);f.ai.Arrive();f.Tick(5999);assert(!f.bossAI.starts);f.Tick(1);assert(f.bossAI.starts==1&&f.actor.conversations==1);++cases;
    }
    for(uint32 wait=0;wait<6;++wait){Fixture f;fresh(f);switch(wait){case 0:f.player.alive=false;break;case 1:f.bossAI.starts=1;break;case 2:f.actor.boss=nullptr;break;case 3:f.actor.state=UNIT_STATE_NOT_MOVE;break;case 4:f.actor.motion.type=5;break;case 5:f.player.gm=true;break;}
        f.ai.UpdateAI(0);assert(f.ai.HasMagnarChamberRequest()&&f.ai._introDone&&!f.actor.motion.moves&&!f.actor.conversations);
        f.player.alive=true;f.player.gm=false;f.bossAI.starts=0;f.actor.boss=&f.boss;f.actor.state=0;f.actor.motion.type=IDLE_MOTION_TYPE;f.ai.UpdateAI(1000);assert(f.actor.motion.moves==1);++cases;
    }
    std::cout<<"PASS fresh chamber actual-method cases="<<cases<<": completed instance receipt/exact original companion/current AI/scenario/local claim guards,metadata-only adoption,unchanged epoch on repeat,real13s offer and cached option/6s start; controlled world endpoints,actual LOS/native lifecycle separately tested.\n";

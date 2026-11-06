    uint32 cases=0;
    auto prepare=[](Fixture& f){
        scenarioManager={};scenarioManager.scenario.step=1;
        instanceMap.players={{&f.player}};otherMap.players.clear();
        f.actor.request.pillikDead=true;f.actor.request.pillikOwner=f.player.GetGUID();
        f.boss.alive=false;f.boss.entry=94714;f.ai.QueuePillikHandoff();f.ai.ArmPillikHandoff();
        assert(f.ai.pillikProgress->approached);
    };
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u})for(uint32 fault=0;fault<19;++fault){
        Fixture f;prepare(f);f.player.faction=faction;f.player.status=status;CreatureAI other;
        switch(fault){
        case 0:f.actor.request.pillikDead=false;break;
        case 1:f.actor.world=false;break;
        case 2:f.actor.alive=false;break;
        case 3:f.actor.entry=1;break;
        case 4:f.actor.mapId=1;break;
        case 5:f.actor.map=&otherMap;break;
        case 6:f.actor.IsAIEnabled=false;break;
        case 7:f.actor.ai=&other;break;
        case 8:f.actor.phase=false;break;
        case 9:f.player.contact=false;break;
        case 10:f.player.status=0;break;
        case 11:f.player.rewarded=true;break;
        case 12:f.player.playerClass=2;break;
        case 13:f.player.startDistance=101;break;
        case 14:++f.actor.request.generation;break;
        case 15:scenarioManager.scenario.identity=1068;break;
        case 16:scenarioManager.scenario.step=0;break;
        case 17:f.actor.request.pillikCompanion={99,1};break;
        case 18:f.boss.alive=true;break;
        }
        f.ai.TryPillikHandoff();assert(!f.player.pillikCredits&&!f.actor.motion.moves&&f.ai.texts.empty());
        f.actor.request.pillikDead=true;f.actor.world=f.actor.alive=f.actor.IsAIEnabled=f.actor.phase=true;
        f.actor.entry=96468;f.actor.mapId=1495;f.actor.map=&instanceMap;f.actor.ai=&f.ai;
        f.player.contact=true;f.player.status=status;f.player.rewarded=false;f.player.playerClass=1;f.player.startDistance=1;
        f.actor.request.generation=1;f.actor.request.pillikCompanion=f.actor.GetGUID();f.boss.alive=false;
        scenarioManager={};scenarioManager.scenario.step=1;
        f.ai.TryPillikHandoff();f.ai.TryPillikHandoff();f.ai.UpdatePillikHandoff(30000);
        assert(f.player.pillikCredits==1&&f.actor.motion.moves==1&&f.ai.texts==std::vector<uint32>{5}&&f.ai.pillikProgress->complete);
        ++cases;
    }
    for(uint32 mode=0;mode<3;++mode){
        Fixture f;prepare(f);CreatureAI temporary;
        if(mode==0)f.actor.IsAIEnabled=false;
        if(mode==1){f.actor.ai=&temporary;f.actor.disabledAI=&f.ai;}
        if(mode==2){f.actor.ai=&temporary;f.actor.disabledAI=nullptr;}
        f.ai.pillikProgress=std::make_shared<HruthnirAI::PillikProgress>();f.ai.QueuePillikHandoff();f.ai.ArmPillikHandoff();
        assert(f.ai.pillikProgress->approached==(mode!=2));
        f.actor.ai=&f.ai;f.actor.disabledAI=nullptr;f.actor.IsAIEnabled=true;
        f.ai.TryPillikHandoff();assert(f.player.pillikCredits==uint32(mode!=2));++cases;
    }
    {
        Fixture f;prepare(f);f.player.pillikCreditWorks=false;auto warnings=pillikWarnings;
        f.ai.UpdatePillikHandoff(0);assert(f.player.pillikCredits==1&&pillikWarnings==warnings+1&&!f.ai.pillikProgress->complete);
        f.ai.UpdatePillikHandoff(499);assert(f.player.pillikCredits==1);f.ai.UpdatePillikHandoff(1);
        assert(f.player.pillikCredits==2&&pillikWarnings==warnings+1&&f.actor.motion.moves==1);
        f.player.pillikCreditWorks=true;f.ai.UpdatePillikHandoff(500);
        assert(f.player.pillikCredits==3&&f.ai.pillikProgress->complete&&f.ai.texts.size()==1);++cases;
    }
    for(uint32 callback=0;callback<3;++callback){
        Fixture f;prepare(f);auto reenter=[&]{f.ai.TryPillikHandoff();};
        if(callback==0)f.ai.onTalk=reenter;if(callback==1)f.actor.motion.reenter=reenter;if(callback==2)f.player.onPillikCredit=reenter;
        f.ai.TryPillikHandoff();assert(f.player.pillikCredits==1&&f.actor.motion.moves==1&&f.ai.texts.size()==1);++cases;
    }
    for(uint32 callback=0;callback<3;++callback){
        Fixture f;prepare(f);auto live=std::make_unique<HruthnirAI>(&f.actor);f.actor.ai=live.get();live->QueuePillikHandoff();live->ArmPillikHandoff();auto state=live->pillikProgress;
        auto remove=[&]{f.actor.ai=nullptr;live.reset();};
        if(callback==0)live->onTalk=remove;if(callback==1)f.actor.motion.reenter=remove;if(callback==2)f.player.onPillikCredit=remove;
        live->UpdateAI(0);assert(!live&&!state->busy&&f.player.pillikCredits==uint32(callback==2));++cases;
    }
    {
        Fixture f;prepare(f);Player closer;closer.startDistance=0.1f;instanceMap.players={{&closer},{&f.player}};
        f.ai.TryPillikHandoff();assert(f.player.pillikCredits==1&&!closer.pillikCredits);++cases;
    }
    {
        Fixture f;prepare(f);Player peer;f.player.contact=false;instanceMap.players={{&f.player},{&peer}};
        f.ai.TryPillikHandoff();assert(peer.pillikCredits==1&&!f.player.pillikCredits);++cases;
    }
    {
        Fixture f;prepare(f);scenarioManager.scenario.step=2;f.ai.TryPillikHandoff();
        assert(!f.player.pillikCredits&&f.actor.motion.moves==1&&f.ai.pillikProgress->complete);++cases;
    }
    for(uint32 mode=0;mode<3;++mode){
        Fixture f;prepare(f);f.ai.pillikProgress=std::make_shared<HruthnirAI::PillikProgress>();scenarioManager.scenario.step=0;
        f.ai.MovementInform(WAYPOINT_MOTION_TYPE,6);assert(f.player.pillikArrivals==1&&!f.ai.pillikProgress->approached);
        if(mode){
            f.ai._presentationFunctions.Update(1000);++f.actor.request.generation;
            if(mode==1)f.ai.MovementInform(WAYPOINT_MOTION_TYPE,6);
            f.ai._presentationFunctions.Update(4000);f.ai.UpdateAI(0);assert(!f.player.pillikCredits&&!f.ai.pillikProgress->approached);
            f.ai._presentationFunctions.Update(1000);
        }else{
            f.ai._presentationFunctions.Update(4999);f.ai.UpdateAI(0);assert(!f.player.pillikCredits);
            f.ai._presentationFunctions.Update(1);
        }
        f.ai.UpdateAI(500);assert(f.player.pillikCredits==uint32(mode!=2)&&f.actor.motion.moves==uint32(mode!=2));++cases;
    }
    {Fixture f;prepare(f);f.player.alive=false;f.ai.TryPillikHandoff();assert(f.player.pillikCredits==1&&f.ai.pillikProgress->complete);++cases;}
    std::cout<<"PASS "<<cases<<" ASAN actual Pillik handoff contexts: recorded real death/current instance and original companion, native phase/quest/player admission and owner preference, stage0/1/2,500ms refused-credit retry, paused/suspended cue, exactly-once/reentrant movement/text and AI deletion. Service endpoints/earlier approach clock controlled; real Kill/criterion/route data tested separately, not client traversal.\n";

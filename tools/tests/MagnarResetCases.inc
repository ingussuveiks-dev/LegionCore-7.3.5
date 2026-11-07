    uint32 cases=0;
    auto started=[](Fixture&f){instanceMap.players={{&f.player}};f.Select();f.ai.Arrive();f.Tick(6000);assert(f.ai.magnarStartToken&&*f.ai.magnarStartToken&&f.ai.magnarStartedBoss==f.boss.GetGUID());};
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u})for(uint32 fault=0;fault<25;++fault){
        Fixture f;started(f);f.player.faction=faction;f.player.status=status;auto token=f.ai.magnarStartToken;auto story=f.ai.magnarConversation;f.bossAI.starts=0;CreatureAI foreign;
        switch(fault){
        case 0:*token=false;break;case 1:f.ai.magnarStartedBoss={22,1};break;
        case 2:f.actor.world=false;break;case 3:f.actor.alive=false;break;case 4:f.actor.despawn=true;break;
        case 5:f.actor.entry=1;break;case 6:f.actor.mapId=1;break;case 7:f.actor.ai=&foreign;break;
        case 8:f.boss.world=false;break;case 9:f.boss.alive=false;break;case 10:f.boss.despawn=true;break;case 11:f.boss.entry=1;break;
        case 12:f.boss.map=&otherMap;break;case 13:++f.boss.instance;break;case 14:f.boss.IsAIEnabled=false;break;case 15:f.boss.ai=nullptr;break;
        case 16:f.bossAI.starts=1;break;case 17:f.ai.magnarChamberStarted=false;break;case 18:f.actor.request.credit=false;break;case 19:++f.actor.request.generation;break;
        case 20:f.actor.instanceScript=nullptr;break;case 21:f.actor.request.pillikCompanion={9,1};break;case 22:scenarioManager.available=false;break;
        case 23:scenarioManager.scenario.identity=1068;break;case 24:scenarioManager.scenario.step=4;break;
        }
        f.ai.ResetMagnarOffer(&f.boss);assert(f.ai.magnarStartToken==token&&f.ai.magnarConversation==story&&f.actor.motion.moves==1);++cases;
    }
    for(uint32 faction:{0u,1u})for(uint32 status:{1u,3u})for(uint32 suspended:{0u,1u}){
        Fixture f;started(f);f.player.faction=faction;f.player.status=status;auto token=f.ai.magnarStartToken;f.bossAI.starts=0;
        CreatureAI other;if(suspended){f.actor.ai=&other;f.actor.disabledAI=&f.ai;f.actor.IsAIEnabled=false;}
        MagnarStart::ResetOffer(&f.actor,&f.boss);assert(!f.ai.magnarStartToken&&!f.ai.magnarStartedBoss&&!f.ai.magnarConversation&&*token&&f.actor.motion.moves==1);
        f.actor.ai=&f.ai;f.actor.disabledAI=nullptr;f.actor.IsAIEnabled=true;f.actor.motion.type=IDLE_MOTION_TYPE;
        MagnarStart::ResetOffer(&f.actor,&f.boss);f.ai.UpdateMagnarReoffer(0);assert(f.actor.motion.moves==2&&!f.ai.GetData(1061)&&!f.bossAI.starts);
        f.ai.OfferArrive();f.Tick(12999);assert(!f.ai.GetData(1061));f.Tick(1);assert(f.ai.GetData(1061));
        f.player.talk.menu._menuItems[0]={};f.Select(false);f.ai.Arrive();f.Tick(5999);assert(!f.bossAI.starts);f.Tick(1);assert(f.bossAI.starts==1&&f.actor.conversations==2&&*f.ai.magnarStartToken);++cases;
    }
    {Fixture f;started(f);f.bossAI.starts=0;f.ai.ResetMagnarOffer(nullptr);assert(f.ai.magnarStartToken);++cases;}
    // Actual owned Conversation::Remove may reenter or delete the controller.
    for(uint32 destroy:{0u,1u}){
        Fixture f;auto live=std::make_unique<HruthnirAI>(&f.actor);f.actor.ai=live.get();live->magnarChamberStarted=true;live->magnarIntroInstanceGeneration=1;
        live->magnarStartToken=std::make_shared<bool>(true);live->magnarStartedBoss=f.boss.GetGUID();live->magnarStartInstance=f.actor.instance;
        live->magnarStartOwner=f.player.GetGUID();live->magnarStartAttempt=f.player.GetQuestCancellationReceipt(39191);live->magnarStartPresented=true;
        auto story=new Conversation;assert(story->CreateConversation(++guidGenerator.current,497,&f.actor,nullptr,f.actor));live->magnarConversation=story->guid;
        story->onRemove=[&]{if(destroy){f.actor.ai=nullptr;live.reset();}else live->ResetMagnarOffer(&f.boss);};
        MagnarStart::ResetOffer(&f.actor,&f.boss);assert(!story->world);if(live){assert(!live->magnarStartToken);f.actor.ai=&f.ai;live.reset();}else f.actor.ai=&f.ai;++cases;
    }
    std::cout<<"PASS Magnar reset actual-method cases="<<cases<<": exact acknowledged boss GUID/current-or-suspended companion/guard epoch/scenario/lifecycle guards,original13s reoffer then cached option/6s,reentrant owned-story cleanup. Boss Reset and rendering separately native/client.\n";

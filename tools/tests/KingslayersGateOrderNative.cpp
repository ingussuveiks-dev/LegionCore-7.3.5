#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "SmallEvents.inc"
#include "GaronaOpening.inc"
int main(int argc,char** argv) { try {
    InitializeAltheaGuardsNative(argc,argv);uint32 cases=0;
    
#ifdef KINGSLAYERS_GATE_ORDER_BASELINE
    uint32 modes=5;
#else
    #ifdef KINGSLAYERS_GATE_ORDER_FAULTS
    uint32 modes=4;
#else
    uint32 modes=25;
#endif
#endif
    for(uint32 quest:{42504u,42627u}) for(uint32 mode=0;mode<modes;++mode) {
        MapInstanced parent(1620,300);InstanceMap map(1620,300,9966000+cases,DIFFICULTY_N_SCENARIO,&parent);
        map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(0);auto first=Session(),second=Session();
        {
            Position clayPose=mode==19?Position{-8484.5f,386.571f,115.942f,2.23474f}:mode==24?Position{-8846.73f,646.828f,96.5303f,2.05503f}:Position{-8953.95f,521.019f,96.5399f,3.83972f};Position playerPose{clayPose.GetPositionX()-10,clayPose.GetPositionY(),clayPose.GetPositionZ(),0};
            NativePlayer rogue(first,map,101,playerPose),visitor(second,map,102,playerPose);
            for(auto p:{&rogue,&visitor}) {p->SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(p->getRace());p->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,quest);}
            Actor clay(&map,108057,26001,clayPose);Require(map.AddToMap(static_cast<Creature*>(&clay)),"Actual Clay role not admitted");
            auto ai=new npc_kingslayers_small_probe::npc_kingslayers_small_eventsAI(&clay);Require(clay.AIM_Initialize(ai),"Whole Clay AI missing");
#ifdef KINGSLAYERS_GATE_ORDER_BASELINE
            if(mode<3) {
                if(mode==0)visitor.SetGameMaster(true);if(mode==1)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==2){visitor.SetQuestStatus(quest,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}
                ai->MoveInLineOfSight(&visitor);Require(ai->introDone&&Stories(map,2384)==1,"Pinned ineligible Clay consumption differs");
                ai->MoveInLineOfSight(&rogue);Require(Stories(map,2384)==1,"Pinned Clay replay differs");
                for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2384)Require(story->GetCasterGUID()==visitor.GetGUID(),"Pinned speech no longer belongs to first ineligible visitor");
                std::cout<<"COUNTERFACTUAL quest="<<quest<<" mode="<<mode<<" Clay2384 one-shot consumed for GM/wrong-class/questless visitor, real Rogue ignored\n";
            } else if(mode==3) {
                auto rows=const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(2384));Require(rows&&rows->size()==1,"Native2384 actor row missing");auto saved=(*rows)[0].creatureId;(*rows)[0].creatureId=1;
                ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&!Stories(map,2384),"Pinned failed factory no longer consumes Clay");(*rows)[0].creatureId=saved;
                ai->MoveInLineOfSight(&rogue);Require(!Stories(map,2384),"Pinned failed factory retried");
                std::cout<<"COUNTERFACTUAL quest="<<quest<<" Clay sets one-shot before actual refused native factory; restored native row cannot recover\n";
            } else {
                Position opening{-9101.9f,406.198f,92.7232f,.643928f};rogue.Relocate(opening);
                Actor garona(&map,108015,26002,opening),shaw(&map,98094,26003,{-9078.58f,423.865f,92.6397f,3.83066f});Require(map.AddToMap(static_cast<Creature*>(&garona))&&map.AddToMap(static_cast<Creature*>(&shaw)),"Native opening actors missing");
                auto intro=new npc_garona_opening_probe::npc_garona_halforcenAI(&garona);Require(garona.AIM_Initialize(intro),"Whole opening AI missing");
                std::cout<<"CONTEXT native visibility="<<rogue.GetVisibilityRange()<<" original Clay distance="<<rogue.GetExactDist(&clay)<<'\n';
                intro->MoveInLineOfSight(&rogue);intro->UpdateAI(40840);rogue.Relocate(playerPose);
                ai->MoveInLineOfSight(&rogue);Require(Stories(map,2384)==1,"Native Clay approach did not create ordinary speech");
                intro->UpdateAI(47679);Require(intro->openingComplete&&scenario->GetCurrentStep()==1&&Stories(map,2384)==2,"Pinned two independent2384 callers no longer duplicate");
                std::cout<<"COUNTERFACTUAL quest="<<quest<<" actual Clay LOS plus qualified opening completion publish two separate2384 objects to same Rogue\n";
            }
#else
#ifdef KINGSLAYERS_GATE_ORDER_FAULTS
            auto rows=const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(2384));Require(rows&&rows->size()==1,"Native2384 row missing");auto saved=(*rows)[0];
            if(mode==1)(*rows)[0].id=1;else(*rows)[0].creatureId=1;
            ai->SetGUID(rogue.GetGUID(),2384);Require(!ai->introDone&&!Stories(map,2384)&&ai->gateOrderRetry==1000,"Refused native factory consumed receipt or failed retry");
            ai->SetGUID(visitor.GetGUID(),2384);Require(ai->gateOrderRecipient==rogue.GetGUID(),"Qualified pending recipient stolen during cooldown");
            if(mode==1){ai->UpdateAI(1000);Require(!ai->introDone&&!Stories(map,2384),"Repeated invalid native row admitted");}
            (*rows)[0]=saved;
            if(mode==2)rogue.SetQuestStatus(quest,QUEST_STATUS_NONE);
            if(mode==3)clay.SetPhaseMask(2,false);
            ai->Reset();ai->UpdateAI(999);Require(!ai->introDone&&!Stories(map,2384),"Native retry ran before1s floor");ai->UpdateAI(1);
            if(mode==2||mode==3){Require(!ai->introDone&&!Stories(map,2384)&&ai->gateOrderRecipient.IsEmpty(),"Ineligible pending recipient not cancelled");if(mode==3)clay.SetPhaseMask(1,false);ai->SetGUID((mode==2?visitor:rogue).GetGUID(),2384);}
            Require(ai->introDone&&Stories(map,2384)==1,"Restored native gate context failed to recover");
            for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2384)Require(story->GetCasterGUID()==(mode==2?visitor:rogue).GetGUID(),"Native recovered recipient differs");
#else
            if(mode<3) {
                if(mode==0)visitor.SetGameMaster(true);if(mode==1)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==2){visitor.SetQuestStatus(quest,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}
                ai->MoveInLineOfSight(&visitor);Require(!ai->introDone&&!Stories(map,2384),"Ineligible visitor consumed gate speech");
                ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&Stories(map,2384)==1,"Qualified Rogue did not receive real gate speech");
            } else if(mode==3) {
                ai->SetGUID(rogue.GetGUID(),2384);Require(ai->introDone&&Stories(map,2384)==1,"Actual GUID request did not create gate speech");
                ai->MoveInLineOfSight(&rogue);ai->SetGUID(visitor.GetGUID(),2384);ai->Reset();ai->UpdateAI(200000);
                Require(ai->introDone&&Stories(map,2384)==1,"GUID/LOS/Reset replayed admitted gate speech");
            } else if(mode==4) {
                Position opening{-9101.9f,406.198f,92.7232f,.643928f};rogue.Relocate(opening);
                Actor garona(&map,108015,26002,opening),shaw(&map,98094,26003,{-9078.58f,423.865f,92.6397f,3.83066f});Require(map.AddToMap(static_cast<Creature*>(&garona))&&map.AddToMap(static_cast<Creature*>(&shaw)),"Native opening actors missing");
                auto intro=new npc_garona_opening_probe::npc_garona_halforcenAI(&garona);Require(garona.AIM_Initialize(intro),"Whole opening AI missing");intro->MoveInLineOfSight(&rogue);intro->UpdateAI(40840);rogue.Relocate(playerPose);
                ai->MoveInLineOfSight(&rogue);intro->UpdateAI(47679);Require(intro->openingComplete&&scenario->GetCurrentStep()==1&&Stories(map,2384)==1,"Actual opening/LOS routes failed shared gate receipt");
            } else if(mode==5||mode==6) {
                // Neither a real Pet nor Guardian may be substituted for Player.
                CreditPet pet(rogue,map);CreditGuardian guardian(rogue,map);ai->MoveInLineOfSight(mode==5?static_cast<Unit*>(&pet):static_cast<Unit*>(&guardian));
                Require(!ai->introDone&&!Stories(map,2384),"Pet/guardian consumed gate speech");ai->MoveInLineOfSight(&rogue);Require(ai->introDone,"Rogue after pet/guardian not admitted");
            } else if(mode==7) {
                auto wrong=rogue.GetGUID();wrong.SetRawValue(wrong.GetHighPart()^1,wrong.GetLowPart());ai->SetGUID(wrong,2384);Require(!ai->introDone&&!Stories(map,2384),"Wrong full Player GUID admitted indexed character");ai->SetGUID(rogue.GetGUID(),2384);Require(ai->introDone,"Actual full Player GUID not admitted");
            } else if(mode==8) {
                rogue.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&Stories(map,2384)==1,"Active complete quest lost optional speech");
            } else if(mode==9||mode==10) {
                auto scope=std::make_shared<QuestObjectScope>();scope->Owner=visitor.GetGUID();clay.BindQuestObjectScope(scope);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Foreign scope admitted speech");scope->Owner=rogue.GetGUID();if(mode==10)scope->Cancel();ai->MoveInLineOfSight(&rogue);Require(ai->introDone==(mode==9),"Native source scope/cancellation differs");
            } else if(mode==11) {
                clay.SetPhaseMask(2,false);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Hidden source admitted speech");clay.SetPhaseMask(1,false);ai->MoveInLineOfSight(&rogue);Require(ai->introDone,"Restored phase did not admit speech");
            } else if(mode==12) {
                rogue.SetQuestStatus(quest,QUEST_STATUS_NONE);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Abandoned quest consumed speech");rogue.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);ai->MoveInLineOfSight(&rogue);Require(ai->introDone,"Reaccepted quest did not admit speech");
            } else if(mode==13||mode==14) {
                // Authored stage1 and later stages share this static General role.
                scenario->SetCurrentStep(mode==13?1:7);ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&Stories(map,2384)==1,"Invented stage gate removed authored shared speech");
            } else if(mode==15) {
                // A closer living duplicate cannot replace the actual speaking actor.
                Actor duplicate(&map,108057,26004,playerPose);Require(map.AddToMap(static_cast<Creature*>(&duplicate)),"Duplicate actor missing");ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&Stories(map,2384)==1,"Scoped actual source not admitted despite duplicate");
                for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2384){auto actor=story->GetCreatures()[0];Require(actor->GuidLow==clay.GetGUID().GetLowPart()&&actor->GuidHight==clay.GetGUID().GetHighPart(),"Closer duplicate replaced actual gate speaker");}
            } else if(mode==16) {
                // Remote opening requests use real configured visibility, not an invented large search radius.
                rogue.Relocate(clayPose.GetPositionX()+rogue.GetVisibilityRange()+10,clayPose.GetPositionY(),clayPose.GetPositionZ(),0);ai->SetGUID(rogue.GetGUID(),2384);Require(!ai->introDone,"Out-of-visibility remote request consumed gate speech");rogue.Relocate(playerPose);ai->SetGUID(rogue.GetGUID(),2384);Require(ai->introDone,"Restored visible remote request refused");
            } else if(mode==17) {
                auto other=new npc_kingslayers_small_probe::npc_kingslayers_small_eventsAI(&clay);Require(clay.AIM_Initialize(other),"Replacement native AI missing");other->MoveInLineOfSight(&rogue);Require(other->introDone&&Stories(map,2384)==1,"Fresh actual actor AI not admitted");
            } else if(mode==19||mode==24) {
                if(mode==24)scenario->SetCurrentStep(3);ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&Stories(map,2384)==1,"Original additional Clay360625/360814 pose failed shared speech");
            } else if(mode==20||mode==21) {
                auto wrong=clay.GetGUID();wrong.SetRawValue(wrong.GetHighPart()^(mode==20?1:0),wrong.GetLowPart()^(mode==21?(uint64(1)<<40):0));
                auto speech=new npc_kingslayers_small_probe::npc_kingslayers_small_eventsAI::GateConversation(wrong);
                Require(!speech->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2384,&rogue,nullptr,rogue)&&!speech->IsInWorld()&&!speech->IsInGrid(),"Wrong full source GUID bound indexed Clay actor");speech->AddToWorld();Require(!speech->IsInWorld(),"Direct publication admitted wrong full source GUID");delete speech;
            } else if(mode==22) {
                auto scope=std::make_shared<QuestObjectScope>();scope->Owner=rogue.GetGUID();clay.BindQuestObjectScope(scope);ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&Stories(map,2384)==1,"Owned source scope did not admit story");
                Conversation* speech=nullptr;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2384)speech=story;
                Require(speech&&speech->GetQuestObjectScope().lock()==scope,"Actual story did not inherit actor cancellation scope");auto cancelled=scope->Cancel();Require(std::find(cancelled.begin(),cancelled.end(),speech->GetGUID())!=cancelled.end(),"Source cancellation did not register actual story GUID");speech->Update(0);Require(!speech->IsInWorld(),"Cancelled native gate speech remained in world");map.RemoveAllObjectsInRemoveList();
            } else if(mode==23) {
                CreditParty party(&rogue,&visitor);ai->SetGUID(rogue.GetGUID(),2384);Require(ai->introDone&&Stories(map,2384)==1,"Legitimate native group request failed");
                for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2384)Require(story->IsInPersonnalVisibilityList(rogue.GetGUID())&&story->IsInPersonnalVisibilityList(visitor.GetGUID()),"Native group audience changed");
            } else {
                ai->SetGUID(rogue.GetGUID(),1234);Require(!ai->introDone&&!Stories(map,2384),"Unrelated AI GUID command acquired gate speech");ai->MoveInLineOfSight(&rogue);Require(ai->introDone,"Unrelated command consumed gate speech");
            }
#endif
            Require(!rogue.HasItemCount(128870,1)&&!visitor.HasItemCount(128870,1),"Optional gate speech granted artifact");
            if(mode!=4&&mode!=13&&mode!=14&&mode!=24)Require(scenario->GetCurrentStep()==0,"Optional gate speech granted progression");
#endif
            CleanStories(map);Require(objectCountInWorld[uint8(HighGuid::Conversation)]==0,"Gate-order story allocation leaked");
        }
        for(auto session:{first,second}){session->SetPlayer(nullptr);delete session;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
    std::cout<<"PASS "<<cases<<" whole-source native gate-order contexts; actual factories/Players/Scenario, seeded earlier state/LOS/clock, no client playback claim\n";
    CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;} }

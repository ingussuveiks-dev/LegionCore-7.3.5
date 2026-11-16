// Whole production Eliza AI, real Player/Scenario/Conversation; earlier stage,
// actor poses, accepted quests and explicit LOS calls are controlled inputs.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "ElizaIntro.inc"
static unsigned Stories(Map& map) {
    unsigned count=0;
    for(auto c:*Access(StoriesTag{})) if(c&&c->GetMap()==&map&&c->GetEntry()==1178) ++count;
    return count;
}
int main(int argc,char**argv) {try {
    InitializeAltheaNative(argc,argv);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1545,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1545,{}}});
    unsigned cases=0;
#ifdef ELIZA_INTRO_BASELINE
    std::vector<unsigned> modes={1};
#elif defined(ELIZA_INTRO_FAULTS)
    std::vector<unsigned> modes={20,21,22};
#else
    std::vector<unsigned> modes={0,1,2,3,4,5,6,7,8,9,10,11,12};
#endif
    for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}) for(unsigned mode:modes) {
        MapInstanced parent(1545,300);InstanceMap map(1545,300,9937000+cases++,DIFFICULTY_N_SCENARIO,&parent);
        auto scenario=sScenarioMgr->AddScenario(&map,1012);scenario->SetCurrentStep(3);
        auto session=Session();{
            Position pos{-1359.75f,6322.29f,9.57465f,0};
            NativePlayer player(session,map,101,pos);
            player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);
            player.setFactionForRace(race);player.SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40849);
            Actor boss(&map,102293,17001,pos),speaker(&map,102294,17002,pos);
            for(auto actor:{&boss,&speaker})Require(map.AddToMap(static_cast<Creature*>(actor)),"Native Eliza actor admission failed");
            for(Unit* unit:{static_cast<Unit*>(&player),static_cast<Unit*>(&boss),static_cast<Unit*>(&speaker)})unit->SetPhaseId({6199},false);
            Require(boss.AIM_Initialize(new npc_dread_admiral_eliza::npc_dread_admiral_elizaAI(&boss)),"Whole Eliza AI absent");
            auto ai=static_cast<npc_dread_admiral_eliza::npc_dread_admiral_elizaAI*>(boss.AI());
            if(mode==1)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
            if(mode==2)player.SetQuestStatus(40849,QUEST_STATUS_NONE);
            if(mode==3)player.SetQuestStatus(40849,QUEST_STATUS_COMPLETE);
            if(mode==4)player.SetGameMaster(true);
            if(mode==5){player.SetHealth(0);player.setDeathState(CORPSE);}
            if(mode==6)player.Relocate(pos.GetPositionX()+100,pos.GetPositionY(),pos.GetPositionZ(),0);
            if(mode==7)player.SetPhaseId({999},false);
            if(mode==8)scenario->SetCurrentStep(2);
            if(mode==9)scenario->SetCurrentStep(4);
            if(mode==10){boss.SetHealth(0);boss.setDeathState(CORPSE);}
            if(mode==11){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=nullptr;}
            if(mode==12){ai->MoveInLineOfSight(&speaker);Require(!ai->intro,"NPC consumed Eliza intro");}
#ifdef ELIZA_INTRO_FAULTS
            auto data=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(1178)));
            Require(data&&!data->empty(),"Native1178 lines absent");auto saved=*data;data->clear();
#endif
            std::cout<<"CASE race="<<race<<" mode="<<mode<<std::endl;
            ai->MoveInLineOfSight(&player);
#ifdef ELIZA_INTRO_BASELINE
            Require(ai->intro&&Stories(map)==1,"Expected-broken questless-class intro control changed");
            std::cout<<"COUNTERFACTUAL wrong-class Player consumes Eliza1178 and advances native1012 stage"<<std::endl;
#elif defined(ELIZA_INTRO_FAULTS)
            Require(!ai->intro&&!Stories(map)&&scenario->GetCurrentStep()==3,"Failed native story consumed intro/credit");
            ai->UpdateAI(999);Require(!ai->intro,"Retry ran early");
            *data=saved;
            if(mode==21){player.SetQuestStatus(40849,QUEST_STATUS_NONE);player.SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);}
            if(mode==22)player.Relocate(pos.GetPositionX()+100,pos.GetPositionY(),pos.GetPositionZ(),0);
            ai->UpdateAI(1);
            if(mode==20)Require(ai->intro&&Stories(map)==1&&scenario->GetCurrentStep()==4,"Stationary original-player retry failed");
            else {
                Require(!ai->intro&&!Stories(map)&&ai->introPlayer.IsEmpty()&&scenario->GetCurrentStep()==3,"Cancelled/ineligible pending intro survived");
                player.Relocate(pos);ai->MoveInLineOfSight(&player);
                Require(ai->intro&&Stories(map)==1&&scenario->GetCurrentStep()==4,"Fresh eligible attempt failed");
            }
#else
            bool accept=mode==0||mode==12;
            Require(ai->intro==accept&&Stories(map)==unsigned(accept),"Eliza admission result differs");
            if(scenario)Require(scenario->GetCurrentStep()==(accept?4:mode==8?2:mode==9?4:3),"Rejected/accepted intro changed native progression incorrectly");
            if(accept){ai->MoveInLineOfSight(&player);ai->UpdateAI(5000);ai->Reset();ai->MoveInLineOfSight(&player);Require(Stories(map)==1,"Repeated LOS/update/reset replayed accepted story");}
#endif
            CleanStories(map);
        }delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());map.RemoveAllObjectsInRemoveList();
    }
    CloseAltheaNative();std::cout<<"PASS Eliza native "<<cases<<" contexts; controlled earlier quest/stage/poses, actual native1178 and48538. Full journey/client/combat separate."<<std::endl;return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}

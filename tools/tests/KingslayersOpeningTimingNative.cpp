#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GaronaOpening.inc"
static uint32 AcceptedOpeningDuration(Map& map, uint32 entry) {
    for(auto story:*Access(StoriesTag{})) if(story&&story->GetMap()==&map&&story->GetEntry()==entry) return story->GetDuration();
    return 0;
}
int main(int argc,char** argv) { try {
    InitializeAltheaGuardsNative(argc,argv); uint32 cases=0;
#ifdef KINGSLAYERS_OPENING_TIMING_BASELINE
    uint32 modes=1;
#else
    uint32 modes=4;
#endif
    for(uint32 quest:{42504u,42627u}) for(uint32 mode=0;mode<modes;++mode) {
        MapInstanced parent(1620,300); InstanceMap map(1620,300,9976000+cases,DIFFICULTY_N_SCENARIO,&parent);
        map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(0);auto session=Session();
        {
            Position pose{-9101.9f,406.198f,92.7232f,.643928f};NativePlayer player(session,map,101,pose);
            player.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);
            player.setFactionForRace(player.getRace());player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,quest);
            Actor garona(&map,108015,27001,pose),shaw(&map,98094,27002,{-9078.58f,423.865f,92.6397f,3.83066f});
            Require(map.AddToMap(static_cast<Creature*>(&garona))&&map.AddToMap(static_cast<Creature*>(&shaw)),"Actual opening actors missing");
            if(mode==2)map.RemoveFromMap(static_cast<Creature*>(&shaw),false);
            auto ai=new npc_garona_opening_probe::npc_garona_halforcenAI(&garona);Require(garona.AIM_Initialize(ai),"Whole opening AI missing");
            ai->MoveInLineOfSight(&player);uint32 first=AcceptedOpeningDuration(map,2361);
            Require(first==(mode==2?0u:40840u),"Installed first conversation/native factory differs");
#ifdef KINGSLAYERS_OPENING_TIMING_BASELINE
            ai->UpdateAI(15000);Require(ai->openingMiddle&&AcceptedOpeningDuration(map,2377)==47679,"Pinned15s overlap changed");
            ai->UpdateAI(15000);Require(ai->openingComplete&&scenario->GetCurrentStep()==1,"Pinned30s departure changed");
            std::cout<<"COUNTERFACTUAL quest="<<quest<<" full native first40840/second47679 payloads but middle15s and progression/departure30s, before first ends and before second's last36110ms line begins; no captured client claim\n";
#else
            if(mode==3)player.Relocate(pose.GetPositionX()+500,pose.GetPositionY(),pose.GetPositionZ(),pose.GetOrientation());
            if(mode==1) {ai->UpdateAI(200000);Require(ai->openingMiddle&&!ai->openingComplete&&scenario->GetCurrentStep()==0,"Coarse tick skipped admitted second speech");}
            else {
                uint32 wait=first?first:15000;
                if(first){ai->UpdateAI(15000);Require(!ai->openingMiddle&&!AcceptedOpeningDuration(map,2377),"Middle overlapped first accepted native speech");wait-=15000;}
                ai->UpdateAI(wait-1);Require(!ai->openingMiddle&&scenario->GetCurrentStep()==0,"First phase shortened accepted payload/minimum");
                ai->UpdateAI(1);Require(ai->openingMiddle&&!ai->openingComplete&&scenario->GetCurrentStep()==0,"Middle phase missing or skipped second budget");
            }
            uint32 second=AcceptedOpeningDuration(map,2377);Require(second==(mode==3?0u:47679u),"Second optional native factory/refusal differs");
            uint32 wait=second?second:15000;ai->UpdateAI(wait-1);
            Require(!ai->openingComplete&&scenario->GetCurrentStep()==0,"Departure shortened accepted second payload/minimum");ai->UpdateAI(1);
            Require(ai->openingComplete&&scenario->GetCurrentStep()==1,"Exactly-once native opening receipts missing");
            auto progress=scenario->GetAchievementMgr().GetCriteriaProgressMap();for(uint32 leaf:{48526u,49374u}){auto found=progress->find(leaf);Require(found!=progress->end()&&found->second.Counter==1,"Native opening receipt is not exactly one");}
            ai->UpdateAI(200000);ai->MoveInLineOfSight(&player);Require(scenario->GetCurrentStep()==1&&!player.HasItemCount(128870,1),"Opening repeated progression/granted artifact");
#endif
            CleanStories(map);Require(objectCountInWorld[uint8(HighGuid::Conversation)]==0,"Native timing stories leaked");
        }
        session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
    std::cout<<"PASS "<<cases<<" whole-source native opening timing contexts; actual accepted payloads, EventMap and scenario receipts; clocks/poses/LOS seeded, movement commands distinct from physical/client traversal\n";
    CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;} }

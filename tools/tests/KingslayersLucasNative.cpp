// Whole shared production AI, real AIM_Initialize/EventMap/Conversation/MotionMaster.
// Earlier scenario admission, LOS delivery and elapsed clocks are controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "SmallEvents.inc"
struct LucasFunctionsTag { using Type=FunctionProcessor Unit::*; friend Type Access(LucasFunctionsTag); };
template struct FixtureAccess<LucasFunctionsTag,&Unit::m_Functions>;
using LucasAI=npc_kingslayers_small_probe::npc_kingslayers_small_eventsAI;
int main(int argc,char** argv) { try {
    InitializeAltheaGuardsNative(argc,argv); uint32 cases=0;
    for(uint32 quest:{42504u,42627u}) for(uint32 mode=0;mode<6;++mode) {
        MapInstanced parent(1620,300); InstanceMap map(1620,300,9934000+cases,DIFFICULTY_N_SCENARIO,&parent);
        map.CreateInstanceData(nullptr); auto scenario=sScenarioMgr->AddScenario(&map,1123); scenario->SetCurrentStep(7); auto session=Session();
        {
            Position pose{-8535.f,440.f,106.01f,0}; Position visitor=pose; visitor.m_positionX-=10;
            NativePlayer player(session,map,101,visitor);
            player.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);
            player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE); player.setFactionForRace(player.getRace());
            player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE); player.SetQuestSlot(0,quest);
            Actor lucas(&map,108399,31001,pose); Require(map.AddToMap(static_cast<Creature*>(&lucas)),"Native Lucas missing");
            auto install=[&] { auto ai=new LucasAI(&lucas); Require(lucas.AIM_Initialize(ai),"Native Lucas AI missing"); return ai; };
            auto ai=install();
            ai->MoveInLineOfSight(&player); ai->MoveInLineOfSight(&player);
            Require(ai->introDone&&Stories(map,2483)==1&&!Stories(map,2476),"Native Lucas first conversation missing/duplicated");
            Require((lucas.*Access(LucasFunctionsTag{})).SizeQueue()==0,"Lucas retained creature-owned delayed work");
            auto spline=lucas.movespline->GetId();
            if(mode==1||mode==2||mode==3||mode==5) {
                if(mode==2||mode==5)ai->UpdateAI(mode==2?4900:4999);
                if(mode==3)ai->Reset(); // Ordinary Reset retains the existing AI's authored one-shot.
                else { Require(lucas.AIM_Initialize(new NullCreatureAI(&lucas)),"Native AI replacement failed"); ai=nullptr; }
                (lucas.*Access(LucasFunctionsTag{})).Update(10000);
                if(!ai) {
                    Require(!Stories(map,2476)&&lucas.movespline->GetId()==spline,"Destroyed AI's old work survived native replacement");
                    CleanStories(map); ai=install(); ai->MoveInLineOfSight(&player);
                }
            }
            if(mode==4)ai->UpdateAI(10000);
            else { ai->UpdateAI(mode==3?4999:4900); Require(!Stories(map,2476),"Lucas follow-up before original5s"); ai->UpdateAI(mode==3?1:100); }
            Require(Stories(map,2476)==1,"Native follow-up2476 missing");
            Require(lucas.movespline->GetId()!=spline,"Native MovePoint missing");
            auto destination=lucas.movespline->FinalDestination();
            Require(std::abs(destination.x+8527.94f)<.01f&&std::abs(destination.y-442.f)<.01f&&std::abs(destination.z-106.01f)<.01f,"Lucas original MovePoint destination changed");
            auto moved=lucas.movespline->GetId(); ai->UpdateAI(30000); ai->MoveInLineOfSight(&player);
            Require(Stories(map,2476)==1&&lucas.movespline->GetId()==moved,"Lucas follow-up replayed");
            Require(scenario->GetCurrentStep()==7&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->empty(),"Lucas lifetime repair granted progression");
            CleanStories(map); ++cases;
        }
        session->SetPlayer(nullptr); delete session; sScenarioMgr->RemoveScenario(map.GetInstanceId());
    }
    std::cout<<"PASS "<<cases<<" whole shared Lucas AI/native conversations2483+2476,5s movement once, native AIM_Initialize replacement at0/4900/4999ms and fresh LOS, Reset and coarse tick, both quests; no progression grants. Controlled admission/LOS/clocks, no full client traversal or full-core ASAN claim\n";
    CloseAltheaNative();
} catch(std::exception const& error) { std::cerr<<error.what()<<'\n'; return 1; } }

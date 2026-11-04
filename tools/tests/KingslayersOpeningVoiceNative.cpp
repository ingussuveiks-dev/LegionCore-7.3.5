// Native factory/payload/accessor/removal. Actor poses and playback-clock
// endpoints are controlled; this is not authenticated client playback.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
struct OpeningClock : Conversation
{
    void Age(Player const& player, uint32 milliseconds) { playing[player.GetGUID()] = getMSTime() - milliseconds; }
};
int main(int argc, char** argv) { try {
    InitializeAltheaGuardsNative(argc, argv);
#ifdef KINGSLAYERS_OPENING_VOICE_ORIGINAL
    bool original = true;
#else
    bool original = false;
#endif
    uint32 cases = 0;
    for (uint32 entry : {2361u, 2377u})
    {
        auto actors = const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(entry));
        auto lines = const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(entry));
        uint32 count = entry == 2361 ? 6 : 4, last = count - 1;
        uint32 oldDuration = entry == 2361 ? 1950 : 3650, newDuration = entry == 2361 ? 26090 : 31729;
        std::vector<uint32> oldStarts = entry == 2361 ? std::vector<uint32>{0,4650,9550,16450,23300,28500} : std::vector<uint32>{0,7250,17100,24250};
        std::vector<uint32> newStarts = entry == 2361 ? std::vector<uint32>{0,5874,11511,18863,25351,32864} : std::vector<uint32>{0,12307,24696,36110};
        Require(actors && actors->size() == count && lines && lines->size() == count, "Native exact opening definition missing");
#if defined(KINGSLAYERS_OPENING_VOICE_ORIGINAL) || defined(KINGSLAYERS_OPENING_VOICE_SHADOW)
        for (auto& actor : *actors) if (actor.id == last) {
            Require(actor.duration == oldDuration || actor.duration == newDuration, "Custom opening duration; refuse shadow");
            actor.duration = original ? oldDuration : newDuration;
        }
        for (auto& line : *lines) {
            Require(line.idx < count && (line.textId == oldStarts[line.idx] || line.textId == newStarts[line.idx]), "Custom opening start; refuse shadow");
            line.textId = original ? oldStarts[line.idx] : newStarts[line.idx];
        }
#endif
        for (uint32 quest : {42504u,42627u}) {
            MapInstanced parent(1620,300); InstanceMap map(1620,300,9986000+cases,DIFFICULTY_N_SCENARIO,&parent);
            map.CreateInstanceData(nullptr); auto scenario=sScenarioMgr->AddScenario(&map,1123); scenario->SetCurrentStep(0); auto session=Session();
            {
                Position pose{-9101.9f,406.198f,92.7232f,.643928f}; NativePlayer player(session,map,101,pose);
                player.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC); player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);
                player.setFactionForRace(player.getRace()); player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE); player.SetQuestSlot(0,quest);
                Actor garona(&map,108015,28001,pose), shaw(&map,98094,28002,{-9078.58f,423.865f,92.6397f,3.83066f});
                Require(map.AddToMap(static_cast<Creature*>(&garona))&&map.AddToMap(static_cast<Creature*>(&shaw)),"Native opening actors not admitted");
                auto story=new OpeningClock;
                Require(story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),entry,&player,nullptr,player)&&story->IsInWorld(),"Native opening factory refused ordinary actors");
                uint32 duration=original?(entry==2361?16700:19600):(entry==2361?40840:47679);
                Require(story->GetDuration()==duration&&story->GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME)==duration,"Native published duration differs");
                Require(story->GetCreatures().size()==count&&story->GetLines().size()==count,"Native original slot counts differ");
                for(auto const& actor:*actors) {
                    auto packed=story->GetCreatures()[actor.id]; auto guid=actor.creatureId==108015?garona.GetGUID():shaw.GetGUID();
                    Require(packed->GuidLow==guid.GetLowPart()&&packed->GuidHight==guid.GetHighPart()&&packed->unk1==actor.unk1&&packed->unk2==actor.unk2,"Native original role/flags/GUID differs");
                }
                for(auto const& line:*lines) {
                    auto packed=story->GetLines()[line.idx];
                    Require(packed->id==line.id&&packed->textId==(original?oldStarts:newStarts)[line.idx]&&packed->unk1==line.unk1&&packed->unk2==line.unk2,"Native original line/flags/start differs");
                }
                Require(!story->CanNeverSee2(&player),"Native player playback key not admitted");
                auto guid=story->GetGUID();
                // Source departure does not itself remove the native object;
                // preserving a GUID does not certify voice/actor rendering.
                map.RemoveFromMap(static_cast<Creature*>(&garona),false);
                uint32 content=entry==2361?35840:42679;
                story->Age(player,original?duration+100:content); story->Update(0);
                Require(story->IsInWorld()!=original,"Native actual player-clock retention/expiry differs");
                if(!original) {
                    Require(ObjectAccessor::GetConversation(player,guid)==story&&!story->CanNeverSee2(&player),"Native repaired conversation missing during full content");
                    story->Age(player,duration+100); story->Update(0); Require(!story->IsInWorld()&&story->CanNeverSee2(&player),"Native repaired conversation failed to expire");
                }
                map.RemoveAllObjectsInRemoveList();
                // Missing/phase-hidden actors refuse the original native factory.
                if(entry==2361) {
                    Require(map.AddToMap(static_cast<Creature*>(&garona)),"Native source re-admission failed"); shaw.SetPhaseMask(2,false);
                    auto refused=new Conversation; Require(!refused->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),entry,&player,nullptr,player)&&!refused->IsInWorld()&&!refused->IsInGrid(),"Hidden required actor unexpectedly admitted"); delete refused;
                }
                Require(scenario->GetCurrentStep()==0&&!player.HasItemCount(128870,1),"Presentation changed progression/inventory");
                CleanStories(map); Require(objectCountInWorld[uint8(HighGuid::Conversation)]==0,"Native opening conversation allocation leaked");
            }
            session->SetPlayer(nullptr); delete session; sScenarioMgr->RemoveScenario(map.GetInstanceId()); ++cases;
        }
    }
    std::cout<<(original?"COUNTERFACTUAL":"PASS")<<" "<<cases<<" actual native2361/2377 factories/payloads and real player-clock removals; both quest contexts, source departure and hidden required actor refusal. Clocks/poses seeded, no client/caller/full1123 claim\n";
    CloseAltheaNative(); return 0;
} catch(std::exception const& e) { std::cerr<<e.what()<<'\n'; return 1; } }

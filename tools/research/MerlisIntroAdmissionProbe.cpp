// Pinned broken counterfactual, not a repaired regression. Stop before the
// five-second callback: observe its actual player queue and scene recipient.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "../tests/AltheaIntroNative.cpp"
#include "MerlisBeforeIntro.inc"
struct MerlisFunctionsTag
{
    using Type = FunctionProcessor Unit::*;
    friend Type Access(MerlisFunctionsTag);
};
template struct FixtureAccess<MerlisFunctionsTag, &Unit::m_Functions>;
static uint32 Pending(Player& player)
{
    auto& queue = player.*Access(MerlisFunctionsTag{});
    return queue.Size() + queue.SizeQueue();
}
static uint32 ScenesFor(Map& map, Player& player)
{
    uint32 count = 0;
    for (auto story : *Access(StoriesTag{}))
        if (story && story->GetMap() == &map && story->GetEntry() == 2484 && story->GetCasterGUID() == player.GetGUID())
            ++count;
    return count;
}
int main(int argc, char** argv)
{
    try
    {
        InitializeAltheaGuardsNative(argc, argv);
        auto leaf = sCriteriaTreeStore.LookupEntry(48587);
        auto criterion = leaf ? sCriteriaStore.LookupEntry(leaf->CriteriaID) : nullptr;
        Require(leaf && leaf->Parent == 48586 && leaf->Amount == 1 && criterion && criterion->ID == 30474 &&
            criterion->Type == CRITERIA_TYPE_SCRIPT_EVENT_2 && criterion->Asset == 51614,
            "Matching native stage6 intro leaf/event differs");
        uint32 cases = 0;
        for (uint32 quest : {42504u, 42627u})
            for (uint32 mode : {0u, 1u, 2u})
            {
                MapInstanced parent(1620, 300);
                InstanceMap map(1620, 300, 9999000 + cases, DIFFICULTY_N_SCENARIO, &parent);
                map.CreateInstanceData(nullptr);
                auto scenario = sScenarioMgr->AddScenario(&map, 1123);
                scenario->SetCurrentStep(6);
                Require(scenario->CanUpdateCriteria(48587), "Actual native stage6 cannot admit original intro event");
                auto first = Session(), second = Session();
                {
                    Position center{-8321.93f, 287.852f, 156.916f, 0.7101f};
                    NativePlayer player(first, map, 101, {center.GetPositionX() + 10, center.GetPositionY(), center.GetPositionZ(), 0});
                    NativePlayer visitor(second, map, 102, {center.GetPositionX() + (mode ? 5 : 80), center.GetPositionY(), center.GetPositionZ(), 0});
                    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_ROGUE);
                    player.SetQuestStatus(quest, QUEST_STATUS_INCOMPLETE); player.SetQuestSlot(0, quest);
                    visitor.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_ROGUE);
                    if (mode == 1) visitor.SetGameMaster(true);
                    Actor merlis(&map, 107831, 22001, center);
                    Require(map.AddToMap(static_cast<Creature*>(&merlis)), "Native Merlis admission failed");
                    // Original2484 requires a real108397 actor. Use the actual
                    // nearby360610 spawn pose, not a synthetic actor payload.
                    Actor speaker(&map, 108397, 22002, {-8360.25f, 252.679f, 155.43f, 0.858926f});
                    Require(map.AddToMap(static_cast<Creature*>(&speaker)), "Original2484 actor admission failed");
                    auto ai = new npc_merlis_malagan_before_intro::npc_merlis_malaganAI(&merlis);
                    Require(merlis.AIM_Initialize(ai), "Pinned complete Merlis AI failed");
                    // FindNearestPlayer is actually a first-match PlayerSearcher
                    // using visibility distance. Control who arrives first;
                    // do not assume that a closer player wins grid iteration.
                    if (mode)
                        player.Relocate(center.GetPositionX() + 250, center.GetPositionY(), center.GetPositionZ(), 0);
                    else
                        visitor.Relocate(center.GetPositionX() + 250, center.GetPositionY(), center.GetPositionZ(), 0);
                    Require(merlis.FindNearestPlayer(70.0f) == (mode ? &visitor : &player), "Native first-arrival search endpoint not established");
                    auto playerBefore = Pending(player), visitorBefore = Pending(visitor);
                    ai->UpdateAI(1000);
                    Require(ai->introDone, "Pinned original poll did not consume nearest Player");
                    if (!mode)
                    {
                        Require(Pending(player) == playerBefore + 1 && Pending(visitor) == visitorBefore && ScenesFor(map, player) == 1, "Eligible native control did not own delayed intro/scene");
                        std::cout << "CONTROL native Merlis quest=" << quest << " eligible Rogue owns original2484 and pending5s callback\n";
                    }
                    else
                    {
                        Require(Pending(visitor) == visitorBefore + 1 && Pending(player) == playerBefore && ScenesFor(map, visitor) == 1 && !ScenesFor(map, player), "Pinned ineligible-first counterfactual not reproduced");
                        visitor.Relocate(center.GetPositionX() + 250, center.GetPositionY(), center.GetPositionZ(), 0);
                        player.Relocate(center.GetPositionX() + 10, center.GetPositionY(), center.GetPositionZ(), 0);
                        Require(merlis.FindNearestPlayer(70.0f) == &player, "Later eligible native search endpoint missing");
                        ai->UpdateAI(1000);
                        Require(ai->introDone && Pending(player) == playerBefore && !ScenesFor(map, player), "Later eligible Rogue unexpectedly recovered consumed intro");
                        std::cout << "COUNTERFACTUAL native Merlis quest=" << quest << " first=" << (mode == 1 ? "GM" : "questless Rogue")
                            << " consumes one-shot, owns2484/pending5s callback; later eligible Rogue receives neither. Delay not executed; no actual teleport/criteria outcome claimed\n";
                    }
                    // This investigation deliberately ends before any teleport.
                    player.KillAllDelayedEvents(); visitor.KillAllDelayedEvents(); CleanStories(map);
                }
                first->SetPlayer(nullptr); second->SetPlayer(nullptr); delete first; delete second;
                sScenarioMgr->RemoveScenario(map.GetInstanceId()); ++cases;
            }
        std::cout << "PASS " << cases << " pinned native research controls/counterfactuals; admission remains OPEN, not a repaired ordinary run or client traversal\n";
        std::cout.flush(); CloseAltheaNative();
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

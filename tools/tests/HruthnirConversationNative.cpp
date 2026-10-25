#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include <filesystem>
#include <cstdlib>
#include <limits>
#include <memory>

struct HruthnirConversationOwner
{
    Creature* me;
    ObjectGuid magnarConversation = ObjectGuid::Empty;
#include "HruthnirCleanup.inc"
};

struct HruthnirNativeLifecycle : ScriptedAI
{
    explicit HruthnirNativeLifecycle(Creature* actor) : ScriptedAI(actor) { }
#include "HruthnirFields.inc"
#include "HruthnirCleanup.inc"
#include "HruthnirCancel.inc"
#include "HruthnirReset.inc"
#include "HruthnirDamage.inc"
};

struct HruthnirDeathLifecycle : HruthnirNativeLifecycle
{
    using HruthnirNativeLifecycle::HruthnirNativeLifecycle;
#include "HruthnirDeath.inc"
};

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 2, "Expected native dbc directory");
        auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
        Require(config, "Private native configuration missing");
        auto directory = std::filesystem::path(argv[1]).lexically_normal();
        if (directory.filename().empty()) directory = directory.parent_path();
        std::string runtime = directory.parent_path().parent_path().string() + '/';
        char* arguments[] = {argv[0], config, runtime.data()};
        InitializeNative(3, arguments);
        sConversationDataStore->LoadConversationData();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495, {}}});
        {
            InstanceMap map(1495, 300, 9999993, DIFFICULTY_N_SCENARIO, nullptr);
            Require(map.ToInstanceMap() == &map && !map.GetInstanceScript(), "Isolated native death map type/script changed");
            Actor caster(&map, 96468, 10001, {4720.80f, 347.91f, -37.71f, 0.0f});
            map.LoadGrid(caster.GetPositionX(), caster.GetPositionY());
            Require(map.AddToMap(static_cast<Creature*>(&caster)), "Native Hruthnir failed map admission");
            caster.SetPhaseId({6999}, true);
            auto create = [&]()
            {
                auto conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 497, &caster, nullptr, caster))
                {
                    Require(!conversation->IsInWorld(), "Failed497 entered the world");
                    delete conversation;
                    return static_cast<Conversation*>(nullptr);
                }
                return conversation;
            };
            Require(!create(), "Missing native Magnar actor unexpectedly admitted497");
            Actor boss(&map, 96034, 10002, {4714.82f, 349.11f, -37.77f, 0.0f});
            Require(map.AddToMap(static_cast<Creature*>(&boss)), "Native Magnar failed map admission");
            boss.SetPhaseId({6999}, true);
            auto conversation = create();
            Require(conversation && conversation->IsInWorld(), "Native497 retry did not enter the world");
            Require(conversation->GetDuration() == 73000 && conversation->GetLines().size() == 10 && conversation->GetCreatures().size() == 3, "Native497 payload/lifetime changed");
            Require(conversation->GetLine(0)->id == 1324 && conversation->GetLine(0)->textId == 0 &&
                conversation->GetLine(9)->id == 1334 && conversation->GetLine(9)->textId == 41000 && conversation->GetLine(9)->unk2 == 258, "Native497 first/final line metadata changed");
            Require(conversation->GetCreature(0)->GuidLow == caster.GetGUID().GetLowPart() &&
                conversation->GetCreature(0)->GuidHight == caster.GetGUID().GetHighPart() &&
                conversation->GetCreature(1)->GuidLow == boss.GetGUID().GetLowPart() &&
                conversation->GetCreature(1)->GuidHight == boss.GetGUID().GetHighPart(), "Native497 actors did not resolve actual Hruthnir/Magnar");
            auto other = create();
            Require(other && other->IsInWorld(), "Independent native497 creation failed");
            auto guid = conversation->GetGUID();
            Require(ObjectAccessor::GetConversation(caster, guid) == conversation, "Native497 map lookup failed");
            Map foreignMap(1495, 300, 9999992, DIFFICULTY_N_SCENARIO);
            Actor foreignCaster(&foreignMap, 96468, 10003, {4720.80f, 347.91f, -37.71f, 0.0f});
            HruthnirConversationOwner foreignOwner{&foreignCaster, guid};
            foreignOwner.RemoveMagnarConversation();
            Require(!foreignOwner.magnarConversation && conversation->IsInWorld(), "Foreign map cleanup removed497 or retained the claim");
            HruthnirConversationOwner owner{&caster, guid};
            owner.RemoveMagnarConversation();
            Require(!owner.magnarConversation, "Actual cleanup helper retained native497 GUID");
            Require(!conversation->IsInWorld() && !ObjectAccessor::GetConversation(caster, guid) && other->IsInWorld(), "Scoped native497 removal failed or removed another object");
            owner.RemoveMagnarConversation();
            map.RemoveAllObjectsInRemoveList();
            owner.magnarConversation = guid; owner.RemoveMagnarConversation();
            Require(!owner.magnarConversation && other->IsInWorld(), "Expired native497 lookup affected another conversation");
            Actor control(&map, 96468, 10004, {4721.80f, 347.91f, -37.71f, 0.0f});
            Require(map.AddToMap(static_cast<Creature*>(&control)), "Native default-hook control admission failed");
            control.SetPhaseId({6999}, true);
            auto baseline = new HruthnirNativeLifecycle(&control);
            Require(control.AIM_Initialize(baseline), "Native default-hook control AI failed");
            auto retained = new Conversation;
            Require(retained->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),497,&control,nullptr,control), "Native default-hook497 creation failed");
            baseline->magnarConversation = retained->GetGUID();
            baseline->magnarStartToken = std::make_shared<bool>(false);
            control.Kill(&control,false);
            Require(!control.IsAlive() && retained->IsInWorld() && baseline->magnarConversation && baseline->magnarStartToken, "Default native death unexpectedly invokes Reset/497 cleanup");
            baseline->RemoveMagnarConversation(); map.RemoveAllObjectsInRemoveList();
            auto dying = create();
            Require(dying && dying->IsInWorld(), "Native497 death fixture creation failed");
            auto lifecycle = new HruthnirDeathLifecycle(&caster);
            Require(caster.AIM_Initialize(lifecycle), "Native death lifecycle AI admission failed");
            lifecycle->magnarConversation = dying->GetGUID();
            lifecycle->magnarStartToken = std::make_shared<bool>(false);
            lifecycle->magnarOfferToken = std::make_shared<bool>(false);
            lifecycle->magnarArrivalPending = lifecycle->magnarOfferReady = true;
            caster.SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            uint32 damage = std::numeric_limits<uint32>::max();
            lifecycle->DamageTaken(&boss, damage, DIRECT_DAMAGE);
            Require(!damage && caster.IsAlive(), "Ordinary Hruthnir zero-damage guard changed");
            caster.Kill(&caster, false);
            Require(!caster.IsAlive() && !dying->IsInWorld() && other->IsInWorld(), "Native Kill did not dispatch scoped Hruthnir death cleanup");
            Require(!lifecycle->magnarConversation && !lifecycle->magnarStartToken && !lifecycle->magnarOfferToken &&
                !lifecycle->magnarArrivalPending && !lifecycle->magnarOfferReady && !caster.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP), "Native death hook did not execute actual Reset retirement");
            lifecycle->JustDied(nullptr);
            Require(other->IsInWorld(), "Repeated null-killer cleanup removed unrelated497");
            map.RemoveAllObjectsInRemoveList();
            other->Remove(); map.RemoveAllObjectsInRemoveList();
            std::cout << "PASS: actual native497 fails with missing96034, then resolves real96468/96034 actors and creates10 lines/3 actor slots/73000ms lifetime after retry. Extracted cleanup handles matching/foreign/empty/expired lookups. Real Unit::Kill/default-hook control retains497; repaired extracted JustDied/Reset/Cancel/cleanup retires native GUID/offer/cue/gossip state while another497 survives. Ordinary zero-damage handler and repeated null-killer hook retained. Native queued map deletion succeeds. Isolated InstanceMap without instance script, extracted lifecycle and seeded accepted state, read-only world/hotfix, no Players/character saves; not full production AI/instance traversal, real player/pet/guardian combat, sentinel delivery, audio or rendered client evidence.\n";
        }
        WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

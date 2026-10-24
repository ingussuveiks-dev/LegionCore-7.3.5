#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include <filesystem>
#include <cstdlib>

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 2, "Expected native dbc directory");
        auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
        Require(config, "Private native configuration missing");
        auto directory = std::filesystem::path(argv[1]).lexically_normal();
        if (directory.filename().empty())
            directory = directory.parent_path();
        std::string runtime = directory.parent_path().parent_path().string() + '/';
        char* arguments[] = {argv[0], config, runtime.data()};
        InitializeNative(3, arguments);
        sConversationDataStore->LoadConversationData();
        Require(!sConversationDataStore->GetConversationData(uint32(9000843)), "Negative conversation ID is no longer unused");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511, {}}});
        {
            Map map(1511, 300, 9999995, DIFFICULTY_N_SCENARIO);
            Actor caster(&map, 98602, 10001, {3515.15f, 1800.61f, 0.630465f, 0.0f});
            map.LoadGrid(caster.GetPositionX(), caster.GetPositionY());
            Require(map.AddToMap(static_cast<Creature*>(&caster)), "Native caster failed map admission");
            caster.SetPhaseId({7003}, true);
            auto create = [&](uint32 identity, Unit* owner)
            {
                auto conversation = new Conversation;
                if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), identity, owner, nullptr, caster))
                {
                    Require(!conversation->IsInWorld(), "Failed conversation entered the world");
                    delete conversation;
                    return static_cast<Conversation*>(nullptr);
                }
                return conversation;
            };
            Require(!create(843, nullptr), "Null caster was admitted");
            Require(!create(9000843, &caster), "Missing conversation data was admitted");
            auto conversation = create(843, &caster);
            Require(conversation && conversation->IsInWorld() && conversation->GetCasterGUID() == caster.GetGUID(), "Actual843 creation was not acknowledged");
            auto guid = conversation->GetGUID();
            Require(ObjectAccessor::GetConversation(caster, guid) == conversation, "Native map-local conversation lookup failed");
            {
                Map otherMap(1511, 300, 9999994, DIFFICULTY_N_SCENARIO);
                Actor observer(&otherMap, 98602, 10002, {3515.15f, 1800.61f, 0.630465f, 0.0f});
                Require(!ObjectAccessor::GetConversation(observer, guid), "Native conversation lookup crossed maps");
            }
            Require(conversation->GetDuration() == 43200 && conversation->GetLines().size() == 2 && conversation->GetActors().size() == 2, "Native843 payload/lifetime changed");
            Require(conversation->GetLine(0)->id == 2066 && conversation->GetLine(1)->id == 2067, "Native843 line order changed");
            Require(conversation->GetLine(0)->textId == 0 && conversation->GetLine(1)->textId == 12150, "Native843 absolute line start times changed");
            Require(conversation->GetLine(0)->unk1 == 135 && conversation->GetLine(1)->unk1 == 620 &&
                conversation->GetLine(0)->unk2 == 0 && conversation->GetLine(1)->unk2 == 1, "Native843 camera/actor payload changed");
            conversation->Remove();
            Require(!conversation->IsInWorld(), "Canceled conversation remained in world");
            Require(!ObjectAccessor::GetConversation(caster, guid), "Removed conversation remained accessible");
            map.RemoveAllObjectsInRemoveList();
            conversation = create(843, &caster);
            Require(conversation && conversation->IsInWorld(), "Native843 retry failed");
            conversation->Remove();
            map.RemoveAllObjectsInRemoveList();
            std::cout << "PASS: actual native843 creation returns false for null caster/missing data and true only after native generation/map insertion; map-local GUID lookup finds it, rejects another map, and returns null after native removal; recreation works. Two lines2066/2067 at0/12150ms, cameras135/620, actors0/1 and43200 lifetime retained. Real Creature caster/no Players or character saves; not player/group visibility, audio playback or client traversal.\n";
        }
        WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

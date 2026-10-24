#define main WarswordsChannelTargetMain
#include "WarswordsHelyaTargetNative.cpp"
#undef main

struct BubbleActor : Actor
{
    BubbleActor(Map* map, uint32 entry, uint32 guid, uint64 spawn, Position const& position) : Actor(map, entry, guid, position)
    {
        m_DBTableGuid = spawn;
    }
};

static unsigned BubbleCount(Creature const& actor)
{
    unsigned count = 0;
    for (auto const& aura : actor.GetOwnedAuras())
        if (aura.second->GetId() == 205424)
            ++count;
    return count;
}

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
        auto row = WorldDatabase.Query("SELECT guid,path_id,mount,bytes1,bytes2,emote,auras FROM creature_addon WHERE guid=361283");
        Require(row && row->Fetch()[6].GetString() == "205424", "Deployed spawn bubble addon missing");
        for (uint32 column = 1; column < 6; ++column)
            Require(row->Fetch()[column].GetUInt32() == (column == 4 ? 1u : 0u), "Unexpected addon pose/mount/path or changed melee sheath");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE warswords_bubble_addon AS SELECT * FROM creature_addon WHERE guid=361283");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon LIKE warswords_bubble_addon");
        WorldDatabase.DirectExecute("INSERT INTO creature_addon SELECT * FROM warswords_bubble_addon");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        auto readOnly = WorldDatabase.Query("SELECT @@session.transaction_read_only");
        Require(readOnly && readOnly->Fetch()[0].GetBool(), "Native loader world session is not read-only");
        auto count = WorldDatabase.Query("SELECT COUNT(*) FROM creature_addon");
        Require(count && count->Fetch()[0].GetUInt32() == 1, "Scoped temporary addon table failed");
        auto& spawn = sObjectMgr->NewOrExistCreatureData(361283);
        spawn.id = 98602;
        spawn.movementType = IDLE_MOTION_TYPE;
        sObjectMgr->LoadCreatureAddons();
        auto addon = sObjectMgr->GetCreatureAddon(361283);
        Require(addon && addon->auras.size() == 1 && addon->auras[0] == 205424, "Native addon loader lost scoped spell");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511, {}}});
        {
            Map map(1511, 300, 9999995, DIFFICULTY_N_SCENARIO);
            Actor helya(&map, 99043, 10000, {3578.37f, 1783.26f, -8.34128f, 3.13469f});
            BubbleActor vigfus(&map, 98602, 10001, 361283, {3515.15f, 1800.61f, 0.630465f, 6.09758f});
            BubbleActor unrelated(&map, 98602, 10002, 361284, {3516.15f, 1800.61f, 0.54f, 0.0f});
            Require(!vigfus.IsInWorld() && vigfus.GetCreatureAddon() == addon, "Native spawn-specific addon lookup failed");
            Require(unrelated.GetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_SHEATH_STATE) == SHEATH_STATE_MELEE, "Original no-addon melee sheath changed");
            vigfus.SetSheath(SHEATH_STATE_UNARMED);
            Require(vigfus.LoadCreaturesAddon() && vigfus.HasAura(205424, vigfus.GetGUID()), "Native pre-world addon aura failed");
            Require(vigfus.GetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_SHEATH_STATE) == SHEATH_STATE_MELEE, "Spawn addon hides the existing melee weapons");
            Require(!unrelated.LoadCreaturesAddon() && !unrelated.HasAura(205424) && !helya.HasAura(205424), "Bubble leaked into another spawn or Helya");
            Require(BubbleCount(vigfus) == 1 && vigfus.GetAura(205424)->IsPermanent() && !vigfus.GetChannelSpellId(), "Bubble is not the native infinite non-channel aura");
            map.LoadGrid(vigfus.GetPositionX(), vigfus.GetPositionY());
            Require(map.AddToMap(static_cast<Creature*>(&helya)) && map.AddToMap(static_cast<Creature*>(&vigfus)), "Native actors failed map admission");
            helya.SetPhaseId({7003}, true);
            vigfus.SetPhaseId({7003}, true);
            vigfus.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            Require(helya.CastSpell(&vigfus, 195062) == SPELL_CAST_OK, "Native Helya channel rejected while bubble exists");
            TickChannel(helya, vigfus, 100);
            Require(vigfus.HasAura(195062, helya.GetGUID()) && vigfus.HasAura(205424, vigfus.GetGUID()), "Native bubble and Helya channel cannot coexist");
            TickChannel(helya, vigfus, 7100);
            Require(!vigfus.HasAura(195062) && vigfus.HasAura(205424) && !helya.GetChannelSpellId(), "Finite channel incorrectly expires infinite bubble");
            Require(!helya.isInCombat() && !vigfus.isInCombat(), "Cosmetic aura/channel starts combat");
            Require(vigfus.LoadCreaturesAddon() && BubbleCount(vigfus) == 1, "Repeated addon load duplicates bubble");
            vigfus.RemoveAura(205424);
            Require(!BubbleCount(vigfus), "Original intro removal leaves a real bubble aura");
            Require(vigfus.LoadCreaturesAddon() && BubbleCount(vigfus) == 1, "Native evade-style addon reload cannot restore bubble");
            vigfus.RemoveAura(205424);
            Require(!BubbleCount(vigfus), "Completed-intro Reset cleanup cannot remove reapplied aura");
            Require(vigfus.LoadCreaturesAddon() && BubbleCount(vigfus) == 1, "Native death cleanup control is missing its aura");
            auto bubble = vigfus.GetAura(205424);
            std::cout << "BUBBLE passive=" << bubble->IsPassive() << " death-persistent=" << bubble->IsDeathPersistent() << '\n';
            Require(!bubble->IsPassive() && bubble->IsDeathPersistent(), "Matching bubble death policy changed");
            vigfus.RemoveAllAurasOnDeath();
            Require(BubbleCount(vigfus) == 1, "Native death-persistent control no longer reproduces retained sphere");
            vigfus.RemoveAura(205424);
            Require(!BubbleCount(vigfus), "Explicit JustDied-style cleanup cannot clear death-persistent sphere");
            Require(vigfus.LoadCreaturesAddon() && BubbleCount(vigfus) == 1, "Native corpse-state control has no starting aura");
            vigfus.Unit::setDeathState(CORPSE);
            Require(!vigfus.IsAlive() && BubbleCount(vigfus) == 1, "Native Unit death without script callback did not retain death-persistent sphere");
            vigfus.RemoveAura(205424);
            Require(!BubbleCount(vigfus), "Lifecycle-style cleanup cannot clear actual corpse aura");
            Require(vigfus.LoadCreaturesAddon() && !BubbleCount(vigfus), "Native spell-ID AddAura no longer rejects fresh205424 on a corpse; addon success is not aura acknowledgement");
            std::cout << "PASS: actual native addon loader/lookup and pre-world infinite205424, melee sheath, no other-spawn/Helya leak, idempotent reload and simultaneous real195062/7s expiry. Death-persistent205424 survives ordinary/Unit corpse transition without script callback; explicit removal clears it on the actual corpse. Addon returns true but ID-based AddAura rejects fresh corpse application. Temporary one-row loader table/controlled DB GUID/Null AIs; not full Create/evade/Creature death/scenario/client traversal.\n";
        }
        HotfixDatabase.Close(); WorldDatabase.Close();
        return 0;
    }
    catch (std::exception const& failure)
    {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}

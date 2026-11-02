#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "MapInstanced.h"
#include <filesystem>
#include <cstdlib>

struct RitualObserver : NullCreatureAI
{
    using NullCreatureAI::NullCreatureAI;
    std::vector<std::pair<uint32, uint32>> hits;
    std::vector<std::pair<uint32, SpellCastResult>> summons;
    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        hits.push_back({spell->Id, target->GetEntry()});
        std::cout << "HIT\t" << spell->Id << '\t' << target->GetEntry() << '\t' << target->IsAlive() << '\n';
        if (spell->Id == 205287 && (target->GetEntry() == 102964 || target->GetEntry() == 102965))
        {
            uint32 id = target->GetEntry() == 102964 ? 205295 : 205296;
            auto result = target->CastSpell(target, id);
            summons.push_back({id, result});
            std::cout << "CORPSE_CAST\t" << id << '\t' << uint32(result) << '\n';
        }
    }
};

int main(int argc, char** argv)
{
    try
    {
        std::cout << std::unitbuf;
        Require(argc == 2, "Expected native dbc directory");
        auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
        Require(config, "Private native configuration missing");
        auto directory = std::filesystem::path(argv[1]).lexically_normal();
        if (directory.filename().empty()) directory = directory.parent_path();
        std::string runtime = directory.parent_path().parent_path().string() + '/';
        char* arguments[] = {argv[0], config, runtime.data()};
        InitializeNative(3, arguments);
        auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(205287,205295,205296)");
        Require(conditions && conditions->Fetch()[0].GetUInt32() == 0, "Load newly introduced native ritual conditions before testing");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1500, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1500, {}}});
        {
            MapInstanced parent(1500, 300);
            std::cout << "RESEARCH native parent ready\n";
            InstanceMap map(1500, 300, 9999992, DIFFICULTY_N_SCENARIO, &parent);
            std::cout << "RESEARCH native InstanceMap ready\n";
            Actor danica(&map, 103608, 11000, {-1980.04f, -1251.96f, 4.83f, 0});
            Actor first(&map, 102964, 11001, {-1981, -1252, 4.83f, 0});
            Actor second(&map, 102965, 11002, {-1979, -1252, 4.83f, 0});
            Actor ally(&map, 97166, 11003, {-1980, -1250, 4.83f, 0});
            map.LoadGrid(danica.GetPositionX(), danica.GetPositionY());
            for (Actor* actor : {&danica, &first, &second, &ally})
                Require(map.AddToMap(static_cast<Creature*>(actor)), "Native ritual actor map admission failed");
            std::cout << "RESEARCH native actors admitted\n";
            Require(danica.AIM_Initialize(new RitualObserver(&danica)), "Native ritual observer initialization failed");
            first.setDeathState(JUST_DIED); second.setDeathState(JUST_DIED);
            Require(!first.IsAlive() && !second.IsAlive(), "Native corpse setup failed");
            auto info = sSpellMgr->GetSpellInfo(205287);
            Require(info, "Native205287 missing");
            std::cout << "SPELL\t205287\t" << info->GetDuration() << '\t' << unsigned(UnitAI::AISpellInfo[205287].target)
                << '\t' << info->HasAttribute(SPELL_ATTR2_CAN_TARGET_DEAD) << '\t' << info->HasAttribute(SPELL_ATTR3_ONLY_TARGET_GHOSTS)
                << '\t' << info->CalcCastTime() << '\n';
            danica.AI()->DoCast(205287);
            std::cout << "CAST native AI casting=" << danica.HasUnitState(UNIT_STATE_CASTING) << '\n';
            for (uint32 i = 0; i < 1200; ++i)
                for (Actor* actor : {&danica, &first, &second, &ally}) actor->Unit::Update(10);
            auto observer = static_cast<RitualObserver*>(danica.AI());
            std::set<uint32> hitEntries;
            for (auto const& hit : observer->hits) { Require(hit.first == 205287, "Unexpected native hit spell"); hitEntries.insert(hit.second); }
            Require(observer->hits.size() == 4 && hitEntries == std::set<uint32>({103608,102964,102965,97166}), "Native205287 corpse and unrelated living-target control changed");
            Require(observer->summons.size() == 2, "Native205287 did not hit both exact corpses");
            for (auto const& result : observer->summons) Require(result.second == SPELL_CAST_OK, "Original normal corpse summon refused");
            for (auto pair : {std::make_pair(103688u, &first), std::make_pair(103687u, &second)})
            {
                std::list<Creature*> spirits;
                danica.GetCreatureListWithEntryInGrid(spirits, pair.first, 100.0f);
                Require(spirits.size() == 1, "Original corpse summon did not create one actual spirit");
                Require(spirits.front()->ToTempSummon() && spirits.front()->ToTempSummon()->GetSummonerGUID() == pair.second->GetGUID(), "Native spirit belongs to a different corpse");
                std::cout << "SPIRIT\t" << pair.first << '\t' << spirits.front()->ToTempSummon()->GetSummonerGUID().ToString() << '\n';
                spirits.front()->DespawnOrUnsummon();
            }
            map.RemoveAllObjectsInRemoveList();
            for (Actor* actor : {&danica, &first, &second, &ally})
                std::cout << "AURA\t" << actor->GetEntry() << '\t' << actor->HasAura(205287) << '\n';
            std::cout << "PASS native205287 real InstanceMap/MapInstanced parent, AI self-target choice, normal5000ms cast and four actual live/corpse hit receipts; original normal205295/205296 casts return OK and create one103688/103687 with exact original corpse summoner GUIDs. No Player, character write, native933 advancement, full production Danica handler, departure or client playback is asserted.\n";
        }
        WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

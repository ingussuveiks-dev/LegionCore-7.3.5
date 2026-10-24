#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include <filesystem>
#include <cstdlib>

static void TickChannel(Actor& helya, Actor& vigfus, uint32 milliseconds)
{
    for (uint32 elapsed = 0; elapsed < milliseconds; elapsed += 10)
    {
        helya.Unit::Update(10);
        vigfus.Unit::Update(10);
    }
}

static bool ChannelTargets(Actor const& caster, ObjectGuid target)
{
    for (auto guid : caster.GetChannelObjects())
        if (guid == target)
            return true;
    return false;
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
        auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceEntry IN(195062,205424)");
        Require(conditions && !conditions->Fetch()[0].GetUInt32(), "New spell conditions require native admission loading before this probe");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511, {}}});
        {
            Map map(1511, 300, 9999996, DIFFICULTY_N_SCENARIO);
            Actor helya(&map, 99043, 10000, {3578.37f, 1783.26f, -8.34128f, 3.13469f});
            Actor vigfus(&map, 98602, 10001, {3515.15f, 1800.61f, 0.54f, 0.0f});
            map.LoadGrid(vigfus.GetPositionX(), vigfus.GetPositionY());
            Require(map.AddToMap(static_cast<Creature*>(&helya)) && map.AddToMap(static_cast<Creature*>(&vigfus)), "Native actors failed map admission");
            helya.SetPhaseId({7003}, true);
            vigfus.SetPhaseId({7003}, true);
            vigfus.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            auto spell = sSpellMgr->GetSpellInfo(195062);
            Require(spell && spell->IsChanneled() && spell->GetDuration() == 7000, "Native channel metadata changed");
            std::cout << "METADATA positive=" << spell->IsPositive() << " effect=" << spell->GetEffect(0, DIFFICULTY_N_SCENARIO)->Effect << '\n';
            auto selfResult = helya.CastSpell(&helya, 195062);
            TickChannel(helya, vigfus, 100);
            std::cout << "SELF result=" << uint32(selfResult) << " caster-aura=" << helya.HasAura(195062, helya.GetGUID())
                << " vigfus-aura=" << vigfus.HasAura(195062, helya.GetGUID()) << " self-link=" << ChannelTargets(helya, helya.GetGUID()) << '\n';
            Require(selfResult == SPELL_CAST_OK && helya.HasAura(195062, helya.GetGUID()) && !vigfus.HasAura(195062, helya.GetGUID()) && ChannelTargets(helya, helya.GetGUID()), "Native self-target control did not reproduce original recipient");
            helya.CastStop();
            helya.RemoveAurasDueToSpell(195062);
            auto targetResult = helya.CastSpell(&vigfus, 195062);
            TickChannel(helya, vigfus, 100);
            std::cout << "VIGFUS result=" << uint32(targetResult) << " caster-aura=" << helya.HasAura(195062, helya.GetGUID())
                << " vigfus-aura=" << vigfus.HasAura(195062, helya.GetGUID()) << " target-link=" << ChannelTargets(helya, vigfus.GetGUID()) << '\n';
            Require(targetResult == SPELL_CAST_OK && !helya.HasAura(195062, helya.GetGUID()) && vigfus.HasAura(195062, helya.GetGUID()) && ChannelTargets(helya, vigfus.GetGUID()), "Native channel did not reach actual Vigfus target");
            Require(!helya.isInCombat() && !vigfus.isInCombat(), "Native cosmetic channel prematurely starts combat");
            Require(helya.GetChannelSpellId() == 195062 && vigfus.GetChannelSpellId() == 0, "Caster/channel ownership reversed");
            TickChannel(helya, vigfus, 7100);
            Require(!vigfus.HasAura(195062, helya.GetGUID()) && !helya.GetChannelSpellId() && !ChannelTargets(helya, vigfus.GetGUID()), "Native finite channel did not clean up");
            Require(helya.CastSpell(&vigfus, 195062) == SPELL_CAST_OK, "Native recipient-removal channel rejected");
            TickChannel(helya, vigfus, 100);
            Require(vigfus.HasAura(195062, helya.GetGUID()), "Recipient-removal control has no real aura");
            map.RemoveFromMap(static_cast<Creature*>(&vigfus), false);
            for (uint32 elapsed = 0; elapsed < 500; elapsed += 10)
                helya.Unit::Update(10);
            Require(!helya.GetChannelSpellId() && !ChannelTargets(helya, vigfus.GetGUID()), "Native channel retained a missing recipient");
            std::cout << "PASS: real native Spell/Aura at matching1511 positions distinguishes Helya-self from Helya-to-Vigfus, preserves caster/recipient channel GUIDs and native7s cleanup. Null AIs/isolated map, not production intro or client visual proof.\n";
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

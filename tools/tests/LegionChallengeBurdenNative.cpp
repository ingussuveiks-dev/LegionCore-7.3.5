// Actual registered player-affix scripts, native challenge admission and auras.
// Player publication/health changes are controlled; no full client combat claim.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"
#ifdef CHALLENGE_BURDEN_BASELINE
#include "ChallengeBurdenActual.inc"
#else
void AddSC_challenge_scripts();
#endif

struct BurdenBindingsTag
{
    using Type = SpellScriptsContainer ScriptDataStoreMgr::*;
    friend Type Access(BurdenBindingsTag);
};
template struct FixtureAccess<BurdenBindingsTag, &ScriptDataStoreMgr::_spellScriptsStore>;

static void InitializeBurden()
{
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadScenarioData();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_challenge_scripts();
    sScriptMgr->SwapScriptContext(true);
    for (auto pair : {std::make_pair(206151u, "spell_challengers_burden"),
                      std::make_pair(240559u, "spell_challengers_grievous_wound")})
    {
        auto rows = WorldDatabase.Query(("SELECT ScriptName FROM spell_script_names WHERE spell_id=" + std::to_string(pair.first)).c_str());
        Require(rows && rows->GetRowCount() == 1 && rows->Fetch()[0].GetString() == pair.second,
            "Review changed native player-affix script binding");
        auto id = sObjectMgr->GetScriptId(pair.second);
        Require(sScriptMgr->GetSpellScriptLoader(id), "Actual player-affix registration missing");
        (sScriptDataStore->*Access(BurdenBindingsTag{})).emplace(pair.first, std::make_pair(id, true));
    }
    // Native periodic damage accesses these loader vectors. Restrict every
    // loader to actual selected rows in connection-local TEMP tables.
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
    {
        std::string name = table;
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE burden_" + name + " AS SELECT * FROM " + name + " WHERE entry IN(206151,240559)").c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE burden_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM burden_" + name).c_str());
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellPrcoCheck();
    sSpellMgr->LoadSpellBonusess();
}

struct BurdenScenario
{
    InstanceMap& map;
    Scenario* scenario;
    BurdenScenario(InstanceMap& m, LifecyclePlayer& owner) : map(m), scenario(sScenarioMgr->AddScenario(&m, 1175))
    {
        Require(scenario, "Native Burden scenario publication failed");
        scenario->CreateChallenge(&owner);
        Require(scenario->GetChallenge() && scenario->GetChallenge()->_canRun, "Native Burden challenge missing");
    }
    ~BurdenScenario()
    {
        map.m_scenarios.erase(scenario);
        sScenarioMgr->RemoveScenario(map.GetInstanceId());
    }
};

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true);
        InitializeBurden();
        for (auto id : {206151u, 240559u})
        {
            auto info = sSpellMgr->GetSpellInfo(id);
            Require(info, "Native player-affix spell missing");
            std::cout << "META spell=" << id << " duration=" << info->GetDuration(8)
                      << " stacks=" << info->GetAuraOptions(8)->CumulativeAura << '\n';
            auto original = sSpellStore.LookupEntry(id);
            if (original) std::cout << "META description=" << original->Description->Str[LOCALE_enUS]
                                    << " auraDescription=" << original->AuraDescription->Str[LOCALE_enUS] << '\n';
            for (unsigned i = 0; i < 4; ++i)
            {
                auto effect = info->GetEffect(static_cast<SpellEffIndex>(i), 8);
                if (effect && effect->IsEffect())
                    std::cout << "META effect=" << i << " aura=" << effect->ApplyAuraName
                              << " base=" << effect->BasePoints << " period=" << effect->ApplyAuraPeriod << '\n';
            }
        }
        unsigned contexts = 0;
        for (uint32 level : {2u, 4u, 7u, 10u})
        for (auto affix : {Affixes(0), Affixes::Overflowing, Affixes::Skittish, Affixes::Grievous})
        for (bool tank : {false, true})
        {
            MapInstanced parent(1492, 300);
            InstanceMap map(1492, 300, 9937000 + contexts, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
            map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map, 601, Position{2934.49f, 711.757f, 554.286f, 1.55255f});
            owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, tank ? CLASS_WARRIOR : CLASS_HUNTER);
            owner.SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID, tank ? 73 : 254);
            Check(owner.isInTankSpec() == tank, "Native specialization role differs");
            auto route = sMapChallengeModeStore.LookupEntry(208);
            Require(route, "Native Maw route missing");
            auto key = owner.Key(*route);
            key->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL, level);
            owner.m_challengeKeyInfo.Level = level;
            owner.m_challengeKeyInfo.Affix = 0;
            owner.m_challengeKeyInfo.Affix1 = uint32(affix);
            owner.m_challengeKeyInfo.Affix2 = 0;
            BurdenScenario progress(map, owner);
            auto challenge = progress.scenario->GetChallenge();
            challenge->OnPlayerEnterForScript(&owner);
            auto aura = owner.GetAura(ChallengersBurden);
            Require(aura && aura->m_loadedScripts.size() == 1, "Actual enter did not load Burden aura script");
            Check(aura->GetEffect(EFFECT_1)->GetAmount() == float(challenge->HasAffix(Affixes::Overflowing)), "Native Overflowing amount differs");
            bool skittish = challenge->HasAffix(Affixes::Skittish) && tank;
            float threatAmount = 0;
#ifdef CHALLENGE_BURDEN_BASELINE
            threatAmount = skittish ? 1.f : 0.f;
#else
            threatAmount = skittish ? aura->GetEffect(EFFECT_2)->GetSpellEffectInfo()->CalcValue(&owner) : 0.f;
#endif
            Check(aura->GetEffect(EFFECT_2)->GetAmount() == threatAmount, "Native Skittish role/affix amount differs");
            for (unsigned school = 0; school < MAX_SPELL_SCHOOL; ++school)
                Check(std::fabs(owner.ApplyTotalThreatModifier(100.f, SpellSchoolMask(1u << school)) - (100.f + threatAmount)) < .001f,
                    "Actual native Skittish threat modifier differs");
            auto periodic = aura->GetEffect(EFFECT_3);
            Require(periodic && periodic->GetPeriod() > 0, "Native Grievous periodic controller missing");
            Check(periodic->GetAmount() == periodic->GetSpellEffectInfo()->CalcValue(&owner), "Original uncalculated Grievous amount changed");
            owner.SetHealth(899999);
            aura->UpdateOwner(periodic->GetPeriod(), &owner);
            Check(owner.HasAura(ChallengerGrievousWound) == bool(periodic->GetAmount()), "Actual Burden tick did not honor its activation amount");
            std::cout << "BURDEN level=" << level << " affix=" << unsigned(affix) << " tank=" << tank
                      << " amount=" << periodic->GetAmount() << " wound=" << owner.HasAura(ChallengerGrievousWound) << '\n';
            owner.SetHealth(900000);
            aura->UpdateOwner(periodic->GetPeriod(), &owner);
            Check(!owner.HasAura(ChallengerGrievousWound), "Actual 90 percent Burden tick did not clear Grievous");
            challenge->OnPlayerLeaveForScript(&owner);
            Check(!owner.HasAura(ChallengersBurden), "Actual leave retained Burden");
            Check(std::fabs(owner.ApplyTotalThreatModifier(100.f) - 100.f) < .001f,
                "Actual native leave retained a threat modifier");
            if (level == 10 && affix == Affixes::Grievous && !tank)
            {
                owner.SetHealth(899999);
                Check(owner.CastSpell(&owner, ChallengerGrievousWound, true) == SPELL_CAST_OK,
                    "Native manual wound cast failed");
                auto wound = owner.GetAura(ChallengerGrievousWound);
                Require(wound && wound->m_loadedScripts.size() == 1, "Native wound script missing");
                std::cout << "WOUND amount=" << wound->GetEffect(EFFECT_0)->GetAmount()
                          << " period=" << wound->GetEffect(EFFECT_0)->GetPeriod() << '\n';
                auto health = owner.GetHealth();
                wound->UpdateOwner(wound->GetEffect(EFFECT_0)->GetPeriod(), &owner);
                std::cout << "WOUND before=" << health << " after=" << owner.GetHealth()
                          << " stacks=" << unsigned(wound->GetStackAmount()) << '\n';
                owner.RemoveAurasDueToSpell(ChallengerGrievousWound);
            }
            ++contexts;
        }
        Check(contexts == 32, "Player-affix test scope changed");
#ifdef CHALLENGE_BURDEN_BASELINE
        std::cout << "COUNTERFACTUAL " << checked << " native Burden checks: Skittish changes 100 threat to101 instead of25; Grievous activation remains separately OPEN. Not ordinary successful gameplay\n";
#else
        std::cout << "PASS " << checked << " native Burden checks: Skittish preserves native -75 tank threat with actual level/affix/role admission and leave cleanup; Grievous activation/damage and full client combat remain separately OPEN\n";
#endif
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

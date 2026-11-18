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
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            MapInstanced parent(1492, 300);
            InstanceMap map(1492, 300, 9937000 + contexts, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
            map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map, 601, Position{2934.49f, 711.757f, 554.286f, 1.55255f});
            owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
            owner.setFactionForRace(race);
            owner.SetDisplayId(sChrRacesStore.LookupEntry(race)->MaleDisplayID);
            owner.SetNativeDisplayId(owner.GetDisplayId());
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
            threatAmount = skittish ? aura->GetEffect(EFFECT_2)->GetSpellEffectInfo()->CalcValue(&owner) : 0.f;
            Check(aura->GetEffect(EFFECT_2)->GetAmount() == threatAmount, "Native Skittish role/affix amount differs");
            for (unsigned school = 0; school < MAX_SPELL_SCHOOL; ++school)
                Check(std::fabs(owner.ApplyTotalThreatModifier(100.f, SpellSchoolMask(1u << school)) - (100.f + threatAmount)) < .001f,
                    "Actual native Skittish threat modifier differs");
            auto periodic = aura->GetEffect(EFFECT_3);
            Require(periodic && periodic->GetPeriod() > 0, "Native Grievous periodic controller missing");
#ifdef CHALLENGE_BURDEN_BASELINE
            Check(periodic->GetAmount() == periodic->GetSpellEffectInfo()->CalcValue(&owner), "Original uncalculated Grievous amount changed");
#else
            Check(periodic->GetAmount() == float(challenge->HasAffix(Affixes::Grievous)), "Native Grievous amount ignores actual affix admission");
#endif
            owner.SetHealth(899999);
            aura->UpdateOwner(periodic->GetPeriod(), &owner);
            Check(owner.HasAura(ChallengerGrievousWound) == bool(periodic->GetAmount()), "Actual Burden tick did not honor its activation amount");
            std::cout << "BURDEN level=" << level << " affix=" << unsigned(affix) << " tank=" << tank
                      << " race=" << unsigned(race) << " amount=" << periodic->GetAmount()
                      << " wound=" << owner.HasAura(ChallengerGrievousWound) << '\n';
            if (periodic->GetAmount())
            {
                auto wound = owner.GetAura(ChallengerGrievousWound);
                Require(wound && wound->m_loadedScripts.size() == 1, "Actual controller cast did not load wound script");
                aura->UpdateOwner(periodic->GetPeriod(), &owner);
                Check(owner.GetAura(ChallengerGrievousWound) == wound && wound->GetStackAmount() == 1,
                    "Repeated Burden ticks replaced or stacked an existing wound");
                owner.RemoveAurasDueToSpell(ChallengerGrievousWound);
                owner.ApplySpellImmune(0, IMMUNITY_ID, ChallengerGrievousWound, true);
                aura->UpdateOwner(periodic->GetPeriod(), &owner);
                Check(!owner.HasAura(ChallengerGrievousWound), "Native wound immunity did not reject the cast");
                owner.ApplySpellImmune(0, IMMUNITY_ID, ChallengerGrievousWound, false);
                aura->UpdateOwner(periodic->GetPeriod(), &owner);
                Check(owner.HasAura(ChallengerGrievousWound), "Failed native wound cast consumed the retry");
            }
            owner.SetHealth(900000);
            aura->UpdateOwner(periodic->GetPeriod(), &owner);
            Check(!owner.HasAura(ChallengerGrievousWound), "Actual 90 percent Burden tick did not clear Grievous");
            challenge->OnPlayerLeaveForScript(&owner);
            Check(!owner.HasAura(ChallengersBurden), "Actual leave retained Burden");
            Check(std::fabs(owner.ApplyTotalThreatModifier(100.f) - 100.f) < .001f,
                "Actual native leave retained a threat modifier");
            challenge->OnPlayerEnterForScript(&owner);
            aura = owner.GetAura(ChallengersBurden);
            Require(aura, "Actual challenge re-entry lost Burden");
            owner.SetHealth(899999);
            aura->UpdateOwner(aura->GetEffect(EFFECT_3)->GetPeriod(), &owner);
            Check(owner.HasAura(ChallengerGrievousWound) == bool(aura->GetEffect(EFFECT_3)->GetAmount()),
                "Actual challenge re-entry changed Grievous eligibility");
            challenge->OnPlayerLeaveForScript(&owner);
            owner.RemoveAurasDueToSpell(ChallengerGrievousWound);
            if (level == 10 && affix == Affixes::Grievous && !tank && race == RACE_HUMAN)
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
#ifdef CHALLENGE_BURDEN_BASELINE
                Check(health - owner.GetHealth() == 500000 && wound->GetStackAmount() == 2,
                    "Original tenfold native wound damage changed");
                owner.SetHealth(900000);
                wound->UpdateOwner(wound->GetEffect(EFFECT_0)->GetPeriod(), &owner);
                Check(owner.HasAura(ChallengerGrievousWound) && owner.GetHealth() == 150000,
                    "Original healed-player damage after leaving was not reproduced");
#else
                Check(health - owner.GetHealth() == 50000 && wound->GetStackAmount() == 2,
                    "Actual native first wound tick does not deal two 2.5-percent stacks");
                for (unsigned stacks : {3u, 4u, 5u, 5u})
                {
                    owner.SetHealth(899999);
                    health = owner.GetHealth();
                    wound->UpdateOwner(wound->GetEffect(EFFECT_0)->GetPeriod(), &owner);
                    std::cout << "WOUND tickStacks=" << unsigned(wound->GetStackAmount())
                              << " damage=" << health - owner.GetHealth() << '\n';
                    Check(wound->GetStackAmount() == stacks, "Native wound exceeded its matching five-stack cap");
                    Check(health - owner.GetHealth() == 25000u * stacks, "Native wound lost fractional percentage or multiplied stacks twice");
                }
                wound->SetStackAmount(3);
                float partial = 35000.f; // Native 7 percent, scaled to half a tick.
                wound->CallScriptEffectChangeTickDamageHandlers(wound->GetEffect(EFFECT_0), partial, &owner);
                Check(std::fabs(partial - 37500.f) < .01f,
                    "Fractional wound correction discarded the incoming partial-tick scale");
                float prevented = 0.f;
                wound->CallScriptEffectChangeTickDamageHandlers(wound->GetEffect(EFFECT_0), prevented, &owner);
                Check(prevented == 0.f, "Fractional wound correction restored already-zero damage");
                for (uint32 healed : {900000u, 1000000u})
                {
                    owner.RemoveAurasDueToSpell(ChallengerGrievousWound);
                    owner.SetHealth(899999);
                    Check(owner.CastSpell(&owner, ChallengerGrievousWound, true) == SPELL_CAST_OK,
                        "Native wound recast failed");
                    wound = owner.GetAura(ChallengerGrievousWound);
                    Require(wound, "Actual wound recast not applied");
                    owner.SetHealth(healed);
                    wound->UpdateOwner(wound->GetEffect(EFFECT_0)->GetPeriod(), &owner);
                    Check(!owner.HasAura(ChallengerGrievousWound) && owner.GetHealth() == healed,
                        "Healed wound after Burden removal dealt a final damage tick or remained active");
                }
#endif
                owner.RemoveAurasDueToSpell(ChallengerGrievousWound);
            }
            ++contexts;
        }
        Check(contexts == 64, "Player-affix test scope changed");
        {
            MapInstanced parent(1492, 300);
            InstanceMap map(1492, 300, 9937999, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
            map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map, 611, Position{2934.49f, 711.757f, 554.286f, 1.55255f});
            LifecyclePlayer member(map, 612, owner.GetPosition());
            auto route = sMapChallengeModeStore.LookupEntry(208);
            Require(route, "Native group route missing");
            auto key = owner.Key(*route);
            LifecycleGroup group(owner, &member, *route, *key);
            group.group.m_affixes = {0, uint32(Affixes::Grievous), 0};
            BurdenScenario progress(map, owner);
            auto challenge = progress.scenario->GetChallenge();
            for (auto player : {&owner, &member})
            {
                challenge->OnPlayerEnterForScript(player);
                player->SetHealth(899999);
                auto burden = player->GetAura(ChallengersBurden);
                Require(burden, "Actual group member lacks Burden");
                burden->UpdateOwner(burden->GetEffect(EFFECT_3)->GetPeriod(), player);
            }
#ifdef CHALLENGE_BURDEN_BASELINE
            Check(!owner.HasAura(ChallengerGrievousWound) && !member.HasAura(ChallengerGrievousWound),
                "Original group Grievous unexpectedly enabled");
#else
            Check(owner.HasAura(ChallengerGrievousWound, owner.GetGUID()) && member.HasAura(ChallengerGrievousWound, member.GetGUID()),
                "Native group wounds lost their actual self-casters");
            owner.SetHealth(900000);
            auto burden = owner.GetAura(ChallengersBurden);
            burden->UpdateOwner(burden->GetEffect(EFFECT_3)->GetPeriod(), &owner);
            Check(!owner.HasAura(ChallengerGrievousWound) && member.HasAura(ChallengerGrievousWound),
                "Healing one native group player removed another player's wound");
#endif
            for (auto player : {&owner, &member})
            {
                challenge->OnPlayerLeaveForScript(player);
                player->RemoveAurasDueToSpell(ChallengerGrievousWound);
            }
        }
#ifdef CHALLENGE_BURDEN_BASELINE
        std::cout << "COUNTERFACTUAL " << checked << " native Burden checks: Grievous activation absent, wound uses tenfold percentage and damages a healed player after Burden removal. Not ordinary successful gameplay\n";
#else
        std::cout << "PASS " << checked << " native Burden checks: native affix/role admission and threat, real Grievous activation/stack damage/healing/retry; full client combat remains separate\n";
#endif
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

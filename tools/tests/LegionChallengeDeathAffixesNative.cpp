// Actual native death/affix casts and aura damage; actor AI/publication controlled.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"
#ifdef CHALLENGE_DEATH_BASELINE
#include "ChallengeDeathActual.inc"
#else
void AddSC_challenge_scripts();
#endif

struct DeathAffixBindingsTag
{
    using Type = SpellScriptsContainer ScriptDataStoreMgr::*;
    friend Type Access(DeathAffixBindingsTag);
};
template struct FixtureAccess<DeathAffixBindingsTag, &ScriptDataStoreMgr::_spellScriptsStore>;

static void InitializeDeathAffixes()
{
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon(); sObjectMgr->LoadScenarioData();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_challenge_scripts(); sScriptMgr->SwapScriptContext(true);
    auto bindings = WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(206150,209859,240443,243237)");
    Require(bindings && bindings->GetRowCount() == 3, "Review changed death-affix script binding scope");
    do
    {
        auto f = bindings->Fetch(); auto name = f[1].GetString();
        auto id = sObjectMgr->GetScriptId(name.c_str());
        Require(sScriptMgr->GetSpellScriptLoader(id), "Actual death-affix script not registered");
        (sScriptDataStore->*Access(DeathAffixBindingsTag{})).emplace(f[0].GetUInt32(), std::make_pair(id, true));
    } while (bindings->NextRow());
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(206150,209859,240443,243237)");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "Review and load changed native affix spell conditions");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
    {
        std::string name = table;
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE death_" + name + " AS SELECT * FROM " + name + " WHERE entry IN(206150,209859,240443,243237)").c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE death_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM death_" + name).c_str());
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellPrcoCheck(); sSpellMgr->LoadSpellBonusess();
}

struct DeathAffixActor : Creature
{
    DeathAffixActor(InstanceMap& map, uint32 entry, uint32 guid, float x = 2934.49f)
    {
        Require(Create(guid, &map, 1, entry, 0, ALLIANCE, x, 715.757f, 554.286f, 1.55255f), "Native death-affix actor create failed");
        SetHomePosition(*this);
        Require(map.AddToMap(static_cast<Creature*>(this)), "Native death-affix actor publication failed");
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)), "Controlled death-affix AI failed");
        GetInstanceScript()->OnCreatureCreate(this); GetInstanceScript()->OnCreatureCreateForScript(this);
    }
    ~DeathAffixActor()
    {
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

struct DeathAffixScenario
{
    InstanceMap& map; Scenario* scenario;
    DeathAffixScenario(InstanceMap& m, LifecyclePlayer& owner) : map(m), scenario(sScenarioMgr->AddScenario(&m, 1175))
    {
        Require(scenario, "Native death-affix scenario missing"); scenario->CreateChallenge(&owner);
        Require(scenario->GetChallenge() && scenario->GetChallenge()->_canRun, "Native death-affix challenge unavailable");
    }
    ~DeathAffixScenario() { map.m_scenarios.erase(scenario); sScenarioMgr->RemoveScenario(map.GetInstanceId()); }
};

#ifndef CHALLENGE_DEATH_SUPPORT_ONLY
int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true); InitializeDeathAffixes();
        for (auto id : {209859u, 240443u, 243237u})
        {
            auto info = sSpellMgr->GetSpellInfo(id); auto original = sSpellStore.LookupEntry(id);
            Require(info && original, "Native death-affix metadata missing");
            std::cout << "META spell=" << id << " duration=" << info->GetDuration(8)
                      << " stacks=" << info->GetAuraOptions(8)->CumulativeAura
                      << " description=" << original->Description->Str[LOCALE_enUS]
                      << " auraDescription=" << original->AuraDescription->Str[LOCALE_enUS] << '\n';
            for (unsigned i = 0; i < 4; ++i)
            {
                auto e = info->GetEffect(static_cast<SpellEffIndex>(i), 8);
                if (e && e->IsEffect()) std::cout << "META effect=" << i << " type=" << e->Effect
                    << " aura=" << e->ApplyAuraName << " base=" << e->BasePoints << " period=" << e->ApplyAuraPeriod
                    << " targetA=" << e->TargetA.GetTarget() << " targetB=" << e->TargetB.GetTarget()
                    << " radius=" << e->CalcRadius() << '\n';
            }
        }
        unsigned contexts = 0;
        for (auto affix : {Affixes::Bolstering, Affixes::Bursting})
        for (unsigned killerKind = 0; killerKind < 4; ++killerKind)
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            MapInstanced parent(1492, 300);
            InstanceMap map(1492, 300, 9938000 + contexts, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
            map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map, 701, Position{2934.49f, 711.757f, 554.286f, 1.55255f});
            LifecyclePlayer member(map, 702, Position{2934.49f, 710.757f, 554.286f, 1.55255f});
            for (auto player : {&owner, &member})
            {
                player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player->setFactionForRace(race);
                auto display = sChrRacesStore.LookupEntry(race)->MaleDisplayID;
                player->SetDisplayId(display); player->SetNativeDisplayId(display);
            }
            auto route = sMapChallengeModeStore.LookupEntry(208); Require(route, "Native Maw route missing");
            auto key = owner.Key(*route); owner.m_challengeKeyInfo.Affix = uint32(affix);
            owner.m_challengeKeyInfo.Affix1 = owner.m_challengeKeyInfo.Affix2 = 0;
            LifecycleGroup group(owner, &member, *route, *key); group.group.m_affixes = {uint32(affix), 0, 0};
            DeathAffixScenario progress(map, owner);
            DungeonPet pet(owner, map); DungeonGuardian guardian(owner, map);
            DeathAffixActor source(map, 97043, 172001), second(map, 97043, 172002), receiver(map, 97043, 172003);
            DeathAffixActor boss(map, 96754, 172004), idle(map, 97043, 172005), distant(map, 97043, 172006, 3034.49f);
            for (auto npc : {&source, &second, &receiver, &boss, &distant}) npc->SetInCombatWith(&owner);
            owner.SetInCombatWith(&source); member.SetInCombatWith(&source);
            Unit* killer = killerKind == 0 ? static_cast<Unit*>(&owner) : killerKind == 1 ? static_cast<Unit*>(&pet)
                : killerKind == 2 ? static_cast<Unit*>(&guardian) : static_cast<Unit*>(&receiver);
            auto hp = receiver.GetMaxHealth();
            std::cout << "BOLSTER beforeMod=" << receiver.GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT)
                      << " baseValue=" << receiver.GetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE)
                      << " isSummon=" << receiver.isAnySummons()
                      << " vehicle=" << receiver.IsVehicle()
                      << " vehicleId=" << receiver.GetCreatureTemplate()->VehicleId << '\n';
            auto damageMod = receiver.GetModifierValue(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT);
            killer->Kill(&source);
            Check(!source.IsAlive(), "Native death was not applied");
            Check(receiver.GetAuraCount(ChallengerBolstering) == (affix == Affixes::Bolstering ? 1u : 0u), "Native first death Bolster dispatch differs");
            Check(owner.GetAuraCount(ChallengerBursting) == (affix == Affixes::Bursting ? 1u : 0u)
                && member.GetAuraCount(ChallengerBursting) == owner.GetAuraCount(ChallengerBursting), "Native first death group Burst dispatch differs");
            Check(!pet.HasAura(ChallengerBursting) && !guardian.HasAura(ChallengerBursting), "Native Burst selected player pet/guardian");
            Check(!boss.HasAura(ChallengerBolstering) && !idle.HasAura(ChallengerBolstering) && !distant.HasAura(ChallengerBolstering), "Native Bolster admitted boss/idle/out-of-range actor");
            std::cout << "DEATH affix=" << unsigned(affix) << " killer=" << killerKind
                      << " sourceDisabled=" << source.disableAffix << " receiverCount=" << receiver.GetAuraCount(ChallengerBolstering)
                      << " receiverHpBefore=" << hp << " after=" << receiver.GetMaxHealth()
                      << " ownerBurst=" << owner.GetAuraCount(ChallengerBursting)
                      << " memberBurst=" << member.GetAuraCount(ChallengerBursting)
                      << " petBurst=" << pet.GetAuraCount(ChallengerBursting)
                      << " bossBolster=" << boss.GetAuraCount(ChallengerBolstering)
                      << " idleBolster=" << idle.GetAuraCount(ChallengerBolstering)
                      << " farBolster=" << distant.GetAuraCount(ChallengerBolstering) << '\n';
            if (auto bolster = receiver.GetAura(ChallengerBolstering))
            {
                auto application = bolster->GetApplicationOfTarget(receiver.GetGUID());
                std::cout << "BOLSTER mask=" << application->GetEffectMask()
                          << " canStats=" << receiver.CanModifyStats()
                          << " totalHealth=" << receiver.GetTotalAuraModValue(UNIT_MOD_HEALTH)
                          << " hpMod=" << receiver.GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT)
                          << " hpAmount=" << bolster->GetEffect(EFFECT_0)->GetAmount()
                          << " auraOwner=" << bolster->GetOwner()->GetGUID().GetGUIDLow()
                          << " caster=" << bolster->GetCasterGUID().GetGUIDLow()
                          << " stacks=" << unsigned(bolster->GetStackAmount()) << '\n';
                Check(std::fabs(double(receiver.GetMaxHealth()) / hp - 1.2) < .00001,
                    "Native first Bolster did not increase NPC-vehicle health");
            }
            killer->Kill(&source);
            Check(owner.GetAuraCount(ChallengerBursting) == (affix == Affixes::Bursting ? 1u : 0u)
                && receiver.GetAuraCount(ChallengerBolstering) == (affix == Affixes::Bolstering ? 1u : 0u), "Repeated native Kill duplicated death affix");
            killer->Kill(&second);
            if (affix == Affixes::Bolstering)
            {
                Check(receiver.GetAuraCount(ChallengerBolstering) == 2, "Second real death did not apply a second native Bolster");
                Check(std::fabs(receiver.GetModifierValue(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT) / damageMod - 1.44f) < .0001f,
                    "Native two Bolster damage modifiers differ from matching 20 percent effects");
                Check(std::fabs(double(receiver.GetMaxHealth()) / hp - 1.44) < .00001,
                    "Native two Bolster health modifiers differ from matching 20 percent effects");
                receiver.CombatStop(true);
                Check(!receiver.HasAura(ChallengerBolstering), "Native NPC leave-combat retained Bolster");
                Check(std::fabs(double(receiver.GetMaxHealth()) / hp - 1.0) < .00001,
                    "Native Bolster combat-exit cleanup retained extra health");
            }
            if (affix == Affixes::Bursting)
            {
                Check(owner.GetAuraCount(ChallengerBursting) == 2 && member.GetAuraCount(ChallengerBursting) == 2,
                    "Second real death did not apply two group Burst receipts");
                // Remove both actual corpse casters before ticking. The owned
                // periodic handler must keep working without those map objects.
                source.RemoveAllAuras(); second.RemoveAllAuras();
                map.RemoveFromMap(static_cast<Creature*>(&source), false);
                map.RemoveFromMap(static_cast<Creature*>(&second), false);
                auto health = owner.GetHealth(); unsigned auras = 0;
                auto range = owner.GetAppliedAuras().equal_range(ChallengerBursting);
                std::vector<Aura*> pulses;
                for (auto i = range.first; i != range.second; ++i)
                {
                    auto aura = i->second->GetBase(); ++auras; pulses.push_back(aura);
                    std::cout << "BURST caster=" << aura->GetCasterGUID().GetGUIDLow()
                              << " stacks=" << unsigned(aura->GetStackAmount())
                              << " period=" << aura->GetEffect(EFFECT_0)->GetPeriod() << '\n';
                }
                for (auto aura : pulses) aura->UpdateOwner(aura->GetEffect(EFFECT_0)->GetPeriod(), &owner);
                Check(auras == 2, "Review changed native per-caster Burst aura cardinality");
#ifdef CHALLENGE_DEATH_BASELINE
                Check(health - owner.GetHealth() == 200000, "Expected original double-counted Burst tick changed");
#else
                Check(health - owner.GetHealth() == 100000, "Two native Burst applications did not deal two 5-percent ticks");
#endif
                std::cout << "BURST auras=" << auras << " totalStacks=" << owner.GetAuraCount(ChallengerBursting)
                          << " damage=" << health - owner.GetHealth() << '\n';
                owner.SetHealth(1000000);
                pulses[0]->ModStackAmount(1);
                health = owner.GetHealth();
                for (auto aura : pulses) aura->UpdateOwner(aura->GetEffect(EFFECT_0)->GetPeriod(), &owner);
#ifdef CHALLENGE_DEATH_BASELINE
                Check(health - owner.GetHealth() == 300000, "Expected original mixed-stack Burst amplification changed");
#else
                Check(health - owner.GetHealth() == 150000, "Mixed native Burst stacks multiplied other aura stacks");
#endif
                owner.RemoveAurasDueToSpell(ChallengerBursting);
                owner.SetHealth(1000000);
                // Fresh real casts for actual Unit aura scheduling/expiry.
                receiver.CastSpell(&receiver, ChallengerBursting, true);
                owner.Unit::Update(1000);
                Check(owner.GetHealth() == 1000000, "Native Burst ticked before its 2000ms period");
                owner.Unit::Update(1000);
                Check(owner.GetHealth() == 950000, "Single native Burst did not tick for 5 percent at2000ms");
                owner.Unit::Update(2000);
                auto expired = owner.GetAura(ChallengerBursting);
                Check(owner.GetHealth() == 900000 && expired && expired->IsExpired(), "Native four-second Burst duration differs");
                // Native Unit removes expired owned auras before the next
                // frame's UpdateOwner dispatch, rather than inside that call.
                owner.Unit::Update(1);
                Check(!owner.HasAura(ChallengerBursting) && owner.GetHealth() == 900000, "Native next-frame Burst expiry differs");
                health = owner.GetHealth(); owner.Unit::Update(4000);
                Check(owner.GetHealth() == health, "Expired native Burst still dealt damage");
            }
            ++contexts;
        }
        Check(contexts == 16, "Native death-affix scope changed");
#ifdef CHALLENGE_DEATH_BASELINE
        std::cout << "COUNTERFACTUAL " << checked << " native death-affix checks: distinct Burst auras each multiply all applications' stacks; not ordinary successful gameplay\n";
#else
        std::cout << "PASS " << checked << " native death-affix checks: player/pet/guardian/NPC kills, two races, native group area selection, once-only death, NPC-vehicle Bolster health/damage/cleanup, per-aura Burst stacks, missing casters and actual periodic expiry. No full client combat claim\n";
#endif
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
#endif

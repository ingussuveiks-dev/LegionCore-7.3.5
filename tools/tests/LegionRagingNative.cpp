// Actual Might/Raging proc and native incoming damage dispatch.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "LegionChallengeDeathAffixesNative.cpp"
#include <fstream>

static void InitializeRaging()
{
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon(); sObjectMgr->LoadScenarioData();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_challenge_scripts(); sScriptMgr->SwapScriptContext(true);
    auto binding = WorldDatabase.Query("SELECT ScriptName FROM spell_script_names WHERE spell_id=206150");
    Require(binding && binding->GetRowCount() == 1 && binding->Fetch()[0].GetString() == "spell_challengers_might", "Native Might binding changed");
    auto script = sObjectMgr->GetScriptId("spell_challengers_might"); Require(sScriptMgr->GetSpellScriptLoader(script), "Actual Might script absent");
    (sScriptDataStore->*Access(DeathAffixBindingsTag{})).emplace(206150, std::make_pair(script, true));
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17,24) AND SourceEntry IN(206150,228318)");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "Load changed Raging conditions before native run");
    if (std::getenv("LEGION_RAGING_VERIFY_LIVE"))
    {
        auto rows = WorldDatabase.Query("SELECT effectmask,procFlags,SchoolMask,SpellFamilyName,SpellFamilyMask0,SpellFamilyMask1,SpellFamilyMask2,SpellFamilyMask3,procEx,ppmRate,CustomChance,Cooldown FROM spell_proc_event WHERE entry=206150 ORDER BY effectmask");
        Check(!rows, "Live Might proc data changed;review effect dispatch before this native run");
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
    {
        std::string name = table;
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE raging_" + name + " AS SELECT * FROM " + name + " WHERE entry IN(206150,228318)").c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE raging_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM raging_" + name).c_str());
    }
#ifdef RAGING_DATA_ONLY
    auto path = std::getenv("LEGION_RAGING_MIGRATION"); Require(path, "Raging migration path missing");
    std::ifstream migration(path); Require(bool(migration), "Actual Raging migration unavailable");
    std::string text, line;
    while (std::getline(migration, line)) if (line.find("--") != 0) text += line + '\n';
    std::istringstream statements(text);
    while (std::getline(statements, line, ';')) if (line.find_first_not_of(" \t\r\n") != std::string::npos) WorldDatabase.DirectExecute(line.c_str());
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellPrcoCheck(); sSpellMgr->LoadSpellBonusess();
    Require(sSpellMgr->GetSpellInfo(206150)->EffectMask == 16383, "Matching fourteen-effect Might mask changed");
}

static void Metadata()
{
    for (uint32 id : {206150u, 228318u})
    {
        auto info = sSpellMgr->GetSpellInfo(id); auto row = sSpellStore.LookupEntry(id); Require(info && row, "Matching native Raging spell missing");
        std::cout << "META spell=" << id << " name=" << row->Name->Str[LOCALE_enUS] << " description=" << row->Description->Str[LOCALE_enUS]
            << " duration=" << info->GetDuration(8) << " dispel=" << unsigned(info->Categories.DispelType) << " interrupt=" << info->AuraInterruptFlags[0]
            << " stacks=" << info->GetAuraOptions(8)->CumulativeAura << " chance=" << unsigned(info->GetAuraOptions(8)->ProcChance)
            << " procFlags=" << info->GetAuraOptions(8)->ProcTypeMask << '\n';
        for (unsigned i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            auto e = info->GetEffect(static_cast<SpellEffIndex>(i), 8);
            if (e && e->IsEffect()) std::cout << "META effect=" << i << " type=" << e->Effect << " aura=" << e->ApplyAuraName
                << " base=" << e->BasePoints << " trigger=" << e->TriggerSpell << " misc=" << e->MiscValue << " target=" << e->TargetA.GetTarget() << '\n';
        }
    }
    if (auto rows = sSpellMgr->GetSpellProcEvent(206150)) for (auto const& row : *rows)
        std::cout << "META proc flags=" << row.procFlags << " extra=" << row.procEx << " effectMask=" << row.effectMask << " chance=" << row.customChance << '\n';
}

static void Receipt(Unit& attacker, Creature& target, uint64 damage, uint32 extra = PROC_EX_NORMAL_HIT, uint64 absorb = 0)
{
    DamageInfo receipt(&attacker, &target, damage + absorb, nullptr, SPELL_SCHOOL_MASK_NORMAL, DIRECT_DAMAGE, 0);
    receipt.AbsorbDamage(absorb);
    attacker.ProcDamageAndSpell(&target, PROC_FLAG_DONE_MELEE_AUTO_ATTACK,
        PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK | PROC_FLAG_TAKEN_DAMAGE, extra, &receipt);
}

#if !defined(RAGING_ORIGINAL_DATA) && !defined(RAGING_DATA_ONLY)
struct RagingContext
{
    MapInstanced parent; InstanceMap map; LifecyclePlayer owner, member; Item* key; LifecycleGroup group;
    DeathAffixScenario progress; DeathAffixActor npc, vehicle, boss;
    static InstanceMap& Ready(InstanceMap& map) { map.CreateInstanceData(nullptr); return map; }
    static MapChallengeModeEntry const& Route() { auto row = sMapChallengeModeStore.LookupEntry(208); Require(row, "Native Maw route missing"); return *row; }
    LifecyclePlayer& Configure(uint8 race, uint32 level, bool enabled)
    {
        for (auto player : {&owner, &member})
        {
            player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player->setFactionForRace(race);
            auto display = sChrRacesStore.LookupEntry(race)->MaleDisplayID; player->SetDisplayId(display); player->SetNativeDisplayId(display);
        }
        key->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL, level); owner.m_challengeKeyInfo.Level = level;
        group.group.m_challengeLevel = level; group.group.m_affixes = {enabled ? uint32(Affixes::Raging) : 0, 0, 0}; return owner;
    }
    RagingContext(uint8 race, uint32 level, bool enabled, unsigned id)
        : parent(1492, 300), map(1492, 300, 9941100 + id, DIFFICULTY_MYTHIC_KEYSTONE, &parent),
          owner(Ready(map), 771, Position{2937.49f, 715.757f, 554.286f, 1.55255f}), member(map, 772, owner.GetPosition()),
          key(owner.Key(Route())), group(owner, &member, Route(), *key), progress(map, Configure(race, level, enabled)),
          npc(map, 97032, 176701), vehicle(map, 97043, 176702), boss(map, 96754, 176703)
    { npc.setFaction(16); }
};

struct EvadeReceiptTag
{
    using Type = bool (CreatureAI::*)(); friend Type Access(EvadeReceiptTag);
};
template struct FixtureAccess<EvadeReceiptTag, &CreatureAI::_EnterEvadeMode>;

static void ResetTarget(Creature& target, uint64 maximum, uint64 health)
{
    target.RemoveAurasDueToSpell(228318); target.SetMaxHealth(maximum); target.SetHealth(health);
    Require(target.GetMaxHealth() == maximum && target.GetHealth() == health, "Native uint64 health fixture differs");
}

static void CheckIncoming(Unit& attacker, Creature& target)
{
    Require(!target.HasScalableLevels(), "Exact health boundary fixture must use a native unscaled target");
    struct Case { uint64 maximum, health; uint32 damage, absorb; bool rage; };
    for (Case const& test : {
        Case{1000000,310001,10000,0,false}, Case{1000000,310000,10000,0,true},
        Case{1000000,310000,20000,0,true}, Case{1000000,450000,200000,0,true},
        Case{1000000,300001,1,0,true}, Case{1000000,290000,1,0,true},
        Case{1000000,310000,1,50000,false}, Case{1000000,290000,0,50000,false},
        Case{1000003,300002,1,0,false}, Case{1000003,300001,1,0,true},
        Case{18014398509481987ull,5404319552844697ull,100,0,false},
        Case{18014398509481987ull,5404319552844696ull,100,0,true}})
    {
        ResetTarget(target, test.maximum, test.health);
        float original = target.GetTotalAuraMultiplier(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        Receipt(attacker, target, test.damage, PROC_EX_NORMAL_HIT | (test.absorb ? PROC_EX_ABSORB : 0), test.absorb);
        Check(!target.HasAura(228318), "Raging fired from the pre-damage receipt");
        auto dealt = attacker.DealDamage(&target, test.damage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        if (target.GetHealth() != test.health - test.damage)
            std::cout << "INSPECT healthMismatch entry=" << target.GetEntry() << " maximum=" << test.maximum << " initial=" << test.health
                << " incoming=" << test.damage << " dealt=" << dealt << " health=" << target.GetHealth() << '\n';
        Check(target.IsAlive() && target.GetHealth() == test.health - test.damage, "Native final incoming health differs");
        Check(target.HasAura(228318) == test.rage, "Raging result changed after native damage dispatch");
        Check(std::fabs(target.GetTotalAuraMultiplier(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE) / original - (test.rage ? 2.f : 1.f)) < .001f,
            "Native Raging did not preserve the authored100-percent damage increase");
    }
    ResetTarget(target, 1000000, 290000); Receipt(attacker, target, 1, PROC_EX_CRITICAL_HIT);
    attacker.DealDamage(&target, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    Check(target.GetAuraCount(228318) == 1, "Critical incoming hit did not apply exactly one native Enrage");
    float enraged = target.GetTotalAuraMultiplier(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    for (unsigned i = 0; i < 4; ++i)
    {
        Receipt(attacker, target, 1); attacker.DealDamage(&target, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    }
    Check(target.GetAuraCount(228318) == 1 && std::fabs(target.GetTotalAuraMultiplier(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE) - enraged) < .001f,
        "Repeated incoming receipts stacked the native non-stacking Enrage");
    target.SetHealth(1000000); Receipt(attacker, target, 1);
    Check(target.GetAuraCount(228318) == 1, "Healing erased the native until-defeated Enrage");
    target.RemoveAurasDueToSpell(228318); target.SetHealth(290000); Receipt(attacker, target, 1);
    attacker.DealDamage(&target, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    Check(target.GetAuraCount(228318) == 1, "Native aura removal prevented a later eligible damage receipt");
}

static void CheckScaledIncoming(Unit& attacker, Creature& target)
{
    Require(target.HasScalableLevels(), "Actual native scaled NPC fixture lost its level range");
    auto maximum = target.GetMaxHealth();
    struct Case { double before, loss; bool rage; };
    for (Case const& test : {Case{31,.2,false},Case{30.1,.2,true},Case{29,.2,true}})
    {
        ResetTarget(target, maximum, uint64(double(maximum) * test.before / 100));
        auto visibleMaximum = target.GetMaxHealth(&attacker);
        float factor = target.GetHealthMultiplierForTarget(&attacker);
        auto damage = uint32(double(visibleMaximum) * test.loss / 100 * factor);
        Require(damage > 0, "Native scale conversion produced no hit");
        Receipt(attacker, target, damage); Check(!target.HasAura(228318), "Scaled NPC enraged before final damage");
        attacker.DealDamage(&target, damage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        Check(target.IsAlive() && std::fabs(target.GetHealthPct() - float(test.before - test.loss)) < .02f,
            "Actual native scaled damage endpoint differs");
        Check(target.HasAura(228318) == test.rage, "Scaled final health did not govern Raging");
        std::cout << "RAGING scaled entry=" << target.GetEntry() << " factor=" << factor << " damage=" << damage << " finalPct=" << target.GetHealthPct() << '\n';
    }
}

struct RagingDamagePolicyAI : NullCreatureAI
{
    uint32 limit;
    RagingDamagePolicyAI(Creature* creature, uint32 value) : NullCreatureAI(creature), limit(value) { }
    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override { damage = std::min(damage, limit); }
};

static void Expanded()
{
    unsigned contexts = 0;
    for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)}) for (uint32 level : {4u, 10u, 15u})
    for (unsigned killer = 0; killer < 3; ++killer)
    {
        auto storage = std::make_unique<RagingContext>(race, level, true, contexts++); auto& context = *storage;
        DungeonPet pet(context.owner, context.map); DungeonGuardian guardian(context.owner, context.map);
        Unit* attacker = killer == 0 ? static_cast<Unit*>(&context.owner) : killer == 1 ? static_cast<Unit*>(&pet) : static_cast<Unit*>(&guardian);
        Check(context.boss.IsDungeonBoss() && !context.npc.IsDungeonBoss() && !context.vehicle.IsDungeonBoss(), "Native boss/ordinary/vehicle classification changed");
        for (auto target : {&context.npc, &context.vehicle})
        {
            if (target->HasScalableLevels()) CheckScaledIncoming(*attacker, *target); else CheckIncoming(*attacker, *target);
            Require(target->AddAura(2479, target), "Unrelated native evade aura missing");
            Require((target->AI()->*Access(EvadeReceiptTag{}))(), "Native evade cleanup endpoint did not run");
            Check(!target->HasAura(228318) && !target->isInCombat(), "Native evade retained old Raging/combat");
            Check(target->HasAura(ChallengersMight) == target->isAnySummons() && target->HasAura(2479) == target->isAnySummons(),
                "Raging evade cleanup altered native vehicle/ordinary aura retention");
            target->SetHealth(target->GetMaxHealth()); target->SetInCombatWith(attacker);
            Check(target->HasAura(ChallengersMight) && !target->HasAura(228318), "Native re-entry failed to restore fresh Might only");
            ResetTarget(*target, 1000000, 290000); Receipt(*attacker, *target, 1);
            attacker->DealDamage(target, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
            Check(target->HasAura(228318), "Fresh post-evade combat lost incoming threshold admission");
            target->CombatStop(true);
            context.progress.scenario->GetChallenge()->OnUnitCharmed(target, &context.owner);
            Check(!target->HasAura(228318) && !target->HasAura(ChallengersMight), "Native charm did not remove enemy Raging/Might");
            context.progress.scenario->GetChallenge()->OnUnitRemoveCharmed(target, &context.owner);
            Check(target->HasAura(ChallengersMight) && !target->HasAura(228318), "Native uncharm did not restore a fresh Might source");
            ResetTarget(*target, 100000, 100000); Receipt(*attacker, *target, 100000);
            Check(!target->HasAura(228318), "Lethal incoming receipt cast a transient corpse Enrage");
            if (!target->HasScalableLevels()) attacker->DealDamage(target, 100000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
            else attacker->Kill(target);
            attacker->Kill(target);
            Check(!target->IsAlive() && !target->HasAura(228318), "Native player/pet/guardian final blow retained corpse Enrage");
        }
        ResetTarget(context.boss, 1000000, 290000); Receipt(*attacker, context.boss, 1);
        attacker->DealDamage(&context.boss, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        Check(!context.boss.HasAura(228318), "Native dungeon boss received Raging");
        for (auto owned : {static_cast<Creature*>(&pet), static_cast<Creature*>(&guardian)})
        {
            context.map.GetInstanceScript()->OnCreatureCreateForScript(owned);
            Check(!owned->HasAura(ChallengersMight) && !owned->HasAura(228318), "Player-owned pet/guardian received enemy Raging");
        }
        Check(!context.owner.HasAura(228318) && !context.member.HasAura(228318), "NPC Raging leaked onto either group player");
        std::cout << "RAGING race=" << unsigned(race) << " level=" << level << " attacker=" << killer << " ordinary/vehicle/boss,pct/absorb/uint64,evade/charm/death=pass\n";
    }
    for (bool enabled : {false, true})
    {
        auto storage = std::make_unique<RagingContext>(RACE_HUMAN, 10, enabled, contexts++); auto& context = *storage;
        if (enabled)
        {
            context.npc.disableAffix |= 1 << Affixes::Raging; context.npc.RemoveAurasDueToSpell(ChallengersMight);
            Require(context.npc.AddAura(ChallengersMight, &context.npc), "Disabled native Might reapplication failed");
        }
        ResetTarget(context.npc, 1000000, 290000); Receipt(context.owner, context.npc, 1);
        context.owner.DealDamage(&context.npc, 1, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
        Check(!context.npc.HasAura(228318), "Absent/disabled Raging flag admitted Enrage");
    }
    {
        auto storage = std::make_unique<RagingContext>(RACE_HUMAN, 10, true, contexts++); auto& context = *storage;
        for (uint32 limit : {0u,5000u,15000u})
        {
            Require(context.vehicle.AIM_Initialize(new RagingDamagePolicyAI(&context.vehicle, limit)), "Native final damage policy AI unavailable");
            ResetTarget(context.vehicle, 1000000, 310000); Receipt(context.owner, context.vehicle, 20000);
            context.owner.DealDamage(&context.vehicle, 20000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
            Check(context.vehicle.GetHealth() == 310000 - limit && context.vehicle.HasAura(228318) == (limit == 15000),
                "Raging bypassed the actual final encounter/AI damage policy");
        }
        Require(context.vehicle.AIM_Initialize(new NullCreatureAI(&context.vehicle)), "Native neutral damage policy unavailable");
        for (DamageEffectType type : {DIRECT_DAMAGE,SPELL_DIRECT_DAMAGE,DOT})
        {
            ResetTarget(context.vehicle, 1000000, 310000);
            context.owner.DealDamage(&context.vehicle, 20000, nullptr, type, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
            Check(context.vehicle.GetHealth() == 290000 && context.vehicle.HasAura(228318), "Native melee/spell/periodic final damage Raging differs");
        }
    }
    Check(contexts == 21, "Native expanded Raging context scope changed");
}
#endif

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true); InitializeRaging(); Metadata();
        MapInstanced parent(1492, 300); InstanceMap map(1492, 300, 9941000, DIFFICULTY_MYTHIC_KEYSTONE, &parent); map.CreateInstanceData(nullptr);
        LifecyclePlayer owner(map, 761, Position{2937.49f, 715.757f, 554.286f, 1.55255f});
        auto route = sMapChallengeModeStore.LookupEntry(208); Require(route, "Native Maw route absent"); auto key = owner.Key(*route);
        owner.m_challengeKeyInfo.Affix = Affixes::Raging; owner.m_challengeKeyInfo.Affix1 = 0;
        DeathAffixScenario progress(map, owner); DeathAffixActor source(map, 97043, 176601);
        auto aura = source.GetAura(ChallengersMight); Require(aura && aura->m_loadedScripts.size() == 1, "Actual scripted Might missing");
        std::cout << "INSPECT ragingAmount=" << aura->GetEffect(EFFECT_2)->GetAmount() << " maxHP=" << source.GetMaxHealth() << '\n';
        for (uint32 health : {310000u, 300000u, 290000u})
        {
            source.RemoveAurasDueToSpell(228318); source.SetMaxHealth(1000000); source.SetHealth(health);
            Receipt(owner, source, 20000);
#ifdef RAGING_ORIGINAL_DATA
            Check(!source.HasAura(228318), "Original incoming-damage failure not reproduced");
#elif defined(RAGING_DATA_ONLY)
            Check(source.HasAura(228318) == (health <= 300000), "Original pre-hit health failure with corrected data not reproduced");
#else
            Check(!source.HasAura(228318), "Actual incoming receipt enraged before final damage");
#endif
            std::cout << "INSPECT before=" << health << " incoming=20000 preDamageEnrage=" << source.HasAura(228318);
            owner.DealDamage(&source, 20000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
#if !defined(RAGING_ORIGINAL_DATA) && !defined(RAGING_DATA_ONLY)
            Check(source.HasAura(228318), "Native final crossing did not enrage the living target");
#endif
            std::cout << " after=" << source.GetHealth() << " postDamageEnrage=" << source.HasAura(228318) << '\n';
        }
        source.RemoveAurasDueToSpell(228318); source.SetHealth(290000);
        DamageInfo outgoing(&source, &owner, 1, nullptr, SPELL_SCHOOL_MASK_NORMAL, DIRECT_DAMAGE, 0);
        source.ProcDamageAndSpell(&owner, PROC_FLAG_DONE_MELEE_AUTO_ATTACK, PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK | PROC_FLAG_TAKEN_DAMAGE, PROC_EX_NORMAL_HIT, &outgoing);
#ifdef RAGING_ORIGINAL_DATA
        Check(source.HasAura(228318), "Original outgoing-only activation not reproduced");
#else
        Check(!source.HasAura(228318), "Raging still triggered from the NPC's outgoing melee receipt");
#endif
        std::cout << "INSPECT npcOutgoingOnly=" << source.HasAura(228318) << '\n';
        source.CombatStop(true); std::cout << "INSPECT afterCombatStop=" << source.HasAura(228318) << '\n';
#if defined(RAGING_ORIGINAL_DATA) || defined(RAGING_DATA_ONLY)
        std::cout << "COUNTERFACTUAL " << checked << " native Raging checks:original incoming denial/outgoing activation or data-only pre-hit threshold delay;actual original game.lib required,not ordinary successful gameplay\n";
#else
        Expanded();
        std::cout << "PASS " << checked << " native Raging checks:21 contexts,ordinary/vehicle/boss,Player/Pet/Guardian receipts,exact30percent/absorb/uint64 thresholds,final AI/scaling,damage multiplier,repeat/heal,evade/re-entry,charm and lethal cleanup. Controlled AI/publication/hit receipts;client combat remains separate\n";
#endif
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End(); return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

// Matching native environmental affix metadata and real core dispatch.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "LegionChallengeDeathAffixesNative.cpp"
#include "AreaTriggerData.h"

static void InitializeEnvironmentRuntime()
{
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon(); sObjectMgr->LoadScenarioData();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_challenge_scripts(); sScriptMgr->SwapScriptContext(true);
    auto binding = WorldDatabase.Query("SELECT ScriptName FROM spell_script_names WHERE spell_id=206150");
    Require(binding && binding->GetRowCount() == 1 && binding->Fetch()[0].GetString() == "spell_challengers_might", "Native Might binding changed");
    auto id = sObjectMgr->GetScriptId("spell_challengers_might");
    Require(sScriptMgr->GetSpellScriptLoader(id), "Actual Might script not registered");
    (sScriptDataStore->*Access(DeathAffixBindingsTag{})).emplace(206150, std::make_pair(id, true));
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    auto shadow = [](std::string const& name, std::string const& predicate)
    {
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE env_" + name + " AS SELECT * FROM " + name + " WHERE " + predicate).c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE env_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM env_" + name).c_str());
    };
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
        shadow(table, "entry IN(206150,226489,226510,226512,209861,209862)");
    for (auto table : {"areatrigger_template", "areatrigger_data", "areatrigger_actions"})
        shadow(table, "entry=8688 OR customEntry=226489");
    for (auto table : {"areatrigger_template_circle", "areatrigger_template_sequence", "areatrigger_polygon", "areatrigger_scripts"})
        shadow(table, "entry IN(8688,226489)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellPrcoCheck(); sSpellMgr->LoadSpellBonusess();
    sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger, 100000);
    sAreaTriggerDataStore->LoadAreaTriggerScripts(); sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
    auto info = sAreaTriggerDataStore->GetAreaTriggerInfo(8688);
    Require(info && info->spellId == 226489 && info->actions.size() == 5, "Native Sanguine area scope changed");
}

struct EnvironmentActor : DeathAffixActor
{
    using DeathAffixActor::DeathAffixActor;
    ~EnvironmentActor() { RemoveAllAreaObjects(); GetMap()->RemoveAllObjectsInRemoveList(); }
};

static AreaTrigger* SanguineField(Creature& corpse)
{
    std::list<AreaTrigger*> fields; corpse.GetAreaObjectList(fields, 226489);
    Require(fields.size() == 1, "Native real death did not create one Sanguine field");
    return fields.front();
}

static void CheckSanguine(uint8 race, unsigned killerKind, unsigned removalKind, unsigned instance)
{
    MapInstanced parent(1492, 300);
    InstanceMap map(1492, 300, 9940000 + instance, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
    map.CreateInstanceData(nullptr);
    LifecyclePlayer owner(map, 731, Position{2934.69f, 715.757f, 554.286f, 1.55255f});
    LifecyclePlayer member(map, 732, owner.GetPosition());
    for (auto player : {&owner, &member})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player->setFactionForRace(race);
        auto display = sChrRacesStore.LookupEntry(race)->MaleDisplayID;
        player->SetDisplayId(display); player->SetNativeDisplayId(display);
    }
    auto route = sMapChallengeModeStore.LookupEntry(208); Require(route, "Native Maw route missing");
    auto key = owner.Key(*route); LifecycleGroup group(owner, &member, *route, *key);
    group.group.m_affixes = {uint32(Affixes::Sanguine), 0, 0};
    DeathAffixScenario progress(map, owner);
    DungeonPet pet(owner, map); DungeonGuardian guardian(owner, map);
    EnvironmentActor first(map, 97043, 175001), second(map, 97043, 175002, 2934.89f), ally(map, 97043, 175003);
    ally.SetHealth(ally.GetMaxHealth() / 2);
    Unit* killer = killerKind == 0 ? static_cast<Unit*>(&owner) : killerKind == 1 ? static_cast<Unit*>(&pet) : static_cast<Unit*>(&guardian);
    killer->Kill(&first); killer->Kill(&first); killer->Kill(&second);
    auto one = SanguineField(first); auto two = SanguineField(second);
    Check(one->GetDuration() == 60000 && one->GetRealEntry() == 8688 && two->GetRealEntry() == 8688,
        "Actual Sanguine native field duration/entry changed");
    one->Update(1499); two->Update(1499);
    Check(!owner.HasAura(226512) && !ally.HasAura(226510), "Sanguine applied before its native activation delay");
    one->Update(1); two->Update(1); one->Update(1); two->Update(1);
    Check(owner.GetAuraCount(226512) == 2 && member.GetAuraCount(226512) == 2 && ally.GetAuraCount(226510) == 2,
        "Two native Sanguine fields did not retain two independent contributions");
    std::cout << "ENV Sanguine overlap playerStacks=" << owner.GetAuraCount(226512)
              << " memberStacks=" << member.GetAuraCount(226512) << " healStacks=" << ally.GetAuraCount(226510)
              << " affected=" << one->IsUnitAffected(owner.GetGUID()) << '/' << two->IsUnitAffected(owner.GetGUID()) << '\n';
    auto health = owner.GetHealth(); auto allyHealth = ally.GetHealth();
    auto dot = owner.GetAura(226512); auto heal = ally.GetAura(226510);
    Require(dot && heal, "Actual native Sanguine child auras absent");
    Check(dot->GetCasterGUID() == owner.GetGUID() && owner.GetAura(226512)->GetEffect(EFFECT_0)->GetAmount() == 30.f
        && dot->GetEffect(EFFECT_0)->GetPeriod() == 1000 && dot->GetSpellInfo()->HasAttribute(SPELL_ATTR5_START_PERIODIC_AT_APPLY),
        "Matching native Sanguine dose/period/initial tick or self-caster changed");
    owner.Unit::Update(1);
    Check(health - owner.GetHealth() == 300000, "Native Sanguine first-frame tick did not deal two15-percent portions");
    owner.Unit::Update(999); ally.Unit::Update(1000);
    Check(health - owner.GetHealth() == 600000, "Native Sanguine initial and one-second tick cadence differs");
    Check(std::fabs(double(ally.GetHealth() - allyHealth) - double(ally.GetMaxHealth()) * .1) < 8,
        "Native Sanguine overlap did not heal two5-percent portions");
    if (removalKind == 0) one->Update(one->GetDuration());
    else if (removalKind == 1) one->Remove(false);
    else one->Despawn();
#ifdef CHALLENGE_DEATH_BASELINE
    unsigned expected = 0;
#else
    unsigned expected = 1;
#endif
    Check(owner.GetAuraCount(226512) == expected && member.GetAuraCount(226512) == expected && ally.GetAuraCount(226510) == expected,
        "One ending Sanguine field removed a surviving field's contribution");
    one->Remove(false); one->Despawn();
    Check(owner.GetAuraCount(226512) == expected && ally.GetAuraCount(226510) == expected,
        "Repeated ending callbacks consumed another Sanguine contribution");
    std::cout << "ENV Sanguine afterOneExpiry playerStacks=" << owner.GetAuraCount(226512)
              << " memberStacks=" << member.GetAuraCount(226512) << " healStacks=" << ally.GetAuraCount(226510)
              << " otherAffected=" << two->IsUnitAffected(owner.GetGUID()) << '\n';
    two->Update(1);
    Check(two->IsUnitAffected(owner.GetGUID()) && owner.GetAuraCount(226512) == expected,
        "Remaining Sanguine field silently repaired or lost its actual membership");
#ifndef CHALLENGE_DEATH_BASELINE
    dot = owner.GetAura(226512);
    Check(dot && dot->GetEffect(EFFECT_0)->GetAmount() == 15.f && ally.GetAura(226510)->GetEffect(EFFECT_0)->GetAmount() == 5.f,
        "Surviving field lost its matching native dose");
    owner.SetHealth(1000000); auto beforeTicks = dot->GetEffect(EFFECT_0)->GetTickNumber();
    owner.Unit::Update(1000);
    auto ticks = dot->GetEffect(EFFECT_0)->GetTickNumber() - beforeTicks;
    Check(ticks >= 1 && ticks <= 2 && 1000000 - owner.GetHealth() == 150000u * ticks,
        "Surviving field's actual periodic damage stopped or changed dose");
#endif
    map.PlayerRelocation(&owner, 2964.69f, 715.757f, 554.286f, 1.55255f); two->Update(1);
    Check(!owner.HasAura(226512) && member.GetAuraCount(226512) == expected && ally.GetAuraCount(226510) == expected,
        "Normal Sanguine walk-out removed another target's contribution");
    map.PlayerRelocation(&owner, 2934.69f, 715.757f, 554.286f, 1.55255f); two->Update(1);
    Check(owner.GetAuraCount(226512) == 1, "Sanguine re-entry failed to apply a real contribution");
    map.RemoveAllObjectsInRemoveList();
    second.RemoveAllAreaObjects(); map.RemoveAllObjectsInRemoveList();
    Check(!owner.HasAura(226512) && !member.HasAura(226512) && !ally.HasAura(226510), "Final Sanguine removal left child auras");
    std::cout << "ENV Sanguine race=" << unsigned(race) << " killer=" << killerKind << " ending=" << removalKind << " remaining=" << expected << '\n';
}

static void CheckVolcanic(uint8 race, bool dodge, unsigned instance)
{
    MapInstanced parent(1492, 300);
    InstanceMap map(1492, 300, 9940500 + instance, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
    map.CreateInstanceData(nullptr);
    LifecyclePlayer ranged(map, 741, Position{2964.49f, 715.757f, 554.286f, 1.55255f});
    LifecyclePlayer closePlayer(map, 742, Position{2937.49f, 715.757f, 554.286f, 1.55255f});
    for (auto player : {&ranged, &closePlayer})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player->setFactionForRace(race);
        auto display = sChrRacesStore.LookupEntry(race)->MaleDisplayID;
        player->SetDisplayId(display); player->SetNativeDisplayId(display);
    }
    auto route = sMapChallengeModeStore.LookupEntry(208); Require(route, "Native Volcanic route missing");
    auto key = ranged.Key(*route); LifecycleGroup group(ranged, &closePlayer, *route, *key);
    group.group.m_affixes = {0, uint32(Affixes::Volcanic), 0};
    DeathAffixScenario progress(map, ranged);
    EnvironmentActor source(map, 97043, 176001);
    source.SetInCombatWith(&ranged); ranged.SetInCombatWith(&source); closePlayer.SetInCombatWith(&source);
    auto might = source.GetAura(ChallengersMight); Require(might && might->m_loadedScripts.size() == 1, "Actual Volcanic Might script absent");
    std::cout << "ENV Volcanic sourceDistance=" << source.GetDistance(&ranged)
              << " valid=" << ranged.IsValidAttackTarget(&source) << " hostile=" << source.IsHostileToPlayers()
              << " sourceFlags=" << source.GetUInt32Value(UNIT_FIELD_FLAGS) << " playerFlags=" << ranged.GetUInt32Value(UNIT_FIELD_FLAGS)
              << " sourceFaction=" << source.getFaction() << " playerFaction=" << ranged.getFaction()
              << " amount=" << might->GetEffect(EFFECT_7)->GetAmount() << '\n';
    Require(ranged.IsValidAttackTarget(&source) && source.GetDistance(&ranged) > 15.f && source.GetDistance(&ranged) < 60.f,
        "Native ranged Volcanic fixture is not eligible");
    std::list<Creature*> plumes;
    might->UpdateOwner(7000, &source); source.GetCreatureListWithEntryInGrid(plumes, 105877, 100.f);
    Check(plumes.empty(), "Volcanic created a plume before its native eighth periodic tick");
    might->UpdateOwner(1000, &source); source.GetCreatureListWithEntryInGrid(plumes, 105877, 100.f);
    Check(plumes.size() == 1, "Native Volcanic selector did not create exactly one distant-player plume");
    auto plume = plumes.front(); auto guid = plume->GetGUID();
    Require(plume->AI(), "Real Volcanic plume AI absent");
    std::cout << "ENV Volcanic ai=" << typeid(*plume->AI()).name() << " distance=" << plume->GetExactDist(&ranged) << '\n';
    Check(plume->GetExactDist(&ranged) < 2.f && closePlayer.GetHealth() == 1000000, "Native plume placement/closePlayer-player exclusion differs");
    // Creature::Update batches out-of-combat work. Keep ordinary50ms frames
    // instead of assuming every single millisecond directly dispatches AI.
    for (unsigned i = 0; i < 4; ++i) plume->Update(50);
    Check(!plume->HasUnitState(UNIT_STATE_CASTING), "Volcanic erupted before its250ms AI delay");
    plume->Update(50);
    std::cout << "ENV Volcanic250 casting=" << plume->HasUnitState(UNIT_STATE_CASTING) << " health=" << ranged.GetHealth() << '\n';
    Check(plume->HasUnitState(UNIT_STATE_CASTING), "Real plume AI did not begin the native2000ms cast");
    if (dodge) map.PlayerRelocation(&ranged, 2974.49f, 715.757f, 554.286f, 1.55255f);
    for (unsigned i = 0; i < 39; ++i) plume->Update(50);
    Check(ranged.GetHealth() == 1000000, "Native Volcanic damaged before cast completion");
    plume->Update(50);
    Check(ranged.GetHealth() == (dodge ? 1000000u : 500000u) && closePlayer.GetHealth() == 1000000,
        "Native Volcanic50-percent damage or dodge/closePlayer selection differs");
    auto health = ranged.GetHealth();
    for (unsigned i = 0; i < 155; ++i) plume->Update(50);
    map.RemoveAllObjectsInRemoveList();
    Check(!map.GetCreature(guid) && ranged.GetHealth() == health, "Native10-second plume lifetime left actor or repeated damage");
    std::cout << "ENV Volcanic race=" << unsigned(race) << " dodge=" << dodge << " health=" << ranged.GetHealth() << '\n';
}

static void DumpEnvironmentRows(char const* sql)
{
    std::cout << "DATA query=" << sql << '\n';
    auto rows = WorldDatabase.Query(sql);
    if (!rows) { std::cout << "DATA rows=0\n"; return; }
    do
    {
        auto fields = rows->Fetch(); std::cout << "DATA row=";
        for (unsigned i = 0; i < rows->GetFieldCount(); ++i)
            std::cout << (i ? "|" : "") << fields[i].GetString();
        std::cout << '\n';
    } while (rows->NextRow());
}

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true);
        for (auto id : {206150u, 226489u, 226510u, 226511u, 226512u, 209861u, 209862u})
        {
            auto info = sSpellMgr->GetSpellInfo(id); auto original = sSpellStore.LookupEntry(id);
            if (!info || !original) { std::cout << "META absent=" << id << '\n'; continue; }
            std::cout << "META spell=" << id << " duration=" << info->GetDuration(8)
                      << " stacks=" << info->GetAuraOptions(8)->CumulativeAura
                      << " castTime=" << info->CalcCastTime()
                      << " name=" << original->Name->Str[LOCALE_enUS]
                      << " description=" << original->Description->Str[LOCALE_enUS] << '\n';
            for (unsigned i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                auto e = info->GetEffect(static_cast<SpellEffIndex>(i), 8);
                if (e && e->IsEffect()) std::cout << "META effect=" << i << " type=" << e->Effect
                    << " aura=" << e->ApplyAuraName << " base=" << e->BasePoints << " period=" << e->ApplyAuraPeriod
                    << " misc=" << e->MiscValue << " miscB=" << e->MiscValueB << " trigger=" << e->TriggerSpell
                    << " targetA=" << e->TargetA.GetTarget() << " targetB=" << e->TargetB.GetTarget()
                    << " radius=" << e->CalcRadius() << '\n';
            }
        }
        DumpEnvironmentRows("SELECT * FROM areatrigger_template WHERE spellId IN(226489,226510,226511,226512)");
        DumpEnvironmentRows("SELECT * FROM areatrigger_data WHERE spellId IN(226489,226510,226511,226512)");
        DumpEnvironmentRows("SELECT * FROM areatrigger_actions WHERE entry IN(SELECT entry FROM areatrigger_template WHERE spellId=226489)");
        DumpEnvironmentRows("SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(206150,226489,226510,226511,226512,209861,209862)");
        DumpEnvironmentRows("SELECT entry,ScriptName,AIName,VehicleId,AffixState FROM creature_template WHERE entry=105877");
        DumpEnvironmentRows("SELECT entry,customEntry,id,moment,targetFlags,spellId FROM areatrigger_actions WHERE actionType=3 AND (moment&8)<>0 ORDER BY entry,customEntry,id");
        InitializeEnvironmentRuntime();
        unsigned contexts = 0;
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
            for (unsigned killer = 0; killer < 3; ++killer)
                for (unsigned ending = 0; ending < 3; ++ending) CheckSanguine(race, killer, ending, contexts++);
        unsigned volcanic = 0;
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
            for (bool dodge : {false, true}) CheckVolcanic(race, dodge, volcanic++);
        Check(contexts == 18 && volcanic == 4, "Environmental test scope changed");
#ifdef CHALLENGE_DEATH_BASELINE
        std::cout << "COUNTERFACTUAL " << checked << " native environmental checks: one Sanguine ending drops both overlapping contributions; actual linked original core required, not ordinary successful gameplay\n";
#else
        std::cout << "PASS " << checked << " native environmental checks:18 Sanguine race/killer/ending contexts and4 real Volcanic summon/AI/cast/dodge/lifetime contexts; controlled publication/health, no client traversal claim\n";
#endif
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

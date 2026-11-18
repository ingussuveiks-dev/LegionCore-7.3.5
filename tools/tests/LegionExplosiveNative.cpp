// Actual Might summon and native Explosive AI/cast/kill paths.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "LegionChallengeDeathAffixesNative.cpp"

static void InitializeExplosive()
{
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon(); sObjectMgr->LoadScenarioData();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_challenge_scripts(); sScriptMgr->SwapScriptContext(true);
    auto binding = WorldDatabase.Query("SELECT ScriptName FROM spell_script_names WHERE spell_id=206150");
    Require(binding && binding->GetRowCount() == 1 && binding->Fetch()[0].GetString() == "spell_challengers_might", "Native Might binding changed");
    auto id = sObjectMgr->GetScriptId("spell_challengers_might");
    Require(sScriptMgr->GetSpellScriptLoader(id), "Actual Might registration missing");
    (sScriptDataStore->*Access(DeathAffixBindingsTag{})).emplace(206150, std::make_pair(id, true));
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(206150,228318,240444,243110,240445,240446)");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "Load changed Explosive spell conditions before native run");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
    {
        std::string name = table;
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE explosive_" + name + " AS SELECT * FROM " + name + " WHERE entry IN(206150,228318,240444,243110,240445,240446)").c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE explosive_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM explosive_" + name).c_str());
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellPrcoCheck(); sSpellMgr->LoadSpellBonusess();
}

static void DumpExplosive()
{
    for (auto spell : {206150u, 228318u, 240444u, 243110u, 240445u, 240446u})
    {
        auto info = sSpellMgr->GetSpellInfo(spell); auto row = sSpellStore.LookupEntry(spell);
        Require(info && row, "Matching native affix spell absent");
        std::cout << "META spell=" << spell << " name=" << row->Name->Str[LOCALE_enUS]
            << " description=" << row->Description->Str[LOCALE_enUS]
            << " duration=" << info->GetDuration(8) << " castTime=" << info->CalcCastTime()
            << " range=" << info->GetMaxRange(false) << " aiTarget=" << unsigned(UnitAI::AISpellInfo[spell].target) << '\n';
        for (unsigned i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            auto effect = info->GetEffect(static_cast<SpellEffIndex>(i), 8);
            if (effect && effect->IsEffect()) std::cout << "META effect=" << i << " type=" << effect->Effect
                << " aura=" << effect->ApplyAuraName << " base=" << effect->BasePoints << " period=" << effect->ApplyAuraPeriod
                << " misc=" << effect->MiscValue << " miscB=" << effect->MiscValueB << " trigger=" << effect->TriggerSpell
                << " targetA=" << effect->TargetA.GetTarget() << " targetB=" << effect->TargetB.GetTarget()
                << " radius=" << effect->CalcRadius() << '\n';
        }
    }
    auto rows = WorldDatabase.Query("SELECT entry,ScriptName,AIName,VehicleId,AffixState,faction,unit_flags,unit_flags2,flags_extra FROM creature_template WHERE entry=120651");
    Require(rows && rows->GetRowCount() == 1, "Native Explosive template missing");
    std::cout << "META template=";
    for (unsigned i = 0; i < rows->GetFieldCount(); ++i) std::cout << (i ? "|" : "") << rows->Fetch()[i].GetString();
    std::cout << '\n';
}

struct ExplosiveContext
{
    MapInstanced parent;
    InstanceMap map;
    LifecyclePlayer owner, member;
    Item* key;
    LifecycleGroup group;
    DeathAffixScenario progress;
    DeathAffixActor source;
    std::vector<ObjectGuid> ownedOrbs;

    static InstanceMap& Ready(InstanceMap& map) { map.CreateInstanceData(nullptr); return map; }
    static MapChallengeModeEntry const& Route()
    {
        auto route = sMapChallengeModeStore.LookupEntry(208); Require(route, "Native Maw route missing"); return *route;
    }
    LifecyclePlayer& Configure(uint8 race, uint32 level, bool enabled)
    {
        for (auto player : {&owner, &member})
        {
            player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player->setFactionForRace(race);
            auto display = sChrRacesStore.LookupEntry(race)->MaleDisplayID;
            player->SetDisplayId(display); player->SetNativeDisplayId(display);
        }
        key->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL, level); owner.m_challengeKeyInfo.Level = level;
        group.group.m_challengeLevel = level;
        group.group.m_affixes = {0, enabled ? uint32(Affixes::FelExplosives) : 0, 0}; return owner;
    }
    ExplosiveContext(uint8 race, uint32 level, unsigned id, bool enabled = true)
        : parent(1492, 300), map(1492, 300, 9940900 + id, DIFFICULTY_MYTHIC_KEYSTONE, &parent),
          owner(Ready(map), 751, Position{2937.49f, 715.757f, 554.286f, 1.55255f}), member(map, 752, owner.GetPosition()),
          key(owner.Key(Route())), group(owner, &member, Route(), *key), progress(map, Configure(race, level, enabled)),
          source(map, 97043, 176501) { }
    ~ExplosiveContext()
    {
        for (auto guid : ownedOrbs) if (auto orb = map.GetCreature(guid)) orb->DespawnOrUnsummon();
        map.RemoveAllObjectsInRemoveList();
    }
    std::list<Creature*> Orbs()
    {
        std::list<Creature*> orbs; source.GetCreatureListWithEntryInGrid(orbs, 120651, 100.f); return orbs;
    }
    Aura* Might()
    {
        auto aura = source.GetAura(ChallengersMight);
        Require(aura && aura->m_loadedScripts.size() == 1, "Actual Explosive Might script absent"); return aura;
    }
    void Renew()
    {
        source.RemoveAurasDueToSpell(ChallengersMight);
        Require(source.AddAura(ChallengersMight, &source), "Native Might reapplication failed"); Might();
    }
    void Combat()
    {
        source.SetInCombatWith(&owner); owner.SetInCombatWith(&source); member.SetInCombatWith(&source);
    }
    Creature* Spawn()
    {
        Combat(); Might()->UpdateOwner(1999, &source);
        Check(Orbs().empty(), "Explosive appeared before native initial2-second minimum");
        Might()->UpdateOwner(8001, &source);
        auto orbs = Orbs(); Check(orbs.size() == 1, "Actual Might did not create exactly one native Explosive for two eligible players");
        auto orb = orbs.front(); ownedOrbs.push_back(orb->GetGUID());
        Check(orb->AI() && std::string(typeid(*orb->AI()).name()).find("npc_challenger_fel_explosives") != std::string::npos,
            "Native Explosive AI binding differs");
        Check(orb->HasAura(240445) && orb->isTotem() && orb->GetReactState() == REACT_PASSIVE && !orb->getVictim(),
            "Native Explosive visual/passive/totem/no-victim state differs");
        Check(orb->GetAnyOwner() == &source && orb->GetHealth() > 0 && orb->IsAffixDisabled(Affixes::FelExplosives),
            "Native Explosive ownership,health or self-spawn exclusion differs");
        return orb;
    }
};

static void Frames(Creature& orb, unsigned count)
{
    for (unsigned i = 0; i < count && orb.IsInWorld(); ++i) orb.Update(50);
}

static void CheckExplosion(uint8 race, uint32 level, bool outside, unsigned id)
{
    ExplosiveContext context(race, level, id); auto orb = context.Spawn(); auto guid = orb->GetGUID();
    if (outside) context.map.PlayerRelocation(&context.member, 3037.49f, 715.757f, 554.286f, 1.55255f);
    Check((orb->GetDistance(&context.member) > 60.f) == outside, "Actual Explosion radius fixture differs");
    Frames(*orb, 9); Check(!orb->HasUnitState(UNIT_STATE_CASTING), "Explosive started before native500ms delay");
    Frames(*orb, 1); Check(orb->HasUnitState(UNIT_STATE_CASTING), "Actual passive Explosive did not start native6000ms cast without a victim");
    Frames(*orb, 119); Check(context.owner.GetHealth() == 1000000 && context.member.GetHealth() == 1000000,
        "Native Explosive dealt damage before cast completion");
    Frames(*orb, 1);
    Check(context.owner.GetHealth() == 500000 && context.member.GetHealth() == (outside ? 1000000u : 500000u),
        "Native Explosive50-percent group/radius damage differs across key levels");
    Frames(*orb, 2); context.map.RemoveAllObjectsInRemoveList();
    Check(!context.map.GetCreature(guid), "Native completed Explosion did not remove its actor after100ms");
    Check(context.owner.GetHealth() == 500000 && context.member.GetHealth() == (outside ? 1000000u : 500000u),
        "Native Explosive repeated its completed damage");
    std::cout << "EXPLOSIVE race=" << unsigned(race) << " level=" << level << " outside=" << outside << " explosion=50percent cleanup=100ms\n";
}

static void CheckKill(uint8 race, unsigned killerKind, unsigned milliseconds, unsigned id)
{
    ExplosiveContext context(race, 10, id); DungeonPet pet(context.owner, context.map); DungeonGuardian guardian(context.owner, context.map);
    auto orb = context.Spawn(); auto guid = orb->GetGUID(); Frames(*orb, milliseconds / 50);
    Check(orb->HasUnitState(UNIT_STATE_CASTING) == (milliseconds >= 500), "Native kill fixture is not in requested cast phase");
    Unit* killer = killerKind == 0 ? static_cast<Unit*>(&context.owner) : killerKind == 1 ? static_cast<Unit*>(&pet) : static_cast<Unit*>(&guardian);
    killer->Kill(orb); killer->Kill(orb);
    Check(!orb->IsAlive() && !orb->HasUnitState(UNIT_STATE_CASTING), "Real player/pet/guardian kill did not cancel native Explosion");
    Frames(*orb, 140);
    Check(context.owner.GetHealth() == 1000000 && context.member.GetHealth() == 1000000,
        "Killed Explosive delivered delayed damage to a group member");
    Check(context.Orbs().size() <= 1, "Killed Explosive produced another Explosive");
    orb->DespawnOrUnsummon(); context.map.RemoveAllObjectsInRemoveList();
    Check(!context.map.GetCreature(guid), "Killed Explosive native cleanup failed");
    std::cout << "EXPLOSIVE race=" << unsigned(race) << " killer=" << killerKind << " killMs=" << milliseconds << " delayedDamage=0\n";
}

static void CheckAdmission(uint8 race, bool enabled, unsigned id)
{
    ExplosiveContext context(race, 10, id, enabled);
    auto none = [&](char const* phase)
    {
        std::cout << "EXPLOSIVE admissionPhase=" << phase << " enabled=" << enabled << " amount=" << context.Might()->GetEffect(EFFECT_12)->GetAmount()
            << " combat=" << context.source.isInCombat() << '/' << context.owner.isInCombat() << '/' << context.member.isInCombat() << '\n';
        context.Might()->UpdateOwner(16000, &context.source); Check(context.Orbs().empty(), "Ineligible source created an Explosive");
    };
    none("notInCombat"); context.source.SetInCombatWith(&context.owner); none("playersNotInCombat"); context.Combat();
    if (!enabled) { none("noAffix"); return; }
    context.map.PlayerRelocation(&context.owner, 3037.49f, 715.757f, 554.286f, 1.55255f);
    context.map.PlayerRelocation(&context.member, 3037.49f, 715.757f, 554.286f, 1.55255f); none("outOfRange");
    context.map.PlayerRelocation(&context.owner, 2937.49f, 715.757f, 554.286f, 1.55255f);
    context.map.PlayerRelocation(&context.member, 2937.49f, 715.757f, 554.286f, 1.55255f);
    auto disabled = context.source.disableAffix; context.source.disableAffix |= 1 << Affixes::FelExplosives; context.Renew(); none("disabled");
    context.source.disableAffix = disabled; context.Renew();
    context.source.SetOwnerGUID(context.owner.GetGUID()); context.source.SetCreatorGUID(context.owner.GetGUID());
    Require(context.source.GetAnyOwner() == &context.owner, "Native player-owner receipt absent"); none("playerOwned");
    context.source.SetOwnerGUID(ObjectGuid::Empty); context.source.SetCreatorGUID(ObjectGuid::Empty);
    {
        DeathAffixActor companion(context.map, 97043, 176502);
        context.source.SetOwnerGUID(companion.GetGUID()); context.source.SetCreatorGUID(companion.GetGUID());
        Require(context.source.GetAnyOwner() == &companion, "Native NPC-owner receipt absent"); none("ordinaryNpcOwned");
        context.source.SetOwnerGUID(ObjectGuid::Empty); context.source.SetCreatorGUID(ObjectGuid::Empty);
    }
    auto faction = context.source.getFaction(); context.source.setFaction(context.owner.getFaction()); none("friendly"); context.source.setFaction(faction);
    context.Renew(); auto orb = context.Spawn(); orb->DespawnOrUnsummon(); context.map.RemoveAllObjectsInRemoveList();
    context.Might()->UpdateOwner(7999, &context.source); Check(context.Orbs().empty(), "Explosive recurred before native8-second minimum");
    context.Might()->UpdateOwner(8001, &context.source); auto orbs = context.Orbs();
    Check(orbs.size() == 1, "Explosive did not recover/recur within native16-second maximum");
    context.ownedOrbs.push_back(orbs.front()->GetGUID());
    std::cout << "EXPLOSIVE race=" << unsigned(race) << " admission=guarded recurrence=8to16seconds\n";
}

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true); InitializeExplosive(); DumpExplosive(); unsigned contexts = 0;
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            for (uint32 level : {7u, 10u, 15u}) for (bool outside : {false, true}) CheckExplosion(race, level, outside, contexts++);
            for (unsigned killer = 0; killer < 3; ++killer) for (unsigned time : {0u, 500u, 6450u}) CheckKill(race, killer, time, contexts++);
            for (bool enabled : {false, true}) CheckAdmission(race, enabled, contexts++);
        }
        Check(contexts == 34, "Native Explosive scope changed");
        std::cout << "PASS " << checked << " native Explosive checks:12 key/race/radius explosions,18 player/pet/guardian kill/cast phases,4 admission profiles;actual Might summon/AI/spell/lifetime. Publication/health/group fixtures controlled;no client traversal claim\n";
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End(); return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

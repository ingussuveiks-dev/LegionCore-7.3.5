#include <unordered_set>
#include <Windows.h>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "MovementPackets.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "World.h"
#include "Map.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "ObjectAccessor.h"
#include "GameTables.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "CreatureAIRegistry.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <set>
#include <stdexcept>

static void Require(bool value, char const* reason)
{
    if (!value)
        throw std::runtime_error(reason);
}

// Native Create/Map/Unit/Spell/Aura/effect implementations are linked, not
// replaced by doubles. Only fixture placement, survival stats and AI binding
// differ from a loaded DB spawn. Rotation/charm policy has separate tests.
struct Actor : Creature
{
    Actor(Map* map, uint32 entry, uint64 guid)
    {
        Require(Create(guid, map, 1, entry, 0, ALLIANCE,
            1816.98f, 734.469f, 85.565f, 0), "Native actor creation failed");
        SetHomePosition(*this); // LoadCreatureFromDB normally supplies the home.
        SetMaxHealth(1000000);
        SetHealth(GetMaxHealth());
        SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 100);
        SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 100);
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)), "Native fixture idle AI failed");
    }
    void RemoveFromWorld() override
    {
        Unit::RemoveFromWorld();
        sObjectAccessor->RemoveObject(static_cast<Creature*>(this));
    }
    void Step(uint32 diff) { Unit::Update(diff); }
    ~Actor()
    {
        CombatStop(true);
        CastStop();
        RemoveAllAuras();
        m_Events.KillAllEvents(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

struct ObservedAggressor : AggressorAI
{
    using AggressorAI::AggressorAI;
    std::map<uint32, uint32> hits;
    void SpellHitTarget(Unit*, SpellInfo const* info) override { ++hits[info->Id]; }
    uint32 Scheduled(uint32 id) const { return spellCasts.GetNextEventTime(id); }
};

static std::list<Creature*> Bones(Actor& caster)
{
    std::list<Creature*> result;
    caster.GetCreatureListWithEntryInGrid(result, 99484, 100);
    caster.GetCreatureListWithEntryInGrid(result, 99485, 100);
    result.remove_if([&](Creature* creature)
    {
        return !creature->isSummon() || creature->ToTempSummon()->GetSummonerGUID() != caster.GetGUID();
    });
    return result;
}

static void Advance(Actor& caster, Actor& victim, uint32 duration)
{
    for (uint32 elapsed = 0; elapsed < duration; elapsed += 10)
    {
        caster.Step(10);
        victim.Step(10);
    }
}

static void CheckCasterRequirements(Actor& caster, Actor& victim)
{
    auto info = const_cast<SpellInfo*>(sSpellMgr->GetSpellInfo(186480));
    auto saved = info->AuraRestrictions;
    // Vary only this private process's metadata to cover all four native gates.
    // Actual Unit aura storage/state and the linked CanAutoCast execute.
    try
    {
        caster.RemoveAllAuras();
        info->AuraRestrictions = {};
        info->AuraRestrictions.CasterAuraSpell = 186553;
        Require(!info->CanAutoCast(&caster, &victim), "Missing required caster aura admitted");
        Require(caster.AddAura(186553, &caster), "Cannot establish required caster aura");
        Require(info->CanAutoCast(&caster, &victim), "Present required caster aura rejected");
        info->AuraRestrictions.CasterAuraSpell = 0;
        info->AuraRestrictions.ExcludeCasterAuraSpell = 186553;
        Require(!info->CanAutoCast(&caster, &victim), "Excluded caster aura admitted");
        caster.RemoveAllAuras();
        Require(info->CanAutoCast(&caster, &victim), "Absent excluded caster aura rejected");
        info->AuraRestrictions.ExcludeCasterAuraSpell = 0;
        info->AuraRestrictions.CasterAuraState = AURA_STATE_FROZEN;
        caster.ModifyAuraState(AURA_STATE_FROZEN, false);
        Require(!info->CanAutoCast(&caster, &victim), "Missing required caster state admitted");
        caster.ModifyAuraState(AURA_STATE_FROZEN, true);
        Require(info->CanAutoCast(&caster, &victim), "Present required caster state rejected");
        info->AuraRestrictions.CasterAuraState = 0;
        info->AuraRestrictions.ExcludeCasterAuraState = AURA_STATE_FROZEN;
        Require(!info->CanAutoCast(&caster, &victim), "Excluded caster state admitted");
        caster.ModifyAuraState(AURA_STATE_FROZEN, false);
        Require(info->CanAutoCast(&caster, &victim), "Absent excluded caster state rejected");
    }
    catch (...)
    {
        info->AuraRestrictions = saved;
        caster.ModifyAuraState(AURA_STATE_FROZEN, false);
        throw;
    }
    info->AuraRestrictions = saved;
    std::cout << "PASS: four caster aura/state gates accept and reject native Unit state.\n";
}

static void CheckActor(Map& map, uint32 entry, uint64 guid, std::set<uint32> expected)
{
    Actor caster(&map, entry, guid), victim(&map, 116600, guid + 1);
    map.LoadGrid(caster.GetPositionX(), caster.GetPositionY());
    Require(map.AddToMap(static_cast<Creature*>(&caster)) &&
        map.AddToMap(static_cast<Creature*>(&victim)), "Native map admission failed");
    victim.setFaction(1); // Isolated hostile surrogate, not a personal character.
    Require(ObjectAccessor::GetUnit(caster, victim.GetGUID()) == &victim &&
        caster.IsValidAttackTarget(&victim), "Fixture registry/hostility is invalid");
    std::set<uint32> actual;
    for (auto const& spell : *caster.CreatureSpells)
    {
        Require(spell.second.CanUseInDifficulty(DIFFICULTY_NONE), "Outdoor spell rejected by difficulty");
        actual.insert(spell.first);
    }
    Require(actual == expected, "Live native creature spell pool changed");

    for (uint32 id : actual)
    {
        auto info = sSpellMgr->GetSpellInfo(id);
        auto ai = GetAISpellInfo(id);
        std::cout << "POLICY " << id << " casterState=" << int(info->AuraRestrictions.CasterAuraState)
            << " excludeState=" << int(info->AuraRestrictions.ExcludeCasterAuraState)
            << " requireAura=" << info->AuraRestrictions.CasterAuraSpell
            << " excludeAura=" << info->AuraRestrictions.ExcludeCasterAuraSpell << '\n';
        Require(info && ai->condition == AICOND_COMBAT && ai->cooldown == (id == 186476 ? 15000 : 5000),
            "Native AI classification/cooldown changed");
        bool hostile = id == 186327 || id == 222484;
        Require(ai->target == (hostile ? AITARGET_VICTIM : AITARGET_SELF), "Native target policy changed");
        caster.CastStop();
        caster.RemoveAllAuras();
        victim.RemoveAllAuras();
        victim.SetHealth(victim.GetMaxHealth());
        caster.SetFloatValue(UNIT_FIELD_MIN_DAMAGE, 100);
        caster.SetFloatValue(UNIT_FIELD_MAX_DAMAGE, 100);
        Require(info->CanAutoCast(&caster, &victim) && !info->CanAutoCast(&caster, nullptr),
            "Native fresh-target/null-target autocast admission changed");
        auto before = victim.GetHealth();
        auto previousBones = Bones(caster).size();
        Require(caster.CastSpell(hostile ? &victim : &caster, id, false) == SPELL_CAST_OK,
            "Native non-triggered cast rejected");
        Advance(caster, victim, 5000);
        if (id == 186480 || id == 186553)
        {
            Require(caster.HasAura(id) && !victim.HasAura(id) && !info->CanAutoCast(&caster, &caster),
                "Native self aura did not apply or duplicate admission changed");
        }
        else if (id == 195585)
        {
            auto summons = Bones(caster);
            Require(summons.size() == previousBones + 3, "Call of the Dead did not create three native summons");
            for (auto summon : summons)
                Require(summon->IsInWorld() && summon->IsAlive() && summon->GetMap() == &map &&
                    summon->GetNativeDisplayId(), "Summon initialization/ownership failed");
        }
        else
        {
            Require(victim.GetHealth() < before, "Native damage did not reach the victim");
            if (id == 222484)
            {
                Require(victim.HasAura(id), "Foul Slash disease did not apply");
                before = victim.GetHealth();
                Advance(caster, victim, 1600);
                Require(victim.GetHealth() < before, "Native Foul Slash periodic tick did not deal damage");
            }
        }
        std::cout << "DIRECT " << entry << ' ' << id << " PASS\n";
    }

    if (entry == 115846)
    {
        Require(sSpellMgr->GetSpellInfo(186476)->AuraRestrictions.ExcludeCasterAuraSpell == 186480 &&
            sSpellMgr->GetSpellInfo(186480)->AuraRestrictions.ExcludeCasterAuraSpell == 186553 &&
            sSpellMgr->GetSpellInfo(186553)->AuraRestrictions.ExcludeCasterAuraSpell == 186480,
            "Native Vrekt form exclusions changed");
        caster.RemoveAllAuras();
        Require(caster.AddAura(186480, &caster), "Cannot establish native Rage state");
        for (auto id : {186476u, 186553u})
        {
            Require(!sSpellMgr->GetSpellInfo(id)->CanAutoCast(&caster, &caster) &&
                caster.CastSpell(&caster, id, false) == SPELL_FAILED_CASTER_AURASTATE,
                "Autocast and CheckCast disagree about native Rage exclusion");
        }
        caster.RemoveAllAuras();
        Require(caster.AddAura(186553, &caster), "Cannot establish native Mist state");
        Require(!sSpellMgr->GetSpellInfo(186480)->CanAutoCast(&caster, &caster) &&
            caster.CastSpell(&caster, 186480, false) == SPELL_FAILED_CASTER_AURASTATE,
            "Autocast and CheckCast disagree about native Mist exclusion");
        std::cout << "PASS: native Rage/Mist/Flurry exclusions agree with full CheckCast.\n";
        CheckCasterRequirements(caster, victim);
    }

    // Execute the linked Aggressor scheduler and UnitAI::DoCast, not a copied
    // timer/target model. Observer records native hit callbacks only.
    caster.CastStop();
    caster.RemoveAllAuras();
    victim.RemoveAllAuras();
    // Direct casts above left native wall-clock cooldowns. Start this separate
    // scheduling case fresh; simulated EventMap time does not advance GameTime.
    for (auto id : actual)
        caster.RemoveCreatureSpellCooldown(id);
    Require(caster.AIM_Initialize(new ObservedAggressor(&caster)), "Native Aggressor initialization failed");
    auto ai = static_cast<ObservedAggressor*>(caster.AI());
    Require(caster.Attack(&victim, true), "Native stationary attack failed");
    caster.SetInCombatWith(&victim);
    victim.SetInCombatWith(&caster);
    caster.AddThreat(&victim, 1.0f);
    std::srand(1);
    ai->EnterCombat(&victim);
    for (auto id : actual)
    {
        auto cooldown = GetAISpellInfo(id)->cooldown;
        Require(ai->Scheduled(id) >= cooldown && ai->Scheduled(id) < 2 * cooldown,
            "Native initial schedule is outside its existing interval");
    }
    std::set<uint32> observedAuras;
    for (uint32 elapsed = 0; elapsed < 65000; elapsed += 10)
    {
        // Survival fixture keeps the surrogate alive across repeated scaled
        // hits. It does not establish exact balance or killing-blow behavior.
        victim.SetHealth(victim.GetMaxHealth());
        caster.Step(10);
        victim.Step(10);
        ai->UpdateAI(10);
        for (auto id : actual)
            if (caster.HasAura(id))
                observedAuras.insert(id);
    }
    for (auto id : actual)
    {
        Require(ai->hits[id] > 0, "Native automatic cast never reached a hit callback");
        if (id == 186480 || id == 186553)
            Require(observedAuras.count(id), "Native scheduled self aura never appeared");
        std::cout << "SCHEDULE " << entry << ' ' << id << " hits=" << ai->hits[id] << '\n';
    }
    ai->Reset();
    for (auto id : actual)
        Require(!ai->Scheduled(id), "Reset retained a queued combat spell");
    caster.CombatStop(true);
    victim.CombatStop(true);
    caster.CastStop();
    victim.CastStop();
    for (auto summon : Bones(caster))
        summon->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    caster.RemoveAllAuras();
    victim.RemoveAllAuras();
    caster.m_Events.KillAllEvents(true);
    victim.m_Events.KillAllEvents(true);
    map.RemoveFromMap(static_cast<Creature*>(&caster), false);
    map.RemoveFromMap(static_cast<Creature*>(&victim), false);
}

int Run(int argc, char** argv)
{
    Require(argc == 3, "Expected private config and canonical runtime directory");
    std::string error;
    Require(sConfigMgr->LoadInitial(argv[1], {}, error), "Cannot load private test configuration");
    sLog->Initialize(nullptr);
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo", ""), 1, 1);
    Require(!HotfixDatabase.Open() && HotfixDatabase.PrepareStatements(), "Hotfix connection failed");
    Require(!WorldDatabase.Open() && WorldDatabase.PrepareStatements(), "World connection failed");
    // CheckCreatureTemplate can repair unrelated invalid slots. Keep both of
    // this private pool's connections read-only, including its one FIFO worker.
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = WorldDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readOnly && readOnly->Fetch()[0].GetBool(), "World test connection is not read-only");
    // No updater, character/login pool, persistent spawn load, forced world
    // quest or map update thread is run. Unrelated template repairs cannot write.
    sDB2Manager.LoadStores(argv[2], LOCALE_enUS);
    LoadGameTables(argv[2]);
    sSpellMgr->LoadSpellInfoStore();
    sSpellMgr->LoadTalentSpellInfo();
    sSpellMgr->LoadSkillLineAbilityMap();
    sSpellMgr->LoadSpellCustomAttr();
    sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadCreatureModelInfo();
    sObjectMgr->LoadWDBCreatureTemplates();
    sObjectMgr->LoadCreatureTemplates();
    sObjectMgr->LoadCreatureClassLevelStats();
    sObjectMgr->LoadCreatureScalingData();
    sObjectMgr->LoadCreatureDifficultyStat();
    AIRegistry::Initialize();
    UnitAI::FillAISpellInfo();
    auto base = offsetof(SpellInfo, AuraRestrictions);
    std::cout << "LAYOUT " << base + offsetof(SpellInfo::SpellAuraRescrictions, CasterAuraSpell) << ' '
        << base + offsetof(SpellInfo::SpellAuraRescrictions, ExcludeCasterAuraSpell) << ' '
        << base + offsetof(SpellInfo::SpellAuraRescrictions, CasterAuraState) << ' '
        << base + offsetof(SpellInfo::SpellAuraRescrictions, ExcludeCasterAuraState) << '\n';
    sWorld->LoadConfigSettings();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220, {}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220, {}}});
    HashMapHolder<Creature>::SetSize(20000);
    {
        Map map(1220, 300, 9999999, DIFFICULTY_NONE);
        CheckActor(map, 115846, 10000, {186476, 186480, 186553});
        CheckActor(map, 116180, 10010, {186327, 195585});
        CheckActor(map, 116600, 10020, {186553, 222484});
        CheckActor(map, 116601, 10030, {186476, 186480});
    }
    HotfixDatabase.Close();
    WorldDatabase.Close();
    std::cout << "PASS: native live pool admission, nine direct cast/hit paths, self auras, damage, disease ticks, three summons, automatic Aggressor schedules and reset. No client or exact retail balance/timing claim.\n";
    return 0;
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try { return Run(argc, argv); }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

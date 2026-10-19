#include <unordered_set>
#include <Windows.h>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Creature.h"
#include "PassiveAI.h"
#include "PetDefines.h"
#include "TemporarySummon.h"
#include "SpellAuras.h"
#include "World.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "GameTables.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "CreatureAIRegistry.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "QuestData.h"
#include "ScriptsData.h"
#include "GameTime.h"
#include "Group.h"
#include "Player.h"
#include "SpellScript.h"
#include <array>
#include <algorithm>
#include <iostream>
#include <stdexcept>

static void Require(bool ok, char const* message) { if (!ok) throw std::runtime_error(message); }

// The scheduler lookup is the sole controlled service in the production
// header. No native quest creation/save/reset or live rotation is requested.
struct RotationFixture
{
    WorldQuest rotation{};
    bool active = true;
    Quest const* GetQuestTemplate(uint32) { return nullptr; }
    WorldQuest const* GetWorldQuest(Quest const*) { return active ? &rotation : nullptr; }
} rotationFixture;
#undef sQuestDataStore
#define sQuestDataStore (&rotationFixture)
#include "rockaway_coast.h"
#undef sQuestDataStore

struct Actor : Creature
{
    Actor(Map* map, uint32 entry, uint32 guid, Position const& pos)
    {
        Require(Create(guid, map, 1, entry, 0, ALLIANCE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()), "Native actor creation failed");
        SetHomePosition(*this);
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(GetEntry() == 115133 ? static_cast<CreatureAI*>(new npc_rockaway_vision(this)) : new NullCreatureAI(this)), "Native actor AI failed");
    }
    void RemoveFromWorld() override
    {
        Unit::RemoveFromWorld();
        sObjectAccessor->RemoveObject(static_cast<Creature*>(this));
    }
    ~Actor()
    {
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

static void Pump(Actor& vision, uint32 milliseconds, bool updateController = true)
{
    auto ai = static_cast<npc_rockaway_vision*>(vision.AI());
    for (uint32 elapsed = 0; elapsed < milliseconds; elapsed += 10)
    {
        vision.Unit::Update(10);
        if (updateController) ai->UpdateAI(10);
        auto slots = std::array<ObjectGuid, 4>{ai->slots[0], ai->slots[1], ai->slots[2], ai->slots[3]};
        for (auto guid : slots)
            if (auto ritualist = ObjectAccessor::GetCreature(vision, guid))
                ritualist->Update(10);
        vision.GetMap()->RemoveAllObjectsInRemoveList();
    }
}

static unsigned AuraCount(Actor const& vision)
{
    unsigned count = 0;
    for (auto const& entry : vision.GetOwnedAuras()) if (entry.second->GetId() == 231433) ++count;
    return count;
}
static void CheckChannels(Actor& vision, unsigned expected, bool allowOffensiveCast = false)
{
    auto ai = static_cast<npc_rockaway_vision*>(vision.AI());
    unsigned found = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (auto caster = ObjectAccessor::GetCreature(vision, ai->slots[i]))
        {
            bool aura = vision.HasAura(231433, caster->GetGUID());
            bool linked = false;
            for (auto guid : caster->GetChannelObjects()) if (guid == vision.GetGUID()) linked = true;
            std::cout << "CHANNEL slot=" << i << " alive=" << caster->IsAlive() << " aura=" << aura
                << " spell=" << caster->GetChannelSpellId() << " owner-link=" << linked << '\n';
            if (aura) { Require(caster->GetChannelSpellId() == 231433 && linked, "Aura exists without actual channel ownership"); ++found; }
        }
    Require(found == expected, "Unexpected native simultaneous channel count");
    Require(AuraCount(vision) == expected, "Native aura map contains an unaccounted channel");
    std::cout << "VISION channel=" << vision.GetChannelSpellId() << " casting=" << vision.HasUnitState(UNIT_STATE_CASTING) << '\n';
    Require(vision.GetChannelSpellId() != 231433, "Vision became a ritual channel caster");
    if (!allowOffensiveCast)
        Require(vision.GetChannelSpellId() == 0 && !vision.HasUnitState(UNIT_STATE_CASTING), "Vision retained a pending native cast");
}

int Run(int argc, char** argv)
{
    Require(argc == 3, "Expected private configuration and runtime directory");
    std::string error;
    Require(sConfigMgr->LoadInitial(argv[1], {}, error), "Private configuration failed");
    sLog->Initialize(nullptr); MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo", ""), 1, 1);
    Require(!HotfixDatabase.Open() && HotfixDatabase.PrepareStatements(), "Hotfix pool failed");
    Require(!WorldDatabase.Open() && WorldDatabase.PrepareStatements(), "World pool failed");
    for (auto pool : {&WorldDatabase})
    {
        pool->DirectExecute("SET SESSION TRANSACTION READ ONLY"); pool->Execute("SET SESSION TRANSACTION READ ONLY");
        auto result = pool->Query("SELECT @@session.transaction_read_only");
        Require(result && result->Fetch()[0].GetBool(), "World session is not read-only");
    }
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto hotfixReadOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    Require(hotfixReadOnly && hotfixReadOnly->Fetch()[0].GetBool(), "Hotfix session is not read-only");
    sDB2Manager.LoadStores(argv[2], LOCALE_enUS); LoadGameTables(argv[2]);
    sSpellMgr->LoadSpellInfoStore(); sSpellMgr->LoadSpellRanks(); sSpellMgr->LoadTalentSpellInfo();
    sSpellMgr->LoadSkillLineAbilityMap(); sSpellMgr->LoadSpellCustomAttr(); sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadScriptNames(); sObjectMgr->LoadCreatureModelInfo(); sObjectMgr->LoadWDBCreatureTemplates();
    sObjectMgr->LoadCreatureTemplates(); sObjectMgr->LoadCreatureClassLevelStats();
    sObjectMgr->LoadCreatureScalingData(); sObjectMgr->LoadCreatureDifficultyStat();
    sScriptDataStore->LoadSpellScriptNames();
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceEntry IN(231433,231296,231329)");
    Require(conditions && conditions->Fetch()[0].GetUInt32() == 0, "Native probe must load newly added spell conditions before testing");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    RegisterCreatureAI(npc_rockaway_ritualist);
    RegisterCreatureAI(npc_rockaway_tentacle);
    RegisterCreatureAI(npc_rockaway_slime);
    RegisterSpellScript(spell_rockaway_channel);
    RegisterAuraScript(spell_rockaway_taint);
    sScriptMgr->SwapScriptContext(true);
    Require(sScriptMgr->GetSpellScriptLoader(sObjectMgr->GetScriptId("spell_rockaway_channel")), "Native channel hook is not registered");
    auto bindings = sScriptDataStore->GetSpellScriptsBounds(231433); unsigned channelBindings = 0;
    for (auto it = bindings.first; it != bindings.second; ++it)
        if (it->second.first == sObjectMgr->GetScriptId("spell_rockaway_channel") && it->second.second) ++channelBindings;
    Require(channelBindings == 1, "Actual native spell binding did not enable the channel hook");
    AIRegistry::Initialize(); UnitAI::FillAISpellInfo(); sWorld->LoadConfigSettings();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220, {}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220, {}}});
    HashMapHolder<Creature>::SetSize(20000);
    rotationFixture.rotation.ResetTime = GameTime::GetGameTime() + 3600;
    {
        Map map(1220, 300, 9999998, DIFFICULTY_NONE);
        Actor vision(&map, 115133, 10000, {6229.235f, 4951.045f, 0.0f, 0.0f});
        map.LoadGrid(vision.GetPositionX(), vision.GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(&vision)), "Vision map admission failed");
        Pump(vision, 1500);
        auto ai = static_cast<npc_rockaway_vision*>(vision.AI());
        Require(ai->enabled && ai->ready, "Production AI did not summon all four native ritualists");
        CheckChannels(vision, 4);
        Pump(vision, 2000); CheckChannels(vision, 4);
        std::cout << "PASS: four native Spell/Aura channels coexist and remain attached to the admitted owner.\n";

        Actor decoy(&map, 115133, 10001, {6218.0f, 5017.0f, 0.9f, 0.0f});
        Require(map.AddToMap(static_cast<Creature*>(&decoy)), "Decoy map admission failed");
        auto first = ObjectAccessor::GetCreature(vision, ai->slots[0]);
        Require(first, "Missing first ritualist");
        first->CastStop();
        Require(first->CastSpell(&decoy, 231433, true) == SPELL_CAST_OK, "Native explicit-decoy cast rejected");
        Pump(vision, 1500); CheckChannels(vision, 4);
        Require(!decoy.HasAura(231433), "Native nearby resolver attached a channel to a decoy");
        auto outsider = vision.SummonCreature(115132, *first, TEMPSUMMON_MANUAL_DESPAWN);
        Require(outsider, "Native outsider summon failed");
        outsider->CastSpell(&vision, 231433, true);
        for (unsigned i = 0; i < 100; ++i) outsider->Update(10);
        Require(!vision.HasAura(231433, outsider->GetGUID()) && outsider->GetChannelSpellId() == 0,
            "Native CheckCast admitted an unslotted ritualist");
        outsider->DespawnOrUnsummon(); map.RemoveAllObjectsInRemoveList();
        std::cout << "PASS: registered native hook redirects an explicit closer decoy and rejects an unslotted summon.\n";

        vision.RemoveAurasDueToSpell(231433, first->GetGUID());
        Pump(vision, 1500); CheckChannels(vision, 4);
        ai->ready = false; Pump(vision, 1500, false); CheckChannels(vision, 0);
        ai->ready = true; Pump(vision, 1500); CheckChannels(vision, 4);
        ai->outro = true; ai->finishTime = 10000; Pump(vision, 1500); CheckChannels(vision, 0);
        ai->outro = false; ai->finishTime = 0; Pump(vision, 1500); CheckChannels(vision, 4);
        first->SetPhaseMask(2, true); Pump(vision, 1500); CheckChannels(vision, 3);
        first->SetPhaseMask(1, true); Pump(vision, 1500); CheckChannels(vision, 4);
        std::cout << "PASS: actual lost-aura retry, readiness/outro cancellation and phase restoration use native channel cleanup.\n";

        Actor victim(&map, 115846, 10002, {6229.0f, 4952.0f, 0.0f, 0.0f});
        Require(map.AddToMap(static_cast<Creature*>(&victim)), "Offensive target map admission failed");
        victim.setFaction(1); // A hostile target for the Vision's player-hostile faction.
        victim.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        victim.SetMaxHealth(1000000000); victim.SetHealth(1000000000);
        Require(sSpellMgr->GetSpellInfo(231329)->HasAttribute(SPELL_ATTR3_ONLY_TARGET_PLAYERS) &&
            vision.CastSpell(&victim, 231329, TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_DONT_REPORT_CAST_ERROR)) == SPELL_FAILED_TARGET_NOT_PLAYER,
            "Native player-only taint admission changed");
        ai->started = true;
        auto attackResult = vision.CastSpell(&victim, 231296, true);
        Require(attackResult == SPELL_CAST_OK, "Four incoming channels blocked the native tentacle cast");
        Pump(vision, 500);
        std::list<Creature*> tentacles; vision.GetCreatureListWithEntryInGrid(tentacles, 116187, 30.0f);
        Require(tentacles.size() == 1 && tentacles.front()->ToTempSummon() &&
            tentacles.front()->ToTempSummon()->GetSummonerGUID() == vision.GetGUID(), "Native offensive cast did not create its owned tentacle");
        CheckChannels(vision, 4, true); ai->started = false;
        std::cout << "OFFENSIVE channeled=" << sSpellMgr->GetSpellInfo(231296)->IsChanneled()
            << " duration=" << sSpellMgr->GetSpellInfo(231296)->GetDuration() << '\n';
        Require(vision.GetChannelSpellId() == 231296 && sSpellMgr->GetSpellInfo(231296)->GetDuration() == 10000,
            "Reinspect the native offensive channel duration");
        Pump(vision, 10000); CheckChannels(vision, 4);
        std::cout << "PASS: native offensive231296 creates its actual116187 summon while four channels remain;231329 correctly rejects a non-Player surrogate.\n";

        std::array<unsigned, 4> order = {0, 1, 2, 3}; unsigned permutations = 0;
        do
        {
            ai->Reset(); Pump(vision, 1500); CheckChannels(vision, 4);
            for (unsigned i = 0; i < 4; ++i)
            {
                auto caster = ObjectAccessor::GetCreature(vision, ai->slots[order[i]]);
                Require(caster, "Missing native death-order actor");
                auto guid = caster->GetGUID();
                victim.Kill(caster, false);
                Pump(vision, 100);
                Require(!vision.HasAura(231433, guid), "Native death retained the dead caster's aura");
                CheckChannels(vision, 3 - i);
                for (unsigned j = i + 1; j < 4; ++j)
                    Require(vision.HasAura(231433, ai->slots[order[j]]), "One native death removed another caster's channel");
            }
            Require(ai->outro, "Actual summoner death dispatcher did not reach final state");
            ++permutations;
        } while (std::next_permutation(order.begin(), order.end()));
        std::cout << "PASS: " << permutations << " native Unit::Kill death orders remove only each caster's channel and reach the production outro.\n";

        ai->Reset(); Pump(vision, 1500); CheckChannels(vision, 4);
        Require(victim.AddAura(231433, &vision), "Native foreign aura setup failed");
        Require(AuraCount(vision) == 5, "Foreign aura replaced a native ritual channel");
        ai->ClearRun(); map.RemoveAllObjectsInRemoveList();
        Require(AuraCount(vision) == 1 && vision.HasAura(231433, victim.GetGUID()), "Production reset removed a foreign caster's aura");
        ai->ClearRun();
        Require(AuraCount(vision) == 1, "Repeated empty-slot reset removed a foreign aura");
        vision.RemoveAurasDueToSpell(231433, victim.GetGUID());
        ai->Reset(); Pump(vision, 1500); CheckChannels(vision, 4);
        rotationFixture.active = false; Pump(vision, 1500);
        Require(!ai->enabled && !vision.HasAura(231433), "Native rotation expiry left a channel aura");
        for (auto guid : ai->slots) Require(guid.IsEmpty(), "Native expiry retained an actor slot");
        rotationFixture.active = true; Pump(vision, 1500); CheckChannels(vision, 4);
        rotationFixture.rotation.ResetTime = GameTime::GetGameTime(); Pump(vision, 1500);
        Require(!ai->enabled && !vision.HasAura(231433), "Exact native clock expiry retained channels");
        rotationFixture.rotation.ResetTime = GameTime::GetGameTime() + 3600; Pump(vision, 1500); CheckChannels(vision, 4);
        std::cout << "PASS: real reset/empty-slot reset preserves foreign aura; rotation exit cleans all four and re-entry restores them.\n";

        auto orphanGuids = std::array<ObjectGuid, 4>{ai->slots[0], ai->slots[1], ai->slots[2], ai->slots[3]};
        vision.CleanupBeforeRemoveFromMap(true);
        map.RemoveFromMap(static_cast<Creature*>(&vision), false);
        for (auto guid : orphanGuids)
            if (auto caster = map.GetCreature(guid))
            {
                for (unsigned i = 0; i < 150; ++i) caster->Update(10);
                Require(caster->GetChannelSpellId() == 0 && caster->GetChannelObjects().size() == 0, "Native orphan retained channel fields");
            }
        Require(!vision.HasAura(231433), "Removed owner retained native aura objects");
        // RemoveFromMap resets the owner's Map pointer. Orphan summons use
        // their own map for lookup and native removal rather than stale AI.
        for (auto guid : orphanGuids) if (auto caster = map.GetCreature(guid)) caster->DespawnOrUnsummon();
        map.RemoveAllObjectsInRemoveList();
        std::cout << "PASS: native owner map removal clears applied auras and orphan channels.\n";
    }
    HotfixDatabase.Close(); WorldDatabase.Close();
    return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try { return Run(argc, argv); }
    catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}

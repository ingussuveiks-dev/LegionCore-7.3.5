// Real Player, Spell, Aura, TempSummon and production actor AI execution.
// Shared bootstrap links the same generated final-link game.lib as the server.
#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "WorldSession.h"
#include "CellImpl.h"
#include "Opcodes.h"
#include "ConditionMgr.h"
#include "SpellAuraEffects.h"
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

static LONG WINAPI CrashTrace(EXCEPTION_POINTERS* exception)
{
    auto process = GetCurrentProcess(); SymInitialize(process, nullptr, TRUE);
    CONTEXT context = *exception->ContextRecord;
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = context.Rip; frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp; frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp; frame.AddrStack.Mode = AddrModeFlat;
    std::cerr << "NATIVE EXCEPTION " << std::hex << exception->ExceptionRecord->ExceptionCode << std::dec << '\n';
    for (unsigned i = 0; i < 24 && frame.AddrPC.Offset; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
        auto symbol = reinterpret_cast<SYMBOL_INFO*>(storage); symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 offset = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol)) std::cerr << symbol->Name << '+' << offset << '\n';
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) break;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static uint32 clockMs;
// Explicit instantiation grants member-pointer access only for isolated fixture
// setup. It does not rewrite class layout or replace native eligibility code.
template<class Tag, typename Tag::Type member> struct FixtureAccess
{ friend typename Tag::Type Access(Tag) { return member; } };
struct QuestVectorTag { using Type = QuestVector QuestDataStoreMgr::*; friend Type Access(QuestVectorTag); };
struct QuestMaxTag { using Type = uint32 QuestDataStoreMgr::*; friend Type Access(QuestMaxTag); };
template struct FixtureAccess<QuestVectorTag, &QuestDataStoreMgr::_questVTemplates>;
template struct FixtureAccess<QuestMaxTag, &QuestDataStoreMgr::_maxQuestId>;
struct AttachConditionTag { using Type = bool (ConditionMgr::*)(Condition*); friend Type Access(AttachConditionTag); };
template struct FixtureAccess<AttachConditionTag, &ConditionMgr::addToSpellImplicitTargetConditions>;
struct ValidateSourceTag { using Type = bool (ConditionMgr::*)(Condition*); friend Type Access(ValidateSourceTag); };
template struct FixtureAccess<ValidateSourceTag, &ConditionMgr::isSourceTypeValid>;

static void LoadOffensiveConditions()
{
    auto rows = WorldDatabase.Query("SELECT SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorTextId, ScriptName FROM conditions WHERE SourceEntry IN (231296,231298,231329,231340,231351,231353,231354,231359,231386,232681,232679)");
    unsigned count = 0;
    if (rows) do
    {
        auto f = rows->Fetch();
        auto condition = new Condition;
        Require(f[0].GetInt32() == 13 && f[5].GetInt32() >= 0 && f[12].GetString().empty(), "Probe must support new condition sources/references/scripts before proceeding");
        condition->SourceType = ConditionSourceType(f[0].GetInt32()); condition->SourceGroup = f[1].GetUInt32();
        condition->SourceEntry = f[2].GetUInt32(); condition->SourceId = f[3].GetInt32(); condition->ElseGroup = f[4].GetUInt32();
        condition->ConditionType = ConditionTypes(f[5].GetInt32()); condition->ConditionTarget = f[6].GetUInt8();
        condition->ConditionValue1 = f[7].GetUInt32(); condition->ConditionValue2 = f[8].GetUInt32(); condition->ConditionValue3 = f[9].GetUInt32();
        condition->NegativeCondition = f[10].GetBool(); condition->ErrorTextId = f[11].GetUInt32();
        Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition) &&
            (sConditionMgr->*Access(AttachConditionTag{}))(condition), "Native scoped condition admission failed");
        std::cout << "CONDITION spell=" << condition->SourceEntry << " mask=" << condition->SourceGroup << " type=" << condition->ConditionType << " value=" << condition->ConditionValue1 << '\n';
        ++count;
        // Native effect condition lists own the references for process lifetime,
        // like ConditionMgr's ordinary allocated-condition store.
    } while (rows->NextRow());
    Require(count == 1, "Reinspect changed encounter conditions");
}
struct ObservedVisionAI : npc_rockaway_vision
{
    using npc_rockaway_vision::npc_rockaway_vision;
    std::map<uint32, unsigned> hits, spawned;
    std::map<uint32, std::vector<uint32>> hitTimes, summonTimes;
    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        ++hits[spell->Id];
        hitTimes[spell->Id].push_back(clockMs);
        std::cout << "HIT time=" << clockMs << " spell=" << spell->Id << " player=" << target->IsPlayer() << std::endl;
    }
    void JustSummoned(Creature* actor) override
    {
        npc_rockaway_vision::JustSummoned(actor);
        ++spawned[actor->GetEntry()];
        summonTimes[actor->GetEntry()].push_back(clockMs);
        std::cout << "SUMMON time=" << clockMs << " entry=" << actor->GetEntry() << std::endl;
    }
};
struct Vision : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new ObservedVisionAI(this)), "Observed production Vision AI failed");
    }
};
struct NativePlayer : Player
{
    NativePlayer(WorldSession* session, Map& map, uint64 guid, Position const& pos) : Player(session)
    {
        _Create(ObjectGuid::Create<HighGuid::Player>(guid));
        SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_WARRIOR);
        SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_MALE);
        SetLevel(110); setFaction(1); SetMaxHealth(1000000000); SetHealth(1000000000);
        SetFloatValue(UNIT_FIELD_BOUNDING_RADIUS, 0.389f); SetFloatValue(UNIT_FIELD_COMBAT_REACH, 1.5f);
        Relocate(pos); SetMap(&map); session->SetPlayer(this); sWorld->IncreasePlayerCount();
        sObjectAccessor->AddObject(static_cast<Player*>(this));
        map.LoadGrid(pos.GetPositionX(), pos.GetPositionY());
        map.AddToGrid(static_cast<Player*>(this), Cell(Trinity::ComputeCellCoord(pos.GetPositionX(), pos.GetPositionY())));
        Unit::AddToWorld();
        // Controlled accepted-task state; no login, character DB or reward path.
        SetQuestStatus(44746, QUEST_STATUS_INCOMPLETE);
    }
    ~NativePlayer()
    {
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        Unit::RemoveFromWorld(); RemoveFromGrid();
        sObjectAccessor->RemoveObject(static_cast<Player*>(this)); ResetMap();
    }
};

static void Advance(Vision& vision, NativePlayer& player, uint32 duration, bool controller = true)
{
    auto ai = static_cast<ObservedVisionAI*>(vision.AI());
    for (uint32 elapsed = 0; elapsed < duration; elapsed += 10)
    {
        clockMs += 10;
        player.Unit::Update(10);
        vision.Unit::Update(10);
        if (controller) ai->UpdateAI(10);
        GuidList actors = ai->summons;
        for (auto guid : actors) if (auto actor = ObjectAccessor::GetCreature(vision, guid)) actor->Update(10);
        vision.GetMap()->MoveAllCreaturesInMoveList();
        vision.GetMap()->RemoveAllObjectsInRemoveList();
    }
}

#ifndef ROCKAWAY_NATIVE_SUPPORT_ONLY
static int OffensiveRun(int argc, char** argv)
{
    InitializeNative(argc, argv);
    opcodeTable.Initialize();
    sSpellMgr->LoadSpellBonusess(); sSpellMgr->LoadSpellThreats();
    LoadOffensiveConditions();
    auto row = WorldDatabase.Query(
#include "NativeRockawayQuest.inc"
    );
    Require(bool(row), "Actual Rockaway quest row missing");
    auto quest = new Quest(row->Fetch());
    Require(quest->GetQuestId() == 44746 && quest->IsWorld(), "Actual Rockaway quest type changed");
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[44746] = quest;
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(44747); quests[44746] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = 44746;
    WorldQuestTemplate policy{};
    auto& rotation = const_cast<WorldQuestMap&>(*sQuestDataStore->GetWorldQuestMap())[quest->QuestSortID][44746];
    rotation.ResetTime = GameTime::GetGameTime() + 3600; rotation.worldQuest = &policy;
    // Native session constructor reads account_rates. All login sessions are
    // read-only; no real account/socket/character is attached. This process-
    // lifetime session intentionally skips the destructor's account UPDATE.
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Read-only login pool failed");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readonly = LoginDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readonly && readonly->Fetch()[0].GetBool(), "Login session is not read-only");
    auto session = new WorldSession(0, std::string("Rockaway native probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    HashMapHolder<Player>::SetSize(32);
    {
        Map map(1220, 300, 9999997, DIFFICULTY_NONE);
        Vision vision(&map, 115133, 20000, {6229.235f, 4951.045f, 0.0f, 0.0f});
        map.LoadGrid(vision.GetPositionX(), vision.GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(&vision)), "Vision map admission failed");
        NativePlayer player(session, map, 100, {6235.0f, 4951.0f, 0.0f, 0.0f});
        Advance(vision, player, 1500);
        auto ai = static_cast<ObservedVisionAI*>(vision.AI());
        Require(Rockaway::Eligible(&vision, &player), "Native Player eligibility failed");
        ai->SetGUID(player.GetGUID(), 0);
        Require(ai->started, "Actual production contributor start failed");
        auto start = clockMs;
        std::cout << "BEGIN combat" << std::endl;
        Advance(vision, player, 40000);
        std::cout << "RESULT health=" << player.GetHealth() << " tentacles=" << ai->spawned[116187]
            << " slimes=" << ai->spawned[116195] << " taint=" << ai->hits[231329] << std::endl;
        Require(ai->spawned[116187] >= 2, "Native scheduled tentacles missing");
        Require(ai->spawned[116195] == 5, "Native five-missile slime chain missing or duplicated");
        Require(ai->hits[231329] > 0, "Native scheduled player-only taint missing");
        Require(ai->hits[232679] == 1, "Native natural taint expiry did not hit the Player exactly once");
        Require(ai->summonTimes[116187][0] - start >= 5000 && ai->summonTimes[116187][0] - start <= 5020,
            "First native tentacle schedule changed");
        Require(ai->hitTimes[231351][0] - start >= 15000 && ai->hitTimes[231351][0] - start <= 15030 &&
            ai->hitTimes[231329][0] - start >= 20000, "Queued attacks did not wait for native 10-second and 5-second channels");
        CheckChannels(vision, 4, true);
        std::cout << "PASS: actual scheduler waits for native channels; repeated tentacles, exactly five slimes and player-only taint/expiry damage execute with all four ritual channels intact.\n";

        auto restart = [&]()
        {
            ai->Reset(); map.RemoveAllObjectsInRemoveList();
            player.RemoveAllAuras(); player.SetFullHealth(); map.PlayerRelocation(&player, 6235.0f, 4951.0f, 0.0f, 0.0f);
            Advance(vision, player, 1500); ai->SetGUID(player.GetGUID(), 0); ai->events.Reset();
            Require(ai->started && ai->ready, "Isolated native run did not restart");
        };

        restart();
        ai->attackTarget = player.GetGUID();
        Require(vision.CastSpell(&player, 231296, true) == SPELL_CAST_OK, "Isolated native tentacle rejected");
        Advance(vision, player, 100, false);
        auto tentacle = ai->summons.GetCreature(116187);
        Require(tentacle && tentacle->AI(), "Native tentacle/factory AI missing");
        auto tentacleGuid = tentacle->GetGUID();
        map.PlayerRelocation(&player, tentacle->GetPositionX() + std::cos(tentacle->GetOrientation()) * 4.0f,
            tentacle->GetPositionY() + std::sin(tentacle->GetOrientation()) * 4.0f, tentacle->GetPositionZ(), 0.0f);
        auto health = player.GetHealth();
        Advance(vision, player, 2800, false);
        Require(player.GetHealth() == health && !player.HasAura(231298), "Native tentacle slammed before its warning delay");
        Advance(vision, player, 600, false);
        std::cout << "SLAM timer=" << static_cast<npc_rockaway_tentacle*>(tentacle->AI())->timer
            << " hp=" << health << "->" << player.GetHealth() << " stun=" << player.HasAura(231298)
            << " caster=" << tentacle->GetPositionX() << ',' << tentacle->GetPositionY() << ',' << tentacle->GetPositionZ() << ',' << tentacle->GetOrientation()
            << " player=" << player.GetPositionX() << ',' << player.GetPositionY() << ',' << player.GetPositionZ()
            << " width=" << sSpellMgr->GetSpellInfo(231298)->TargetRestrictions.Width
            << " radius=" << sSpellMgr->GetSpellInfo(231298)->GetEffect(EFFECT_0, DIFFICULTY_NONE)->CalcRadius(tentacle)
            << " casttime=" << sSpellMgr->GetSpellInfo(231298)->CalcCastTime(tentacle) << std::endl;
        Require(player.GetHealth() < health && player.HasAura(231298), "Native delayed directional slam did not damage and stun the Player");
        auto healthAfterSlam = player.GetHealth();
        Advance(vision, player, 1800, false);
        Require(player.GetHealth() == healthAfterSlam, "Native tentacle repeated its one-shot slam");
        Advance(vision, player, 1500, false);
        Require(!ObjectAccessor::GetCreature(vision, tentacleGuid), "Native tentacle did not expire after six seconds");
        std::cout << "PASS: actual tentacle waits three seconds, damages/stuns a Player in its forward lane once, then expires at six seconds.\n";

        restart(); ai->attackTarget = player.GetGUID();
        Require(vision.CastSpell(&player, 231296, true) == SPELL_CAST_OK, "Rear-lane native tentacle rejected");
        Advance(vision, player, 100, false); tentacle = ai->summons.GetCreature(116187);
        Require(tentacle, "Rear-lane native tentacle missing");
        map.PlayerRelocation(&player, tentacle->GetPositionX() - std::cos(tentacle->GetOrientation()) * 4.0f,
            tentacle->GetPositionY() - std::sin(tentacle->GetOrientation()) * 4.0f, tentacle->GetPositionZ(), 0.0f);
        health = player.GetHealth(); Advance(vision, player, 3400, false);
        Require(player.GetHealth() == health && !player.HasAura(231298), "Native forward-lane slam hit a Player behind the tentacle");
        std::cout << "PASS: actual native between-target geometry excludes a Player behind the tentacle.\n";

        restart();
        auto slimesBefore = ai->spawned[116195];
        ai->events.ScheduleEvent(2, 0);
        Advance(vision, player, 2200);
        Require(ai->spawned[116195] == slimesBefore + 5, "Production breath did not dispatch exactly five native missile/summon chains");
        GuidList slimeGuids;
        for (auto guid : ai->summons) if (auto slime = ObjectAccessor::GetCreature(vision, guid)) if (slime->GetEntry() == 116195)
        {
            Require(slime->ToTempSummon() && slime->ToTempSummon()->GetSummonerGUID() == vision.GetGUID() &&
                slime->getVictim() == &player, "Native slime lost owner or actual Player attack target");
            slimeGuids.push_back(guid);
        }
        Require(slimeGuids.size() == 5, "Native map has a missing or additional breath slime");
        health = player.GetHealth();
        Advance(vision, player, 6000, false);
        std::cout << "MELEE health=" << health << "->" << player.GetHealth() << std::endl;
#ifdef ROCKAWAY_OFFENSIVE_BASELINE
        Require(player.GetHealth() == health, "Pinned baseline no longer reproduces inert slime melee");
        for (auto guid : slimeGuids) Require(ObjectAccessor::GetCreature(vision, guid)->IsDespawn(), "Pinned baseline did not mark delayed slimes as despawning");
        vision.CastStop();
        Require(vision.CastSpell(&player, 231296, true) == SPELL_CAST_OK, "Baseline cleanup tentacle summon failed");
        Advance(vision, player, 100, false);
        GuidList hazards;
        for (auto guid : ai->summons) if (auto actor = ObjectAccessor::GetCreature(vision, guid))
            if (actor->GetEntry() == 116195 || actor->GetEntry() == 116187) hazards.push_back(guid);
        Require(hazards.size() == 6, "Baseline cleanup setup missing hazards");
        ai->ClearRun(); map.RemoveAllObjectsInRemoveList();
        for (auto guid : hazards)
        {
            auto actor = ObjectAccessor::GetCreature(vision, guid);
            Require(actor && actor->IsInWorld(), "Pinned baseline no longer reproduces reset hazard leak");
            actor->ToTempSummon()->UnSummon();
        }
        map.RemoveAllObjectsInRemoveList();
        std::cout << "PASS: pinned a788f5d2 reproduces five combat-capable slimes blocked by IsDespawn and all six hazards surviving ClearRun.\n";
#else
        Require(player.GetHealth() < health, "Native slime AI did not execute melee damage");
        for (auto guid : slimeGuids) Require(!ObjectAccessor::GetCreature(vision, guid)->IsDespawn(), "Live slime was prematurely marked despawning");
        Advance(vision, player, ai->summonTimes[116195].back() + 44000 - clockMs, false);
        for (auto guid : slimeGuids) Require(ObjectAccessor::GetCreature(vision, guid), "Native slime expired before 45 seconds");
        Advance(vision, player, 2000, false);
        for (auto guid : slimeGuids) Require(!ObjectAccessor::GetCreature(vision, guid), "Native slime exceeded its 45-second lifetime");
        std::cout << "PASS: five native breath missiles create five owned slimes with Player victims; native melee damages the target and all expire.\n";

        restart();
        auto applyTaint = [&]()
        {
            auto count = ai->hits[231329];
            auto before = player.GetHealth();
            Require(vision.CastSpell(&player, 231329, true) == SPELL_CAST_OK, "Native taint cast rejected a real Player");
            Advance(vision, player, 500, false);
            auto aura = player.GetAura(231329, vision.GetGUID());
            Require(aura && ai->hits[231329] == count + 1 && player.GetHealth() < before, "Native taint did not deliver initial damage/aura");
            return aura;
        };
        auto aura = applyTaint();
        auto effect = aura->GetEffect(EFFECT_1);
        Require(effect && effect->GetPeriod() > 0 && effect->GetTickNumber() == 0, "Native taint periodic setup changed");
        auto period = effect->GetPeriod();
        auto burstBefore = ai->hits[232679]; health = player.GetHealth();
        Advance(vision, player, period, false);
        Require(effect->GetTickNumber() == 1 && player.GetHealth() < health && ai->hits[232679] == burstBefore,
            "Native taint periodic damage did not tick independently before expiry");
        Advance(vision, player, 11000, false);
        Require(!player.HasAura(231329) && ai->hits[232679] == burstBefore + 1, "Native taint expiry did not deliver one essence burst");
        Advance(vision, player, 1000, false);
        Require(ai->hits[232679] == burstBefore + 1, "Native taint expiry duplicated its burst");
        std::cout << "PASS: native Player taint delivers direct damage, actual periodic ticks and exactly one expiry missile/essence hit.\n";

        for (auto mode : {AURA_REMOVE_BY_ENEMY_SPELL, AURA_REMOVE_BY_DEFAULT, AURA_REMOVE_BY_DEATH})
        {
            restart(); applyTaint(); burstBefore = ai->hits[232679];
            player.RemoveAurasDueToSpell(231329, vision.GetGUID(), 0, mode);
            // Native232681 uses Speed4 as a fixed missile delay, not an
            // immediate impact. Advance beyond its four-second travel.
            Advance(vision, player, 4500, false);
            std::cout << "REMOVE mode=" << mode << " aura=" << player.HasAura(231329) << " active=" << ai->GetData(1)
                << " bursts=" << burstBefore << "->" << ai->hits[232679] << " pos=" << player.GetPositionX() << ',' << player.GetPositionY()
                << " grid=" << player.IsInGrid() << " world=" << player.IsInWorld()
                << " speed=" << sSpellMgr->GetSpellInfo(232681)->GetMisc()->MiscData.Speed << std::endl;
            Require(ai->hits[232679] == burstBefore + (mode == AURA_REMOVE_BY_ENEMY_SPELL ? 1 : 0), "Native taint removal mode created a missing/unwanted essence burst");
        }
        restart(); applyTaint(); burstBefore = ai->hits[232679];
        ai->ClearRun(); map.RemoveAllObjectsInRemoveList(); Advance(vision, player, 11000, false);
        Require(!player.HasAura(231329) && ai->hits[232679] == burstBefore, "Native reset cleanup left taint or created a new burst");
        restart(); applyTaint(); burstBefore = ai->hits[232679];
        ai->outro = true; player.RemoveAurasDueToSpell(231329, vision.GetGUID(), 0, AURA_REMOVE_BY_EXPIRE);
        Advance(vision, player, 5000, false);
        Require(ai->hits[232679] == burstBefore, "Native outro expiry created a fresh hazard");
        ai->ClearRun(); map.RemoveAllObjectsInRemoveList();
        std::cout << "PASS: actual Aura removal dispatch triggers essence only on active expiry/enemy removal; default/death/reset/outro cleanup produces no new hazard.\n";

        restart(); ai->events.ScheduleEvent(2, 0); Advance(vision, player, 2200);
        vision.CastStop(); ai->attackTarget = player.GetGUID();
        Require(vision.CastSpell(&player, 231296, true) == SPELL_CAST_OK, "Reset cleanup tentacle summon failed");
        Advance(vision, player, 100, false); applyTaint();
        GuidList hazards;
        for (auto guid : ai->summons) if (auto actor = ObjectAccessor::GetCreature(vision, guid))
            if (actor->GetEntry() == 116195 || actor->GetEntry() == 116187)
            {
                Require(!actor->IsDespawn(), "Live helper is already marked pending removal"); hazards.push_back(guid);
            }
        Require(hazards.size() == 6, "Reset cleanup setup missing six real hazards");
        burstBefore = ai->hits[232679]; ai->ClearRun(); map.RemoveAllObjectsInRemoveList();
        for (auto guid : hazards) Require(!ObjectAccessor::GetCreature(vision, guid), "Native ClearRun left a live tentacle/slime");
        Require(!player.HasAura(231329), "Native ClearRun retained the controller's taint");
        Advance(vision, player, 5000, false);
        Require(ai->hits[232679] == burstBefore, "Reset cleanup emitted delayed essence damage");
        restart(); ai->events.ScheduleEvent(2, 0); Advance(vision, player, 10);
        ai->ClearRun(); map.RemoveAllObjectsInRemoveList(); Advance(vision, player, 5000, false);
        std::list<Creature*> leaked; vision.GetCreatureListWithEntryInGrid(leaked, 116195, 100.0f);
        Require(leaked.empty(), "A pre-reset missile created a surviving post-reset slime");
        std::cout << "PASS: native ClearRun immediately removes all six live hazards and taint; in-flight breath missiles cannot leave post-reset helpers.\n";
#endif
    }
    LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close();
    std::cout << "PASS: native Player-targeted production offensive scheduler executes tentacles, five-slime missiles and taint.\n";
    return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try { return OffensiveRun(argc, argv); }
    catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
#endif

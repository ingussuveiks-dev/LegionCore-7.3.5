// Research, not a repaired regression or a client traversal. The complete
// production Althea class runs without the older probes' manually seeded aura.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "../tests/AltheaIntroNative.cpp"
#include "SmartAI.h"
#include "SmartScriptMgr.h"

static void LoadAuthoredSmartRows()
{
    // These names shadow tables only on this owned synchronous connection.
    // MariaDB rejects even temporary DDL inside a READ ONLY transaction. Only
    // this owned connection's temporary-input setup is writable; restore and
    // assert READ ONLY before invoking any loader or actor operation.
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE althea_smart_seed LIKE smart_scripts");
    WorldDatabase.DirectExecute("INSERT INTO althea_smart_seed SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid IN(108214,108216,108217)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts LIKE althea_smart_seed");
    WorldDatabase.DirectExecute("INSERT INTO smart_scripts SELECT * FROM althea_smart_seed");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto rows = WorldDatabase.Query("SELECT COUNT(*) FROM smart_scripts");
    auto readOnly = WorldDatabase.Query("SELECT @@session.transaction_read_only");
    if (rows)
        std::cout << "SCOPED_INPUT\trows=" << rows->Fetch()[0].GetUInt32() << "\treadonly=" << (readOnly ? readOnly->Fetch()[0].GetString() : "NO RESULT") << '\n';
    Require(rows && rows->Fetch()[0].GetUInt32() == 8 && readOnly && readOnly->Fetch()[0].GetBool(), "Scoped actual SmartAI inputs/read-only session differ");
    auto combined = WorldDatabase.Query("SELECT COUNT(*),@@session.transaction_read_only FROM smart_scripts");
    Require(bool(combined), "Combined read-only diagnostic missing");
    std::cout << "COMBINED_QUERY_CONTROL\tcount=" << combined->Fetch()[0].GetString()
        << "\treadonly-raw=" << combined->Fetch()[1].GetString()
        << "\treadonly-bool=" << combined->Fetch()[1].GetBool() << '\n';
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry IN(108214,108216,108217,108218)) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(214873,183928,183934,128044,195898,183345,79852,10022,60195))");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "New actor/spell conditions need their native loader before this probe");
    sSmartScriptMgr->LoadSmartAIFromDB();
    for (auto pair : {std::make_pair(108214, 3u), std::make_pair(108216, 3u), std::make_pair(108217, 2u)})
        Require(sSmartScriptMgr->GetScript(pair.first, SMART_SCRIPT_TYPE_CREATURE).size() == pair.second, "Actual scoped SmartAI loader rejected an authored row");
}

int main(int argc, char** argv)
{
    try
    {
        InitializeAltheaGuardsNative(argc, argv);
        LoadAuthoredSmartRows();
        for (uint32 quest : {42504u, 42627u})
        {
            MapInstanced parent(1620, 300);
            InstanceMap map(1620, 300, 9998000 + quest, DIFFICULTY_N_SCENARIO, &parent);
            map.CreateInstanceData(nullptr);
            auto scenario = sScenarioMgr->AddScenario(&map, 1123);
            scenario->SetCurrentStep(4);
            auto session = Session();
            {
                NativePlayer player(session, map, 101, {-8630, 410, 103, 0});
                player.SetByteValue(UNIT_FIELD_BYTES_0, 0, quest == 42504 ? RACE_HUMAN : RACE_ORC);
                player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_ROGUE);
                player.setFactionForRace(player.getRace());
                player.SetQuestStatus(quest, QUEST_STATUS_INCOMPLETE);
                player.SetQuestSlot(0, quest);
                Actor source(&map, 108218, 21001, {-8607.59f, 394.141f, 110.255f, 5.36554f});
                Actor garona(&map, 108214, 21002, {-8643.02f, 421.559f, 102.051f, 1.82069f});
                Require(map.AddToMap(static_cast<Creature*>(&source)) && map.AddToMap(static_cast<Creature*>(&garona)), "Native original boss/ally admission failed");
                auto ai = new npc_sister_althea_ebonlocke::npc_sister_althea_ebonlockeAI(&source);
                Require(source.AIM_Initialize(ai) && garona.AIM_Initialize(new SmartAI(&garona)), "Actual boss/ally AI initialization failed");
                std::cout << "STARTUP\tquest=" << quest << "\taura=" << source.HasAura(214873)
                    << "\tphysical-immune=" << source.IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL)
                    << "\tshadow-immune=" << source.IsImmunedToDamage(SPELL_SCHOOL_MASK_SHADOW)
                    << "\tflags=" << source.GetUInt32Value(UNIT_FIELD_FLAGS) << '\n';
                // Deliberate isolated control only, never startup evidence.
                source.AddAura(214873, &source);
                Require(source.HasAura(214873), "Native manual aura control failed");
                std::cout << "MANUAL_AURA_CONTROL\tquest=" << quest
                    << "\tphysical-immune=" << source.IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL)
                    << "\tshadow-immune=" << source.IsImmunedToDamage(SPELL_SCHOOL_MASK_SHADOW) << '\n';
                source.RemoveAura(214873);
                Guard a(&map, 108216, 21003, {-8616.6f, 409.425f, 103.058f, 3.72312f});
                Guard b(&map, 108216, 21004, {-8620.01f, 406.714f, 103.058f, 0.679793f});
                Guard c(&map, 108217, 21005, {-8616.99f, 397.823f, 103.011f, 4.6208f});
                a.Bind(360869); b.Bind(360872); c.Bind(360871);
                for (auto guard : {&a, &b, &c})
                    Require(map.AddToMap(static_cast<Creature*>(guard)) && guard->AIM_Initialize(new SmartAI(guard)), "Native original guard SmartAI initialization failed");
                c.AI()->UpdateAI(2100);
                c.Unit::Update(10);
                std::cout << "OOC_CHANNEL\tquest=" << quest << "\taura=" << c.HasAura(195898)
                    << "\tchannel=" << c.GetChannelSpellId() << '\n';
                ai->MoveInLineOfSight(&player);
                Require(ai->introDone && ai->introDone2 && IntroCounter(scenario) == 1, "Native actual SmartAI handoff did not consume original intro");
                std::cout << "COMBAT_CHANNEL\tquest=" << quest << "\taura=" << c.HasAura(195898)
                    << "\tchannel=" << c.GetChannelSpellId() << '\n';
                for (auto guard : {&a, &b, &c})
                {
                    auto spell = sSpellMgr->GetSpellInfo(guard->GetEntry() == 108217 ? 183345 : 183928);
                    Require(spell, "Native authored guard spell absent");
                    std::cout << "HANDOFF\tquest=" << quest << "\tguard=" << guard->GetDBTableGUIDLow()
                        << "\tvictim-is-garona=" << (guard->getVictim() == &garona)
                        << "\tcombat=" << guard->isInCombat()
                        << "\tvalid-hostile-target=" << guard->IsValidAttackTarget(&garona)
                        << "\tspell-target-result=" << spell->CheckTarget(guard, &garona, false) << '\n';
                    guard->AI()->UpdateAI(100);
                    std::cout << "SMART_UPDATE\tquest=" << quest << "\tguard=" << guard->GetDBTableGUIDLow()
                        << "\tvictim-is-garona=" << (guard->getVictim() == &garona)
                        << "\tevading=" << guard->IsInEvadeMode() << '\n';
                }
                Require(!map.GetInstanceScript()->GetData(KingslayersGuards::AllDefeated) && !ai->introReleased, "SmartAI handoff manufactured guard deaths");
                // Native motion, spells and combat run from original positions.
                // Map scheduling/LOS visits and clock remain controlled here.
                for (uint32 elapsed = 0; elapsed < 45000; elapsed += 100)
                {
                    for (auto actor : {static_cast<Actor*>(&a), static_cast<Actor*>(&b), static_cast<Actor*>(&c), &garona})
                    {
                        actor->Unit::Update(100);
                        if (actor->IsAlive())
                            actor->AI()->UpdateAI(100);
                    }
                    ai->UpdateIntro(100);
                    map.MoveAllCreaturesInMoveList();
                }
                for (auto guard : {&a, &b, &c})
                    std::cout << "COMBAT_45S\tquest=" << quest << "\tguard=" << guard->GetDBTableGUIDLow()
                        << "\talive=" << guard->IsAlive() << "\tvictim-is-garona=" << (guard->getVictim() == &garona)
                        << "\tdistance-to-garona=" << guard->GetExactDist(&garona)
                        << "\tdeath-receipt=" << map.GetInstanceScript()->GetData(uint32(guard->GetDBTableGUIDLow())) << '\n';
                std::cout << "ALLY_45S\tquest=" << quest << "\talive=" << garona.IsAlive()
                    << "\thealth=" << garona.GetHealth() << "\treleased=" << ai->introReleased << '\n';
                CleanStories(map);
            }
            session->SetPlayer(nullptr); delete session;
            sScenarioMgr->RemoveScenario(map.GetInstanceId());
        }
        std::cout << "PASS two native research contexts; startup/manual-aura controls and original SmartAI handoff observed, no repair/client/full-combat claim\n";
        std::cout.flush();
        CloseAltheaNative();
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

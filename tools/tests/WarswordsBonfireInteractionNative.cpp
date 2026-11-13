// Actual native packet/use/cast/lock/criteria paths with controlled encounter
// setup and event-clock advancement. No persistent character writes or client
// packet capture is claimed. The retained helper supplies the read-only pools.
#define main RetainedBonfireMain
#include "WarswordsBonfireNative.cpp"
#undef main
#include "GameObjectPackets.h"
#include "GameObjectModel.h"

static void AdvanceBonfire(Player& player)
{
    for (uint32 tick = 0; tick < 40; ++tick)
        player.m_Events.Update(500);
}

int main(int argc, char** argv)
{
    try
    {
        InitializeAltheaGuardsNative(argc, argv);
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        auto info = sObjectMgr->GetGameObjectTemplate(245188);
        Require(info && info->type == GAMEOBJECT_TYPE_GOOBER && info->GetLockId() == 2474 &&
            info->goober.spell == 198513, "Actual bonfire type/lock/spell changed");
        Require(sSpellMgr->GetSpellInfo(198513)->CalcCastTime() == 3000, "Native Lighting Bonfire cast time changed");
        Require(sSpellMgr->GetSpellInfo(198513)->HasAttribute(SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS), "Matching26972 Lighting Bonfire must retain its authored no-LOS attribute");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        new BonfireInstanceFactory;
        new go_mystic_bonfire;
        sScriptMgr->SwapScriptContext(true);
#ifdef BONFIRE_INTERACTION_BASELINE
        uint32 const profiles = 6;
#elif defined(BONFIRE_LOS_BASELINE)
        uint32 const profiles = 14;
#else
        uint32 const profiles = 12;
#endif
        uint32 cases = 0;
        bool modelsLoaded = false;
        for (uint32 profile = 0; profile < profiles; ++profile)
            for (uint32 race : {RACE_HUMAN, RACE_ORC})
            {
                MapInstanced parent(1511, 300);
                InstanceMap map(1511, 300, 9987400 + race + 100 * profile, DIFFICULTY_N_SCENARIO, &parent);
                map.CreateInstanceData(nullptr);
                auto script = dynamic_cast<BonfireInstance*>(map.GetInstanceScript());
                Require(script, "Actual native fixture instance missing");
                auto scenario = sScenarioMgr->AddScenario(&map, 944);
                scenario->SetCurrentStep(0);
                auto session = Session();
                {
                    if (profile >= 12 && !modelsLoaded)
                    {
                        LoadGameObjectModelList(sWorld->GetDataPath());
                        modelsLoaded = true;
                    }
                    // Generic near-fire west-side geometry, independent of any
                    // persisted character row. The original controls had not
                    // loaded the GO collision-model list.
                    Position pose = profile >= 12 ? Position{3398.01f,1476.4f,58.8142f,0} : Position{3402.01f,1475.4f,58.5142f,0};
                    NativePlayer player(session, map, 901, pose);
                    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
                    player.SetQuestStatus(40043, QUEST_STATUS_INCOMPLETE);
                    player.SetQuestSlot(0, 40043);
                    auto fire = new GameObject;
                    Require(fire->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(), 245188, &map, 1,
                        {3402.01f,1477.4f,58.5142f,0}, G3D::Quat(), 0, GO_STATE_READY), "Native bonfire creation failed");
                    fire->*Access(BonfireDbTag{}) = 134117;
                    Require(map.AddToMap(fire), "Actual bonfire insertion failed");
                    if (profile >= 12)
                    {
                        Require(fire->m_model, "Native bonfire collision model must actually load");
                        Require(!player.IsWithinLOSInMap(fire), "Spatial regression requires a real blocked LOS");
                    }
                    auto count = [&]() {
                        auto rows = scenario->GetAchievementMgr().GetCriteriaProgressMap();
                        auto found = rows->find(45849);
                        return found == rows->end() ? uint64(0) : found->second.Counter;
                    };
                    auto use = [&]() {
                        WorldPackets::GameObject::GameObjectUse packet{WorldPacket(CMSG_GAME_OBJ_USE, 0)};
                        packet.Guid = fire->GetGUID();
                        session->HandleGameObjectUse(packet);
                    };
                    SpellCastResult result = SPELL_CAST_OK;
                    switch (profile)
                    {
                        case 0: result = player.CastSpell(fire, 198513, true); break;
                        case 1: result = player.CastSpell(fire, 198513, false); break;
                        case 2: fire->Use(&player); break;
                        case 3: result = player.CastSpell(fire, 198513, false); use(); break;
                        case 4: use(); result = player.CastSpell(fire, 198513, false); break;
                        case 5: use(); break;
                        case 6: use(); player.InterruptNonMeleeSpells(false, 198513); break;
                        case 7: player.SetQuestStatus(40043, QUEST_STATUS_NONE); use(); break;
                        case 8:
                        {
                            WorldPackets::GameObject::GameObjReportUse packet{WorldPacket(CMSG_GAME_OBJ_REPORT_USE, 0)};
                            packet.Guid = fire->GetGUID();
                            session->HandleGameobjectReportUse(packet);
                            break;
                        }
                        case 9: use(); player.SetQuestStatus(40043, QUEST_STATUS_NONE); break;
                        case 10: result = player.CastSpell(fire, 198513, false); fire->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE); break;
                        case 11: use(); fire->RemoveFromWorld(); break;
                        case 12:
                        {
                            result = player.CastSpell(fire, 198513, false);
                            WorldPackets::GameObject::GameObjReportUse packet{WorldPacket(CMSG_GAME_OBJ_REPORT_USE, 0)};
                            packet.Guid = fire->GetGUID();
                            session->HandleGameobjectReportUse(packet);
                            break;
                        }
                        case 13: use(); break;
                    }
#ifndef BONFIRE_INTERACTION_BASELINE
                    if (profile == 5)
                    {
                        Require(!count() && scenario->GetCurrentStep() == 0 && fire->GetGoState() == GO_STATE_READY,
                            "Starting a native cast must not award progress or light the fire");
                        Spell* pending = player.GetCurrentSpell(CURRENT_GENERIC_SPELL);
                        Require(pending && pending->m_spellInfo->Id == 198513 && pending->m_targets.GetGOTargetGUID() == fire->GetGUID(),
                            "Actual Use packet did not prepare the player's native bonfire cast");
                        use();
                        Require(player.GetCurrentSpell(CURRENT_GENERIC_SPELL) == pending, "Duplicate Use replaced a pending native cast");
                    }
#endif
                    AdvanceBonfire(player);
#ifdef BONFIRE_LOS_BASELINE
                    if (profile >= 12)
                    {
                        Require(!count() && scenario->GetCurrentStep() == 0 && fire->GetGoState() == GO_STATE_READY,
                            "Pinned extra-LOS guard no longer reproduces the client-context rejection");
                        std::cout << "COUNTERFACTUAL native blocked-LOS/model-present/no-LOS-spell profile=" << profile << " race=" << race
                            << " remains0/step0;not an ordinary passing encounter\n";
                    }
                    else
#endif
#ifdef BONFIRE_INTERACTION_BASELINE
                    if (profile >= 2)
                    {
                        Require(!count() && scenario->GetCurrentStep() == 0 && fire->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
                            "Pinned old native Use/IN_USE counterfactual no longer reproduces");
                        std::cout << "COUNTERFACTUAL profile=" << profile << " race=" << race
                            << " native Use leaves0 credit/step0/IN_USE;not an ordinary passing encounter\n";
                    }
                    else
#endif
                    {
                        bool const success = profile < 6 || profile == 10 || profile >= 12;
                        Require(count() == uint64(success) && scenario->GetCurrentStep() == uint32(success) &&
                            script->transitions == uint32(success), "Native bonfire progression outcome differs");
#ifndef BONFIRE_INTERACTION_BASELINE
                        Require(fire->GetGoState() == (success ? GO_STATE_ACTIVE : GO_STATE_READY),
                            "Fire presentation changed before actual native acknowledgement");
#endif
                        if (success)
                        {
                            use();
                            fire->AI()->GossipHello(&player, false);
                            AdvanceBonfire(player);
                            Require(count() == 1 && script->transitions == 1, "Duplicate packet/report re-awarded bonfire progress");
                        }
                        if (profile == 6 || profile == 9)
                        {
                            player.SetQuestStatus(40043, QUEST_STATUS_INCOMPLETE);
                            use();
                            AdvanceBonfire(player);
                            Require(count() == 1 && scenario->GetCurrentStep() == 1 && script->transitions == 1,
                                "Interrupted or abandoned pending cast did not permit a legitimate retry");
                        }
                        std::cout << "PASS native profile=" << profile << " race=" << race << " castResult=" << result
                            << " criterion=" << count() << " step=" << scenario->GetCurrentStep() << '\n';
                    }
                    fire->RemoveFromWorld();
                    fire->RemoveFromGrid();
                    fire->ResetMap();
                    delete fire;
                    map.RemoveAllObjectsInRemoveList();
                    CleanStories(map);
                }
                session->SetPlayer(nullptr);
                delete session;
                sScenarioMgr->RemoveScenario(map.GetInstanceId());
                ++cases;
            }
        CloseAltheaNative();
        std::cout << "PASS complete native bonfire interaction cases=" << cases
            << ";controlled encounter/event clock,not captured client packets or full scenario traversal\n";
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

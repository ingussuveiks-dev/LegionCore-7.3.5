#define AREA_DIALOGUE_SUPPORT_ONLY
#include "LegionAreaDialogueNative.cpp"
#include "WorldStateMgr.h"

struct RotationUpdateTag
{
    using Type = WorldQuestUpdate const* (QuestDataStoreMgr::*)(uint32, uint32, uint32);
    friend Type Access(RotationUpdateTag);
};
template struct FixtureAccess<RotationUpdateTag, &QuestDataStoreMgr::GetWorldQuestUpdate>;

struct RotationGenerateTag
{
    using Type = WorldQuest const* (QuestDataStoreMgr::*)(WorldQuestUpdate const*, WorldQuestTemplate const*);
    friend Type Access(RotationGenerateTag);
};
template struct FixtureAccess<RotationGenerateTag, &QuestDataStoreMgr::GenerateWorldQuest>;

struct RotationAdmissionTag
{
    using Type = bool (QuestDataStoreMgr::*)(WorldQuestTemplate const*, WorldQuestUpdate const*);
    friend Type Access(RotationAdmissionTag);
};
template struct FixtureAccess<RotationAdmissionTag, &QuestDataStoreMgr::CanBeActivate>;

struct RotationRemoveTaskTag
{
    using Type = void (QuestDataStoreMgr::*)(Quest const*);
    friend Type Access(RotationRemoveTaskTag);
};
template struct FixtureAccess<RotationRemoveTaskTag, &QuestDataStoreMgr::RemoveWorldQuestTask>;

struct RotationCallbacksTag
{
    using Type = FunctionProcessor Unit::*;
    friend Type Access(RotationCallbacksTag);
};
template struct FixtureAccess<RotationCallbacksTag, &Unit::m_Functions>;

struct RotationAchievementsTag
{
    using Type = std::vector<CompletedAchievementData*> AchievementMgr<Player>::*;
    friend Type Access(RotationAchievementsTag);
};
template struct FixtureAccess<RotationAchievementsTag, &AchievementMgr<Player>::_completedAchievementsArr>;

class RotationPlayer : public DungeonPlayer
{
public:
    RotationPlayer(Map& map, uint32 low, Position const& location) : DungeonPlayer(map, RACE_HUMAN, low)
    {
        RemoveFromGrid();
        Relocate(location);
        GetZoneAndAreaId(m_zoneId, m_areaId);
        map.LoadGrid(GetPositionX(), GetPositionY());
        map.AddToGrid(static_cast<Player*>(this), Cell(Trinity::ComputeCellCoord(GetPositionX(), GetPositionY())));
        Require(Sessions().emplace(low, WorldSessionPtr(GetSession(), [](WorldSession*) {})).second, "Test session key collision");
    }

    ~RotationPlayer() { Sessions().erase(GetGUID().GetCounter()); }

    void UnlockBrokenShore()
    {
        for (uint32 id : {41694u, 46734u})
        {
            uint32 bit = sDB2Manager.GetQuestUniqueBitFlag(id);
            Require(bit, "Native Broken Shore prerequisite bit is missing");
            SetRewardedQuest(id);
            SetQuestCompletedBit(bit, true);
        }
    }

    void UnlockAssaults()
    {
        static CompletedAchievementData unlocked;
        auto manager = GetAchievementMgr().operator->();
        (manager.operator->()->*Access(RotationAchievementsTag{})).at(11473) = &unlocked;
    }

    void PumpCallbacks(uint32 elapsed = 1) { (this->*Access(RotationCallbacksTag{})).Update(elapsed); }

private:
    static SessionMap& Sessions() { return const_cast<SessionMap&>(sWorld->GetAllSessions()); }
};

struct RotationCase
{
    uint32 QuestId;
    uint32 EventId;
    uint32 Conversation;
    uint32 Area;
    uint32 Timer;
    uint32 Variable;
    uint32 Value;
    uint32 SecondaryVariable;
};

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true);
        DisableMgr::LoadDisables();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sQuestDataStore->LoadQuests();

        std::unordered_map<uint32, std::vector<uint32>> outdoor{{1220, {}}};
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);

        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        ShadowDialogue("world_quest_update", "QuestID IN (45743, 45744, 46011, 46021, 46046, 46063, 46073, 46821)");
        ShadowDialogue("smart_scripts", "source_type=13 AND entryorguid IN (219, 230, 232)");
        ShadowDialogue("conditions", "(SourceTypeOrReferenceId=22 AND SourceId=13 AND SourceEntry IN (219, 230, 232)) OR "
            "(SourceTypeOrReferenceId=23 AND ((SourceGroup=7541 AND SourceEntry=141) OR (SourceGroup=7543 AND SourceEntry IN (126, 142))))");
        auto migration = std::getenv("LEGION_ROTATION_MIGRATION");
        Require(migration, "Rotation SQL path is missing");
        std::ifstream input(migration);
        Require(bool(input), "Rotation SQL is unavailable");
        std::string line, sql;
        while (std::getline(input, line))
            if (line.rfind("--", 0) != 0)
                sql += line + '\n';

        WorldDatabase.DirectExecute("DELETE FROM world_quest_update WHERE QuestID IN (46021, 46063, 46073)");
        WorldDatabase.DirectExecute("INSERT INTO world_quest_update (QuestID, Timer, VariableID, Value, AreaID) VALUES (46063, 1234, 777, 9, '123')");
        WorldDatabase.DirectExecute(sql.c_str());
        auto custom = WorldDatabase.Query("SELECT Timer, VariableID, Value, AreaID FROM world_quest_update WHERE QuestID=46063");
        Require(custom && custom->GetRowCount() == 1 && custom->Fetch()[0].GetUInt32() == 1234 &&
            custom->Fetch()[1].GetUInt32() == 777 && custom->Fetch()[2].GetUInt32() == 9 &&
            custom->Fetch()[3].GetString() == "123", "Migration overwrote local rotation policy");
        WorldDatabase.DirectExecute("DELETE FROM world_quest_update WHERE QuestID=46063");
        for (unsigned replay = 0; replay < 2; ++replay)
            WorldDatabase.DirectExecute(sql.c_str());
        auto count = WorldDatabase.Query("SELECT COUNT(*) FROM world_quest_update");
        Require(count && count->Fetch()[0].GetUInt32() == 8, "Repeated migration disturbed the local rotation cohort");

        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE native_rotation_seed LIKE world_quest");
        CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE world_quest LIKE native_rotation_seed");
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        sQuestDataStore->LoadWorldQuestTemplates();
        auto rotations = const_cast<WorldQuestMap*>(sQuestDataStore->GetWorldQuestMap());
        Require(rotations && rotations->empty(), "Fixture loaded persisted rotation data");

        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_SmartScripts();
        sScriptMgr->SwapScriptContext(true);
        sEventObjectDataStore->LoadEventObjectTemplates();
        sSmartScriptMgr->LoadSmartAIFromDB();
        sObjectMgr->LoadPhaseDefinitions();
        sConditionMgr->LoadConditions();
        sConversationDataStore->LoadConversationData();
        HashMapHolder<EventObject>::SetSize(1000);
        HashMapHolder<Conversation>::SetSize(20000);

        RotationCase const cases[] = {
            {46021, 219, 4603, 7651, 21600, 13005, 1, 13084},
            {46063, 230, 4429, 8584, 86400, 13114, 2, 0},
            {46073, 232, 4439, 8551, 86400, 12978, 3, 0}
        };
        for (RotationCase const& test : cases)
        {
            Quest const* quest = sQuestDataStore->GetQuestTemplate(test.QuestId);
            auto task = sQuestV2CliTaskStore.LookupEntry(test.QuestId);
            Require(quest && quest->IsWorld() && task, "Not a native world task");
            auto update = (sQuestDataStore->*Access(RotationUpdateTag{}))(test.QuestId, quest->QuestInfoID, 0);
            Require(update && update->worldQuest && update->Timer == test.Timer && update->VariableID == test.Variable &&
                update->VariableID1 == test.SecondaryVariable && update->Value == test.Value && !update->EventID &&
                update->Value1 == (test.SecondaryVariable ? 1u : 0u), "Loader changed the scoped rotation policy");
            auto areas = sDB2Manager.GetAreasForGroup(quest->AreaGroupID);
            Require(std::find(areas.begin(), areas.end(), test.Area) != areas.end(), "Event is outside its native area group");
            std::set<WorldQuestState> states{{test.Variable, test.Value}};
            if (test.SecondaryVariable)
                states.emplace(test.SecondaryVariable, 1);
            std::set<WorldQuestState> nativeStates;
            auto expression = sWorldStateExpressionStore.LookupEntry(task->WorldStateExpressionID);
            Require(expression, "Native world state expression missing");
            expression->Eval(nullptr, &nativeStates);
            Require(states == nativeStates, "Rotation data differ from the client's published states");

            auto spawn = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM eventobject WHERE id=%u AND map=1220", test.EventId);
            Require(spawn && spawn->GetRowCount() == 1, "Unexpected outdoor trigger spawn count");
            auto fields = spawn->Fetch();
            Position origin{fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
            Map map(1220, 300, 0, DIFFICULTY_NONE);
            RotationPlayer eligible(map, 101, origin);
            RotationPlayer locked(map, 102, origin);
            RotationPlayer late(map, 103, origin);
            Require(eligible.GetCurrentAreaID() == test.Area, "Actual client map resolves outside the authored task area");
            if (test.QuestId == 46021)
            {
                Require(update->worldQuest->PrimaryID == QUEST_INFO_LEGION_INVASION_WORLD_QUEST_WRAPPER, "Assault task lost its native wrapper gate");
                eligible.UnlockAssaults();
                late.UnlockAssaults();
                sQuestDataStore->WorldLegionInvasionZoneID = quest->QuestSortID;
            }
            else
            {
                eligible.UnlockBrokenShore();
                late.UnlockBrokenShore();
            }
            Require(!sConditionMgr->IsPlayerMeetingCondition(&locked, task->ConditionID), "Native unlock condition unexpectedly offered the task");
            eligible.GetPhaseMgr().NotifyStoresReloaded();
            if (test.QuestId == 46021)
                Require(!eligible.HasPhaseId(6788), "Inactive Thel'draz phase was preseeded");
            AreaDialogueEvent event(map, eligible, test.EventId);
            event.Update(1);
            Require(!event.ActivatedForPlayer(&eligible) && LiveConversations().empty(), "Inactive rotation consumed entry or played a conversation");
            Require((sQuestDataStore->*Access(RotationAdmissionTag{}))(update->worldQuest, update), "Scheduler rejected a compatible candidate");

            auto published = (sQuestDataStore->*Access(RotationGenerateTag{}))(update, update->worldQuest);
            Require(published && published->State == states, "Native scheduler did not publish the task states");
            for (auto const& state : states)
                Require(sWorldStateMgr.GetWorldStateValue(state.first) == uint32(state.second), "World state manager lost the published variant");
            for (uint32 area : areas)
                Require(sQuestDataStore->GetWorldQuestTask(area) && sQuestDataStore->GetWorldQuestTask(area)->count(quest), "Publication omitted a native area group member");

            eligible.PumpCallbacks();
            Require(eligible.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "Fixture moved or admitted the player before publication was queued");
            sQuestDataStore->QueueWorldQuestTaskRefreshes();
            Require(eligible.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "World thread admitted the task directly");
            eligible.PumpCallbacks();
            locked.PumpCallbacks();
            bool added = eligible.FindQuestSlot(test.QuestId) < MAX_QUEST_LOG_SIZE;
            std::cout << "ROTATION " << test.QuestId << " area=" << test.Area << " added=" << added
                      << " condition=" << sConditionMgr->IsPlayerMeetingCondition(&eligible, task->ConditionID) << std::endl;
            Require(added && locked.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "Stationary world task eligibility failed");
            eligible.PumpCallbacks(100);
            if (test.QuestId == 46021)
                Require(eligible.NeedPhaseRecalculate, "Native quest acceptance did not request a phase update");
            eligible.GetPhaseMgr().NotifyStoresReloaded();
            uint32 questPhase = test.QuestId == 46021 ? 6788 : test.QuestId == 46063 ? 7061 : 7147;
            Require(eligible.HasPhaseId(questPhase), "Accepted world task cannot see its native objective phase");
            event.Update(1);
            auto conversations = LiveConversations();
            Require(conversations.size() == 1 && conversations[0]->GetEntry() == test.Conversation &&
                conversations[0]->GetCasterGUID() == eligible.GetGUID(), "Stationary accepted task failed its personal introduction");
            ClearConversations(map);
            sQuestDataStore->QueueWorldQuestTaskRefreshes();
            eligible.PumpCallbacks();
            event.Update(1);
            Require(LiveConversations().empty() && eligible.GetQuestStatus(test.QuestId) == QUEST_STATUS_INCOMPLETE,
                "Repeated polling replayed the introduction or changed objective progress");

            (*rotations)[quest->QuestSortID][test.QuestId].ResetTime = GameTime::GetGameTime();
            late.PumpCallbacks();
            Require(late.FindQuestSlot(test.QuestId) == MAX_QUEST_LOG_SIZE, "An expired task was offered from a queued refresh");

            std::vector<uint32> alternatives;
            if (test.QuestId == 46063)
                alternatives = {46821, 46046};
            if (test.QuestId == 46073)
                alternatives = {45743, 45744};
            for (uint32 otherId : alternatives)
            {
                auto otherQuest = sQuestDataStore->GetQuestTemplate(otherId);
                Require(otherQuest, "Native phase competitor quest missing");
                auto alternative = (sQuestDataStore->*Access(RotationUpdateTag{}))(otherId, otherQuest->QuestInfoID, 0);
                Require(alternative, "Native phase competitor rotation missing");
                Require(!(sQuestDataStore->*Access(RotationAdmissionTag{}))(alternative->worldQuest, alternative),
                    "An alternative state was reused before the expired task was cleaned up");
                (*rotations)[quest->QuestSortID][test.QuestId].ResetTime += 600;
                Require(!(sQuestDataStore->*Access(RotationAdmissionTag{}))(alternative->worldQuest, alternative),
                    "An alternative could overwrite the active task's state or hide its objective phase");
                rotations->clear();
                auto other = (sQuestDataStore->*Access(RotationGenerateTag{}))(alternative, alternative->worldQuest);
                Require(other && !(sQuestDataStore->*Access(RotationAdmissionTag{}))(update->worldQuest, update),
                    "The restored task could overwrite an active alternative state or phase");
                (sQuestDataStore->*Access(RotationRemoveTaskTag{}))(alternative->quest);
                rotations->clear();
                auto original = (sQuestDataStore->*Access(RotationGenerateTag{}))(update, update->worldQuest);
                Require(original, "Native state could not resume after the competing rotation ended");
                (*rotations)[quest->QuestSortID][test.QuestId].ResetTime = GameTime::GetGameTime();
            }
            (sQuestDataStore->*Access(RotationRemoveTaskTag{}))(quest);
            rotations->clear();
            sQuestDataStore->WorldLegionInvasionZoneID = 0;
            sQuestDataStore->QueueWorldQuestTaskRefreshes();
        }

        std::cout << "PASS three native rotation loads, SQL replay/custom rows, client expressions and all area-group members, queued stationary offers and personal introductions, repeated poll, and shared-state exclusion\n";
        CharacterDatabase.Close();
        LoginDatabase.Close();
        WorldDatabase.Close();
        HotfixDatabase.Close();
        MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

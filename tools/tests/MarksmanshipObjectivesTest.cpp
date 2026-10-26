#include "ObjectiveHost.inc"
int main(int argc, char** argv)
{
    assert(argc == 3);
    auto before = Read(argv[1]), after = Read(argv[2]);
    auto baseline = Load(before); store.quest = &baseline;
    Player alias; alias.Credit(103490);
    assert(alias.GetQuestObjectiveData(40419, 103481) == 1);
    // Native save writes use the slot as key: six objectives collapse to five keys.
    auto aliasedSave = alias.Save();
    assert(aliasedSave.rows.size() == 5);
    auto fixed = Load(after); store.quest = &fixed;
    std::vector<uint32> expected{281309, 281978, 281310, 281311, 281980, 281574}, actual;
    for (auto const& objective : fixed.Objectives) actual.push_back(objective.ID);
    assert(actual == expected);
    auto wire = Wire(&fixed); assert(wire.size() == 54);
    for (size_t index = 0; index < 6; ++index)
    {
        assert(wire[index * 9] == expected[index]);
        assert(wire[index * 9 + 2] == fixed.Objectives[index].StorageIndex);
    }
    std::sort(after.begin(), after.end(), [](auto const& a, auto const& b) { return a[0].value < b[0].value; });
    unsigned permutations = 0;
    do {
        auto quest = Load(after); actual.clear();
        for (auto const& objective : quest.Objectives) actual.push_back(objective.ID);
        assert(actual == expected); ++permutations;
    } while (std::next_permutation(after.begin(), after.end(), [](auto const& a, auto const& b) { return a[0].value < b[0].value; }));
    assert(permutations == 720);
    std::vector<uint32> required{102128, 103481, 102129, 102131, 102548};
    // No optional pickup can satisfy the real pre-mission conversation.
    Player pickup; pickup.Credit(103490);
    assert(pickup.GetQuestObjectiveData(40419, 103490) == 1 && pickup.GetQuestObjectiveData(40419, 103481) == 0);
    for (uint32 credit : required) if (credit != 103481) pickup.Credit(credit);
    assert(!pickup.CanCompleteQuest(40419));
    auto saved = pickup.Save(); std::set<int64_t> keys;
    Query query;
    for (auto const& record : saved.rows)
    {
        assert(keys.insert(record.first).second);
        std::array<Field, 3> row; row[0].value = 40419; row[1].value = record.first; row[2].value = record.second;
        query.rows.push_back(row);
    }
    Player loaded; loaded._LoadQuestStatusObjectives({&query});
    assert(loaded.GetQuestObjectiveData(40419, 103481) == 0 && loaded.GetQuestObjectiveData(40419, 103490) == 1);
    assert(!loaded.CanCompleteQuest(40419));
    loaded.Credit(103481); assert(loaded.completions == 1);
    auto updates = loaded.updates; loaded.Credit(103481);
    assert(loaded.completions == 1 && loaded.updates == updates);
    // A native pre-mission conversation must not mark the optional bow as taken.
    Player start; start.Credit(103481);
    assert(start.GetQuestObjectiveData(40419, 103490) == 0);
    for (uint32 missing : required)
    {
        Player player; player.Credit(103490);
        for (uint32 credit : required) if (credit != missing) player.Credit(credit);
        assert(!player.CanCompleteQuest(40419));
    }
    Player withoutOptional;
    for (uint32 credit : required) withoutOptional.Credit(credit);
    assert(withoutOptional.completions == 1 && withoutOptional.GetQuestObjectiveData(40419, 103490) == 0);
    Player absent; absent.active = false; absent.Credit(103490); assert(absent.status.ObjectiveData.empty());
    Player raid; raid.raid = true; raid.Credit(103490); assert(raid.status.ObjectiveData.empty());
    Player rewarded; rewarded.m_RewardedQuests.insert(40419); assert(!rewarded.CanCompleteQuest(40419));
    Quest other; other.identity = 999;
    for (auto row : after) { row[1].value = 999; other.LoadQuestObjective(row.data()); }
    for (size_t index = 0; index < 6; ++index) assert(other.Objectives[index].ID == after[index][0].value);
    auto custom = fixed;
    auto extra = after.front(); extra[0].value = 999999; custom.LoadQuestObjective(extra.data());
    extra[0].value = 999998; custom.LoadQuestObjective(extra.data());
    assert(custom.Objectives[6].ID == 999999 && custom.Objectives[7].ID == 999998);
    std::cout << "PASS native40419 credit/get/set/completion/save/load/query/serialization: pickup and conversation independent, five required credits, optional flags retained,720 load permutations and custom/other-quest order. No character data, live actors/group dispatch or client traversal.\n";
}

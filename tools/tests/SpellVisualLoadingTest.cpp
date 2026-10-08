#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using uint32 = uint32_t;
using int32 = int32_t;
struct Field
{
    int value = 0;
    int32 GetInt32() const { return value; }
    uint16_t GetUInt16() const { return uint16_t(value); }
    uint8_t GetUInt8() const { return uint8_t(value); }
    int8_t GetInt8() const { return int8_t(value); }
    float GetFloat() const { return float(value); }
};
struct Rows
{
    std::vector<std::vector<Field>> rows;
    size_t index = 0;
    Field* Fetch() { return rows[index].data(); }
    bool NextRow() { return ++index < rows.size(); }
};
using QueryResult = std::shared_ptr<Rows>;
struct Database
{
    std::map<std::string, std::vector<std::vector<Field>>> tables;
    QueryResult Query(char const* sql)
    {
        auto name = std::string(sql).substr(std::string(sql).find(" FROM ") + 6);
        auto const& rows = tables[name];
        return rows.empty() ? nullptr : std::make_shared<Rows>(Rows{rows});
    }
} WorldDatabase;
struct Position { void Relocate(float, float, float) {} };
struct SpellVisual
{
    int32 spellId, SpellVisualID;
    uint16_t MissReason, ReflectStatus;
    float TravelSpeed;
    bool SpeedAsTime, HasPosition;
    uint8_t type;
};
struct SpellVisualPlayOrphan
{
    int32 spellId, SpellVisualID;
    float TravelSpeed, UnkFloat;
    bool SpeedAsTime;
    int8_t type;
    Position SourceOrientation;
};
struct SpellVisualKit { int32 spellId, KitType, KitRecID, Duration; };
struct SpellInfo {};
struct SpellMgr
{
    std::map<int32, std::vector<SpellVisual>> mSpellVisualMap;
    std::map<int32, std::vector<SpellVisualPlayOrphan>> mSpellVisualPlayOrphanMap;
    std::map<int32, std::vector<SpellVisualKit>> mSpellVisualKitMap;
    SpellInfo const* GetSpellInfo(int32 id) const
    {
        static SpellInfo valid;
        return id == 999 ? nullptr : &valid;
    }
    void LoadSpellVisual();
};
uint32 getMSTime() { return 0; }
uint32 GetMSTimeDiffToNow(uint32) { return 0; }
template <typename... T> void TestLog(T const&...) {}
#define TC_LOG_INFO(...) TestLog(__VA_ARGS__)
#define TC_LOG_ERROR(...) TestLog(__VA_ARGS__)
#include "SpellVisualLoading.inc"

int main()
{
    SpellMgr mgr;
    WorldDatabase.tables["spell_visual_kit"] = {{{181765}, {0}, {54169}, {2}}};
    mgr.LoadSpellVisual();
    assert(mgr.mSpellVisualKitMap.at(181765).at(0).KitRecID == 54169);
    WorldDatabase.tables["spell_visual_play_orphan"] = {{{193356}, {56834}, {}, {}, {}, {}, {}, {}, {}}};
    mgr.LoadSpellVisual();
    assert(mgr.mSpellVisualPlayOrphanMap.at(193356).size() == 1);
    assert(mgr.mSpellVisualKitMap.at(181765).size() == 1);
    WorldDatabase.tables["spell_visual"] = {{{100}, {45080}, {}, {}, {}, {}, {}, {}},
                                            {{999}, {45080}, {}, {}, {}, {}, {}, {}}};
    WorldDatabase.tables["spell_visual_play_orphan"].clear();
    mgr.LoadSpellVisual();
    assert(mgr.mSpellVisualMap.size() == 1);
    assert(mgr.mSpellVisualKitMap.at(181765).size() == 1);
    assert(mgr.mSpellVisualPlayOrphanMap.empty());
    WorldDatabase.tables.clear();
    mgr.LoadSpellVisual();
    assert(mgr.mSpellVisualMap.empty());
    assert(mgr.mSpellVisualPlayOrphanMap.empty());
    assert(mgr.mSpellVisualKitMap.empty());
    std::cout << "Spell visual loading: independent optional tables, reload and invalid spell checks passed\n";
}

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
using uint8 = uint8_t;
using uint32 = uint32_t;
using int32 = int32_t;
#ifndef ARTIFACT_MODEL_ENTRY
#define ARTIFACT_MODEL_ENTRY 100310
#endif
#ifndef ARTIFACT_MODEL_DISPLAY
#define ARTIFACT_MODEL_DISPLAY 66609
#endif
uint32 randomModel = 0;
uint32 urand(uint32 minimum, uint32 maximum) { assert(maximum >= minimum); return minimum + randomModel % (maximum - minimum + 1); }
constexpr uint32 CREATURE_FLAG_EXTRA_TRIGGER = 128, UNIT_FIELD_FLAGS_2 = 0, UNIT_FLAG2_MIRROR_IMAGE = 0;
int32 HacksMorphesOne[31] = {}, HacksMorphesTwo[31] = {};
struct CreatureModelInfo { bool is_trigger; uint8 gender = 0; uint32 hostileId = 0; };
struct CreatureModelDataEntry { int32 FileDataID; };
struct CreatureDisplayInfoEntry { uint32 ModelID; };
struct ModelStore {
    CreatureModelDataEntry visible{118355}, trigger{124640};
    CreatureModelDataEntry const* LookupEntry(uint32 id) const { return id == 53 ? &visible : id == 3923 ? &trigger : nullptr; }
} sCreatureModelDataStore;
CreatureModelInfo Classify(uint32 model) {
    CreatureModelInfo modelInfo{};
    CreatureDisplayInfoEntry display{model};
    auto creatureDisplay = &display;
#include "Classify.inc"
    return modelInfo;
}
struct CreatureTemplate {
    uint32 Entry = ARTIFACT_MODEL_ENTRY, flags_extra = 0;
    int32 Modelid[4] = {33225, ARTIFACT_MODEL_DISPLAY, 0, 0};
    int32 GetRandomValidModelId() const;
    int32 GetFirstValidModelId() const;
    uint32 GetFirstInvisibleModel() const;
    uint32 GetFirstVisibleModel() const;
};
struct CreatureData { uint32 displayid; };
struct ObjectMgr {
    std::map<uint32, CreatureModelInfo> models{{33225, Classify(53)}, {ARTIFACT_MODEL_DISPLAY, Classify(3923)}};
    CreatureModelInfo const* GetCreatureModelInfo(uint32 id) { auto found = models.find(id); return found == models.end() ? nullptr : &found->second; }
    uint32 GetCreatureDisplay(int32 id) const { return id; }
    CreatureTemplate const* GetCreatureTemplate(uint32) const { return nullptr; }
    CreatureModelInfo const* GetCreatureModelRandomGender(uint32* id) { return GetCreatureModelInfo(*id); }
    static int32 ChooseDisplayId(uint32, CreatureTemplate const*, CreatureData const*);
} manager;
auto* sObjectMgr = &manager;
#define TC_LOG_ERROR(...) assert(false)
#include "GetRandomValidModelId.inc"
#include "GetFirstValidModelId.inc"
#include "GetFirstInvisibleModel.inc"
#include "GetFirstVisibleModel.inc"
#include "ChooseDisplayId.inc"
struct Player { bool gm = false; bool isGameMaster() const { return gm; } };
enum { MAX_SPELL_EFFECTS = 1, SPELL_AURA_TRANSFORM = 56 };
struct SpellEffect { uint32 MiscValue = 0; bool IsAura(int) const { return false; } };
struct SpellInfo { uint32 EffectMask = 0; SpellEffect* Effects[MAX_SPELL_EFFECTS] = {}; };
struct SpellManager { SpellInfo const* GetSpellInfo(uint32) const { return nullptr; } } spellManager;
auto* sSpellMgr = &spellManager;
struct Buffer { uint32 display = 0; Buffer& operator<<(uint32 value) { display = value; return *this; } };
struct Creature {
    CreatureTemplate* definition;
    uint32 outfit = 0, display = 0, native = 0;
    uint8 gender = 0;
    uint32 m_uint32Values[1] = {};
    void SetOutfit(uint32 value) { outfit = value; }
    uint32 GetOutfit() const { return outfit; }
    bool IsMirrorImage() const { return false; }
    void RemoveFlag(uint32, uint32) {}
    void SetDisplayId(uint32 value) { display = value; m_uint32Values[0] = value; }
    void SetNativeDisplayId(uint32 value) { native = value; }
    void SetGender(uint8 value) { gender = value; }
    CreatureTemplate const* GetCreatureTemplate() const { return definition; }
    uint32 getTransForm() const { return 0; }
    bool IsHostileTo(Player const*) const { return false; }
    bool InitModel(CreatureData const* data = nullptr) {
        auto cinfo = definition;
        uint32 entry = cinfo->Entry;
        (void)entry;
#include "InitEntryModel.inc"
        return true;
    }
    uint32 ClientDisplay(Player const* target) {
        Creature* creature = this;
        uint32 index = 0;
        Buffer buffer;
        auto* data = &buffer;
#include "ClientDisplay.inc"
        return buffer.display;
    }
};
int main(int argc, char** argv) {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    CreatureTemplate definition;
    assert(!Classify(53).is_trigger && Classify(3923).is_trigger && !Classify(0).is_trigger);
    definition.flags_extra = static_cast<uint32>(std::stoul(argv[1]));
    Creature actor{&definition};
    Player normal, gm{true};
    for (randomModel = 0; randomModel != 2; ++randomModel) {
        assert(actor.InitModel());
        assert(actor.display == ARTIFACT_MODEL_DISPLAY && actor.native == ARTIFACT_MODEL_DISPLAY);
        assert(actor.ClientDisplay(&normal) == ARTIFACT_MODEL_DISPLAY);
        assert(actor.ClientDisplay(&gm) == 33225);
    }
    CreatureTemplate unrelated;
    unrelated.Entry = 99999;
    actor.definition = &unrelated;
    for (randomModel = 0; randomModel != 2; ++randomModel) {
        assert(actor.InitModel());
        assert(actor.display == uint32(unrelated.Modelid[randomModel]));
        assert(actor.ClientDisplay(&normal) == actor.display && actor.ClientDisplay(&gm) == actor.display);
    }
    CreatureData overrideModel{33225};
    actor.definition = &definition;
    assert(actor.InitModel(&overrideModel) && actor.display == 33225);
    std::cout << "PASS: production selection, InitEntry and client display field retain native artifact display " << ARTIFACT_MODEL_DISPLAY
        << " for normal players, GM debug model and unrelated model/override behavior\n";
}

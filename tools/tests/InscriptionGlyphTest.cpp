#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using uint8 = uint8_t;
using uint32 = uint32_t;
using SpellEffIndex = int;
enum { CLASS_WARRIOR = 1, CLASS_PALADIN, CLASS_HUNTER, CLASS_ROGUE, CLASS_PRIEST,
    CLASS_DEATH_KNIGHT, CLASS_SHAMAN, CLASS_MAGE, CLASS_WARLOCK, CLASS_MONK, CLASS_DRUID, CLASS_DEMON_HUNTER };
constexpr int SKILL_INSCRIPTION = 773, EFFECT_ALL = 255, EFFECT_0 = 0;
constexpr int SPELL_EFFECT_TRIGGER_SPELL = 64, SPELL_EFFECT_DUMMY = 3;
struct SpellInfo {};
struct SpellManager
{
    std::set<uint32> available;
    SpellInfo info;
    SpellInfo* GetSpellInfo(uint32 id) { return available.count(id) ? &info : nullptr; }
} manager;
auto sSpellMgr = &manager;
struct Player
{
    uint8 playerClass = CLASS_PALADIN;
    bool inscription = true, isPlayer = true;
    std::set<uint32> known, quests;
    std::vector<uint32> learned, casts;
    std::function<void()> glyphReward;
    Player* ToPlayer() { return isPlayer ? this : nullptr; }
    bool HasSkill(int id) { return inscription && id == SKILL_INSCRIPTION; }
    uint8 getClass() { return playerClass; }
    bool HasSpell(uint32 id) { return known.count(id) != 0; }
    bool GetQuestRewardStatus(uint32 id) { return quests.count(id) != 0; }
    void learnSpell(uint32 id, bool) { known.insert(id); learned.push_back(id); }
    void CastSpell(Player*, uint32 id, bool) { casts.push_back(id); if (id == 192962) glyphReward(); }
};
struct EffectHook
{
    int index, type;
    std::function<void(int)> handler;
};
struct Hooks : std::vector<EffectHook>
{
    void operator+=(EffectHook hook) { push_back(hook); }
    void Run(int index, int type)
    {
        for (auto& hook : *this)
            if ((hook.index == EFFECT_ALL || hook.index == index) && hook.type == type)
                hook.handler(index);
    }
};
struct CastHooks : std::vector<std::function<void()>>
{
    void operator+=(std::function<void()> hook) { push_back(hook); }
    void Run() { for (auto& hook : *this) hook(); }
};
struct SpellScript
{
    Player* caster = nullptr;
    Hooks OnEffectLaunch, OnEffectLaunchTarget, OnEffectHit;
    CastHooks AfterCast;
    std::set<int> prevented;
    virtual bool Validate(SpellInfo const*) { return true; }
    virtual void Register() = 0;
    Player* GetCaster() { return caster; }
    void PreventHitDefaultEffect(int index) { prevented.insert(index); }
};
#define PrepareSpellScript(name) public:
#define SpellEffectFn(method, index, type) EffectHook{index, type, [this](int i) { method(i); }}
#define SpellCastFn(method) [this]() { method(); }
#include "InscriptionClassGlyph.inc"
;
#include "InscriptionRelearn.inc"
;
void Check(bool condition, char const* message) { if (!condition) throw std::runtime_error(message); }
int main()
{
    // Expectations are pinned independently of the production selector.
    uint32 const recipes[] = {225560, 192846, 192845, 192841, 192838, 192848,
        192844, 192840, 192839, 192843, 192842, 225528};
    manager.available.insert(std::begin(recipes), std::end(recipes));
    for (uint8 playerClass = 1; playerClass <= 12; ++playerClass)
    {
        Player player;
        player.playerClass = playerClass;
        spell_gen_inscription_class_glyph reward;
        reward.caster = &player;
        reward.Register();
        Check(reward.Validate(nullptr), "Available recipes must validate");
        for (Hooks* phase : {&reward.OnEffectLaunch, &reward.OnEffectLaunchTarget})
        {
            reward.prevented.clear();
            for (int i = 0; i < 12; ++i) phase->Run(i, SPELL_EFFECT_TRIGGER_SPELL);
            Check(reward.prevented.size() == 12, "Every missing trigger must be suppressed in both launch phases");
        }
        Check(player.known.empty(), "Launch effects must not craft glyphs or teach twelve recipes");
        reward.AfterCast.Run();
        Check(player.known == std::set<uint32>{recipes[playerClass - 1]}, "Reward must teach only the selected class recipe");
        Check(player.casts.empty(), "Crafting recipe must be learned, not cast to create a free glyph");
        reward.AfterCast.Run();
        Check(player.learned.size() == 1, "Recasting must not duplicate learning");

        player.known.clear(); // Profession removal is covered by Test-ProfessionRegression.
        player.inscription = false;
        reward.AfterCast.Run();
        Check(player.known.empty(), "No profession: reward must not restore recipes");
        player.inscription = true;
        spell_gen_relearn_inscription_quests relearn;
        relearn.caster = &player;
        relearn.Register();
        player.glyphReward = [&]() { reward.AfterCast.Run(); };
        relearn.OnEffectHit.Run(0, SPELL_EFFECT_DUMMY);
        Check(player.known.empty(), "Unfinished quest must not grant the glyph through relearning");
        player.quests.insert(39931);
        relearn.OnEffectHit.Run(0, SPELL_EFFECT_DUMMY);
        Check(player.known == std::set<uint32>{recipes[playerClass - 1]}, "Completed quest must restore the same recipe after relearning");
        manager.available.erase(recipes[playerClass - 1]);
        Check(!reward.Validate(nullptr), "Missing recipe data must fail validation");
        manager.available.insert(recipes[playerClass - 1]);
    }
    for (uint8 invalidClass : {uint8(0), uint8(13), uint8(255)})
    {
        Player player;
        player.playerClass = invalidClass;
        spell_gen_inscription_class_glyph reward;
        reward.caster = &player;
        reward.Register();
        reward.AfterCast.Run();
        Check(player.known.empty(), "Unsupported class must not receive another class's recipe");
        player.playerClass = CLASS_PALADIN;
        player.isPlayer = false;
        reward.AfterCast.Run();
        Check(player.known.empty(), "Non-player caster must not learn a recipe");
    }
    std::cout << "PASS: 12 classes, both trigger phases, idempotence, profession and quest gates, relearning and validation\n";
}

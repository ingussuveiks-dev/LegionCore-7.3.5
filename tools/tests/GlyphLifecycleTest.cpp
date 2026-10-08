#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using uint32 = uint32_t;
using uint16 = uint16_t;
using SpellEffIndex = int;
enum { SPELL_CAST_OK, SPELL_FAILED_GLYPH_NO_SPEC, SPELL_FAILED_NOT_KNOWN,
    SPELL_FAILED_CUSTOM_ERROR, SPELL_FAILED_INVALID_GLYPH, SPELL_FAILED_GLYPH_INVALID_SPEC,
    SPELL_FAILED_UNIQUE_GLYPH, SPELL_FAILED_GLYPH_EXCLUSIVE_CATEGORY };
constexpr int SPELL_EFFECT_APPLY_GLYPH = 74, SPELL_EFFECT_HANDLE_HIT = 1;
constexpr int SPELL_CUSTOM_ERROR_CANT_DO_THAT_WHILE_MYTHIC_KEYSTONE_IS_ACTIVE = 1;
constexpr int PLAYER_FIELD_CURRENT_SPEC_ID = 1;
struct GlyphPropertiesEntry { uint32 SpellID, GlyphExclusiveCategoryID; };
struct GlyphStore : std::map<uint32, GlyphPropertiesEntry>
{
    GlyphPropertiesEntry const* LookupEntry(uint32 id) { auto it = find(id); return it == end() ? nullptr : &it->second; }
    GlyphPropertiesEntry const* AssertEntry(uint32 id) { return &at(id); }
} sGlyphPropertiesStore;
struct DB2Manager
{
    std::map<uint32, std::vector<uint32>> binds, specs;
    std::vector<uint32> const* GetGlyphBindableSpells(uint32 id) { auto it = binds.find(id); return it == binds.end() ? nullptr : &it->second; }
    std::vector<uint32> const* GetGlyphRequiredSpecs(uint32 id) { auto it = specs.find(id); return it == specs.end() ? nullptr : &it->second; }
} sDB2Manager;
namespace WorldPackets { namespace Talent {
struct ActiveGlyphs
{
    std::vector<std::pair<uint32, uint16>> Glyphs;
    bool IsFullUpdate = false;
    ActiveGlyphs* Write() { return this; }
};
}}
struct Map { bool challenge = false; bool isChallenge() { return challenge; } };
struct Player
{
    bool isPlayer = true;
    uint32 active = 0, spec = 70;
    Map map;
    std::map<uint32, std::vector<uint32>> glyphs;
    std::set<uint32> known, auras;
    std::vector<uint32> removed, applied;
    std::vector<WorldPackets::Talent::ActiveGlyphs> packets;
    bool IsPlayer() { return isPlayer; }
    Player* ToPlayer() { return isPlayer ? this : nullptr; }
    bool HasSpell(uint32 id) { return known.count(id) != 0; }
    uint32 GetUInt32Value(int) { return spec; }
    Map* GetMap() { return &map; }
    uint32 GetActiveTalentGroup() { return active; }
    std::vector<uint32>& GetGlyphs(uint32 group) { return glyphs[group]; }
    void RemoveAurasDueToSpell(uint32 id) { auras.erase(id); removed.push_back(id); }
    void CastSpell(Player*, uint32 id, bool) { auras.insert(id); applied.push_back(id); }
    void SendDirectMessage(WorldPackets::Talent::ActiveGlyphs* packet) { packets.push_back(*packet); }
};
struct SpellEffect { uint32 MiscValue = 0; };
struct SpellInfo { SpellEffect effect; SpellEffect* GetEffect(int, int) { return &effect; } };
struct Spell
{
    Player* m_caster;
    SpellInfo* m_spellInfo;
    uint32 m_miscData[1];
    int m_diffMode = 0, m_customError = 0, effectHandleMode = SPELL_EFFECT_HANDLE_HIT;
    int CheckGlyph()
    {
        int i = 0;
        switch (SPELL_EFFECT_APPLY_GLYPH)
        {
#include "GlyphCheck.inc"
        }
        return SPELL_CAST_OK;
    }
    void EffectApplyGlyph(SpellEffIndex);
};
#include "GlyphApply.inc"
void Check(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
int main()
{
    try
    {
        // Actual 26972 category-zero examples: independent paladin spells.
        sGlyphPropertiesStore[190] = {89401, 0};
        sGlyphPropertiesStore[454] = {57979, 0};
        sDB2Manager.binds[190] = {32223};
        sDB2Manager.binds[454] = {31842, 31884};
        // Independent fixture IDs exercise nonzero category exclusion/replacement.
        sGlyphPropertiesStore[2000] = {90000, 7};
        sGlyphPropertiesStore[2001] = {90001, 7};
        sDB2Manager.binds[2000] = {100};
        sDB2Manager.binds[2001] = {101};
        Player player;
        player.known = {32223, 31884, 100, 101};
        player.glyphs[0] = {190};
        player.glyphs[1] = {2000};
        player.auras = {89401};
        SpellInfo info{{454}};
        Spell spell{&player, &info, {31884}};
        Check(spell.CheckGlyph() == SPELL_CAST_OK, "Independent category-zero glyphs must coexist");
        spell.EffectApplyGlyph(0);
        Check(player.glyphs[0] == std::vector<uint32>({190,454}), "Adding glyph must preserve independent glyph");
        Check(player.auras.count(89401) && player.auras.count(57979), "Both cosmetic auras must remain");
        Check(player.glyphs[1] == std::vector<uint32>({2000}), "Inactive specialization must remain unchanged");
        Check(!player.packets.back().IsFullUpdate && player.packets.back().Glyphs[0].second == 454, "Client must receive added glyph");
        Check(spell.CheckGlyph() == SPELL_CAST_OK, "Reapplying to same spell is a replacement");
        spell.EffectApplyGlyph(0);
        Check(player.glyphs[0].size() == 2, "Reapplication must not duplicate glyph storage");
        info.effect.MiscValue = 0;
        Check(spell.CheckGlyph() == SPELL_CAST_OK, "Removal must accept glyph ID zero");
        spell.EffectApplyGlyph(0);
        Check(player.glyphs[0] == std::vector<uint32>({190}) && !player.auras.count(57979), "Removal must erase glyph and old aura");
        Check(player.auras.count(89401), "Removal must preserve unrelated aura");
        Check(player.packets.back().Glyphs[0] == std::make_pair(uint32(31884), uint16(0)), "Client must receive glyph removal");
        player.glyphs[0] = {2000};
        info.effect.MiscValue = 2001;
        spell.m_miscData[0] = 101;
        Check(spell.CheckGlyph() == SPELL_FAILED_GLYPH_EXCLUSIVE_CATEGORY, "Real nonzero category conflicts must remain blocked");
        sDB2Manager.binds[2001] = {100};
        spell.m_miscData[0] = 100;
        Check(spell.CheckGlyph() == SPELL_CAST_OK, "Same-category replacement on same spell must work");
        player.auras = {90000};
        spell.EffectApplyGlyph(0);
        Check(player.glyphs[0] == std::vector<uint32>({2001}) && player.auras == std::set<uint32>({90001}), "Replacement must remove old aura before adding new one");
        sDB2Manager.specs[2001] = {71};
        Check(spell.CheckGlyph() == SPELL_FAILED_GLYPH_INVALID_SPEC, "Wrong specialization must fail");
        player.spec = 0;
        Check(spell.CheckGlyph() == SPELL_FAILED_GLYPH_NO_SPEC, "Missing specialization must fail");
        player.spec = 71;
        Check(spell.CheckGlyph() == SPELL_CAST_OK, "Required specialization must pass");
        spell.m_miscData[0] = 101;
        Check(spell.CheckGlyph() == SPELL_FAILED_INVALID_GLYPH, "Unbindable target must fail");
        spell.m_miscData[0] = 999;
        Check(spell.CheckGlyph() == SPELL_FAILED_NOT_KNOWN, "Unknown target spell must fail");
        spell.m_miscData[0] = 100;
        player.map.challenge = true;
        Check(spell.CheckGlyph() == SPELL_FAILED_CUSTOM_ERROR, "Active challenge restriction must remain");
        player.map.challenge = false;
        info.effect.MiscValue = 9999;
        Check(spell.CheckGlyph() == SPELL_FAILED_INVALID_GLYPH, "Unknown glyph data must fail");
        player.isPlayer = false;
        Check(spell.CheckGlyph() == SPELL_FAILED_GLYPH_NO_SPEC, "Non-player caster must fail");
        std::cout << "PASS: glyph coexistence, apply/replace/remove, spec isolation and cast restrictions\n";
    }
    catch (std::exception const& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}

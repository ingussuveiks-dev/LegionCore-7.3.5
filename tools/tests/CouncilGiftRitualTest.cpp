#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include "CouncilGiftJumpFixture.h"

using uint32 = unsigned;
enum AreaTriggerActionMoment { AT_ACTION_MOMENT_ENTER = 1 };
constexpr uint32 CouncilGiftButtonAura = 234667;
constexpr uint32 PlayerGuid = 73;

class Creature;
class Player
{
public:
    bool eligible = true, near = true, pending = false, button = false, busy = false;
    unsigned map = 1220, removals = 0;
    std::list<Creature*> nearby;

    bool HasPendingTeleport() const { return pending; }
    unsigned GetMapId() const { return map; }
    unsigned GetGUID() const { return PlayerGuid; }
    bool IsWithinDist3d(float, float, float, float, bool) const { return near; }
    bool HasAura(unsigned id) const { return id == CouncilGiftButtonAura ? button : busy; }
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& result, unsigned entry, float range)
    {
        assert(entry == 90418 && range == 25.f);
        result = nearby;
    }
    void RemoveAurasDueToSpell(unsigned id, unsigned caster)
    {
        assert(id == CouncilGiftButtonAura && caster == GetGUID());
        if (button)
            ++removals;
        button = false;
    }
};

class AreaTrigger
{
public:
    unsigned entry = 13567, entries = 0;
    bool world = true, inside = true, member = true;
    int duration = 30000;
    unsigned GetCustomEntry() const { return entry; }
    bool IsInWorld() const { return world; }
    int GetDuration() const { return duration; }
    bool IsInArea(Player*) const { return inside; }
    bool IsUnitAffected(unsigned guid) const { return member && guid == PlayerGuid; }
    void AffectUnit(Player* player, AreaTriggerActionMoment moment)
    {
        assert(moment == AT_ACTION_MOMENT_ENTER);
        ++entries;
        player->button = true;
    }
};

class Creature
{
public:
    unsigned spawn = 377268;
    bool alive = true, samePhase = true;
    std::list<AreaTrigger*> circles;
    unsigned GetDBTableGUIDLow() const { return spawn; }
    bool IsAlive() const { return alive; }
    bool InSamePhase(Player*) const { return samePhase; }
    void GetAreaObjectList(std::list<AreaTrigger*>& result, unsigned spell)
    {
        assert(spell == 0);
        result = circles;
    }
};

bool CanReceiveCouncilArtifactGift(Player& player) { return player.eligible; }
struct SpellManager
{
    SpellTargetPosition position;
    SpellTargetPosition const* GetSpellTargetPosition(unsigned spell) const
    {
        assert(spell == CouncilArtifactGift::Acquire);
        return &position;
    }
} spellManager;
auto sSpellMgr = &spellManager;

#include "Ritual.inc"

int main()
{
    using namespace CouncilArtifactGift;
    Player player;
    Creature modera;
    Creature otherPhase;
    otherPhase.spawn = 272737;
    AreaTrigger circle;
    player.nearby = {&otherPhase, &modera};
    modera.circles = {&circle};

    player.button = true;
    assert(!MaintainRitual(player) && player.button && !player.removals && !circle.entries);
    RemoveRecoveredButton(player);
    assert(!player.button && player.removals == 1);
    player.pending = true;
    assert(!MaintainRitual(player) && !circle.entries);
    player.pending = false;
    circle.member = false;
    assert(!MaintainRitual(player) && !circle.entries);
    circle.member = true;
    circle.inside = false;
    assert(!MaintainRitual(player) && !circle.entries);
    circle.inside = true;
    assert(!MaintainRitual(player) && player.button && circle.entries == 1);
    assert(!MaintainRitual(player) && circle.entries == 1);
    circle.duration = 0;
    assert(MaintainRitual(player) && !player.button && player.removals == 2);
    modera.samePhase = false;
    assert(!MaintainRitual(player));
    modera.samePhase = true;
    player.eligible = false;
    assert(!MaintainRitual(player));
    player.eligible = true;
    player.near = false;
    assert(!MaintainRitual(player));
    std::cout << "PASS Council native field admission, exact Modera, inside/affected guards, aura removal and expired-circle retry\n";
}

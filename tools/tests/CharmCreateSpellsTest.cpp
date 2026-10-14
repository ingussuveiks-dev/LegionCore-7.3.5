#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>
using uint32 = uint32_t;
enum ActiveStates { ACT_DISABLED, ACT_PASSIVE, ACT_ENABLED };
constexpr int SPELL_ATTR0_CASTABLE_WHILE_DEAD = 1, UNIT_MASK_CONTROLABLE_GUARDIAN = 1;
struct SpellInfo {
    bool passive=false, autocast=true, explicitTarget=true;
    bool HasAttribute(int) const { return false; }
    bool IsPassive() const { return passive; }
    bool IsAutocastable() const { return autocast; }
    bool NeedsExplicitUnitTarget() const { return explicitTarget; }
};
struct SpellMgr { SpellInfo spell; SpellInfo const* GetSpellInfo(uint32 id) { return id ? &spell : nullptr; } } manager;
auto sSpellMgr = &manager;
struct Map { bool instance=true; bool Instanceable() const { return instance; } } instanceMap;
struct Creature;
struct Unit {
    bool player=false, guardian=false; Unit* owner=nullptr; int passiveCasts=0;
    bool IsPlayer() const { return player; }
    Creature* ToCreature(); Unit* GetOwner() const { return owner; }
    bool HasUnitTypeMask(int) const { return guardian; }
    void CastSpell(Unit*, SpellInfo const*, bool) { ++passiveCasts; }
};
struct Creature : Unit {
    uint32 level=100, m_templateSpells[4]={123,0,0,0}; Map* map=&instanceMap;
    uint32 getLevel() const { return level; } Map* GetMap() const { return map; }
};
Creature* Unit::ToCreature() { return player ? nullptr : static_cast<Creature*>(this); }
struct Entry { uint32 spell=0; ActiveStates state=ACT_DISABLED; void SetActionAndType(uint32 s, ActiveStates t) { spell=s;state=t; } };
struct CharmInfo {
    Unit* m_unit; Entry m_charmspells[4]; int empty=0,pet=0,added=0,toggled=0;
    explicit CharmInfo(Unit* u):m_unit(u){}
    void InitEmptyActionBar(){++empty;} void InitPetActionBar(){++pet;}
    void ToggleCreatureAutocast(SpellInfo const*,bool){++toggled;}
    void AddSpellToActionBar(SpellInfo const*,ActiveStates){++added;}
    void InitCharmCreateSpells();
};
#include "CharmCreateSpells.inc"
int main() {
    // The crash: an ordinary level-100 instance NPC has a charmer but no owner.
    Creature wild; CharmInfo enslaved(&wild); enslaved.InitCharmCreateSpells();
    assert(enslaved.empty==1 && enslaved.pet==0);
    Unit player; player.player=true;
    Creature owned; owned.owner=&player; CharmInfo pet(&owned); pet.InitCharmCreateSpells();
    assert(pet.empty==0 && pet.pet==1 && pet.added==1 && pet.toggled==1);
    Creature guardian; guardian.guardian=true; CharmInfo guard(&guardian); guard.InitCharmCreateSpells();
    assert(guard.empty==0 && guard.pet==1);
    Unit npcOwner; owned.owner=&npcOwner; CharmInfo npc(&owned); npc.InitCharmCreateSpells(); assert(npc.empty==1);
    wild.level=69; CharmInfo low(&wild); low.InitCharmCreateSpells(); assert(low.pet==1);
    wild.level=100; instanceMap.instance=false; CharmInfo world(&wild); world.InitCharmCreateSpells(); assert(world.pet==1);
    instanceMap.instance=true; wild.map=nullptr; CharmInfo unmapped(&wild); unmapped.InitCharmCreateSpells(); assert(unmapped.pet==1);
    CharmInfo charmedPlayer(&player); charmedPlayer.InitCharmCreateSpells(); assert(charmedPlayer.empty==1);
    std::cout << "PASS: production charm action-bar initialization: ownerless instance NPC, player/NPC owner, guardian, level/map and player branches\n";
}

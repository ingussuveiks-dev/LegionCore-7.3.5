#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>
using uint32=uint32_t;
using int32=int32_t;
using ObjectGuid=uint64_t;
using GuidList=std::vector<ObjectGuid>;
enum { SUMMON_PET, HUNTER_PET, UNIT_FIELD_CREATED_BY_SPELL };
struct Player;
struct AuraEffect { void SetAmount(float) {} };
struct Aura { AuraEffect effect; AuraEffect* GetEffect(int){return &effect;} };
struct Session { bool logout=false; bool PlayerLogout(){return logout;} };
struct Unit {
    Player* player=nullptr;
    std::set<int32> auras;
    std::vector<int32> casts;
    bool HasAura(int32 id){return auras.count(id)!=0;}
    Player* ToPlayer(){return player;}
    Unit* GetTargetUnit(){return nullptr;}
    Unit* getVictim(){return nullptr;}
    ObjectGuid GetGUID(){return 1;}
    GuidList* GetSummonList(float){return nullptr;}
    void RemoveAurasDueToSpell(int32 id){auras.erase(id);}
    Aura* AddAura(int32 id,Unit* target){static Aura aura;target->auras.insert(id);return &aura;}
    void CastSpell(Unit* target,int32 id,bool,void* = nullptr,void* = nullptr,ObjectGuid = 0){casts.push_back(id);AddAura(id,target);}
    void CastCustomSpell(Unit* target,int32 id,float*,float*,float*,bool){CastSpell(target,id,true);}
    template<class F> void AddDelayedEvent(float,F){}
};
struct Player:Unit {
    Session session;
    bool online=true;
    Player(){player=this;}
    Session* GetSession(){return online?&session:nullptr;}
    Unit* GetSelectedUnit(){return nullptr;}
    void learnSpell(int32,bool){}
    void removeSpell(int32){}
};
struct Creature:Unit { void DespawnOrUnsummon(int){} };
namespace ObjectAccessor {
Unit* GetUnit(Unit&,ObjectGuid){return nullptr;}
Creature* GetCreature(Unit&,ObjectGuid){return nullptr;}
}
struct Motion { void MovePoint(int,Unit&){} };
struct PetAura {
    int32 petEntry=-1,spellId=166615,target=1,targetaura=1,option=0;
    float bp0=0,bp1=0,bp2=0;
    int32 aura=137015,casteraura=0,createdspell=0,fromspell=0;
};
struct Manager {
    std::vector<PetAura> rows{PetAura{}};
    std::vector<PetAura> const* GetPetAura(int32 entry){return entry==-1?&rows:nullptr;}
} manager;
Manager* sSpellMgr=&manager;
struct Pet:Unit {
    Unit* petOwner=nullptr;
    uint32 created=79598;
    int type=HUNTER_PET;
    bool alive=true;
    Motion motion;
    Unit* GetAnyOwner(){return petOwner;}
    uint32 GetUInt32Value(int){return created;}
    uint32 GetEntry(){return 42719;}
    int getPetType(){return type;}
    bool IsAlive(){return alive;}
    Motion* GetMotionMaster(){return &motion;}
    void learnSpell(int32){}
    void removeSpell(int32){}
    void CastPetAuras(bool,uint32=0);
    void Loaded(bool current){
#include "Loaded.inc"
    }
    void Created(){
#include "Created.inc"
    }
};
#include "CastPetAuras.inc"
enum { SPELL_CAST_OK, SPELL_FAILED_CASTER_AURASTATE };
struct SpellInfo { struct {uint32 CasterAuraSpell=166615;} AuraRestrictions; } killCommand;
struct Spell {
    Unit* m_caster;
    SpellInfo* m_spellInfo=&killCommand;
    int CheckRequiredAura(){
#include "RequiredAura.inc"
        return SPELL_CAST_OK;
    }
};
int main(){
    Player hunter;
    hunter.auras.insert(137015);
    Pet pet;
    pet.petOwner=&hunter;
    Spell cast{&hunter};
    assert(cast.CheckRequiredAura()==SPELL_FAILED_CASTER_AURASTATE);
    for(uint32 origin:{79598u,13481u,883u,83242u,83243u,83244u,83245u,0u}){
        pet.created=origin;
        pet.CastPetAuras(true);
        assert(hunter.HasAura(166615));
        assert(cast.CheckRequiredAura()==SPELL_CAST_OK);
        pet.CastPetAuras(false);
        assert(!hunter.HasAura(166615));
        assert(cast.CheckRequiredAura()==SPELL_FAILED_CASTER_AURASTATE);
        pet.CastPetAuras(true); // Resurrection/resummon/reconnect.
        assert(hunter.HasAura(166615));
        pet.CastPetAuras(false);
    }
    pet.created=79598;
    pet.Loaded(false); // Player::SummonPet uses LoadPetFromDB(...,false,...).
    assert(hunter.HasAura(166615));
    pet.CastPetAuras(false);
    pet.Loaded(true); // Login/temporary resummon uses current=true.
    assert(hunter.HasAura(166615));
    pet.CastPetAuras(false);
    pet.Created(); // Newly tamed pet, before its first DB reload.
    assert(hunter.HasAura(166615));
    pet.CastPetAuras(false);
    pet.alive=false;
    pet.Loaded(false);
    pet.Loaded(true);
    pet.Created();
    assert(!hunter.HasAura(166615)); // Loading a corpse must not enable abilities.
    pet.alive=true;
    pet.type=SUMMON_PET;
    pet.CastPetAuras(true);
    assert(!hunter.HasAura(166615)); // A demon/other summon is not a hunter pet.
    pet.type=HUNTER_PET;
    hunter.auras.erase(137015);
    pet.CastPetAuras(true);
    assert(!hunter.HasAura(166615)); // Preserve the specialization predicate.
    hunter.auras.insert(137015);
    pet.CastPetAuras(true,123);
    assert(!hunter.HasAura(166615)); // Preserve from-spell filtering.
    manager.rows[0].createdspell=883;
    pet.CastPetAuras(true);
    assert(!hunter.HasAura(166615)); // Per-row creation filters still apply.
    manager.rows[0].createdspell=0;
    hunter.session.logout=true;
    pet.CastPetAuras(true);
    assert(!hunter.HasAura(166615));
    hunter.session.logout=false;
    hunter.online=false;
    pet.CastPetAuras(true);
    assert(!hunter.HasAura(166615));
    Unit nonPlayer;
    pet.petOwner=&nonPlayer;
    pet.CastPetAuras(true);
    pet.petOwner=nullptr;
    pet.CastPetAuras(true);
    std::cout<<"PASS: actual CastPetAuras restores the native Kill Command prerequisite for tamed and called hunter pets; lifecycle, type, owner and row filters retained\n";
}

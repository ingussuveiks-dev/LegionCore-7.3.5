#define NOMINMAX
#include <Windows.h>
#include "PetDefines.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#define ASSERT(x) assert(x)
enum DeathState {ALIVE,JUST_DIED,CORPSE};
enum SpellEffIndex {EFFECT_0};
enum {OBJECT_FIELD_DYNAMIC_FLAGS,UNIT_DYNFLAG_NONE,UNIT_FIELD_FLAGS,UNIT_FLAG_SKINNABLE,UNIT_STATE_ALL_STATE,COMMAND_STAY,SPELL_EFFECT_HANDLE_HIT,SPELL_EFFECT_HANDLE_HIT_TARGET};
struct Player;struct Pet;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}static Player* ToPlayer(Unit* u){return u?u->ToPlayer():nullptr;}};
struct CharmInfo {
 uint32 number=0;int staySaves=0;bool atStay=false,following=true,attacking=true,returning=true;
 uint32 GetPetNumber(){return number;}void SaveStayPosition(){++staySaves;}
 bool HasCommandState(int){return atStay;}void SetIsAtStay(bool v){atStay=v;}
 void SetIsFollowing(bool v){following=v;}void SetIsCommandAttack(bool v){attacking=v;}void SetIsReturning(bool v){returning=v;}
};
struct Creature:Unit {
 DeathState state=ALIVE;uint32 health=1000;
 virtual void setDeathState(DeathState s){state=s==JUST_DIED?CORPSE:s;if(s==JUST_DIED)health=0;}
 DeathState getDeathState(){return state;}bool IsAlive(){return state==ALIVE;}
 uint32 GetHealth(){return health;}void SetHealth(uint32 h){health=h;}
 void SetUInt32Value(int,int){}void RemoveFlag(int,int){}
};
struct Player:Unit {
 PetStable stable;bool hasStable=true,activeAura=false;Pet* pet=nullptr;std::unique_ptr<Pet> loaded;
 int summons=0,saves=0,stopCharm=0,corpseMoves=0,clears=0;
 Player* ToPlayer()override{return this;}PetStable* GetPetStable(){return hasStable?&stable:nullptr;}
 Pet* GetPet(){return pet;}void SetMinion(Pet* p,bool){pet=p;}
 Pet* SummonPet(uint32,Optional<PetSaveMode>,float,float,float,float,uint32);
 void StopCastingCharm(){++stopCharm;}void RemovePet(void*,PetSaveMode){stable.CurrentPetIndex.reset();}
 void* GetTransport(){return nullptr;}float GetOrientation(){return 1;}
 void GetClosePoint(float& x,float& y,float& z,float,float,float){x=1;y=2;z=3;}
 ~Player();
};
struct Pet:Creature {
 Player* owner;PetType type=HUNTER_PET;CharmInfo charm;bool hasCharm=true;
 Pet(Player* p,uint32 number):owner(p){charm.number=number;}
 Unit* GetOwner(){return owner;}CharmInfo* GetCharmInfo(){return hasCharm?&charm:nullptr;}
 PetType getPetType(){return type;}void CastPetAuras(bool apply){if(owner)owner->activeAura=apply;}
 void setDeathState(DeathState s)override;
 static std::pair<PetStable::PetInfo const*,PetSaveMode> GetLoadPetInfo(PetStable const&,uint32,uint32,Optional<PetSaveMode>);
 void RestoreState(Player* owner,PetStable::PetInfo const* petInfo,PetSaveMode slot,bool current){
  PetStable* petStable=owner->GetPetStable();
  #include "Restore.inc"
 }
 void SetReactState(ReactStates){}void SetCanModifyStats(bool){}int GetPowerType(){return 0;}
 void SetFullPower(int){}uint32 GetMaxPower(int){return 100;}void SetPower(int,uint32){}void SetTratsport(void*){}
 float GetCombatReach(){return 1;}float GetFollowAngle(){return 1;}
 void NearTeleportTo(float,float,float,float){++owner->corpseMoves;}void Relocate(float,float,float,float){}
 void ClearUnitState(uint32){++owner->clears;}uint32 CountPctFromMaxHealth(int pct){return uint32(1000*pct/100);}
 void SavePetToDB(PetSaveMode){++owner->saves;auto p=owner->stable.GetCurrentPet();assert(p&&p->PetNumber==charm.number);p->Health=health;}
};
Player::~Player()=default;
#include "Select.inc"
#include "Death.inc"
Pet* Player::SummonPet(uint32 entry,Optional<PetSaveMode> slot,float,float,float,float,uint32){
 ++summons;auto info=Pet::GetLoadPetInfo(stable,entry,0,slot);if(!info.first)return nullptr;
 loaded=std::make_unique<Pet>(this,info.first->PetNumber);loaded->type=info.first->Type;
 loaded->RestoreState(this,info.first,info.second,false);return loaded.get();
}
struct Spell {
 int effectHandleMode=SPELL_EFFECT_HANDLE_HIT,damage=35;Unit* m_caster;
 explicit Spell(Unit* p):m_caster(p){}void EffectResurrectPet(SpellEffIndex);
};
#include "Revive.inc"
PetStable::PetInfo Info(uint32 number,uint32 health){PetStable::PetInfo p;p.PetNumber=number;p.Type=HUNTER_PET;p.Health=health;return p;}
void Reproduce(){Player p;p.stable.ActivePets[0]=Info(42,0);Spell s(&p);s.EffectResurrectPet(EFFECT_0);}
int Baseline(){
 __try {Reproduce();return 1;}
 __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return 0;}
}
int main(int argc,char**){
 if(argc>1){assert(Baseline()==0);std::cout<<"PASS: original dead-pet load reproduces the null-current health write (handled ACCESS_VIOLATION)\n";return 0;}
 // All five active slots: load a dead pet with no current selection, apply
 // native corpse state, bind it, then run the complete native revive handler.
 for(int slot=0;slot<MAX_ACTIVE_PETS;++slot){
  Player p;p.stable.ActivePets[slot]=Info(42,0);Spell s(&p);s.EffectResurrectPet(EFFECT_0);
  assert(p.pet&&p.pet->IsAlive()&&p.pet->GetHealth()==350&&p.summons==1&&p.saves==1&&p.activeAura);
  assert(p.stable.GetCurrentPet()->PetNumber==42&&p.stable.GetCurrentPet()->Health==350&&p.corpseMoves==0);
  assert(!p.pet->charm.attacking&&!p.pet->charm.following&&!p.pet->charm.returning&&p.pet->charm.staySaves==1);
  s.EffectResurrectPet(EFFECT_0);assert(p.saves==1&&p.summons==1);
  p.pet->setDeathState(JUST_DIED);assert(p.stable.GetCurrentPet()->Health==0&&!p.activeAura);
  s.EffectResurrectPet(EFFECT_0);assert(p.saves==2&&p.pet->GetHealth()==350&&p.corpseMoves==1&&p.activeAura);
 }
 // A different current pet must never receive the loading pet's death update.
 Player p;p.stable.ActivePets[0]=Info(10,777);p.stable.ActivePets[1]=Info(20,444);p.stable.SetCurrentActivePetIndex(0);
 Pet pending(&p,20);pending.setDeathState(JUST_DIED);assert(p.stable.ActivePets[0]->Health==777&&p.stable.ActivePets[1]->Health==0&&p.stable.CurrentPetIndex==0);
 p.stable.UnslottedPets={Info(30,333)};Pet unslotted(&p,30);unslotted.setDeathState(JUST_DIED);assert(p.stable.UnslottedPets[0].Health==0&&p.stable.ActivePets[0]->Health==777);
 Pet fresh(&p,99);fresh.setDeathState(JUST_DIED);assert(p.stable.ActivePets[0]->Health==777);
 p.stable.StabledPets[0]=Info(77,222);assert(!p.stable.UpdatePetHealth(77,0)&&p.stable.StabledPets[0]->Health==222);
 assert(!p.stable.UpdatePetHealth(0,0));
 Pet summoned(&p,10);summoned.type=SUMMON_PET;summoned.setDeathState(JUST_DIED);assert(p.stable.ActivePets[0]->Health==777);
 Pet ownerless(nullptr,10);ownerless.setDeathState(JUST_DIED);ownerless.setDeathState(ALIVE);
 // Missing stable/no dead active pet/no pet: do not invent a stable slot or load
 // a living/stabled pet. Non-player and unrelated spell modes stay unchanged.
 for(int variant=0;variant<3;++variant){Player empty;empty.hasStable=variant!=0;if(variant==2)empty.stable.ActivePets[0]=Info(1,500);Spell s(&empty);s.EffectResurrectPet(EFFECT_0);assert(!empty.summons&&!empty.pet);}
 Player gated;Spell s(&gated);s.damage=-1;s.EffectResurrectPet(EFFECT_0);s.damage=35;s.effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;s.EffectResurrectPet(EFFECT_0);assert(!gated.summons);
 Unit nonPlayer;Spell other(&nonPlayer);other.EffectResurrectPet(EFFECT_0);
 // Missing charm/stable on a newly constructed death remains safe.
 Pet noCharm(&p,10);noCharm.hasCharm=false;noCharm.setDeathState(JUST_DIED);p.hasStable=false;pending.setDeathState(JUST_DIED);
 std::cout<<"PASS: native stable records + extracted dead load, death and revive handlers; five slots, unselected/different pet, repeated death/revive, health/aura/save/commands, unslotted/new/summoned/missing cases\n";
}

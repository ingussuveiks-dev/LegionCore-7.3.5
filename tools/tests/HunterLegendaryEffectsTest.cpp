// Executes extracted production methods. DB2/SQL admission is audited separately;
// these fixtures do not simulate proc delivery, client effects or a full server.
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
constexpr int AURA_EFFECT_HANDLE_REAL=1,MAX_TALENT_COLUMNS=3,EQUIPMENT_SLOT_START=0,EQUIPMENT_SLOT_END=19,INVENTORY_SLOT_BAG_0=255;
constexpr int CLASS_HUNTER=3,SPELL_AURA_OVERRIDE_PET_SPECS=451,IN_MILLISECONDS=1000,SPELL_TRIGGER_MODIFY_COOLDOWN=23;
constexpr uint32 SPEC_PET_FEROCITY=74,SPEC_PET_TENACITY=81,SPEC_PET_CUNNING=79,SPEC_PET_ADAPTATION_FEROCITY=535,SPEC_PET_ADAPTATION_TENACITY=537,SPEC_PET_ADAPTATION_CUNNING=536;
#define TC_LOG_DEBUG(...) ((void)0)
struct Player;struct Pet;
struct Item {uint32 entry;uint32 GetEntry(){return entry;}};
struct Unit {
 Unit* owner=nullptr;uint32 cls=CLASS_HUNTER;virtual ~Unit()=default;
 virtual Player* ToPlayer(){return nullptr;}virtual Pet* ToPet(){return nullptr;}
 Unit* GetOwner(){return owner;}uint32 getClass(){return cls;}uint32 GetComboPoints(){return 0;}
};
struct Player:Unit {
 std::set<uint32> selected,known;Item* item=nullptr;std::vector<int32> cooldownChanges;unsigned packets=0;
 Player* ToPlayer()override{return this;}int GetActiveTalentGroup(){return 0;}
 bool HasTalent(uint32 id,int){return selected.count(id)!=0;}
 Item* GetItemByPos(int,int slot){return slot==10?item:nullptr;}
 void removeSpell(uint32 id){known.erase(id);}void learnSpell(uint32 id,bool){known.insert(id);}
 void PetSpellInitialize(){++packets;}void SendTalentsInfoData(bool){++packets;}
 void ModifySpellCooldown(uint32 id,int32 delta){assert(id==34026);cooldownChanges.push_back(delta);}
};
struct TalentEntry {uint32 ID,SpellID;};
struct TalentStore {TalentEntry entry{22347,194306};TalentEntry const* LookupEntry(uint32 id){return id==entry.ID?&entry:nullptr;}} sTalentStore;
struct Pet:Unit {
 uint32 m_petSpecialization=SPEC_PET_FEROCITY;bool overrideSpec=false;unsigned changes=0;
 Pet* ToPet()override{return this;}bool HasAuraType(int){return overrideSpec;}
 void UnlearnSpecializationSpell(){}void LearnSpecializationSpell(){}
 void SetSpecialization(uint16 id){m_petSpecialization=id;++changes;}
 void CheckSpecialization();
};
struct AuraApplication {Unit* target;Unit* GetTarget()const{return target;}};
struct AuraEffect {
 int misc=22347,miscB=151641;int GetMiscValue()const{return misc;}int GetMiscValueB()const{return miscB;}
 void HandleEnableExtraTalent(AuraApplication const*,uint8,bool)const;
 void HandleOverridePetSpecs(AuraApplication const*,uint8,bool)const;
};
#include "ExtraTalent.inc"
#include "OverridePet.inc"
#include "PetSpec.inc"
struct Spell {int Id;};struct Rule {int aura=0;};
void DispatchCooldown(Unit* target,float triggerAmount,float bp0,float bp1,float bp2,Spell* procSpell=nullptr,int requiredSpell=0)
{
 Rule row;row.aura=requiredSpell;Rule* itr=&row;int spell_trigger=34026,triggered_spell_id=0;bool check=false;
 // Production dispatcher is inside a rule loop; continue rejects the current rule.
 for(int rule=0;rule<1;++rule) switch(SPELL_TRIGGER_MODIFY_COOLDOWN) {
 #include "Cooldown.inc"
 }
 (void)check;
}
int main()
{
 Player hunter;Item ring{151641};hunter.item=&ring;AuraApplication app{&hunter};AuraEffect effect;
 effect.HandleEnableExtraTalent(&app,0,true);assert(hunter.known.empty());
 effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,true);assert(hunter.known.count(194306));
 effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,true);assert(hunter.known.size()==1);
 effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,false);assert(hunter.known.empty());
 hunter.known.insert(194306);hunter.selected.insert(22347);
 effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,false);assert(hunter.known.count(194306));
 hunter.selected.clear();hunter.item=nullptr;
 effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,true);assert(hunter.known.empty());
 effect.misc=0;effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,true);assert(hunter.known.empty());
 Unit npc;app.target=&npc;effect.HandleEnableExtraTalent(&app,AURA_EFFECT_HANDLE_REAL,true);
 for(auto pair:{std::pair<uint32,uint32>{74,535},{81,537},{79,536}}) {
  Pet pet;pet.owner=&hunter;pet.m_petSpecialization=pair.first;app.target=&pet;pet.overrideSpec=true;
  effect.HandleOverridePetSpecs(&app,0,true);assert(pet.m_petSpecialization==pair.first);
  effect.HandleOverridePetSpecs(&app,AURA_EFFECT_HANDLE_REAL,true);assert(pet.m_petSpecialization==pair.second);
  auto count=pet.changes;effect.HandleOverridePetSpecs(&app,AURA_EFFECT_HANDLE_REAL,true);assert(pet.changes==count);
  pet.overrideSpec=false;effect.HandleOverridePetSpecs(&app,AURA_EFFECT_HANDLE_REAL,false);assert(pet.m_petSpecialization==pair.first);
  pet.owner=nullptr;pet.overrideSpec=true;effect.HandleOverridePetSpecs(&app,AURA_EFFECT_HANDLE_REAL,true);assert(pet.m_petSpecialization==pair.first);
  pet.owner=&hunter;hunter.cls=1;effect.HandleOverridePetSpecs(&app,AURA_EFFECT_HANDLE_REAL,true);assert(pet.m_petSpecialization==pair.first);hunter.cls=CLASS_HUNTER;
 }
 // Native Qa'pla amount is -3000, SQL bp1=-1000: exactly three seconds, not 1ms/3ms or a cooldown extension.
 DispatchCooldown(&hunter,-3000,0,-1000,0);assert(hunter.cooldownChanges==std::vector<int32>{-3000});
 DispatchCooldown(&npc,-3000,0,-1000,0);assert(hunter.cooldownChanges.size()==1);
 DispatchCooldown(&hunter,-3000,0,-1000,0,nullptr,217200);assert(hunter.cooldownChanges.size()==1);
 Spell cobra{193455},frenzy{217200};
 DispatchCooldown(&hunter,-3000,0,-1000,0,&cobra,217200);assert(hunter.cooldownChanges.size()==1);
 DispatchCooldown(&hunter,-3000,0,-1000,0,&frenzy,217200);assert(hunter.cooldownChanges.back()==-3000&&hunter.cooldownChanges.size()==2);
 std::cout<<"PASS: production Soul talent learn/reapply/selected-talent preservation/unequip; Apex all three pet-spec apply/remove and owner guards; Qa'pla production cooldown units and rejected rule paths.\n";
}

#include <cstdint>
#include <iostream>
#include <stdexcept>
using uint32=uint32_t;
#include "NativePerkEnums.inc"
static void Require(bool ok){if(!ok)throw std::runtime_error("Native perk admission changed");}
struct Info {
 uint32 attributes=0;bool passive=false;int duration=30000;
 bool HasAttribute(uint32 bit)const{return (attributes&bit)!=0;}bool IsPassive()const{return passive;}int GetDuration()const{return duration;}
 int Condition()const{auto spellInfo=this;struct {int condition=-1;} value;auto AIInfo=&value;
#include "NativePerkCondition.inc"
 return value.condition;}
 bool AuraAdmission(int aura)const{switch(aura){
#include "NativePerkAura.inc"
 default:break;}return true;}
};
int main(){try{
 Info perk;perk.attributes=SPELL_ATTR0_CASTABLE_WHILE_DEAD;
 Require(perk.Condition()==AICOND_DIE&&!perk.AuraAdmission(SPELL_AURA_MOD_SPEED_ALWAYS)&&!perk.AuraAdmission(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE));
 perk.attributes=0;Require(perk.Condition()==AICOND_COMBAT&&perk.AuraAdmission(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE));
 perk.passive=true;Require(perk.Condition()==AICOND_AGGRO);perk.attributes=SPELL_ATTR0_CANT_USED_IN_COMBAT;Require(perk.Condition()==AICOND_EVADE);
 std::cout<<"PASS: actual native AI condition branch sends castable-while-dead perks to death handling, and native CanAutoCast aura branch rejects both artifact buffs. Removing imported rows is catalog cleanup, not proof that enemies previously gained these bonuses.\n";
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

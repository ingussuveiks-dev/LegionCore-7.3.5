#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using uint32=uint32_t;
using SpellSchoolMask=uint32;
enum {CURRENT_GENERIC_SPELL=1,CURRENT_CHANNELED_SPELL=2,PLAYERSPELL_REMOVED=2,
    SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS=1,SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2=2,
    SPELL_ATTR0_DISABLED_WHILE_ACTIVE=1,SPELL_PREVENTION_TYPE_PACIFY=2,IN_MILLISECONDS=1000};
using CurrentSpellTypes=uint32;
using AuraType=uint32;
double getPreciseTime(){return 100;}
uint32 GetFirstSchoolInMask(uint32 mask){uint32 bit=0;while(!(mask&(1u<<bit)))++bit;return bit;}
struct SpellInfo{
    uint32 Id,school,attributes=0;struct{uint32 PreventionType=1;}Categories;
    bool HasAttribute(uint32 attr)const{return (attributes&attr)!=0;}
    uint32 GetSchoolMask()const{return school;}
};
struct Spell{SpellInfo const* m_spellInfo;};
struct PlayerSpell{uint32 state=0;bool disabled=false;};
using PlayerSpellMap=std::map<uint32,PlayerSpell>;
struct Swap{uint32 from,to;bool IsAffectingSpell(SpellInfo const* s)const{return s->Id==from;}uint32 GetAmount()const{return to;}};
struct Unit{using AuraEffectList=std::list<Swap*>;};
struct Manager{std::map<uint32,SpellInfo> spells;SpellInfo const* GetSpellInfo(uint32 id){auto i=spells.find(id);return i==spells.end()?nullptr:&i->second;}} manager;
auto sSpellMgr=&manager;
namespace WorldPackets{namespace Spells{
struct SpellCooldown{uint32 Caster=0,Flags=0;std::vector<std::pair<uint32,uint32>> SpellCooldowns;SpellCooldown const* Write(){return this;}};
}}
struct Player:Unit{
    PlayerSpellMap m_spells;std::map<uint32,double> cooldowns;Spell* current[3]{};AuraEffectList swaps;
    std::vector<std::pair<uint32,uint32>> sent;
    uint32 GetGUID(){return 7;}
    void GetAuraEffectsByListType(std::list<AuraType>*,AuraEffectList& out){out=swaps;}
    Spell* GetCurrentSpell(CurrentSpellTypes type){return current[type];}
    double GetSpellCooldownDelay(uint32 id){return cooldowns.count(id)?cooldowns.at(id)-getPreciseTime():0;}
    void AddSpellCooldown(uint32 id,uint32,double end){cooldowns[id]=end;}
    void SendDirectMessage(WorldPackets::Spells::SpellCooldown const* packet){sent=packet->SpellCooldowns;}
    void ProhibitSpellSchool(SpellSchoolMask,uint32);
};
#include "PlayerInterruptCooldown.inc"
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
    manager.spells={{1,{1,16}},{2,{2,16}},{3,{3,4}},{4,{4,16}},{5,{5,16}},{6,{6,16}},{7,{7,16}},{8,{8,16}},{9,{9,16}}};
    manager.spells.at(7).attributes=SPELL_ATTR0_DISABLED_WHILE_ACTIVE;
    manager.spells.at(8).Categories.PreventionType=SPELL_PREVENTION_TYPE_PACIFY;
    manager.spells.at(9).Categories.PreventionType=0;
    Spell generic{manager.GetSpellInfo(1)},channel{manager.GetSpellInfo(2)};
    Player p;p.current[1]=&generic;p.current[2]=&channel;p.ProhibitSpellSchool(16,6000);
    Require(p.cooldowns.count(1)&&p.cooldowns.count(2)&&p.sent.size()==2&&p.m_spells.empty(),"Unlearned generic/channeled casts lack cooldown or alter spellbook");
    p.m_spells={{1,{}},{3,{}},{4,{PLAYERSPELL_REMOVED,false}},{5,{0,true}},{6,{}},{7,{}},{8,{}},{9,{}}};
    p.cooldowns[1]=120;p.cooldowns[2]=0;p.sent.clear();Swap override{6,2};p.swaps={&override};p.ProhibitSpellSchool(16,6000);
    Require(p.cooldowns.at(1)==120&&p.cooldowns.at(2)==106&&p.sent.size()==1&&p.sent.front().first==2,"Longer cooldown, override or duplicate learned/current cast was mishandled");
    for(uint32 id:{3u,4u,5u,6u,7u,8u,9u})Require(!p.cooldowns.count(id),"Unrelated/removed/disabled/overridden/prevention-filtered spell received cooldown");
    Player idle;idle.ProhibitSpellSchool(16,6000);Require(idle.sent.empty()&&idle.cooldowns.empty(),"Idle unlearned player received a fictitious spell cooldown");
    std::cout<<"PASS: extracted Player interrupt cooldown covers unlearned generic/channel casts, learned deduplication, overrides, longer cooldown and admission filters.\n";
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}

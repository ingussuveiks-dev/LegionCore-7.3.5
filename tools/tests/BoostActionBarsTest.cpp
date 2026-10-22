#include "BoostActionBars.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
enum {ACTIONBUTTON_DELETED=3,SPELL_ATTR0_HIDDEN_CLIENTSIDE=1,SPELL_ATTR0_TRADESPELL=2};
enum {SKILL_CATEGORY_CLASS=7,SKILL_CATEGORY_LANGUAGES=10,SKILL_RIDING=762};
bool IsProfessionSkill(uint32 id){return id==2;}
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_INFO(...) ++infos
int errors=0,infos=0;
struct SpellInfo {
 uint32 Id;bool passive=false,profession=false,mount=false;uint32 attributes=0;
 bool IsPassive()const{return passive;}bool IsProfession()const{return profession;}
 bool IsMountOrCompanions()const{return mount;}bool HasAttribute(uint32 a)const{return attributes&a;}
};
struct Skill {uint32 ClassMask;uint32 SkillLine=1;uint64 RaceMask=0;};struct SpecSpell {uint32 SpellID;};
struct SkillLine {uint32 CategoryID;};
struct SkillStore {std::map<uint32,SkillLine> lines{{1,{7}},{2,{11}},{3,{12}},{4,{9}}};
 SkillLine const* LookupEntry(uint32 id){auto i=lines.find(id);return i==lines.end()?nullptr:&i->second;}
}sSkillLineStore;
struct SpellManager {
 std::map<uint32,SpellInfo> spells;std::set<uint32> talents;
 std::multimap<uint32,Skill*> skills;
 SpellInfo const* GetSpellInfo(uint32 id){auto i=spells.find(id);return i==spells.end()?nullptr:&i->second;}
 bool IsTalent(uint32 id){return talents.count(id);}
 auto GetSkillLineAbilityMapBounds(uint32 id){return skills.equal_range(id);}
}manager,*sSpellMgr=&manager;
struct DB2Manager {std::vector<SpecSpell const*> specs;auto GetSpecializationSpells(uint32){return &specs;}}sDB2Manager;
struct Button {uint64 action;uint8 type;int uState=0;uint64 GetAction()const{return action;}uint8 GetType()const{return type;}};
struct Player {
 uint32 spec=253,klass=3;std::map<uint32,int> known;std::set<uint32> inactive;
 std::map<uint8,Button> actions;std::map<uint32,uint32> replacements;int sends=0;
 auto const& GetSpellMapConst(){return known;}auto const& GetActionButtons(){return actions;}
 uint32 GetSpecializationId(){return spec;}uint32 getClass(){return klass;}uint32 getClassMask(){return 1u<<(klass-1);}
 char const* GetName(){return "fixture";}
 bool HasActiveSpell(uint32 id){return known.count(id)&&!inactive.count(id);}
 SpellInfo const* GetCastSpellInfo(SpellInfo const* info,uint32){auto i=replacements.find(info->Id);return i==replacements.end()?info:sSpellMgr->GetSpellInfo(i->second);}
 void RemoveActionButton(uint8 b){actions.erase(b);}void AddActionButton(uint8 b,uint64 a,uint8 t){actions[b]={a,t};}
 void SendActionButtons(int){++sends;}
};
struct CharacterService {bool ArrangeBoostActionBar(Player*);};
#include "Arrange.inc"
int main(){
 CharacterService service;Player p;Skill hunter{4};
 for(auto id:{34026u,217200u,120679u,193455u,2643u,19574u,193530u,131894u,147362u,883u,109304u,20572u}){
  p.known[id]=0;manager.spells[id]={id};manager.skills.emplace(id,&hunter);
 }
 p.replacements[120679]=217200;
 for(auto id:{1000u,1001u,1002u,1003u,1004u,1005u,1006u}){p.known[id]=0;manager.spells[id]={id};manager.skills.emplace(id,&hunter);}
 manager.spells[1000].passive=true;manager.spells[1001].profession=true;manager.spells[1002].mount=true;
 manager.spells[1003].attributes=SPELL_ATTR0_HIDDEN_CLIENTSIDE;manager.spells[1004].attributes=SPELL_ATTR0_TRADESPELL;
 p.inactive.insert(1005);manager.spells.erase(1006);p.known[1007]=0;manager.spells[1007]={1007}; // unrelated, no admission
 Skill professionBook{0,2,0},generic{0,3,0},racial{0,4,4};
 for(auto id:{1008u,1009u}){p.known[id]=0;manager.spells[id]={id};}
 manager.skills.emplace(1008,&professionBook);manager.skills.emplace(1009,&generic);
 manager.skills.erase(20572);manager.skills.emplace(20572,&racial);
 SpecSpell specSpell{1101};sDB2Manager.specs.push_back(&specSpell);
 for(auto id:{1100u,1101u,207068u}){p.known[id]=0;manager.spells[id]={id};}
 manager.talents.insert(1100); // chosen talent and native spec ability without skill rows
 p.actions[0]={6603,0};p.actions[2]={0x100000001ULL,0x40};p.actions[5]={12345,0x80};p.actions[72]={999,0};p.actions[131]={998,0};
 assert(service.ArrangeBoostActionBar(&p));
 assert(p.actions.at(0).action==34026&&p.actions.at(1).action==217200&&p.actions.at(2).action==193455);
 std::set<uint64> spells;bool macro=false,item=false;
 for(auto const& a:p.actions)if(a.first<72){if(a.second.type==0){assert(spells.insert(a.second.action).second);assert(a.second.action<1000||a.second.action>1009);assert(a.second.action!=120679&&a.second.action!=6603);}else{macro|=a.second.action==0x100000001ULL;item|=a.second.action==12345;}}
 assert(spells.count(20572));
 assert(spells.count(1100)&&spells.count(1101)&&spells.count(207068));
 assert(macro&&item&&p.actions.at(72).action==999&&p.actions.at(131).action==998&&p.sends==1);
 // Reapplying the one-shot planner is stable; normal login/spec changes do not call it.
 auto before=p.actions;assert(service.ArrangeBoostActionBar(&p));
 for(auto const& a:before){assert(p.actions.at(a.first).action==a.second.action&&p.actions.at(a.first).type==a.second.type);}
 auto infosBefore=infos;p.spec=999;assert(!service.ArrangeBoostActionBar(&p)&&infos==infosBefore&&errors==1);
 assert(!service.ArrangeBoostActionBar(nullptr));
 std::vector<BoostActionBars::Action> old,planned{{99,99,99}};
 for(uint8 b=0;b<72;++b)old.push_back({b,b,0x80});
 assert(!BoostActionBars::Build(253,3,{34026},old,[](uint32){return true;},planned));assert(planned[0].button==99);
 std::set<uint32> forms{768,5487,24858,5221,33917,8921};
 assert(BoostActionBars::Build(103,11,{768,5487,24858,5221,33917,8921},{{96,999,0}},[&](uint32 id){return forms.count(id);},planned));
 bool cat=false,catStealth=false,bear=false,moonkin=false;
 for(auto b:planned){cat|=b.button==72&&b.action==5221;catStealth|=b.button==84&&b.action==5221;bear|=b.button>=96&&b.button<108;moonkin|=b.button==108&&b.action==8921;}
 assert(cat&&catStealth&&!bear&&moonkin); // existing bear page is preserved by the caller
 forms={1784,185313,185438,53};
 assert(BoostActionBars::Build(261,4,{1784,185313,185438,53},{},[&](uint32 id){return forms.count(id);},planned));
 bool stealth=false,dance=false;for(auto b:planned){stealth|=b.button==72&&b.action==185438;dance|=b.button==84&&b.action==185438;}
 assert(stealth&&dance);
 // Every native specialization has its own starter; no faction-specific branch.
 for(auto spec:{71,72,73,65,66,70,253,254,255,259,260,261,256,257,258,250,251,252,262,263,264,62,63,64,265,266,267,268,269,270,102,103,104,105,577,581}){
  auto priority=BoostActionBars::Priority(spec);assert(!priority.empty());
  assert(BoostActionBars::Build(spec,0,priority,{},[](uint32){return true;},planned));
  assert(planned[0].action==priority[0]&&planned[0].button==0);std::set<uint8> buttons;std::set<uint64> actions;
  for(auto const& b:planned){assert(b.button<72&&buttons.insert(b.button).second&&actions.insert(b.action).second);}
 }
 std::cout<<"PASS: production arranger, BM priority, replacement talents, native admission branches, hidden/passive/disabled/unrelated exclusion, 64-bit macros/items, stance pages, atomic overflow, all 36 specs\n";
}

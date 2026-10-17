#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using Difficulty=int;
#define ASSERT assert
#define TC_LOG_DEBUG(...) (void)0
enum {PERMIT_BASE_NO=-1,PERMIT_BASE_REACTIVE=100,PERMIT_BASE_PROACTIVE=200,UNIT_MASK_CONTROLABLE_GUARDIAN=1,UNIT_MASK_GUARDIAN=2,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,CREATURE_TYPE_CRITTER=8,
 AICOND_EVADE,AICOND_DIE,AICOND_AGGRO,AICOND_COMBAT,AITARGET_SELF=0,TEXT_GROUP_COMBAT=1,TEXT_GROUP_DIE=2,CREATURE_ACTION_TYPE_ATTACK=1,EVENT_1=1,SPELL_ATTR0_CANT_USED_IN_COMBAT=0x10000000,SPELL_ATTR0_CASTABLE_WHILE_DEAD=0x800000};
struct CreatureSpell {uint32 SpellID=0,Text=0;std::vector<Difficulty> Difficulties;bool CanUseInDifficulty(Difficulty)const;};
#include "MistsDifficulty.inc"
struct Unit {virtual ~Unit()=default;uint32 GetGUID(){return 1;}bool IsPlayer(){return true;}};
struct Creature:Unit {uint32 entry=0,m_templateSpells[1]{};bool civilian=false,neutral=false,m_CanCallAssistance=false,world=true,dead=false,m_cleanupDone=false;std::string selected;std::map<uint32,CreatureSpell> spells;auto* GetMap(){return this;}int GetDifficultyID(){return 0;}std::map<uint32,CreatureSpell>* CreatureSpells=&spells;std::vector<uint32> casts;Unit victim;Unit* getVictim(){return &victim;}bool IsInWorld(){return world;}
 CreatureSpell const* GetCreatureSpell(uint32 id){auto i=spells.find(id);return i==spells.end()?nullptr:&i->second;}bool isPet(){return false;}std::string GetAIName(){return "";}bool isMinion(){return false;}bool IsVehicle(){return false;}bool CanVehicleAI(){return false;}bool HasUnitTypeMask(uint32){return false;}bool HasFlag64(uint32,uint32){return false;}bool isGuard(){return false;}bool isTotem(){return false;}bool isAnySummons(){return false;}bool isTrigger(){return false;}uint32 GetCreatureType(){return 7;}void SetNPCAIName(std::string s){selected=s;}bool isCivilian()const{return civilian;}bool IsNeutralToAll()const{return neutral;}bool isElite(){return false;}void CastStop(){}void CastSpell(Unit*,uint32 id,bool){casts.push_back(id);}};
struct Guardian:Creature {Unit* GetOwner(){return this;}};
struct CreatureAI {explicit CreatureAI(Creature*){}virtual ~CreatureAI()=default;};struct NullCreatureAI:CreatureAI{using CreatureAI::CreatureAI;};
struct CreatureAICreator {virtual ~CreatureAICreator()=default;virtual std::string key()const=0;virtual CreatureAI* Create(Creature*)const=0;};struct SelectableAI:CreatureAICreator {virtual int Permit(Creature*)const=0;};
struct Registry {std::map<std::string,SelectableAI*> factories;SelectableAI* GetRegistryItem(std::string const& s){auto i=factories.find(s);return i==factories.end()?nullptr:i->second;}auto const& GetRegisteredItems(){return factories;}} registry;auto sCreatureAIRegistry=&registry;
struct Scripts {CreatureAI* GetCreatureAI(Creature*){return nullptr;}} scripts;auto sScriptMgr=&scripts;
namespace FactorySelector {
#include "MistsSelectAI.inc"
}
struct EventMap {std::map<uint32,uint32> scheduled;void Reset(){scheduled.clear();}void ScheduleEvent(uint32 id,uint32 delay){assert(delay>0);scheduled[id]=delay;}uint32 ExecuteEvent(){if(scheduled.empty())return 0;auto i=scheduled.begin();auto id=i->first;scheduled.erase(i);return id;}};
struct SpellInfo {uint32 attrs=0;int32 duration=0;bool allowed=true;bool HasAttribute(uint32 a)const{return attrs&a;}bool IsPassive()const{return attrs&0x40;}int32 GetDuration()const{return duration;}bool CanAutoCast(Creature*,Unit*)const{return allowed;}};
struct Manager {std::map<uint32,SpellInfo> spells;SpellInfo const* GetSpellInfo(uint32 id){auto i=spells.find(id);return i==spells.end()?nullptr:&i->second;}} manager;auto sSpellMgr=&manager;
struct AIInfo {uint32 condition=AICOND_COMBAT,target=AITARGET_SELF,cooldown=10;};std::map<uint32,AIInfo> AISpellInfo;
struct Text {uint32 MinTimer=1;};
bool roll_chance_i(uint32){return false;}
struct AggressorAI:CreatureAI {Creature* me;EventMap events,spellCasts,textCombatEvents;std::vector<Text>* CreatureCombatTexts=nullptr;std::vector<uint32> dispatched;explicit AggressorAI(Creature* c):CreatureAI(c),me(c){}static int Permissible(Creature const*);void Reset();void EnterCombat(Unit*);void JustDied(Unit*);void TalkAuto(uint32,uint32){}void DoCast(uint32 id){dispatched.push_back(id);}};
struct ReactorAI {static int Permissible(Creature const*);};
#include "MistsPermit.inc"
#include "MistsReactorPermit.inc"
#include "MistsCombatReset.inc"
#include "MistsCombatEnter.inc"
#include "MistsCombatDied.inc"
struct ObjectGuid {static constexpr uint32 Empty=0;};
struct TextManager {void SendText(Creature*,uint32,uint32){}} textManager;auto sCreatureTextMgr=&textManager;
void Dispatch(AggressorAI& ai){auto& spellCasts=ai.spellCasts;auto& me=ai.me;auto DoCast=[&](uint32 id){ai.DoCast(id);};auto DoMeleeAttackIfReady=[]{};
#include "MistsCombatDispatch.inc"
}
struct Factory:SelectableAI {std::string name;explicit Factory(std::string n):name(n){}std::string key()const override{return name;}CreatureAI* Create(Creature* c)const override{return name=="AggressorAI"?static_cast<CreatureAI*>(new AggressorAI(c)):new NullCreatureAI(c);}int Permit(Creature* c)const override{return name=="AggressorAI"?AggressorAI::Permissible(c):ReactorAI::Permissible(c);}};
int main(int argc,char** argv){
 assert(argc==3);std::ifstream world(argv[1]),profiles(argv[2]);assert(world&&profiles);std::string line;
 while(std::getline(profiles,line)){std::istringstream in(line);std::string kind;in>>kind;if(kind!="profile")continue;uint32 id,recovery;in>>id;auto& info=manager.spells[id];in>>info.attrs>>info.duration>>recovery;auto spellInfo=&info;auto AIInfo=&AISpellInfo[id];
 #include "MistsAICondition.inc"
 assert(AIInfo->condition==((id==213738||id==215377)?AICOND_DIE:AICOND_COMBAT));}
 assert(manager.spells.size()==13);std::map<uint32,std::map<uint32,CreatureSpell>> spellRows;while(std::getline(world,line)){std::istringstream in(line);std::string kind;in>>kind;if(kind=="spell"){uint32 entry,id;in>>entry>>id;assert(manager.spells.count(id));spellRows[entry][id].SpellID=id;}}
 Factory aggressive("AggressorAI"),reactive("ReactorAI");registry.factories={{"AggressorAI",&aggressive},{"ReactorAI",&reactive}};unsigned total=0,deaths=0;
 for(auto& row:spellRows){Creature c;c.entry=row.first;c.spells=row.second;std::unique_ptr<CreatureAI> chosen(FactorySelector::selectAI(&c));auto ai=dynamic_cast<AggressorAI*>(chosen.get());assert(ai&&c.selected=="AggressorAI");ai->Reset();assert(c.casts.empty()&&ai->spellCasts.scheduled.empty());Unit victim;ai->EnterCombat(&victim);assert(ai->spellCasts.scheduled.size()==c.spells.size()-2);
  auto scheduled=ai->spellCasts.scheduled;for(auto& spell:c.spells){assert(spell.second.CanUseInDifficulty(0));if(AISpellInfo[spell.first].condition==AICOND_DIE){assert(!scheduled.count(spell.first));continue;}assert(scheduled.count(spell.first));ai->spellCasts.scheduled.clear();ai->spellCasts.ScheduleEvent(spell.first,1);manager.spells[spell.first].allowed=false;Dispatch(*ai);assert(ai->dispatched.empty()&&ai->spellCasts.scheduled.at(spell.first)==500);manager.spells[spell.first].allowed=true;Dispatch(*ai);assert(ai->dispatched==std::vector<uint32>{spell.first}&&ai->spellCasts.scheduled.at(spell.first)>0);ai->dispatched.clear();++total;}
  ai->JustDied(&victim);assert(c.casts==std::vector<uint32>({213738,215377}));deaths+=uint32(c.casts.size());
  ai->Reset();assert(ai->spellCasts.scheduled.empty());}
 assert(spellRows.size()==4&&total==13&&deaths==8);Creature neutral;neutral.neutral=true;assert(AggressorAI::Permissible(&neutral)==PERMIT_BASE_NO&&ReactorAI::Permissible(&neutral)==PERMIT_BASE_REACTIVE);
 std::cout<<"PASS: native default AI selector with eligible Aggressor/Reactor factories, native condition classification for13 spell profiles,21 outdoor spell admissions (13 combat,8 death), failed CanAutoCast500ms retry and DoCast dispatch/reset. Registry/world/CanAutoCast/actual target casting and timer values are adapters; not a full combat simulation.\n";
}

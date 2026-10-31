#define ASSERT assert
#include "NativeHost.inc"
#include "RemoveModes.inc"
enum{UNIT_FIELD_NPC_FLAGS=0,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_NPC_FLAG_TRAINER=16,UNIT_NPC_FLAG_TRAINER_CLASS=32,UNIT_NPC_FLAG_TRAINER_PROFESSION=64,
 NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,LINK_ACTION_DEFAULT=0};
struct SpellClickInfo{uint32 spellId=208580,castFlags=3;bool IsFitToRequirements(Unit const*,Unit const*)const{return true;}};
using SpellClickInfoContainer=std::multimap<uint32,SpellClickInfo>;
using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr{SpellClickInfoContainer clicks{{38821,{}}};SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 id){return clicks.equal_range(id);}}objects;auto*sObjectMgr=&objects;
struct SpellInfo{uint32 Id;};
struct Spell{Unit*m_caster;SpellInfo*m_spellInfo;struct Targets{Unit*target;Unit*GetObjectTarget()const{return target;}}m_targets;SpellCastResult Check(){
#include "CastConditions.inc"
return SPELL_CAST_OK;}};
SpellCastResult Unit::CastSpell(Unit*target,uint32 id,bool /*triggered*/){assert(target==this&&(id==208580||id==209992));++casts;SpellInfo info{id};Spell spell{this,&info,{target}};last=spell.Check();if(last==SPELL_CAST_OK){++effects;aura=id==208580;}return last;}
#include "CanSee.inc"
#include "Click.inc"
bool Refresh(Creature*obj){bool buildUpdateBlock=false;
#include "Refresh.inc"
return buildUpdateBlock;}
uint32 Serialize(Player*target,Creature const*creature){uint32 index=0,m_uint32Values[]={UNIT_NPC_FLAG_SPELLCLICK};struct Buffer{uint32 value=0;void operator<<(uint32 n){value=n;}}buffer;auto*data=&buffer;
#include "Flags.inc"
return buffer.value;}
struct NativeRemoval{
 struct Link{uint32 removeMask=0,actiontype=0,effect=209992;};uint32 GetCasterGUID()const{return 1;}
 void Remove(Unit*caster,Unit*target,AuraRemoveMode removeMode,bool loginOut=false){target->aura=false;if(loginOut)return;std::vector<Link>links(1);void*castItem=nullptr;
  for(auto const&itr:links){Unit*_target=target;Unit*_caster=caster?caster:target;
#include "RemovalGuards.inc"
   switch(itr.actiontype){
#include "RemovalCast.inc"
    default:assert(false);
   }
  }
 }
};
void Reset(){scenarioManager={};scenarioManager.scenario.identity=1066;scenarioManager.scenario.step=3;world.instanced=true;}
int main(int argc,char**argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
 // Expected old empty condition stores admit a foreign no-quest participant.
 {Reset();Player foreign;foreign.classMask=1;foreign.status=0;Creature elevator;assert(foreign.canSeeSpellClickOn(&elevator));assert(elevator.HandleSpellClick(&foreign,-1)&&foreign.effects==1);NativeRemoval{}.Remove(&foreign,&foreign,AURA_REMOVE_BY_EXPIRE);assert(foreign.effects==2);}
 std::ifstream input(argv[1]);std::vector<Condition>rows;rows.reserve(36);uint32 source,group,spell;Condition c;std::vector<std::tuple<uint32,uint32,uint32>>profiles;
 while(input>>source>>group>>spell>>c.ElseGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition){rows.push_back(c);profiles.emplace_back(source,group,spell);}assert(rows.size()==36);
 for(size_t i=0;i<rows.size();++i){auto[type,g,id]=profiles[i];if(type==17){assert(g==0);conditionManager.ConditionStore[type][id].push_back(&rows[i]);}else{assert(type==18&&g==38821&&id==208580);conditionManager.SpellClickEventConditionStore[g][id].push_back(&rows[i]);}}
 unsigned cases=0;
 for(uint32 faction:{0u,1u})for(uint32 status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){
  Reset();Player p;p.status=status;p.faction=faction;Creature elevator;assert(p.canSeeSpellClickOn(&elevator)&&(Serialize(&p,&elevator)&UNIT_NPC_FLAG_SPELLCLICK));assert(elevator.HandleSpellClick(&p,-1)&&p.effects==1&&p.aura);NativeRemoval{}.Remove(&p,&p,AURA_REMOVE_BY_EXPIRE);assert(p.effects==2&&!p.aura);++cases;
  for(uint32 denied=0;denied<12;++denied){Reset();Player blocked;blocked.status=status;blocked.faction=faction;
   switch(denied){case 0:blocked.classMask=1;break;case 1:blocked.map=1;break;case 2:blocked.alive=false;break;case 3:blocked.status=0;break;case 4:blocked.status=QUEST_STATUS_REWARDED;break;case 5:blocked.status=QUEST_STATUS_COMPLETE;blocked.rewarded=true;break;case 6:scenarioManager.scenario.step=2;break;case 7:scenarioManager.scenario.step=4;break;case 8:scenarioManager.present=false;break;case 9:world.instanced=false;break;case 10:scenarioManager.scenario.identity=1;break;case 11:blocked.quest=1;break;}
   assert(!blocked.canSeeSpellClickOn(&elevator)&&!(Serialize(&blocked,&elevator)&UNIT_NPC_FLAG_SPELLCLICK)&&!elevator.HandleSpellClick(&blocked,-1));
   assert(blocked.CastSpell(&blocked,208580,false)!=SPELL_CAST_OK&&blocked.CastSpell(&blocked,209992,true)!=SPELL_CAST_OK&&!blocked.effects);++cases;
  }
 }
 for(AuraRemoveMode mode:{AURA_REMOVE_BY_DEFAULT,AURA_REMOVE_BY_CANCEL,AURA_REMOVE_BY_EXPIRE,AURA_REMOVE_BY_ENEMY_SPELL}){Reset();Player p;assert(p.CastSpell(&p,208580,false)==SPELL_CAST_OK);NativeRemoval{}.Remove(&p,&p,mode);assert(p.effects==2);++cases;}
 for(uint32 denial=0;denial<4;++denial){Reset();Player p;p.CastSpell(&p,208580,false);switch(denial){case 0:p.status=0;break;case 1:p.rewarded=true;p.status=QUEST_STATUS_REWARDED;break;case 2:p.map=1;break;case 3:scenarioManager.scenario.step=4;break;}NativeRemoval{}.Remove(&p,&p,AURA_REMOVE_BY_EXPIRE);assert(p.casts==2&&p.effects==1&&!p.aura);++cases;}
 for(bool logout:{false,true}){Reset();Player p;p.CastSpell(&p,208580,false);if(!logout)p.alive=false;NativeRemoval{}.Remove(&p,&p,logout?AURA_REMOVE_BY_DEFAULT:AURA_REMOVE_BY_DEATH,logout);assert(p.casts==1&&p.effects==1&&!p.aura);++cases;}
 {Reset();Unit pet;Creature elevator;assert(!elevator.HandleSpellClick(&pet,-1)&&pet.CastSpell(&pet,208580,false)!=SPELL_CAST_OK&&pet.CastSpell(&pet,209992,true)!=SPELL_CAST_OK&&!pet.effects);++cases;}
 {Reset();Player p;Creature elevator;p.status=0;assert(!p.canSeeSpellClickOn(&elevator));p.status=QUEST_STATUS_INCOMPLETE;assert(Refresh(&elevator)&&p.canSeeSpellClickOn(&elevator));scenarioManager.scenario.step=4;assert(Refresh(&elevator)&&!p.canSeeSpellClickOn(&elevator));++cases;}
 assert(conditionManager.GetConditionsForNotGroupedEntry(17,999999).empty()&&conditionManager.GetConditionsForSpellClickEvent(38821,999998).empty());
 std::cout<<"PASS "<<cases<<" actual native Condition/ElseGroups/source17 bridge/source18 lookup/canSee/flags/click flags3 plus native linked removal mode/default-cast cases: both factions/status1+3, real participant, direct/triggered casts, missing/late/wrong context/pet rejection, abandon/reward/map/stage before delayed exit, default/cancel/expiry versus death/logout. Original empty-store counterfactual admits foreign participant. Geometry/cast engine/scene/teleport/50336 sinks controlled; no automatic route/full1066/client claim.\n";
}

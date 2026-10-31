#define ASSERT assert
#include "NativeHost.inc"
enum{UNIT_FIELD_NPC_FLAGS=0,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_NPC_FLAG_TRAINER=16,UNIT_NPC_FLAG_TRAINER_CLASS=32,UNIT_NPC_FLAG_TRAINER_PROFESSION=64,
 NPC_CLICK_CAST_CASTER_CLICKER=1,NPC_CLICK_CAST_TARGET_CLICKER=2,NPC_CLICK_CAST_ORIG_CASTER_OWNER=4};
struct SpellClickInfo{uint32 spellId=210252,castFlags=1;bool relation=true;bool IsFitToRequirements(Unit const*,Unit const*)const{return relation;}};
using SpellClickInfoContainer=std::multimap<uint32,SpellClickInfo>;
using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr{SpellClickInfoContainer clicks{{105819,{}}};SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 id){return clicks.equal_range(id);}}objects;auto*sObjectMgr=&objects;
struct QuestObjective{uint32 ObjectID;int8 StorageIndex;};struct Quest{std::vector<QuestObjective>objectives{{105805,0},{105787,1},{105806,2},{111286,3}};auto const&GetObjectives()const{return objectives;}}quest;
struct QuestStore{Quest const*GetQuestTemplate(uint32 id){return id==40341?&quest:nullptr;}}questStore;auto*sQuestDataStore=&questStore;
#include "QuestObjective.inc"
#include "CanSee.inc"
#include "Click.inc"
bool Refresh(Creature* obj){bool buildUpdateBlock=false;
#include "Refresh.inc"
return buildUpdateBlock;}
uint32 Serialize(Player* target,Creature const*creature){uint32 index=0;uint32 m_uint32Values[]={UNIT_NPC_FLAG_SPELLCLICK};struct Buffer{uint32 value=0;void operator<<(uint32 n){value=n;}}buffer;auto*data=&buffer;
#include "Flags.inc"
return buffer.value;}
int main(int argc,char**argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);std::vector<Condition>rows;Condition c;
while(input>>c.ElseGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition)rows.push_back(c);assert(rows.size()==18);
auto&conditions=conditionManager.SpellClickEventConditionStore[105819][210252];for(auto&row:rows)conditions.push_back(&row);
unsigned cases=0;
for(uint32 faction:{0u,1u})for(uint32 status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){
 Player p;p.faction=faction;p.status=status;Creature bubble;
 assert(p.GetQuestObjectiveData(40341,105787)==1&&!p.GetQuestObjectiveData(40341,283384));assert(Refresh(&bubble));assert(p.canSeeSpellClickOn(&bubble));assert(Serialize(&p,&bubble)&UNIT_NPC_FLAG_SPELLCLICK);assert(bubble.HandleSpellClick(&p,-1)&&p.casts==1);++cases;
 for(int denial=0;denial<14;++denial){Player blocked=p;blocked.casts=0;scenarioManager.scenario.step=6;world.instanced=true;scenarioManager.present=true;
  switch(denial){case 0:blocked.classMask=1;break;case 1:blocked.map=1;break;case 2:blocked.alive=false;break;case 3:blocked.status=0;break;case 4:blocked.status=QUEST_STATUS_REWARDED;break;case 5:blocked.status=QUEST_STATUS_COMPLETE;blocked.rewarded=true;break;case 6:blocked.objective=0;break;case 7:blocked.owned=false;break;case 8:blocked.owned=false;blocked.bank=true;break;case 9:blocked.aura=true;break;case 10:scenarioManager.scenario.step=5;break;case 11:scenarioManager.present=false;break;case 12:world.instanced=false;break;case 13:scenarioManager.scenario.identity=1;break;}
  if(denial==8){assert(blocked.canSeeSpellClickOn(&bubble));assert(Serialize(&blocked,&bubble)&UNIT_NPC_FLAG_SPELLCLICK);assert(bubble.HandleSpellClick(&blocked,-1)&&blocked.casts==1);}
  else{assert(!blocked.canSeeSpellClickOn(&bubble));assert(!(Serialize(&blocked,&bubble)&UNIT_NPC_FLAG_SPELLCLICK));assert(!bubble.HandleSpellClick(&blocked,-1)&&!blocked.casts);}scenarioManager={};scenarioManager.scenario.identity=1066;scenarioManager.scenario.step=6;world.instanced=true;++cases;
 }
}
{Player p;Creature bubble;p.objective=0;assert(!p.canSeeSpellClickOn(&bubble));p.objective=1;assert(Refresh(&bubble)&&p.canSeeSpellClickOn(&bubble)&&(Serialize(&p,&bubble)&UNIT_NPC_FLAG_SPELLCLICK));p.status=0;assert(Refresh(&bubble)&&!(Serialize(&p,&bubble)&UNIT_NPC_FLAG_SPELLCLICK));++cases;}
{Unit pet;Creature bubble;assert(!bubble.HandleSpellClick(&pet,-1));++cases;}
{Player p;Creature bubble;p.owned=false;p.bank=true;p.objective=0;assert(!p.canSeeSpellClickOn(&bubble)&&!bubble.HandleSpellClick(&p,-1));++cases;}
std::cout<<"PASS "<<cases<<" actual native Condition branches/ElseGroups/source18 lookup, Quest.ObjectID->StorageIndex, canSeeSpellClickOn, Unit::HandleSpellClick pre-cast admission, quest refresh selection and serialized dynamicNPC flag: both factions/incomplete+complete and genuinely acquired banked artifact; rejects premature/no ownership/aura/abandoned/rewarded/wrong-class/map/pet/instance/step, including bank-only before real pickup. Transfer/effect/group/network/client services remain separate.\n";
}

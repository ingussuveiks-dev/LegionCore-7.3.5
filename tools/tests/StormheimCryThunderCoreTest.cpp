#include <array>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "CryCoreFixture.inc"
#include "Talked.inc"
void Unit::NativeDeath(Creature* creature){
#include "NativeDeath.inc"
}
struct Receiver:SmartAI {Player* expected;unsigned calls=0;Receiver(Creature* c,Player* p):SmartAI(c),expected(p){}void JustDied(Unit* killer)override{assert(!me->IsAlive()&&killer->GetCharmerOrOwnerPlayerOrPlayerItself()==expected);++calls;}};
constexpr uint32 MAX_SPELL_CONTROL_BAR=10,CREATURE_MAX_SPELLS=8;
#define MAKE_UNIT_ACTION_BUTTON(id,slot) ((id)|((slot)<<24))
using ConditionList=std::vector<int>;
struct VehicleSpellStore {std::map<uint32,SpellInfo> spells{{183058,{183058}},{183065,{183065}},{183042,{183042}},{183471,{183471}}};SpellInfo const* GetSpellInfo(uint32 id){auto it=spells.find(id);return it==spells.end()?nullptr:&it->second;}} vehicleSpellStore;auto sSpellMgr=&vehicleSpellStore;
struct VehicleConditions {bool allow=true;ConditionList GetConditionsForVehicleSpell(uint32,uint32){return {};}bool IsObjectMeetToConditions(Player*,Creature*,ConditionList const&){return allow;}} vehicleConditions;auto sConditionMgr=&vehicleConditions;
struct VehicleUIPlayer:Player {std::array<uint32,10> Bar(Creature* vehicle){struct {std::array<uint32,10> Buttons{};}spellsMessage;
#include "VehicleBar.inc"
 return spellsMessage.Buttons;}};
int main(){
 auto& q=store.quests[38624];q.id=38624;q.special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_SPEAKTO;q.objectives={{3,91767,1,0,0},{0,92360,150,2,3},{0,91768,1,2,1}};
 auto& other=store.quests[55555];other=q;other.id=55555;
 Player p;p.guid={700};p.guid.high=HighGuid::Player;p.Accept(38624);p.Accept(55555);players[p.guid]=&p;
 for(uint32 entry:{91767u,92360u,91768u}){p.KilledMonsterCredit(entry);assert(!p.GetQuestObjectiveData(38624,entry));assert(p.GetQuestObjectiveData(55555,entry)==1);}
 p.Accept(55555);for(uint32 entry:{91767u,92360u,91768u}){p.TalkedToCreature(entry,{42});assert(!p.GetQuestObjectiveData(38624,entry));assert(p.GetQuestObjectiveData(55555,entry)==1);}
 Creature enemy;enemy.data.Entry=92367;enemy.data.KillCredit[0]=92360;enemy.guid={42};enemy.alive=false;creatures[enemy.guid]=&enemy;
 p.Accept(55555);p.KilledMonster(&enemy.data,enemy.guid);assert(!p.GetQuestObjectiveData(38624,92360)&&p.GetQuestObjectiveData(55555,92360)==1);
 Creature vehicle;vehicle.guid={400};vehicle.guid.high=HighGuid::Vehicle;vehicle.ownerPlayer=&p;Unit pet;pet.ownerPlayer=&p;Receiver ai(&enemy,&p);vehicle.NativeDeath(&enemy);pet.NativeDeath(&enemy);p.NativeDeath(&enemy);assert(ai.calls==3);enemy.IsAIEnabled=false;vehicle.NativeDeath(&enemy);assert(ai.calls==3);
 p.data.clear();p.SetQuestObjectiveData(&q,&q.objectives[0],1);assert(!p.CanCompleteQuest(38624));p.SetQuestObjectiveData(&q,&q.objectives[1],149);assert(!p.CanCompleteQuest(38624));p.SetQuestObjectiveData(&q,&q.objectives[1],150);assert(!p.CanCompleteQuest(38624));p.SetQuestObjectiveData(&q,&q.objectives[2],1);assert(p.CanCompleteQuest(38624));
 VehicleUIPlayer ui;vehicle.entry=92302;vehicle.m_templateSpells[0]=183058;vehicle.m_templateSpells[1]=183065;vehicle.m_templateSpells[2]=183042;vehicle.m_templateSpells[3]=183471;auto bar=ui.Bar(&vehicle);assert((bar[0]&0xFFFFFF)==183058&&(bar[1]&0xFFFFFF)==183065&&(bar[2]&0xFFFFFF)==183042&&(bar[3]&0xFFFFFF)==183471&&(bar[4]&0xFFFFFF)==0);vehicleConditions.allow=false;bar=ui.Bar(&vehicle);assert((bar[1]&0xFFFFFF)==0);
 std::cout<<"PASS: actual VehicleSpellInitialize button loop exposes all four per-actor spells, including summon183065, and respects native conditions.\n";
 std::cout<<"PASS: real generic kill/talk/alias handlers block only38624 bypasses; ordinary matching quests preserved. Native death-to-AI dispatch and real vehicle/pet/player owner resolution; native completion needs begin+150+landing.\n";
}

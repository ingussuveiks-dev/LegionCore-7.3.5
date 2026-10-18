#include "CallFixture.inc"
constexpr unsigned INVALIDREASON_QUEST_FAILED_LOW_LEVEL=99;
#include "CallLevel.inc"
#include "CallNextChain.inc"
#include "CallPrevChain.inc"
void Player::AddQuestAndCheckCompletion(Quest const* quest,WorldObject* questGiver){
#include "CallAccept.inc"
}
int main(){
 auto& q=store.quests[44721];q.id=44721;q.special=QUEST_SPECIAL_FLAGS_KILL;
 uint32 ids[]={114964,115291,114957,114972};
 for(int i=0;i<4;++i){QuestObjective o;o.Type=QUEST_OBJECTIVE_MONSTER;o.ObjectID=ids[i];o.Amount=i==1?4:1;o.StorageIndex=i;o.Flags=(i==1||i==3)?2:0;q.objectives.push_back(o);}
 for(auto faction:{ALLIANCE,HORDE})for(bool bossFirst:{false,true}){
  Player p;p.team=faction;p.Accept(44721);assert(!p.CanCompleteQuest(44721));
  p.KilledMonsterCredit(114964);assert(!p.CanCompleteQuest(44721));
  if(bossFirst)p.KilledMonsterCredit(114957);
  for(int i=0;i<3;++i){p.KilledMonsterCredit(115291);assert(!p.CanCompleteQuest(44721));}
  p.KilledMonsterCredit(115291);if(!bossFirst)p.KilledMonsterCredit(114957);
  assert(!p.CanCompleteQuest(44721));Player nearby;nearby.Accept(44721);GroupReference peer{&nearby};Group party;party.first=&peer;p.group=&party;
  p.KilledMonsterCredit(114972);assert(p.GetQuestStatus(44721)==QUEST_STATUS_COMPLETE&&nearby.GetQuestObjectiveData(&q,3)==0);
  p.KilledMonsterCredit(114972);assert(p.GetQuestObjectiveData(&q,3)==1&&p.GetQuestObjectiveData(&q,1)==4);
 }
 std::cout<<"PASS: native four-objective KilledMonsterCredit/CanCompleteQuest, both factions, boss before/after four Soulthieves, no early completion, personal finale and native1/4/1/1 clamp. Persistence and pet-owner resolution are outside this fixture.\n";
}

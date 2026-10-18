#include "CallFixture.inc"
constexpr unsigned INVALIDREASON_QUEST_FAILED_LOW_LEVEL=99;
#include "CallLevel.inc"
#include "CallNextChain.inc"
#include "CallPrevChain.inc"
void Player::AddQuestAndCheckCompletion(Quest const* quest,WorldObject* questGiver){
#include "CallAccept.inc"
}
int main(){
 auto& q=store.quests[44771];q.id=44771;q.special=QUEST_SPECIAL_FLAGS_KILL;
 QuestObjective o;o.Type=QUEST_OBJECTIVE_MONSTER;o.ObjectID=114962;o.Amount=1;o.StorageIndex=0;q.objectives.push_back(o);
 Creature havi;havi.entry=92539;havi.guid={291280};creatures[havi.guid]=&havi;
 for(auto faction:{ALLIANCE,HORDE}){
  Player p;p.team=faction;p.Accept(44771);
  p.KilledMonsterCredit(114962,havi.guid);assert(p.GetQuestStatus(44771)==QUEST_STATUS_INCOMPLETE); // GUID substitution regression
  Player nearby;nearby.Accept(44771);GroupReference peer{&nearby};Group party;party.first=&peer;p.group=&party;
  p.KilledMonsterCredit(114962);assert(p.GetQuestStatus(44771)==QUEST_STATUS_COMPLETE&&nearby.GetQuestStatus(44771)==QUEST_STATUS_INCOMPLETE);
  p.KilledMonsterCredit(114962);assert(p.GetQuestObjectiveData(&q,0)==1);
 }
 std::cout<<"PASS: native KilledMonsterCredit/CanCompleteQuest, both factions, virtual114962 versus Havi GUID substitution, personal party credit and1/1 clamp. Persistence/world services are adapters.\n";
}

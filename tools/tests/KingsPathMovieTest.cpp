#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
enum QuestStatus{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
constexpr int CONDITION_QUESTTAKEN=9,CONDITION_QUEST_OBJECTIVE_DONE=41;
struct Player;
struct Unit;
struct Threat {Unit* getHostilTarget(){return nullptr;}};
struct Unit {bool IsCreature(){return false;}bool isPet(){return false;}
 Unit* ToUnit(){return this;}Unit* GetOwner(){return nullptr;}
 Threat threat;Threat& getThreatManager(){return threat;}virtual Player* ToPlayer(){return nullptr;}};
struct Player:Unit
{
 QuestStatus status=QUEST_STATUS_NONE;uint32 count=0;
 Player* ToPlayer()override{return this;}
 QuestStatus GetQuestStatus(uint32 id){assert(id==46282);return status;}
 uint32 GetQuestObjectiveData(uint32 quest,uint32 object){assert(quest==46282&&object==119426);return count;}
};
bool NativeCondition(Player* object,int type,uint32 ConditionValue1,uint32 ConditionValue2,uint32 ConditionValue3)
{
 bool condMeets=false;
 switch(type){
#include "QuestTaken.inc"
#include "Objective.inc"
 }
 return condMeets;
}
bool Admitted(Player& p)
{
 return NativeCondition(&p,9,46282,0,0)&&!NativeCondition(&p,41,46282,119426,1);
}
int main()
{
 Player maiko;assert(!Admitted(maiko)); // Ritual Ruination alone gives no King's Path status.
 for(auto status:{QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){maiko.status=status;assert(!Admitted(maiko));}
 Player legitimate;legitimate.status=QUEST_STATUS_INCOMPLETE;assert(Admitted(legitimate));
 legitimate.count=1;assert(!Admitted(legitimate)); // Native 239912 credits 119426.
 Player relog=legitimate;assert(!Admitted(relog));
 legitimate.status=QUEST_STATUS_NONE;legitimate.count=0;assert(!Admitted(legitimate));
 legitimate.status=QUEST_STATUS_INCOMPLETE;assert(Admitted(legitimate)); // Abandon/retry.
 std::cout<<"PASS: native quest conditions exclude unrelated, completed/rewarded, replay and relog; allow real first use/retry\n";
}

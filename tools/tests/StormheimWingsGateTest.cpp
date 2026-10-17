#include "WingsGateFixture.inc"
constexpr uint32 CONDITION_QUESTREWARDED=8,CONDITION_SOURCE_TYPE_QUEST_ACCEPT=19;
struct ConditionSourceInfo {WorldObject* object;explicit ConditionSourceInfo(WorldObject* a,WorldObject* = nullptr):object(a){}};
struct Condition {uint32 ElseGroup=0,ConditionType=8,ConditionValue1=0,ReferenceId=0;bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo& info){WorldObject* object=info.object;bool condMeets=false;switch(ConditionType){
#include "RewardedCondition.inc"
}return condMeets;}};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {ConditionReferenceContainer ConditionReferenceStore;std::map<uint32,ConditionList> gates;
 ConditionList GetConditionsForNotGroupedEntry(uint32 source,uint32 id){return id==38413?gates[source]:ConditionList{};}
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(WorldObject*,ConditionList const&);
 bool IsObjectMeetToConditions(WorldObject*,WorldObject*,ConditionList const&);
 bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const;
} conditionMgr;auto sConditionMgr=&conditionMgr;
#include "ConditionGroups.inc"
#include "QuestConditionsGate.inc"

int main(int argc,char** argv)
{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::vector<std::unique_ptr<Condition>> conditions;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>q.PrevQuestID>>q.NextQuestID;}
  if(kind=="gate"){uint32 source,target,negative;auto c=std::make_unique<Condition>();in>>source>>c->ElseGroup>>c->ConditionType>>target>>c->ConditionValue1>>negative;assert(c->ConditionType==8&&!target&&!negative);conditionMgr.gates[source].push_back(c.get());conditions.push_back(std::move(c));}
  if(kind=="faction"){uint32 id;in>>id;auto& q=store.quests[id];q.id=id;in>>q.AllowableRaces;}
 }
 auto& q=store.quests[38413];assert(conditions.size()==4&&!q.PrevQuestID);store.quests[39652].id=39652;
 // Reproduce the original single-predecessor failure with the real dispatcher.
 Player oldHorde;oldHorde.team=HORDE;oldHorde.race=2;oldHorde.m_RewardedQuests.insert(38618);q.prevQuests={38412};assert(!oldHorde.CanTakeQuest(&q,false));q.prevQuests.clear();
 for(auto team:{ALLIANCE,HORDE}){Player p;p.team=team;p.race=team==ALLIANCE?1:2;uint32 previous=team==ALLIANCE?38412:38618;
  assert(p.SatisfyQuestRace(&q,false)&&p.SatisfyQuestRace(&store.quests[previous],false));
  assert(!p.CanTakeQuest(&q,false)&&!conditionMgr.IsObjectMeetToConditions(&p,conditionMgr.gates[20]));
  p.Accept(previous);p.quests[previous].Status=QUEST_STATUS_COMPLETE;assert(!p.CanTakeQuest(&q,false));p.quests.erase(previous);p.m_RewardedQuests.insert(previous);
  assert(p.CanTakeQuest(&q,false)&&conditionMgr.IsObjectMeetToConditions(&p,conditionMgr.gates[20]));
  p.Accept(q.id);assert(!p.CanTakeQuest(&q,false));p.quests.erase(q.id);p.m_RewardedQuests.insert(q.id);assert(!p.CanTakeQuest(&q,false));
 }
 std::cout<<"PASS: native CanTakeQuest/status/race/previous/condition dispatch and ConditionMgr ElseGroup OR with actual reward predicate; original Horde rejection reproduced, both correct rewarded predecessors admit acceptance/markers, merely complete/active/rewarded boundaries preserved. World registries and other class/level gates are fixtures.\n";
}

#include <array>
#include <algorithm>
#include <sstream>
#include "ShieldsFixture.inc"
#include "HasQuestForGO.inc"
#include "ShieldsProduction.inc"
enum {CONDITION_OBJECT_ENTRY=31,DEFAULT_GOSSIP_MESSAGE=68};
struct ConditionSourceInfo {WorldObject* targets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b=nullptr):targets{a,b}{}};
struct Condition {uint32 ConditionType=31,ConditionTarget=1,ConditionValue1=5,ConditionValue2=0,ConditionValue3=0,ElseGroup=0,ReferenceId=0;bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo& info){auto object=info.targets[ConditionTarget];bool condMeets=false;switch(ConditionType){
#include "ShieldsObjectCondition.inc"
}return condMeets;}};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(WorldObject*,WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const;} conditions;auto sConditionMgr=&conditions;
#include "ShieldsConditionGroups.inc"
struct MenuRow {uint32 TextID;ConditionList Conditions;};
struct GossipStore {std::multimap<uint32,MenuRow> menus;auto GetGossipMenusMapBounds(uint32 id){return menus.equal_range(id);}} gossip;auto sGossipDataStore=&gossip;
#include "ShieldsText.inc"
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::map<uint32,GameObject> shields;std::map<uint32,Condition> gates;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,flagsEx,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>flagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 qid,id;QuestObjective o;int index;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[qid].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
  if(kind=="go"){uint32 id;in>>id;auto& go=shields[id];go.entry=go.info.entry=id;uint32 display,flags;float size;uint32 lock,close,cooldown;std::string script;in>>go.info.type>>display>>flags>>size>>lock>>go.info.goober.questID>>go.info.goober.eventID>>close>>cooldown>>go.info.goober.spell>>go.info.goober.gossipID>>script;assert(!lock&&!cooldown&&script=="go_stormheim_engraved_shield");}
  if(kind=="spawn"){uint64 guid;uint32 entry;in>>guid>>entry;shields[entry].dbGuid=guid;}
  if(kind=="condition"){uint32 text;in>>text;auto& c=gates[text];in>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3;}
 }
 auto& q=store.quests[39063];auto& travel=store.quests[39062];auto& next=store.quests[39092];q.special|=QUEST_SPECIAL_FLAGS_CAST|QUEST_SPECIAL_FLAGS_KILL;assert(q.objectives.size()==7&&shields.size()==6&&gates.size()==6);
 for(auto id:{39060u,39061u,39472u}){auto& prev=store.quests[id];prev.id=id;prev.NextQuestID=39062;}
 QuestMap _questTemplates;for(auto& r:store.quests)_questTemplates[r.first]=&r.second;for(auto const& r:_questTemplates){auto qinfo=r.second;
#include "QuestPrevLoader.inc"
 }
 for(auto const& c:gates)gossip.menus.emplace(18581,MenuRow{c.first,{}});
 Player old;for(auto& row:shields)assert(old.GetGossipTextId(18581,&row.second)==26935); // Original unconditional last row on every shield.
 for(auto& row:gossip.menus)row.second.Conditions={&gates.at(row.second.TextID)};
 std::vector<std::unique_ptr<GameObjectAI>> ais;go_stormheim_engraved_shield script;
 for(auto& row:shields){row.second.scripted=script.GetAI(&row.second);ais.emplace_back(row.second.scripted);assert(old.GetGossipTextId(18581,&row.second)==26930+unsigned(std::distance(shields.begin(),shields.find(row.first))));}
 Player travelPlayer;travelPlayer.race=1;travelPlayer.m_RewardedQuests.insert(39472);assert(travelPlayer.CanTakeQuest(&travel,false));travelPlayer.Accept(39062);assert(travelPlayer.CanCompleteQuest(39062)&&!travelPlayer.CanTakeQuest(&q,false));travelPlayer.quests.clear();travelPlayer.m_RewardedQuests.insert(39062);assert(travelPlayer.CanTakeQuest(&q,false));
 Player horde;horde.race=2;horde.m_RewardedQuests.insert(39062);assert(!horde.CanTakeQuest(&q,false));
 {Player reader,near;reader.Accept(39063);near.Accept(39063);GroupReference b{&near,nullptr},a{&reader,&b};Group group;group.first=&a;reader.group=&group;
  for(unsigned n=0;n<6;++n)reader.RewardPlayerAndGroupAtEvent(109346,&reader);
  assert(reader.GetQuestObjectiveData(39063,109346)==6&&near.GetQuestObjectiveData(39063,109346)==6&&!reader.CanCompleteQuest(39063));
 } // Original SmartAI33 repeats/group-sharing are reproduced with native event reward.
 std::array<uint32,6> order{242673,251557,251558,251559,251560,251561};unsigned permutations=0;
 do{Player p;p.race=1;p.m_RewardedQuests.insert(39062);p.Accept(39063);unsigned n=0;
  for(auto id:order){auto& go=shields[id];go.flags=262144;assert(p.HasQuestForGO(id));go.Use(&p);++n;assert(p.GetQuestObjectiveData(39063,id)==1&&p.GetQuestObjectiveData(39063,109346)==n&&p.completions==unsigned(n==6));auto packets=p.packets;
   go.Use(&p);go.flags=262144;go.Use(&p);assert(p.packets==packets);if(n<6)assert(!p.CanRewardQuest(&q,false));
  }assert(p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&next,false));p.quests.clear();p.m_RewardedQuests.insert(39063);assert(p.CanTakeQuest(&next,false));Creature lorna;lorna.entry=95921;lorna.guid={338652};creatures[lorna.guid]=&lorna;assert(p.GetNextQuest(lorna.guid,&q)==&next);++permutations;
 }while(std::next_permutation(order.begin(),order.end()));assert(permutations==720);
 {Player reader,near;reader.Accept(39063);near.Accept(39063);GroupReference b{&near,nullptr},a{&reader,&b};Group group;group.first=&a;reader.group=&group;
  auto& go=shields[242673];go.flags=0;go.Use(&reader);assert(reader.GetQuestObjectiveData(39063,109346)==1&&!near.packets);for(unsigned n=0;n<6;++n){go.flags=0;go.Use(&reader);}assert(reader.GetQuestObjectiveData(39063,109346)==1&&!reader.completions);
  Player saved;saved.quests=reader.quests;saved.data=reader.data;go.flags=0;go.Use(&saved);assert(saved.GetQuestObjectiveData(39063,109346)==1);
  reader.quests.clear();reader.Accept(39063);go.flags=0;go.Use(&reader);assert(reader.GetQuestObjectiveData(39063,109346)==1);reader.phase=false;auto& other=shields[251557];other.flags=0;other.Use(&reader);assert(!reader.GetQuestObjectiveData(39063,251557));reader.phase=true;other.distance=6;other.Use(&reader);assert(!reader.GetQuestObjectiveData(39063,251557));other.distance=0;reader.los=false;other.Use(&reader);assert(reader.GetQuestObjectiveData(39063,251557)==1); // Native book Use has no GO-origin LOS gate.
 }
 {Player p;p.Accept(39063);for(unsigned n=0;n<6;++n)p.KilledMonsterCredit(109346);assert(!p.CanCompleteQuest(39063));} // Six distinct GO objectives remain mandatory.
 std::cout<<"PASS: extracted shield AI + native goober/quest/group/text/condition handlers; original all-last-text failure reproduced, six different stories and all720 reading orders, repeat/busy/group/partial/abandon/range/phase boundaries and native no-origin-LOS behavior;39062 delivery and39092 reward handoff. World/cast/gossip packets and stored login state remain fixtures; client pending.\n";
}

#include <array>
#include <algorithm>
#include <sstream>
#include "RunesBindFixture.inc"
#include "HasQuestForGO.inc"

enum {GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_GENERIC=5,GAMEOBJECT_TYPE_SPELL_FOCUS=8,BG_AV_OBJECTID_MINE_N=1,BG_AV_OBJECTID_MINE_S=2};
namespace MS{namespace Battlegrounds{namespace BattlegroundTypeId{enum{BattlegroundAlteracValley=1};}}}
struct BattlegroundAlteracValley:Battleground {bool PlayerCanDoMineQuest(uint32,uint32){assert(false);return false;}};
struct QuestLoot {bool HaveQuestLootForPlayer(uint32,Player*){assert(false);return false;}} LootTemplates_Gameobject;
#include "ActivateToQuest.inc"

int main(int argc,char** argv)
{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;
 std::map<uint32,GameObject> stones;
 for(auto id:{38816u,38818u})store.quests[id].id=id;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags;uint32 flagsEx;in>>flagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;assert(!flagsEx&&!start&&!q.ExclusiveGroup&&!q.special);}
  if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[id].objectives.push_back(o);}
  if(kind=="next-starter"){uint32 id,q;in>>id>>q;store.starters.emplace(id,q);}
  if(kind=="go"){uint32 id,type,display,flags;float size;in>>id>>type>>display>>flags>>size;if(type!=10)continue;auto& go=stones[id];go.entry=id;go.info.entry=id;go.info.type=type;go.flags=flags;uint32 lock,close,openText,multi,condition,noAnim,kit;in>>lock>>go.info.goober.questID>>go.info.goober.eventID>>close>>go.info.goober.pageID>>go.info.goober.spell>>openText>>go.info.goober.gossipID>>multi>>condition>>noAnim>>kit;assert(lock==1691&&go.info.goober.questID==38823&&!go.info.goober.eventID&&close==3000&&openText==20138&&!multi&&!condition&&!noAnim);}
 }
 auto& q=store.quests[38823];auto& next=store.quests[38815];assert(q.objectives.size()==4&&stones.size()==3);q.special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;
 QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;
 for(auto const& row:_questTemplates){auto qinfo=row.second;
  #include "QuestPrevLoader.inc"
 }
 Creature ashildir;ashildir.entry=97270;ashildir.guid={338670};creatures[ashildir.guid]=&ashildir;
 for(auto team:{ALLIANCE,HORDE}){
  std::array<uint32,3> order{241849,251412,251413};unsigned orders=0;
  do{
   Player p;p.team=team;p.race=team==ALLIANCE?1:2;assert(p.CanTakeQuest(&q,false)&&!p.CanTakeQuest(&next,false));
   for(auto& row:stones){row.second.flags=262148;assert(!row.second.ActivateToQuest(&p));row.second.Use(&p);assert(!p.packets);}
   p.Accept(q.id);assert(!p.CanCompleteQuest(q.id));unsigned n=0;
   for(auto id:order){auto& go=stones[id];go.flags=262148;assert(go.ActivateToQuest(&p)&&p.HasQuestForGO(id));
    Spell opening;opening.m_caster=&p;SkillType skill=0;int32 required=0,value=0;
    assert(opening.CanOpenLock(0,1690,skill,required,value)!=SPELL_CAST_OK);assert(p.packets==n);
    assert(opening.CanOpenLock(0,1691,skill,required,value)==SPELL_CAST_OK);go.Use(&p);++n;
    assert(p.GetQuestObjectiveData(q.id,id)==1&&p.packets==n&&p.completions==(n==3?1:0));
    go.Use(&p);go.flags=262148;go.Use(&p);assert(p.packets==n); // Busy or repeated click cannot replace another distinct stone.
    if(n<3)assert(!p.CanRewardQuest(&q,false)&&!p.CanCompleteQuest(q.id));
   }
   assert(p.GetQuestObjectiveData(q.id,109061)==0&&p.CanRewardQuest(&q,false)); // Flags28 counter is optional, not an invented fourth gate.
   for(auto& row:stones)assert(!row.second.ActivateToQuest(&p));
   assert(!p.CanTakeQuest(&next,false));p.quests.erase(q.id);p.m_RewardedQuests.insert(q.id);assert(p.CanTakeQuest(&next,false));assert(p.GetNextQuest(ashildir.GetGUID(),&q)==&next);
   Player reentry;reentry.quests=p.quests;reentry.m_RewardedQuests=p.m_RewardedQuests;assert(!reentry.CanCompleteQuest(q.id)&&reentry.CanTakeQuest(&next,false));++orders;
  }while(std::next_permutation(order.begin(),order.end()));assert(orders==6);
  Player retry;retry.team=team;retry.race=team==ALLIANCE?1:2;retry.Accept(q.id);auto& first=stones[241849];first.flags=262148;first.Use(&retry);assert(retry.packets==1&&!retry.CanCompleteQuest(q.id));
  Player partial;partial.quests=retry.quests;partial.data=retry.data;assert(!partial.CanCompleteQuest(q.id)&&!partial.HasQuestForGO(241849)&&partial.HasQuestForGO(251412));
  retry.quests.clear();retry.data.clear();first.flags=262148;first.Use(&retry);assert(retry.packets==1);retry.Accept(q.id);assert(retry.HasQuestForGO(241849));
  Unit pet;pet.ownerPlayer=&retry;first.flags=262148;first.Use(&pet);assert(retry.packets==1);first.flags=262148;first.Use(&retry);assert(retry.packets==2);
  Player counterOnly;counterOnly.Accept(q.id);for(unsigned n=0;n<3;++n)counterOnly.KilledMonsterCredit(109061);
  assert(counterOnly.GetQuestObjectiveData(q.id,109061)==3&&!counterOnly.CanCompleteQuest(q.id));
  for(auto const& row:stones)assert(!counterOnly.GetQuestObjectiveData(q.id,row.first));
 }
 std::cout<<"PASS: both factions and all6 orders; native HasQuestForGO/ActivateToQuest, lock1691 and full goober branch, three distinct credits exactly once, busy/failed-lock/pet/no-quest/abandon/retry/partial-history boundaries, optional109061 counter and Ashildir38815 handoff. Cast scheduling, world/quest menu services and personal GO lifecycle/rendering remain fixtures; client pending.\n";
}

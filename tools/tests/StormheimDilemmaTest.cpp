#include "DilemmaFixture.inc"
#include "NativeDilemmaGetters.inc"
enum {QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE=99,QUEST_DEFAULT_SAVE_TYPE=1};
struct CriteriaTree{};struct CriteriaMgr {CriteriaTree const* GetCriteriaTree(uint32){return nullptr;}} criteriaMgr;auto sAchievementMgr=&criteriaMgr;
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple {uint32 QuestID=0,ObjectID=0,ObjectiveType=0;int Write(){return 0;}};}}
#include "NativeDilemmaSetter.inc"
#include "NativeRewardPlayer.inc"
struct Source {uint32 entry,credit1,credit2,faction,npc,flags,flags2;};
std::map<uint32,Source> sources;
void Victim(Creature& victim,uint32 entry,uint64 serial){auto s=sources.at(entry);victim.entry=entry;victim.guid={serial};victim.data.Entry=entry;victim.data.KillCredit[0]=s.credit1;victim.data.KillCredit[1]=s.credit2;creatures[victim.guid]=&victim;}
void Kill(Player& p,uint32 entry,bool petKill,uint64 serial){
 Creature victim;Victim(victim,entry,serial);Unit pet;pet.ownerPlayer=&p;auto resolved=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&victim);assert(resolved.first==&p&&resolved.second);
 auto packets=p.packets;KillRewarder reward;reward._victim=&victim;reward._RewardPlayer(&p,false);assert(p.packets==packets&&!p.events.empty()&&p.delays.back()==50);Deliver(p);
 assert(victim.IsPlayerRewarded(p.GetGUID()));reward._RewardPlayer(&p,false);assert(p.events.empty());creatures.erase(victim.guid);
}
void Init(Player& p,unsigned id=100){p.team=HORDE;p.race=2;p.guid={id};}
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,ex,item,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>item>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 id,qid;QuestObjective o;int slot;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>slot>>o.Flags;o.StorageIndex=int8(slot);store.quests[qid].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
  if(kind=="actor"){Source s;in>>s.entry>>s.credit1>>s.credit2>>s.faction>>s.npc>>s.flags>>s.flags2;sources[s.entry]=s;}
 }
 auto& q=store.quests[39153];auto& travel=store.quests[39154];q.special|=QUEST_SPECIAL_FLAGS_KILL;assert(q.objectives.size()==1&&q.objectives[0].StorageIndex==2&&q.objectives[0].Amount==10&&sources.size()==5);
 QuestMap _questTemplates;for(auto& r:store.quests)_questTemplates[r.first]=&r.second;for(auto& r:_questTemplates){auto qinfo=r.second;
#include "QuestPrevLoader.inc"
 }
 Creature giver;giver.entry=93603;giver.guid={293893};giver.fields[UNIT_FIELD_NPC_FLAGS]=3;creatures[giver.guid]=&giver;SmartAI giverAI(&giver);
 for(auto const& row:sources){auto s=row.second;assert(!(s.flags&0x02000382)&&!s.credit2);assert((s.entry==94614&&!s.credit1)||(s.entry!=94614&&s.credit1==94614));}
 // One native acceptance/kill/reward-request run per template and killer type.
 unsigned runs=0;for(bool petKill:{false,true})for(auto const& row:sources){Player p;Init(p,100+runs);assert(!p.CanTakeQuest(&q,false));p.Accept(38872);p.CompleteQuest(38872);assert(!p.CanTakeQuest(&q,false));p.quests.clear();p.completions=0;p.m_RewardedQuests.insert(38872);assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&giver);assert(!p.CanCompleteQuest(39153));
  WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{giver.guid,39153};session.HandleQuestGiverRequestReward(request);assert(!p.menu.offers);
  for(unsigned n=1;n<=10;++n){Kill(p,row.first,petKill,10000+100*runs+n);assert(p.GetQuestObjectiveData(39153,94614)==int(n)&&p.packets==n&&p.completions==unsigned(n==10));assert(p.m_QuestStatusSave.count(39153)&&p.slotCounters.at(2)==int(n));assert(!p.quests[39153].ObjectiveData.count(0)&&!p.quests[39153].ObjectiveData.count(1));if(n<10)assert(!p.CanRewardQuest(&q,false));}
  assert(p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&travel,false));Kill(p,row.first,petKill,50000+runs);assert(p.packets==10&&p.completions==1);session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=false;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=true;
  p.quests.clear();p.m_RewardedQuests.insert(39153);assert(p.CanTakeQuest(&travel,false)&&p.GetNextQuest(giver.guid,&q)==&travel);assert(!p.CanTakeQuest(&q,false));++runs;
 }assert(runs==10);
 {Player p;Init(p);p.Accept(39153);unsigned n=0;for(int cycle=0;cycle<2;++cycle)for(auto const& row:sources){Kill(p,row.first,true,60000+n);++n;assert(p.GetQuestObjectiveData(39153,94614)==int(n));}assert(p.completions==1);}
 {Player p;Init(p);p.Accept(39153);p.SetQuestObjectiveData(&q,&q.objectives[0],9,true);assert(!p.m_QuestStatusSave.count(39153)&&p.slotCounters.at(2)==9);Kill(p,109635,true,61000);assert(p.GetQuestStatus(39153)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(39153,94614)==10&&p.m_QuestStatusSave.count(39153));}
 {Player p;Init(p);p.Accept(39153);for(int n=0;n<4;++n)Kill(p,93860,false,62000+n);p.quests.clear();p.Accept(39153);assert(!p.GetQuestObjectiveData(39153,94614));for(int n=0;n<10;++n)Kill(p,94614,false,62100+n);assert(p.completions==1);}
 {Player p;Init(p);p.Accept(39153);p.KilledMonsterCredit(95436);p.KilledMonsterCredit(95391);assert(!p.packets);}
 {Player p;Init(p);Creature v;Victim(v,93860,63000);KillRewarder reward;reward._victim=&v;reward._RewardPlayer(&p,false);Deliver(p);assert(!p.packets);v.rewarded.clear();p.Accept(39153);reward._RewardPlayer(&p,false);p.quests.clear();Deliver(p);assert(!p.packets);v.rewarded.clear();p.Accept(39153);reward._RewardPlayer(&p,false);creatures.erase(v.guid);Deliver(p);assert(!p.packets);}
 {Player killer,near,far,released,offline;unsigned id=200;for(auto p:{&killer,&near,&far,&released,&offline}){Init(*p,id++);p->Accept(39153);}far.rewardRange=false;released.alive=false;released.corpse=true;offline.contact=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&killer,&b};Group group;group.first=&a;Creature v;Victim(v,109633,64000);KillRewarder reward;reward._victim=&v;reward._group=&group;reward.SelectGroup();for(auto p:{&killer,&near,&far,&released,&offline})Deliver(*p);assert(killer.packets==1&&near.packets==1&&!far.packets&&!released.packets&&!offline.packets);creatures.erase(v.guid);}
 // All eight rewarded histories; every positive predecessor must be turned in.
 constexpr unsigned alternatives[]{38873,39153,39385};
 for(unsigned mask=0;mask<8;++mask){Player p;Init(p);p.m_RewardedQuests.insert(38872);for(unsigned i=0;i<3;++i)if(mask&(1u<<i))p.m_RewardedQuests.insert(alternatives[i]);assert(p.CanTakeQuest(&travel,false)==bool(mask));if(mask){p.Accept(39154);assert(p.CanCompleteQuest(39154)&&!p.CanTakeQuest(&store.quests[38878],false));p.quests.clear();p.m_RewardedQuests.insert(39154);assert(p.CanTakeQuest(&store.quests[38878],false));}}
 for(auto previous:alternatives)for(int status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){Player p;Init(p);p.Accept(previous);p.quests[previous].Status=status;assert(!p.CanTakeQuest(&travel,false));}
 unsigned horde=0;for(unsigned bit=0;bit<32;++bit){Player p;Init(p);p.race=uint64(1)<<bit;p.m_RewardedQuests.insert(38872);auto allowed=bool(q.AllowableRaces&p.race);assert(p.CanTakeQuest(&q,false)==allowed);if(allowed)++horde;}assert(horde==9);
 std::cout<<"PASS:39153 native owner/pet reward, five template credit routes, ten-kill cap and per-victim admission, native slot2 getters/setter/save marking and counter update, saved9/abandon/group/range/contact boundaries and Nathanos reward request; all eight39154 rewarded histories across38873/39153/39385 and38878 handoff. World/quest-slot fields/packets/storage loading are fixtures; client pending.\n";
}

#include <array>
#include "MistsFixture.inc"
#include "NativeDilemmaGetters.inc"
enum {QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE=99,QUEST_DEFAULT_SAVE_TYPE=1};
struct CriteriaTree{};struct CriteriaMgr{CriteriaTree const* GetCriteriaTree(uint32){return nullptr;}} criteriaMgr;auto sAchievementMgr=&criteriaMgr;
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple{uint32 QuestID=0,ObjectID=0,ObjectiveType=0;int Write(){return 0;}};}}
#include "NativeDilemmaSetter.inc"
#include "NativeRewardPlayer.inc"
struct Source{uint32 entry,c1,c2,faction,npc,flags,flags2;};std::map<uint32,Source> sources;uint64 serial=100000;
void Victim(Creature& c,uint32 entry){auto d=sources.at(entry);c.entry=entry;c.guid={serial++};c.data.Entry=entry;c.data.KillCredit[0]=d.c1;c.data.KillCredit[1]=d.c2;creatures[c.guid]=&c;}
void Kill(Player& p,uint32 entry,bool petKill){Creature c;Victim(c,entry);Unit pet;pet.ownerPlayer=&p;auto killer=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&c);assert(killer.first==&p&&killer.second);KillRewarder reward;reward._victim=&c;reward._RewardPlayer(&p,false);assert(!p.events.empty()&&p.delays.back()==50);Deliver(p);assert(c.IsPlayerRewarded(p.GetGUID()));auto packets=p.packets;reward._RewardPlayer(&p,false);Deliver(p);assert(p.packets==packets);creatures.erase(c.guid);}
int main(int argc,char** argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==3);std::ifstream input(argv[1]);assert(input);std::string line;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,item,package,ex;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>item>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;assert(!ex&&package==9687);}
  if(kind=="objective"){uint32 q,id;QuestObjective o;int index;in>>q>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[q].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,q;in>>entry>>q;store.starters.emplace(entry,q);}
  if(kind=="actor"){Source s;in>>s.entry>>s.c1>>s.c2>>s.faction>>s.npc>>s.flags>>s.flags2;sources[s.entry]=s;}
 }
 auto& q=store.quests.at(40044);q.special|=QUEST_SPECIAL_FLAGS_KILL;assert(q.objectives.size()==4&&sources.size()==4);
 for(auto const& r:sources)assert(!r.second.c1&&!r.second.c2&&!(r.second.flags&0x02000382));
 Creature havi;havi.entry=97973;havi.guid={369621};havi.fields[UNIT_FIELD_NPC_FLAGS]=3;creatures[havi.guid]=&havi;SmartAI giverAI(&havi);Creature other;other.entry=98587;other.guid={292435};creatures[other.guid]=&other;
 std::array<uint32,4> order{98014,98015,98016,98018};unsigned runs=0;
 do {for(auto team:{ALLIANCE,HORDE})for(bool petKill:{false,true}){
  Player p;p.guid={200+runs};p.team=team;p.race=team==ALLIANCE?1:2;assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&havi);WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{havi.guid,40044};session.HandleQuestGiverRequestReward(request);assert(!p.menu.offers);
  unsigned n=0;for(auto id:order){Kill(p,id,petKill);++n;assert(p.GetQuestObjectiveData(40044,id)==1&&p.packets==n&&p.completions==unsigned(n==4));auto index=std::find_if(q.objectives.begin(),q.objectives.end(),[&](auto o){return o.ObjectID==id;})->StorageIndex;assert(p.slotCounters.at(index)==1&&p.m_QuestStatusSave.count(40044));if(n<4)assert(!p.CanRewardQuest(&q,false));Kill(p,id,petKill);assert(p.packets==n);}
  assert(p.CanRewardQuest(&q,false));WorldPackets::Quest::QuestGiverRequestReward wrong{other.guid,40044};session.HandleQuestGiverRequestReward(wrong);assert(!p.menu.offers);session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);havi.near=false;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);havi.near=true;
  p.quests.clear();p.m_RewardedQuests.insert(40044);assert(!p.CanTakeQuest(&q,false));Kill(p,98014,petKill);assert(p.packets==4);++runs;
 }}while(std::next_permutation(order.begin(),order.end()));assert(runs==96);
 {Player p;p.Accept(40044);for(unsigned n=0;n<3;++n)p.SetQuestObjectiveData(&q,&q.objectives[n],1,true);assert(!p.CanCompleteQuest(40044)&&!p.m_QuestStatusSave.count(40044));Kill(p,98018,true);assert(p.GetQuestStatus(40044)==QUEST_STATUS_COMPLETE&&p.slotCounters.at(3)==1);}
 {Player p;p.Accept(40044);Kill(p,98014,false);p.quests.clear();p.Accept(40044);for(auto id:order)assert(!p.GetQuestObjectiveData(40044,id));for(auto id:order)Kill(p,id,true);assert(p.CanRewardQuest(&q,false));}
 {Player p;Creature c;Victim(c,98014);KillRewarder reward;reward._victim=&c;reward._RewardPlayer(&p,false);Deliver(p);assert(!p.packets);c.rewarded.clear();p.Accept(40044);reward._RewardPlayer(&p,false);p.quests.clear();Deliver(p);assert(!p.packets);c.rewarded.clear();p.Accept(40044);reward._RewardPlayer(&p,false);creatures.erase(c.guid);Deliver(p);assert(!p.packets);}
 {Player killer,near,far,released,offline;unsigned guid=500;for(auto p:{&killer,&near,&far,&released,&offline}){p->guid={guid++};p->Accept(40044);}far.rewardRange=false;released.alive=false;released.corpse=true;offline.contact=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&killer,&b};Group group;group.first=&a;Creature victim;Victim(victim,98016);KillRewarder reward;reward._victim=&victim;reward._group=&group;reward.SelectGroup();for(auto p:{&killer,&near,&far,&released,&offline})Deliver(*p);assert(killer.packets==1&&near.packets==1&&!far.packets&&!released.packets&&!offline.packets);creatures.erase(victim.guid);}
 std::cout<<"PASS: native40044 player/pet kill reward, all24 orders x both factions x both killer types, four sparse counters/save marks/caps, saved3 recovery, abandon/retry, group range/contact/life and Havi reward admission. World/packet/storage/login services are adapters; client pending.\n";
}

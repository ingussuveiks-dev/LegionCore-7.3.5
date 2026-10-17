#include "GreywatchKillFixture.inc"
#include "NativeRewardPlayer.inc"
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;CreatureTemplate bossData;
 auto& q=store.quests[39472];q.id=39472;unsigned objectives=0;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id;in>>id;if(id!=39472)continue;uint32 flagsEx,start,next,special,package;int exclusive;in>>q.flags>>flagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>exclusive>>special>>package;assert(!q.PrevQuestID&&!start&&package==752);}
  if(kind=="objective"){uint32 id,obj;in>>id;if(id!=39472)continue;QuestObjective o;int index;in>>obj>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(obj==279631&&o.Type==0&&o.ObjectID==93987&&o.Amount==1&&!index&&!o.Flags);q.objectives.push_back(o);++objectives;}
  if(kind=="actor"){uint32 id;in>>id;if(id!=93987)continue;uint32 loot;bossData.Entry=id;in>>loot>>bossData.KillCredit[0]>>bossData.KillCredit[1];assert(loot==93987&&bossData.KillCredit[0]==94338&&!bossData.KillCredit[1]);}
 }
 assert(objectives==1&&bossData.Entry==93987);q.special|=QUEST_SPECIAL_FLAGS_KILL;
 Creature giver;giver.entry=94318;giver.guid={338673};giver.fields[UNIT_FIELD_NPC_FLAGS]=2;creatures[giver.guid]=&giver;SmartAI ai(&giver);
 Player horde;horde.race=2;assert(!horde.CanTakeQuest(&q,false));
 for(bool petKill:{false,true}){Player p;p.race=1;p.guid={100+unsigned(petKill)};assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&giver);assert(!p.CanRewardQuest(&q,false));
  Creature boss;boss.entry=93987;boss.guid={292797};boss.data=bossData;creatures[boss.guid]=&boss;Unit pet;pet.ownerPlayer=&p;
  auto killer=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&boss);assert(killer.first==&p&&killer.second);
  p.KilledMonsterCredit(94338);assert(!p.packets);KillRewarder reward;reward._victim=&boss;reward._RewardPlayer(&p,false);assert(!p.packets&&p.events.size()==1);Deliver(p);
  assert(p.GetQuestObjectiveData(39472,93987)==1&&p.completions==1&&p.CanRewardQuest(&q,false));assert(boss.IsPlayerRewarded(p.guid));reward._RewardPlayer(&p,false);assert(p.events.empty()&&p.packets==1);
  WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{giver.guid,39472};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=false;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=true;
  p.quests.clear();p.Accept(39472);assert(!p.GetQuestObjectiveData(39472,93987));boss.rewarded.clear();reward._RewardPlayer(&p,false);p.quests.clear();Deliver(p);assert(!p.GetQuestObjectiveData(39472,93987));
  creatures.erase(boss.guid);
 }
 {Player p;p.Accept(39472);p.guid={200};Creature boss;boss.entry=93987;boss.guid={292797};boss.data=bossData;creatures[boss.guid]=&boss;KillRewarder reward;reward._victim=&boss;reward._RewardPlayer(&p,false);creatures.erase(boss.guid);Deliver(p);assert(!p.packets);}
 std::cout<<"PASS: Curseheart native player/pet killer resolution, rewarded-victim admission and delayed template entry93987 credit; unrelated94338 substitution cannot finish quest, repeated dispatch, abandon/stale callbacks and Genn reward request boundaries. World/death scheduling/storage and client combat remain fixtures.\n";
}

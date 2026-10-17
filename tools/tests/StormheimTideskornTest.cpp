#include "TideskornFixture.inc"
#include "NativeDilemmaGetters.inc"
struct CriteriaTree{};struct CriteriaMgr{CriteriaTree const* GetCriteriaTree(uint32){return nullptr;}} criteriaMgr;auto sAchievementMgr=&criteriaMgr;
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple{uint32 QuestID=0,ObjectID=0,ObjectiveType=0;int Write(){return 0;}};}}
#include "NativeDilemmaSetter.inc"
#include "NativeRewardPlayer.inc"
#include "TideskornObjective.inc"
#include "TideskornComplete.inc"
#include "TideskornRestore.inc"
struct QuestV2CliTaskEntry{uint32 ConditionID=0,FiltMinSkillID=0,FiltMinSkillValue=0;int FiltMinLevel=-1,FiltMaxLevel=-1;uint32 FiltCompletedQuest[3]{};};
struct TaskStore{QuestV2CliTaskEntry task;QuestV2CliTaskEntry const* LookupEntry(uint32 id){return id==39998?&task:nullptr;}} sQuestV2CliTaskStore;
struct ConditionMgr{bool IsPlayerMeetingCondition(Player*,uint32 id){assert(!id);return true;}} conditionMgr;auto sConditionMgr=&conditionMgr;
// Actual non-world bonus-task block, ending immediately before the world-task config gate.
#include "TideskornActivation.inc"
enum {SPELL_EFFECT_HANDLE_HIT_TARGET=1,GO_JUST_DEACTIVATED=3};
struct Spell{int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Unit* unitTarget=nullptr;SpellInfo* m_spellInfo=nullptr;void EffectKillCredit(SpellEffIndex);};
#include "TideskornSpell.inc"
#include "TideskornSharedHook.inc"
struct BonusGo:GameObject{uint32 admitted=0;GameObjectAI fallback;struct Info{uint32 flags;} metadata;void SetUInt32Value(uint32,uint32 n){flags=n;}void Init(uint32 n){metadata.flags=n;auto goinfo=&metadata;
#include "TideskornGoInit.inc"
 ai=&fallback;}
 void TryUse(Unit* user){
#include "TideskornGoAdmission.inc"
 // Remaining GO activation/cast scheduling is an adapter. Admission and credit handlers above are production code.
 ++admitted;flags|=GO_FLAG_IN_USE;SpellInfo info;info.MiscValue=entry==244704?97947:97949;Spell spell;spell.unitTarget=user;spell.m_spellInfo=&info;spell.EffectKillCredit(0);
 }};
struct Source{uint32 entry,c1,c2,faction,flags,spawns;};std::map<uint32,Source> sources;uint64 serial=100000;
void Victim(Creature& c,uint32 entry){auto d=sources.at(entry);c.entry=entry;c.guid={serial++};c.data.Entry=entry;c.data.KillCredit[0]=d.c1;c.data.KillCredit[1]=d.c2;creatures[c.guid]=&c;}
void Kill(Player& p,uint32 entry,bool petKill){Creature c;Victim(c,entry);Unit pet;pet.ownerPlayer=&p;auto killer=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&c);assert(killer.first==&p&&killer.second);KillRewarder reward;reward._victim=&c;reward._RewardPlayer(&p,false);Deliver(p);auto packets=p.packets;reward._RewardPlayer(&p,false);Deliver(p);assert(p.packets==packets&&c.IsPlayerRewarded(p.GetGUID()));creatures.erase(c.guid);}
void Enter(Player& p){p.m_RewardedQuests.insert(40002);p.UpdateAreaQuestTasks(7643,0);assert(p.FindQuestSlot(39998)<MAX_QUEST_LOG_SIZE);}
float Progress(Player& p,Quest const& q){float n=0;for(auto const& o:q.objectives)if(o.TaskStep)n+=p.GetQuestObjectiveData(&q,o.StorageIndex)*o.TaskStep;return n;}
int main(int argc,char** argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);assert(argc==3);
 std::map<uint32,uint32> objectFlags;std::ifstream input(argv[1]);assert(input);std::string line;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,flagsEx,area,info,start,package,prev,next,special;in>>id;auto& q=store.quests[id];q.id=id;in>>q.Type>>q.flags>>flagsEx>>q.AllowableRaces>>area>>info>>start>>package>>prev>>next>>special;assert(q.Type==3&&area==4306&&!info&&!start&&!package&&!prev&&!next&&!special);q.special=QUEST_SPECIAL_FLAGS_KILL;}
  if(kind=="objective"){uint32 q,id;QuestObjective o;int index;in>>q>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags>>o.TaskStep;o.StorageIndex=int8(index);store.quests.at(q).objectives.push_back(o);}
  if(kind=="source"){Source s;std::string ai,script;in>>s.entry>>s.c1>>s.c2>>s.faction>>s.flags;auto tab=line.find_last_of('\t');s.spawns=std::stoul(line.substr(tab+1));sources[s.entry]=s;}
  if(kind=="go"){uint32 entry,type,flags;in>>entry>>type>>flags;assert(type==10);objectFlags[entry]=flags;}
 }
 std::ifstream native(argv[2]);assert(native);bool taskRead=false,areaRead=false;
 while(std::getline(native,line)){std::istringstream in(line);std::string kind;in>>kind;if(kind=="area"){uint32 area;in>>area;assert(area==7643);areaRead=true;}
  if(kind=="task"){uint32 id;std::string label;auto& t=sQuestV2CliTaskStore.task;in>>id>>label>>t.ConditionID>>label>>t.FiltMinLevel>>t.FiltMaxLevel>>label>>t.FiltMinSkillID>>t.FiltMinSkillValue;assert(id==39998);for(unsigned i=0;i<4;++i)in>>label;in>>label>>t.FiltCompletedQuest[0]>>t.FiltCompletedQuest[1]>>t.FiltCompletedQuest[2];assert(t.FiltCompletedQuest[0]==40002);taskRead=true;}}
 assert(taskRead&&areaRead);auto& q=store.quests.at(39998);assert(q.objectives.size()==6&&sources.size()==18);store.tasks[7643].insert(&q);
 go_grey_shoals_supplies shared;sScriptMgr->gossip=[&](Player* p,GameObject* g){return g->entry==244704&&shared.OnGossipHello(p,g);};
 // Reproduce original template flag1 through native initialization and native admission.
 for(auto const& row:objectFlags){Player p;Enter(p);BonusGo g;g.entry=row.first;g.area=7643;g.Init(1);g.TryUse(&p);assert(!g.admitted&&!p.packets);}
 assert(objectFlags.size()==6);for(auto const& row:objectFlags)for(auto team:{ALLIANCE,HORDE}){Player p;p.team=team;Enter(p);BonusGo g;g.entry=row.first;g.area=7643;g.Init(row.second);g.TryUse(&p);assert(g.admitted==1&&Progress(p,q)==(g.entry==244704?2:5));g.TryUse(&p);assert(g.admitted==1);}
 {Player p;p.Accept(44737);BonusGo g;g.entry=244704;g.area=8297;g.Init(objectFlags.at(244704));g.TryUse(&p);assert(!g.admitted&&g.state==GO_JUST_DEACTIVATED);}
 {Player p;Enter(p);BonusGo g;g.entry=244703;g.area=7643;g.Init(0);g.spawned=false;g.TryUse(&p);assert(!g.admitted);g.spawned=true;g.flags=GO_FLAG_NOT_SELECTABLE;g.TryUse(&p);assert(!g.admitted);}
 {Player p;Enter(p);SpellInfo info;info.MiscValue=97949;Spell spell;spell.m_spellInfo=&info;spell.EffectKillCredit(0);Creature target;spell.unitTarget=&target;spell.EffectKillCredit(0);spell.unitTarget=&p;spell.effectHandleMode=0;spell.EffectKillCredit(0);assert(!p.packets);}
 for(uint32 entry:{244703u,244704u}){Player owner,near,far,released,offline;for(auto p:{&owner,&near,&far,&released,&offline})Enter(*p);far.rewardRange=false;released.alive=false;released.corpse=true;offline.world=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&owner,&b};Group group;group.first=&a;owner.group=&group;BonusGo g;g.entry=entry;g.area=7643;g.Init(objectFlags.at(entry));g.TryUse(&owner);assert(Progress(owner,q)==(entry==244704?2:5)&&Progress(near,q)==Progress(owner,q)&&!far.packets&&!released.packets&&!offline.packets);}
 // Native activation: level endpoints, rewarded prerequisite, full log and wrong area.
 for(auto team:{ALLIANCE,HORDE})for(uint32 level:{97u,98u,109u,110u})for(int previous:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){Player p;p.team=team;p.level=level;if(previous==QUEST_STATUS_REWARDED)p.m_RewardedQuests.insert(40002);else if(previous)p.quests[40002].Status=previous;p.UpdateAreaQuestTasks(7643,0);assert((p.FindQuestSlot(39998)<MAX_QUEST_LOG_SIZE)==(level>=98&&level<=109&&previous==QUEST_STATUS_REWARDED));}
 {Player p;p.m_RewardedQuests.insert(40002);p.UpdateAreaQuestTasks(7852,0);assert(p.FindQuestSlot(39998)==MAX_QUEST_LOG_SIZE);p.slots.fill(123);p.UpdateAreaQuestTasks(7643,0);assert(p.FindQuestSlot(39998)==MAX_QUEST_LOG_SIZE);}
 unsigned runs=0;
 // Every spawned credit source, owning pet and direct killing blow, both factions.
 for(auto const& row:sources)if(row.second.spawns)for(auto team:{ALLIANCE,HORDE})for(bool pet:{false,true}){Player p;p.team=team;Enter(p);auto const& s=row.second;auto o=std::find_if(q.objectives.begin(),q.objectives.end(),[&](auto const& o){return o.ObjectID==s.c1;});assert(o!=q.objectives.end());unsigned required=unsigned(std::ceil(100.0f/o->TaskStep));for(unsigned n=1;n<=required;++n){Kill(p,s.entry,pet);assert(p.GetQuestObjectiveData(&q,o->StorageIndex)==n&&p.slotCounters.at(o->StorageIndex)==n);assert(p.rewards==unsigned(n==required));}assert(p.rewards==1&&p.completions==1&&Progress(p,q)>=100);auto packets=p.packets;Kill(p,s.entry,pet);assert(p.packets==packets&&p.rewards==1);p.UpdateAreaQuestTasks(7643,7643);assert(p.FindQuestSlot(39998)==MAX_QUEST_LOG_SIZE);++runs;}assert(runs==60);
 // All five counters, exact native threshold and caps (5% comes from circle spell193191).
 for(auto const& o:q.objectives)if(o.TaskStep){Player p;Enter(p);auto count=uint32(std::ceil(100.0f/o.TaskStep));for(uint32 n=1;n<count;++n){p.KilledMonsterCredit(o.ObjectID);assert(!p.rewards&&!p.CanCompleteQuest(39998));}p.KilledMonsterCredit(o.ObjectID);assert(p.rewards==1&&p.GetQuestObjectiveData(&q,o.StorageIndex)==count);p.KilledMonsterCredit(o.ObjectID);assert(p.GetQuestObjectiveData(&q,o.StorageIndex)==count&&p.rewards==1);}
 {Player p;Enter(p);for(unsigned n=0;n<199;++n)p.KilledMonsterCredit(97945);assert(Progress(p,q)==99.5f&&!p.rewards);Kill(p,97816,true);assert(Progress(p,q)==100.5f&&p.rewards==1);}
 {Player p;Enter(p);for(unsigned n=0;n<8;++n)for(uint32 credit:{97945u,97946u,97947u,97948u,97949u})p.KilledMonsterCredit(credit);assert(Progress(p,q)==92&&!p.rewards);p.KilledMonsterCredit(97949);p.KilledMonsterCredit(97948);assert(Progress(p,q)==100&&p.rewards==1);}
 {Player p;Enter(p);for(unsigned n=0;n<33;++n)p.KilledMonsterCredit(97948);assert(Progress(p,q)==99&&!p.rewards);Kill(p,97859,false);assert(Progress(p,q)==102&&p.rewards==1);}
 {Player p;Enter(p);for(unsigned n=0;n<33;++n)Kill(p,97822,true);assert(Progress(p,q)==99);p.UpdateAreaQuestTasks(7852,7643);assert(p.FindQuestSlot(39998)==MAX_QUEST_LOG_SIZE&&p.sourceRemovals==1);Kill(p,97816,false);assert(Progress(p,q)==99&&!p.rewards);p.UpdateAreaQuestTasks(7643,7852);assert(p.slotCounters.at(4)==33&&Progress(p,q)==99);Kill(p,97816,false);assert(Progress(p,q)==100&&p.rewards==1);}
 {Player p;Enter(p);p.SetQuestObjectiveData(&q,&q.objectives[1],199,true);assert(!p.m_QuestStatusSave.count(39998));assert(!p.HasQuestObjectiveComplete(&q,q.objectives[0]));p.UpdateQuestObjectiveData(&q);assert(p.slotCounters.at(1)==199);Kill(p,95620,true);assert(p.rewards==1&&p.m_QuestStatusSave.count(39998));}
 {Player p;Kill(p,95620,false);assert(!p.packets);Enter(p);Creature c;Victim(c,95620);KillRewarder r;r._victim=&c;r._RewardPlayer(&p,false);p.SetQuestSlot(p.FindQuestSlot(39998),0);Deliver(p);assert(!p.packets);creatures.erase(c.guid);}
 {Player killer,near,far,released,offline;unsigned guid=500;for(auto p:{&killer,&near,&far,&released,&offline}){p->guid={guid++};Enter(*p);}far.rewardRange=false;released.alive=false;released.corpse=true;offline.contact=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&killer,&b};Group group;group.first=&a;Creature victim;Victim(victim,97825);KillRewarder reward;reward._victim=&victim;reward._group=&group;reward.SelectGroup();for(auto p:{&killer,&near,&far,&released,&offline})Deliver(*p);assert(Progress(killer,q)==2&&Progress(near,q)==2&&!far.packets&&!released.packets&&!offline.packets);creatures.erase(victim.guid);}
 std::cout<<"PASS: original six GO flag1 failures reproduced via native initialization/admission; repaired banner/circle admission, native effect134/group credit and shared Grey Shoals hook; native bonus-area activation/exit,98..109 and rewarded40002 gates;60 full player/pet/faction runs across15 spawned sources; five weights and mixed99.5/100/100.5/102 thresholds, caps, auto-reward dispatch once, partial re-entry, saved counters/save marks, deferred exit and group gates. GO activation/cast scheduling, reward persistence/XP/money, AddQuest/storage/world/packets are adapters; no client verification.\n";
}

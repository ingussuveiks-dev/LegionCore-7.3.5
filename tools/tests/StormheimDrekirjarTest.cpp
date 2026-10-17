#include "TideskornFixture.inc"
#include "NativeDilemmaGetters.inc"
struct CriteriaTree{};struct CriteriaMgr{CriteriaTree const* GetCriteriaTree(uint32){return nullptr;}} criteriaMgr;auto sAchievementMgr=&criteriaMgr;
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple{uint32 QuestID=0,ObjectID=0,ObjectiveType=0;int Write(){return 0;}};}namespace Character{struct FailedPlayerCondition{explicit FailedPlayerCondition(int){}int Write(){return 0;}};}}
#include "NativeDilemmaSetter.inc"
#include "NativeRewardPlayer.inc"
#include "TideskornObjective.inc"
#include "TideskornComplete.inc"
#include "TideskornRestore.inc"
#include "DrekirjarCounter.inc"
struct QuestV2CliTaskEntry{uint32 ConditionID=0,FiltMinSkillID=0,FiltMinSkillValue=0;int FiltMinLevel=-1,FiltMaxLevel=-1;uint32 FiltCompletedQuest[3]{};};
struct TaskStore{QuestV2CliTaskEntry task;QuestV2CliTaskEntry const* LookupEntry(uint32 id){return id==38374?&task:nullptr;}} sQuestV2CliTaskStore;
struct PlayerConditionEntry{};struct PlayerConditionStore{PlayerConditionEntry const* LookupEntry(int32 id){assert(id==33048);return nullptr;}}sPlayerConditionStore;
struct ConditionMgr{bool IsPlayerMeetingCondition(Unit*,int32,bool=false);bool IsPlayerMeetingCondition(Unit*,PlayerConditionEntry const*){assert(false);return false;}}conditionMgr;auto sConditionMgr=&conditionMgr;
#include "DrekirjarTaskCondition.inc"
#include "TideskornActivation.inc"
enum {SPELL_EFFECT_HANDLE_HIT_TARGET=1,CONDITION_ALIVE=36};
struct Spell{int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Unit* unitTarget=nullptr;SpellInfo* m_spellInfo=nullptr;void EffectKillCredit(SpellEffIndex);};
#include "TideskornSpell.inc"
struct Source{uint32 entry,c1,c2,faction,flags,spawns;};std::map<uint32,Source> sources;uint64 serial=100000;
struct ObjectData{uint32 entry,flags,quest,spell;};std::map<uint32,ObjectData> objectData;
std::vector<std::string> Split(std::string const& line){std::istringstream row(line);std::vector<std::string> result;std::string value;while(std::getline(row,value,'\t'))result.push_back(value);return result;}
void Victim(Creature& c,uint32 entry){auto d=sources.at(entry);c.entry=entry;c.guid={serial++};c.data.Entry=entry;c.data.KillCredit[0]=d.c1;c.data.KillCredit[1]=d.c2;creatures[c.guid]=&c;}
void Kill(Player& p,uint32 entry,bool petKill){Creature c;Victim(c,entry);Unit pet;pet.ownerPlayer=&p;auto killer=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&c);assert(killer.first==&p&&killer.second);KillRewarder reward;reward._victim=&c;reward._RewardPlayer(&p,false);assert(p.delays.back()==50);Deliver(p);auto packets=p.packets;reward._RewardPlayer(&p,false);Deliver(p);assert(p.packets==packets&&c.IsPlayerRewarded(p.GetGUID()));creatures.erase(c.guid);}
void Enter(Player& p){p.area=7608;p.UpdateAreaQuestTasks(7608,0);assert(p.FindQuestSlot(38374)<MAX_QUEST_LOG_SIZE);}
float Progress(Player& p,Quest const& q){float n=0;for(auto const& o:q.objectives)if(o.TaskStep)n+=p.GetQuestObjectiveData(&q,o.StorageIndex)*o.TaskStep;return n;}
uint16 Counter(Player& p,uint8 counter){return uint16(p.packed[QUEST_COUNTS_OFFSET+counter/2]>>(16*(counter%2)));}
struct BonusGo:GameObject{
 uint32 admitted=0;GameObjectAI fallback;struct Info{uint32 flags;struct{uint32 questID;}goober;}metadata;uint32 configuredSpell=0;bool castSuccess=true;
 void SetUInt32Value(uint32,uint32 n){flags=n;}void Init(ObjectData data){entry=data.entry;area=7608;configuredSpell=data.spell;metadata.flags=data.flags;metadata.goober.questID=data.quest;auto goinfo=&metadata;
#include "TideskornGoInit.inc"
 ai=&fallback;}
 bool QuestGate(Player* player){bool allowed=false;auto info=&metadata;do{
#include "DrekirjarGoQuest.inc"
 allowed=true;}while(false);return allowed;}
 void TryUse(Unit* user){
#include "TideskornGoAdmission.inc"
 if(!QuestGate(user->ToPlayer()))return;
 // GO activation and cast scheduling are adapters. Native admission, quest gate and final134 handler execute.
 ++admitted;flags|=GO_FLAG_IN_USE;if(!castSuccess)return;uint32 credit=configuredSpell==183181?92238:92241;auto player=user->ToPlayer();player->AddDelayedEvent(10,[player,credit](){SpellInfo info;info.MiscValue=credit;Spell spell;spell.unitTarget=player;spell.m_spellInfo=&info;spell.EffectKillCredit(0);});
 }
};
bool DyingActorCondition(WorldObject* object){bool condMeets=false;switch(CONDITION_ALIVE){
#include "DrekirjarAlive.inc"
 }return !condMeets;}
int main(int argc,char** argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);assert(argc==3);
 std::ifstream input(argv[1]);assert(input);std::string line;std::map<uint32,std::array<float,3>> elitePositions;std::set<uint32> hrydshalSources;
 while(std::getline(input,line)){auto v=Split(line);if(v.empty())continue;
  if(v[0]=="quest"){auto& q=store.quests[38374];q.id=38374;q.Type=std::stoul(v[2]);q.flags=std::stoul(v[3]);q.special=QUEST_SPECIAL_FLAGS_KILL;assert(q.Type==3&&v[6]=="4410");}
  if(v[0]=="objective"){QuestObjective o;o.Type=std::stoi(v[3]);o.ObjectID=std::stoul(v[4]);o.Amount=std::stoul(v[5]);o.StorageIndex=int8(std::stoi(v[6]));o.Flags=std::stoul(v[7]);o.TaskStep=std::stof(v[8]);store.quests.at(38374).objectives.push_back(o);}
  if(v[0]=="source"){Source s{std::stoul(v[1]),std::stoul(v[2]),std::stoul(v[3]),std::stoul(v[4]),std::stoul(v[5]),std::stoul(v[13])};sources[s.entry]=s;}
  if(v[0]=="go"){ObjectData d{std::stoul(v[1]),std::stoul(v[3]),std::stoul(v[5]),std::stoul(v[12])};objectData[d.entry]=d;}
  if(v[0]=="spawn"){auto id=std::stoul(v[2]);if(v[3]=="1220"&&v[5]=="7608")hrydshalSources.insert(id);if(id>=91892&&id<=91895)elitePositions[id]={std::stof(v[10]),std::stof(v[11]),std::stof(v[12])};}
 }
 std::ifstream native(argv[2]);assert(native);bool taskRead=false;while(std::getline(native,line))if(line.rfind("task ",0)==0){std::istringstream in(line);std::string label;uint32 id;auto& t=sQuestV2CliTaskStore.task;in>>label>>id>>label>>t.ConditionID>>label>>t.FiltMinLevel>>t.FiltMaxLevel;assert(id==38374&&t.ConditionID==33048);taskRead=true;}
 assert(taskRead&&sources.size()==33&&objectData.size()==7);auto& q=store.quests.at(38374);assert(q.objectives.size()==7);store.tasks[7608].insert(&q);
 // Old brew template blocks the actual native Use admission before any credit spell.
 {Player p;Enter(p);BonusGo go;auto d=objectData.at(241429);d.flags=1;go.Init(d);go.TryUse(&p);Deliver(p);assert(!go.admitted&&!p.packets);}
 for(auto team:{ALLIANCE,HORDE})for(auto const& row:objectData){Player p;p.team=team;Enter(p);BonusGo go;go.Init(row.second);go.TryUse(&p);assert(go.admitted==1&&!p.packets&&p.delays.back()==10);go.TryUse(&p);assert(go.admitted==1);Deliver(p);assert(Progress(p,q)==(go.entry==241429?1:5));}
 {Player p;BonusGo cage;cage.Init(objectData.at(245667));cage.TryUse(&p);assert(!cage.admitted);Enter(p);cage.castSuccess=false;cage.TryUse(&p);Deliver(p);assert(cage.admitted==1&&!p.packets);}
 {Player p;Enter(p);BonusGo go;go.Init(objectData.at(241429));go.TryUse(&p);p.UpdateAreaQuestTasks(7611,7608);Deliver(p);assert(!p.packets);}
 for(uint32 entry:{241429u,245667u}){Player owner,near,far,released,offline;for(auto p:{&owner,&near,&far,&released,&offline})Enter(*p);far.rewardRange=false;released.alive=false;released.corpse=true;offline.world=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&owner,&b};Group group;group.first=&a;owner.group=&group;BonusGo go;go.Init(objectData.at(entry));go.TryUse(&owner);Deliver(owner);assert(Progress(owner,q)==(entry==241429?1:5)&&Progress(near,q)==Progress(owner,q)&&!far.packets&&!released.packets&&!offline.packets);}
 for(auto team:{ALLIANCE,HORDE})for(uint32 level:{97u,98u,109u,110u}){Player p;p.team=team;p.level=level;p.UpdateAreaQuestTasks(7608,0);assert((p.FindQuestSlot(38374)<MAX_QUEST_LOG_SIZE)==(level>=98&&level<=109));}
 {Player p;p.UpdateAreaQuestTasks(7611,0);assert(p.FindQuestSlot(38374)==MAX_QUEST_LOG_SIZE);p.slots.fill(123);p.UpdateAreaQuestTasks(7608,0);assert(p.FindQuestSlot(38374)==MAX_QUEST_LOG_SIZE);}
 assert(hrydshalSources.size()==16);unsigned runs=0;for(auto const& row:sources)if(hrydshalSources.count(row.first))for(auto team:{ALLIANCE,HORDE})for(bool pet:{false,true}){Player p;p.team=team;Enter(p);auto o=std::find_if(q.objectives.begin(),q.objectives.end(),[&](auto const& o){return o.ObjectID==row.second.c1;});assert(o!=q.objectives.end());unsigned count=unsigned(std::ceil(100.0f/o->TaskStep));for(unsigned n=1;n<=count;++n){Kill(p,row.first,pet);assert(p.GetQuestObjectiveData(&q,o->StorageIndex)==n&&p.rewards==unsigned(n==count));if(n<count)assert(Counter(p,o->StorageIndex)==n);}assert(p.rewards==1&&p.completions==1);auto packets=p.packets;Kill(p,row.first,pet);assert(p.packets==packets&&p.rewards==1);p.UpdateAreaQuestTasks(7608,7608);assert(p.FindQuestSlot(38374)==MAX_QUEST_LOG_SIZE);++runs;}assert(runs==64);
 for(auto const& o:q.objectives)if(o.TaskStep){Player p;Enter(p);unsigned count=unsigned(std::ceil(100.0f/o.TaskStep));for(unsigned n=1;n<count;++n){p.KilledMonsterCredit(o.ObjectID);assert(!p.rewards&&!p.CanCompleteQuest(38374));}p.KilledMonsterCredit(o.ObjectID);assert(p.rewards==1&&Progress(p,q)>=100);}
 {Player p;Enter(p);for(auto const& o:q.objectives)if(o.TaskStep)p.KilledMonsterCredit(o.ObjectID);assert(Progress(p,q)==36);for(unsigned n=0;n<4;++n)Kill(p,91529,true);assert(Progress(p,q)==96&&!p.rewards);Kill(p,91892,true);assert(Progress(p,q)==106&&p.rewards==1);}
 {Player p;Enter(p);for(unsigned n=0;n<99;++n)p.KilledMonsterCredit(92238);assert(Progress(p,q)==99);p.UpdateAreaQuestTasks(7611,7608);Kill(p,91529,true);assert(Progress(p,q)==99);p.UpdateAreaQuestTasks(7608,7611);assert(Counter(p,17)==99);BonusGo brew;brew.Init(objectData.at(241429));brew.TryUse(&p);Deliver(p);assert(Progress(p,q)==100&&p.rewards==1);}
 {Player p;Enter(p);auto o=q.objectives[1];assert(o.StorageIndex==17);p.SetQuestObjectiveData(&q,&o,99,true);assert(!p.m_QuestStatusSave.count(38374));p.UpdateQuestObjectiveData(&q);assert(Counter(p,17)==99);BonusGo brew;brew.Init(objectData.at(241429));brew.TryUse(&p);Deliver(p);assert(p.rewards==1&&p.m_QuestStatusSave.count(38374));}
 // Production uint16 packing must retain sparse17..20 and adjacent halves/slots.
 {Player p;for(uint8 i:{0,1,17,18,19,20,23}){p.SetQuestSlotCounter(0,i,100+i);assert(Counter(p,i)==100+i);}auto before=p.packed;p.SetQuestSlotCounter(0,24,999);p.SetQuestSlotCounter(0,uint8(-1),999);assert(before==p.packed);p.SetQuestSlotCounter(1,17,200);assert(p.packed[MAX_QUEST_OFFSET+QUEST_COUNTS_OFFSET+8]>>16==200&&Counter(p,17)==117);}
 for(uint32 entry:{91529u,91892u}){Player killer,near,far,released,offline;unsigned guid=500;for(auto p:{&killer,&near,&far,&released,&offline}){p->guid={guid++};Enter(*p);}far.rewardRange=false;released.alive=false;released.corpse=true;offline.contact=false;GroupReference e{&offline,nullptr},d{&released,&e},c{&far,&d},b{&near,&c},a{&killer,&b};Group group;group.first=&a;Creature victim;Victim(victim,entry);KillRewarder reward;reward._victim=&victim;reward._group=&group;reward.SelectGroup();for(auto p:{&killer,&near,&far,&released,&offline})Deliver(*p);assert(Progress(killer,q)==(entry==91529?15:10)&&Progress(near,q)==Progress(killer,q)&&!far.packets&&!released.packets&&!offline.packets);creatures.erase(victim.guid);}
 // Actual death condition branch evaluates base actor, independent of player/pet/null killer.
 for(uint32 id:{91893u,91895u,91894u}){Creature actor;actor.alive=true;assert(!DyingActorCondition(&actor));actor.alive=false;assert(DyingActorCondition(&actor));uint32 next=id==91893?91895:id==91895?91894:91892;auto a=elitePositions.at(id),b=elitePositions.at(next);assert(std::sqrt(std::pow(a[0]-b[0],2)+std::pow(a[1]-b[1],2)+std::pow(a[2]-b[2],2))<50);}
 std::cout<<"PASS: original brew IN_USE reproduced/native admission repaired; seven GO links and native quest gate/134 credit; actual missing33048 admission;64 player/pet/faction death-credit runs across16 Hrydshal source templates, six weights/mixed100..106, rare10%/15% group gates, sparse17..20 native uint16 packing, saved counters/re-entry/reward once, elite base-actor condition and static death-chain distances. GO cast/trigger scheduling, full elite AI/combat/phase and reward/storage/packet services remain adapters or inspected data; client pending.\n";
}

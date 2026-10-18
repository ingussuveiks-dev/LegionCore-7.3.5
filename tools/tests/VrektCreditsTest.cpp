#include <array>
#include <cstdint>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t; using int8=int8_t; using uint16=uint16_t; using uint32=uint32_t; using int32=int32_t; using uint64=uint64_t;
#include "QuestType.inc"
#include "QuestStatus.inc"
#include "QuestObjectiveType.inc"
#include "__QuestFlags.inc"
#include "__QuestSpecialFlags.inc"
#include "QuestObjectiveFlags.inc"
#include "QuestObjective.inc"
constexpr uint8 MAX_KILL_CREDIT=2,MAX_QUEST_LOG_SIZE=25;
constexpr int QUEST_STATE_COMPLETE=1;
enum {CRITERIA_TYPE_KILL_CREATURE_TYPE,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD,CRITERIA_TYPE_KILL_CREATURE,CRITERIA_TIMED_TYPE_CREATURE};
#define TC_LOG_DEBUG(...) ((void)0)
void Check(bool b,char const* why){if(!b)throw std::runtime_error(why);}
struct ObjectGuid{
 uint64 value=0; bool player=false;
 bool IsEmpty()const{return !value;} bool IsPlayer()const{return value&&player;}
 bool operator<(ObjectGuid const& b)const{return value<b.value;}
 explicit operator bool()const{return !IsEmpty();}
 static ObjectGuid const Empty;
};
ObjectGuid const ObjectGuid::Empty{};
struct Player;struct Creature;
struct SpellInfo{};
struct Unit{
 ObjectGuid guid,owner; Player* self=nullptr; bool dead=false,m_IsInKillingProcess=false;
 virtual ~Unit()=default;
 ObjectGuid GetCharmerOrOwnerGUID()const{return owner;}
 Player const* ToPlayer()const{return self;} bool IsPlayer()const{return self!=nullptr;}
 uint32 GetGUIDLow()const{return uint32(guid.value);}
 virtual uint32 GetEntry()const{return 0;} virtual Creature* ToCreature(){return nullptr;}
 bool isDead(bool)const{return dead;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
 void SelectRecipientNative(Unit*,Player*&);
 Player* SelectRecipient(Unit* victim){Player* recipient=nullptr;SelectRecipientNative(victim,recipient);return recipient;}
};
struct CreatureTemplate{uint32 Entry=0;uint32 KillCredit[2]={};};
struct Creature:Unit{
 CreatureTemplate data;bool enough=true;Player* loot=nullptr;std::set<ObjectGuid> rewarded;
 uint32 GetEntry()const override{return data.Entry;} Creature* ToCreature()override{return this;}
 ObjectGuid GetGUID()const{return guid;} uint32 GetCreatureType()const{return 7;}
 CreatureTemplate const* GetCreatureTemplate()const{return &data;}
 bool IsDamageEnoughForLootingAndReward()const{return enough;}
 void SetLootRecipient(Player*p){loot=p;}Player*GetLootRecipient(){return loot;}
 void AddRewardPlayer(ObjectGuid g){rewarded.insert(g);}
};
void Unit::SelectRecipientNative(Unit* victim,Player*& recipient){
#include "KillRecipient.inc"
 recipient=isRewardAllowed?player:nullptr;
}
std::map<ObjectGuid,Creature*> creatures;std::map<ObjectGuid,Player*> players;
struct Map{Creature* GetCreature(ObjectGuid g){auto i=creatures.find(g);return i==creatures.end()?nullptr:i->second;}int GetDifficultyID()const{return 0;}} worldMap;
namespace ObjectAccessor{
 Player*GetPlayer(Unit const&,ObjectGuid g){auto i=players.find(g);return i==players.end()?nullptr:i->second;}
 Creature*GetCreature(Player const&,ObjectGuid g){return worldMap.GetCreature(g);}
}
struct Group{bool raid=false;bool isRaidGroup()const{return raid;}};
struct Quest{
 uint32 Type=0,flags=0,rewardSpell=0,special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;
 std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return 44923;}auto const&GetObjectives()const{return objectives;}
 bool HasFlag(uint32 f)const{return (flags&f)!=0;}bool HasSpecialFlag(uint32 f)const{return (special&f)!=0;}
 bool IsWeekly()const{return HasFlag(QUEST_FLAGS_WEEKLY);}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return HasSpecialFlag(QUEST_SPECIAL_FLAGS_REPEATABLE);}
 bool IsAutoComplete()const{return Type==QUEST_TYPE_AUTOCOMPLETE;}
 bool IsAllowedInRaid(int)const{return false;}
};
struct Store{Quest quest;Quest const*GetQuestTemplate(uint32 id){return id==44923?&quest:nullptr;}} store;
Store*sQuestDataStore=&store;
struct QuestStatusData{QuestStatus Status=QUEST_STATUS_INCOMPLETE;uint32 Timer=1;};
struct Guild{template<class...T>void UpdateAchievementCriteria(T...){}};
struct GuildMgr{Guild*GetGuildById(uint32){return nullptr;}} guildMgr;
GuildMgr*sGuildMgr=&guildMgr;
struct Achievements{void StartTimedAchievement(int,uint32){}};
struct Reputation{int32 GetReputation(int32)const{return 0;}};
struct Scripts{int completions=0;void OnQuestComplete(Player*,Quest const*){++completions;}} scripts;
Scripts*sScriptMgr=&scripts;
struct Player:Unit{
 QuestStatusData status;std::array<int32,2> counts{};bool accepted=true,alive=true,corpse=false;
 Group*group=nullptr;std::set<uint32>m_RewardedQuests;Achievements achievement;
 Achievements*m_achievementMgr=&achievement;int rewardCalls=0,updates=0;bool announced=false;
 std::vector<std::function<void()>> pending;
 Player(){self=this;guid={1,true};players[guid]=this;}
 ~Player(){players.erase(guid);}
 ObjectGuid GetGUID()const{return guid;}Map*GetMap(){return &worldMap;}
 Group*GetGroup(){return group;}bool IsAlive()const{return alive;}void*GetCorpse(){return corpse?this:nullptr;}
 uint32 GetGuildId()const{return 0;}
 template<class...T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}
 void AddTrackingQuestIfNeeded(ObjectGuid){}
 uint32 GetQuestSlotQuestId(uint8 i)const{return i==0&&accepted?44923:0;}
 QuestStatusData*getQuestStatus(uint32 id){return id==44923&&accepted?&status:nullptr;}
 int32 GetQuestObjectiveData(Quest const*,int8 i){return counts.at(i);}
 void SetQuestObjectiveData(Quest const*,QuestObjective const*o,int32 n){counts.at(o->StorageIndex)=n;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++updates;}
 void SetQuestStatus(uint32,QuestStatus s){status.Status=s;}
 bool IsQuestWeekRewarded(uint32)const{return false;}bool IsQuestSeasonalRewarded(uint32)const{return false;}
 bool CanTakeQuest(Quest const*,bool)const{return accepted;}
 Reputation GetReputationMgr()const{return {};}
 bool HasEnoughMoney(uint64)const{return false;}bool HasSpell(int32)const{return false;}bool HasCurrency(int32,int32)const{return false;}
 uint16 FindQuestSlot(uint32)const{return accepted?0:MAX_QUEST_LOG_SIZE;}
 void SetQuestSlotState(uint16,int){}
 // The real CompleteQuest dispatch is tested; inventory, currency, mail,
 // character writes, reward spell and network playback are fixture leaves.
 void RewardQuest(Quest const*q,uint32,Player*,bool announce){Check(q==&store.quest,"wrong reward quest");++rewardCalls;announced=announce;accepted=false;status.Status=QUEST_STATUS_REWARDED;}
 void AddDelayedEvent(uint32 delay,std::function<void()> f){Check(delay==50,"native credit delay changed");pending.push_back(f);}
 void Flush(){auto q=std::move(pending);pending.clear();for(auto&f:q)f();}
 void KilledMonster(CreatureTemplate const*,ObjectGuid);
 void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);
 bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&);
 bool CanCompleteQuest(uint32);void CompleteQuest(uint32);
};
struct KillRewarder{
 Unit*_victim;Group*_group;
 void _RewardKillCredit(Player*);
};
#include "Owner.inc"
#include "KilledMonster.inc"
#include "KilledMonsterCredit.inc"
#include "HasObjective.inc"
#include "CanComplete.inc"
#include "Complete.inc"
#include "KillCredit.inc"
std::map<uint32,CreatureTemplate> templates;
uint64 nextGuid=100;
void Kill(Player&p,uint32 entry,bool pet=false){
 Creature c;c.data=templates.at(entry);c.guid={++nextGuid,false};creatures[c.guid]=&c;
 Unit controlled;controlled.owner=p.guid;
 Unit&killer=pet?controlled:static_cast<Unit&>(p);
 auto recipient=killer.SelectRecipient(&c);Check(recipient==&p,"player/pet reward owner");
 KillRewarder reward{&c,p.GetGroup()};reward._RewardKillCredit(recipient);
 Check(c.m_IsInKillingProcess,"native killing guard");
 Check(!killer.SelectRecipient(&c),"reentrant kill must not dispatch reward");
 p.Flush();creatures.erase(c.guid);
}
int Run(char const*path){
 std::ifstream file(path);Check(bool(file),"missing SQL snapshot");std::string line;
 int questRows=0,markerRows=0;
 while(std::getline(file,line)){
  std::istringstream in(line);std::string tag;in>>tag;
  if(tag=="quest"){uint32 id;in>>id>>store.quest.Type>>store.quest.flags>>store.quest.rewardSpell;
   Check(id==44923&&store.quest.Type==QUEST_TYPE_TASK&&store.quest.rewardSpell==230157,"native task/reward configuration");++questRows;}
  else if(tag=="objective"){
   QuestObjective o;int type,index;in>>o.ID>>o.QuestID>>type>>index>>o.ObjectID>>o.Amount>>o.Flags>>o.Flags2;o.Type=uint8(type);o.StorageIndex=int8(index);store.quest.objectives.push_back(o);}
  else if(tag=="creature"){CreatureTemplate t;in>>t.Entry>>t.KillCredit[0]>>t.KillCredit[1];templates[t.Entry]=t;}
  else if(tag=="marker"){in>>markerRows;}
 }
 Check(questRows==1&&markerRows==1&&templates.size()==4&&store.quest.objectives.size()==2,"complete live input");
 for(auto const&o:store.quest.objectives)Check(o.Type==QUEST_OBJECTIVE_MONSTER&&!o.Flags&&!o.Flags2&&
  ((o.ID==287303&&o.QuestID==44923&&o.StorageIndex==0&&o.ObjectID==115845&&o.Amount==10)||
   (o.ID==287309&&o.QuestID==44923&&o.StorageIndex==1&&o.ObjectID==115846&&o.Amount==1)),"7.3.5 mandatory objectives");
 for(auto const&kv:templates)Check(kv.second.KillCredit[0]==(kv.first==115846?0u:115845u)&&!kv.second.KillCredit[1],"alternate marker mapping");
 for(bool pet:{false,true})for(bool bossFirst:{false,true}){
  Player p;Check(!p.CanCompleteQuest(44923),"empty objectives");
  if(bossFirst){Kill(p,115846,pet);Check(p.counts[0]==0&&p.counts[1]==1&&!p.rewardCalls,"boss cannot replace ten kills");}
  for(int i=0;i<10;++i){Kill(p,std::array<uint32,3>{116180,116600,116601}[i%3],pet);
   Check(p.counts[0]==i+1,"one alternate credit per death");if(i<9||!bossFirst)Check(!p.rewardCalls,"premature task reward");}
  if(!bossFirst){Check(!p.CanCompleteQuest(44923),"ten kills still require boss");Kill(p,115846,pet);}
  Check(p.counts[0]==10&&p.counts[1]==1&&p.rewardCalls==1&&p.announced,"both mandatory objectives auto reward once");
  Kill(p,116180,pet);Kill(p,115846,pet);Check(p.rewardCalls==1&&p.counts[0]==10&&p.counts[1]==1,"post-completion kills");
 }
 {Player p;for(int i=0;i<15;++i)Kill(p,116180);Check(p.counts[0]==10&&!p.rewardCalls,"trash count saturates");
  p.KilledMonsterCredit(99999);Check(p.counts[0]==10&&p.counts[1]==0,"unrelated marker");}
 {Player p;p.accepted=false;Kill(p,116600,true);Check(p.counts[0]==0&&!p.rewardCalls,"not accepted");}
 for(auto state:{QUEST_STATUS_FAILED,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){
  Player p;p.status.Status=state;Kill(p,116600,true);Check(p.counts[0]==0&&!p.rewardCalls,"non-incomplete status must not advance");}
 {Player p;Group raid;raid.raid=true;p.group=&raid;Kill(p,116600);Check(p.counts[0]==0,"raid admission leaf denies nonraid quest");}
 {Player p;Group party;p.group=&party;Kill(p,116601,true);Check(p.counts[0]==1,"party member credit");
  p.alive=false;p.corpse=true;Kill(p,116601);Check(p.counts[0]==1,"released grouped member denied");
  p.corpse=false;Kill(p,116601);Check(p.counts[0]==2,"unreleased grouped member credit");}
 {Player p;Creature c;c.data=templates.at(116180);c.guid={++nextGuid,false};creatures[c.guid]=&c;
  KillRewarder r{&c,nullptr};r._RewardKillCredit(&p);Check(p.counts[0]==0,"credit waits for callback");
  creatures.erase(c.guid);p.Flush();Check(p.counts[0]==0,"missing delayed corpse safely ignored");}
 {Player p;Unit pet;pet.owner=p.guid;Creature c;c.data=templates.at(116600);c.enough=false;c.loot=&p;
  Check(!pet.SelectRecipient(&c)&&!c.loot,"insufficient damage denies reward and clears recipient");}
 {Player p;Unit stranger;Creature c;c.data=templates.at(116600);c.loot=&p;
  Check(stranger.SelectRecipient(&c)==&p,"tap recipient selected over killing blow");
  Unit ownerless;Check(!ownerless.GetCharmerOrOwnerPlayerOrPlayerItself(),"ownerless kill has no player");}
 Check(scripts.completions==4,"one completion hook for each completed case");
 std::cout<<"PASS: both kill orders with player/pet owners, 10+1 thresholds, caps, unrelated/absent quest, party/dead/raid gates, deferred/missing target, tap/insufficient damage and native task reward dispatch.\n";
 return 0;
}
int main(int argc,char**argv){try{Check(argc==2,"expected snapshot path");return Run(argv[1]);}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

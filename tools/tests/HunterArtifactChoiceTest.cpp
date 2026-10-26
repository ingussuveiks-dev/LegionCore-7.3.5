#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
#define ASSERT assert
#define TC_LOG_DEBUG(...)
#include "Status.inc"
enum {CONDITION_QUEST_NONE=14,CONDITION_QUESTTAKEN=9,CONDITION_LEVEL=27,CONDITION_CLASS=15,CONDITION_QUESTREWARDED=8,CONDITION_QUEST_COMPLETE=28,
 CONDITION_SOURCE_TYPE_PLAYER_CHOICE=32,CONDITION_SOURCE_TYPE_PLAYER_CHOICE_RESPONS=33,CONDITION_SOURCE_TYPE_QUEST_ACCEPT=19,DEFAULT_LOCALE=0,
 DISABLE_TYPE_QUEST,INVALIDREASON_QUEST_ALREADY_ON,INVALIDREASON_DONT_HAVE_REQ,INVALIDREASON_QUEST_FAILED_LOW_LEVEL,INVALIDREASON_QUEST_FAILED_WRONG_RACE,
 PLAYER_DYNAMIC_FIELD_AVAILABLE_QUEST_LINE_XQUEST_IDS};
namespace DisableMgr{template<class T>bool IsDisabledFor(int,uint32,T*){return false;}}
struct Quest{uint32 id=44089,MinLevel=102,MaxLevel=0;int32 AllowableClasses=0;uint64 AllowableRaces=uint64(-1);uint32 GetQuestId()const{return id;}};
struct QuestStore{std::map<uint32,Quest> rows;Quest const* GetQuestTemplate(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}} questStore;
auto sQuestDataStore=&questStore;
struct QuestLine{uint32 ID;};struct QuestLineXQuest{uint32 ID,QuestLineID,QuestID,OrderIndex;};
std::vector<QuestLine*> sQuestLineStore;
struct DB2Manager{std::map<uint32,std::vector<QuestLineXQuest*>> rows;auto GetQuestsByQuestLine(uint32 id){return &rows[id];}} sDB2Manager;
enum ComparisionType { COMP_TYPE_EQ=0,COMP_TYPE_HIGH=1,COMP_TYPE_LOW=2,COMP_TYPE_HIGH_EQ=3,COMP_TYPE_LOW_EQ=4 };
template<class T> bool CompareValues(ComparisionType t,T a,T b){switch(t){case COMP_TYPE_EQ:return a==b;case COMP_TYPE_HIGH:return a>b;case COMP_TYPE_LOW:return a<b;case COMP_TYPE_HIGH_EQ:return a>=b;case COMP_TYPE_LOW_EQ:return a<=b;}return false;}
struct Player;
struct Unit {struct Threat {Unit* getHostilTarget(){return nullptr;}} threat; bool IsCreature(){return false;} Threat& getThreatManager(){return threat;} bool isPet(){return false;} Unit* GetOwner(){return nullptr;} virtual uint32 getLevel()const{return 0;} virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}virtual uint32 getClassMask()const{return 0;}};
struct Condition;
struct ConditionSourceInfo {Unit* object;Condition* mLastFailedCondition=nullptr;};
struct Scripts {int calls=0;bool OnConditionCheck(Condition*,ConditionSourceInfo&){return true;}void OnPlayerChoiceResponse(Player*,uint32,uint32){++calls;}} scripts;
auto sScriptMgr=&scripts;
using ObjectGuid=uint32;
struct Session {int GetSessionDbLocaleIndex(){return 0;}} playerSession;
struct Player:Unit {
 uint32 classes=256;std::map<uint32,QuestStatus> questStatuses;int packets=0,casts=0;
 uint32 level=106;bool otherPrerequisites=true;std::vector<uint32> mapIds;
 std::vector<int> responses;
 Player* ToPlayer()override{return this;}uint32 getClassMask()const override{return classes;}
 QuestStatus GetQuestStatus(uint32 id){auto i=questStatuses.find(id);return i==questStatuses.end()?QUEST_STATUS_NONE:i->second;}
 bool GetQuestRewardStatus(uint32 id){return GetQuestStatus(id)==QUEST_STATUS_REWARDED;}
 uint32 getLevel()const{return level;}uint64 getRaceMask()const{return uint64(1)<<9;}
 template<class... T>void SendCanTakeQuestResponse(T...)const{}
 bool CanTakeQuest(Quest const*,bool);bool SatisfyQuestConditions(Quest const*,bool);bool SatisfyQuestStatus(Quest const*,bool);
 bool SatisfyQuestLevel(Quest const*,bool);bool SatisfyQuestClass(Quest const*,bool)const;bool SatisfyQuestRace(Quest const*,bool);
 // These unrelated admission gates have no requirements for quest 44089.
 bool SatisfyQuestExclusiveGroup(Quest const*,bool){return otherPrerequisites;}
 bool SatisfyQuestSkill(Quest const*,bool){return true;}bool SatisfyQuestReputation(Quest const*,bool){return true;}
 bool SatisfyQuestPreviousQuest(Quest const*,bool){return true;}bool SatisfyQuestTimed(Quest const*,bool){return true;}
 bool SatisfyQuestNextChain(Quest const*,bool){return true;}bool SatisfyQuestPrevChain(Quest const*,bool){return true;}
 bool SatisfyQuestDay(Quest const*,bool){return true;}bool SatisfyQuestWeek(Quest const*){return true;}bool SatisfyQuestSeasonal(Quest const*){return true;}
 void ClearDynamicValue(int){mapIds.clear();}void AddDynamicValue(int,uint32 id){mapIds.push_back(id);}void UpdateAvailableQuestLines();
 Session* GetSession(){return &playerSession;}void CastSpell(Player*,uint32,bool){++casts;}
 void SendDisplayPlayerChoice(ObjectGuid,int32);
 template<class T>void SendDirectMessage(T packet){++packets;responses.clear();for(auto& r:packet->Responses)responses.push_back(r.ResponseID);}
};
struct Condition {
 uint32 ElseGroup=0,ConditionType=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;
 bool NegativeCondition=false;bool isLoaded()const{return true;}
 bool Meets(ConditionSourceInfo& sourceInfo){auto object=sourceInfo.object;bool condMeets=false;switch(ConditionType){
#include "Cases.inc"
 default:assert(false);}
#include "Tail.inc"
 }
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {
 ConditionReferenceContainer ConditionReferenceStore;
 std::map<std::tuple<int,int,int>,std::vector<Condition>> rows;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 ConditionList GetConditionsForNotGroupedEntry(int type,int entry){return List(type,0,entry);}
 ConditionList List(int type,int group,int entry){ConditionList list;for(auto& c:rows[{type,group,entry}])list.push_back(&c);return list;}
 bool IsObjectMeetToConditions(Player* p,ConditionList const& list){ConditionSourceInfo info{p};return list.empty()||IsObjectMeetToConditionList(info,list);}
} manager;
auto sConditionMgr=&manager;
#include "List.inc"
#include "Conditions.inc"
#include "QuestStatus.inc"
#include "Level.inc"
#include "Class.inc"
#include "Race.inc"
#include "Take.inc"
#include "Map.inc"
struct ItemReward {struct {int ItemID=0;struct Bonus{std::vector<int> BonusListIDs;};std::optional<Bonus> ItemBonus;} Item;int Quantity=0;};
struct ResponseReward {int TitleId=0,PackageId=0,SkillLineId=0,SkillPointCount=0,ArenaPointCount=0,HonorPointCount=0,Money=0,Xp=0;uint32 SpellID=0;
 struct Item {int Id=0,Quantity=0;std::vector<int> BonusListIDs;};std::vector<Item> Items,Currency,Faction;};
struct Response {int ResponseId=0,ChoiceArtFileId=0;std::string Answer,Header,Description,Confirmation;std::optional<ResponseReward> Reward;};
struct Choice {int UiTextureKitId=0;std::string Question;bool HideWarboardHeader=false;std::vector<Response> Responses;
 Response const* GetResponse(int id)const{for(auto& r:Responses)if(r.ResponseId==id)return &r;return nullptr;}};
struct ChoiceLocale {std::string Question;std::map<int,Response> Responses;};
struct ObjectMgr {std::map<int,Choice> rows;Choice const* GetPlayerChoice(int id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}
 ChoiceLocale* GetPlayerChoiceLocale(int){return nullptr;}template<class T>static void GetLocaleString(T const&,int,std::string&){} } objects;
auto sObjectMgr=&objects;
namespace Trinity{namespace Containers{template<class M>auto MapGetValuePtr(M& m,int id)->typename M::mapped_type*{auto i=m.find(id);return i==m.end()?nullptr:&i->second;}}}
namespace WorldPackets{namespace Quest{
struct Reward {int TitleID=0,PackageID=0,SkillLineID=0,SkillPointCount=0,ArenaPointCount=0,HonorPointCount=0,Money=0,Xp=0;std::vector<ItemReward> Items;};
struct PlayerChoiceResponse {int ResponseID=0,ChoiceArtFileID=0;std::string Answer,Header,Description,Confirmation;std::optional<Quest::Reward> Reward;};
struct DisplayPlayerChoice {int SenderGUID=0,ChoiceID=0,UiTextureKitID=0;std::string Question;bool CloseChoiceFrame=false,HideWarboardHeader=false;std::vector<PlayerChoiceResponse> Responses;DisplayPlayerChoice* Write(){return this;}};
}namespace Misc{struct ChoiceResponse {uint32 ChoiceID=245,ResponseID=0;};}}
struct WorldSession {Player* _player;void HandleChoiceResponse(WorldPackets::Misc::ChoiceResponse&);};
#include "Display.inc"
#include "Handle.inc"


using uint8=uint8_t;using uint16=uint16_t;
#include "Enums.inc"
struct GossipMenu{static uint64 GetRequiredNpcFlagForOption(GossipOptionNpc);};
#include "RequiredFlags.inc"
bool GossipAdmitted(uint32 flags,uint32 flags2){
 uint64 npcflags=uint64(flags)|(uint64(flags2)<<32);auto optionNpc=GossipOptionNpc::None;
 for(int i=0;i<1;++i){
#include "Admission.inc"
 return true;}return false;
}
int main(int argc,char** argv){
 assert(argc==3);std::ifstream actors(argv[2]);assert(actors);uint32 actor,flags,flags2;int actorCount=0;
 while(actors>>actor>>flags>>flags2){assert(actor==107317||actor==107973);assert(GossipAdmitted(flags,flags2));++actorCount;}assert(actorCount==2);
 assert(!GossipAdmitted(2,0)); // Original107317 could not display an ordinary gossip option.
std::ifstream file(argv[1]);assert(file);int type,group,entry;Condition c;
 while(file>>type>>group>>entry>>c.ElseGroup>>c.ConditionType>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition)manager.rows[{type,group,entry}].push_back(c);
 assert(manager.rows.size()==6);
 for(auto id:{504,505,506}){Response r;r.ResponseId=id;r.Reward.emplace();r.Reward->SpellID=id;objects.rows[240].Responses.push_back(r);}
 int cases=0;int chosen[]={40621,40619,40620},ends[]={42185,40385,40419};
 for(uint32 level:{98u,101u,102u,105u})for(int selected=0;selected<8;++selected)for(int done=0;done<8;++done)for(auto finish:{QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){
  Player p;p.classes=4;p.level=level;p.questStatuses[40618]=QUEST_STATUS_REWARDED;
  for(int i=0;i<3;++i){if(selected&(1<<i))p.questStatuses[chosen[i]]=QUEST_STATUS_REWARDED;if(done&(1<<i))p.questStatuses[ends[i]]=finish;}
  p.SendDisplayPlayerChoice(1,240);int offeredCount=0;
  for(int i=0;i<3;++i){bool eligible=!(done&(1<<i))&&(!selected||(selected&(1<<i))||(level>=102&&done&&finish==QUEST_STATUS_REWARDED));
   offeredCount+=eligible;assert((std::find(p.responses.begin(),p.responses.end(),504+i)!=p.responses.end())==eligible);
   int casts=p.casts,calls=scripts.calls;WorldSession session{&p};WorldPackets::Misc::ChoiceResponse response{240,uint32(504+i)};
   session.HandleChoiceResponse(response);assert(p.casts==casts+eligible&&scripts.calls==calls+eligible);
  }
  assert(p.packets==int(offeredCount>0));
  for(int menu:{19115,20227})assert(manager.IsObjectMeetToConditions(&p,manager.List(15,menu,0))==(offeredCount>0));
  ++cases;
 }
 for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){
  Player p;p.classes=4;p.level=98;p.questStatuses[40618]=status;p.SendDisplayPlayerChoice(1,240);assert(p.responses.size()==3);
  assert(manager.IsObjectMeetToConditions(&p,manager.List(15,20227,0))==(status==QUEST_STATUS_REWARDED));
 }
 for(int classes:{1,4,256})for(uint32 level:{97u,105u}){
  Player p;p.classes=classes;p.level=level;p.questStatuses[40618]=QUEST_STATUS_REWARDED;p.SendDisplayPlayerChoice(1,240);
  bool valid=classes==4&&level>=98;assert(p.packets==int(valid));WorldSession session{&p};WorldPackets::Misc::ChoiceResponse r{240,505};session.HandleChoiceResponse(r);assert(p.casts==int(valid));
 }
 Player noIntro;noIntro.classes=4;noIntro.SendDisplayPlayerChoice(1,240);assert(!noIntro.packets);
 Player acquired;acquired.classes=4;acquired.level=105;for(int q:{40618,40621,42185,44043})acquired.questStatuses[q]=QUEST_STATUS_REWARDED;
 acquired.SendDisplayPlayerChoice(1,240);assert((acquired.responses==std::vector<int>{505,506}));
 acquired.questStatuses[40385]=QUEST_STATUS_COMPLETE;WorldSession session{&acquired};WorldPackets::Misc::ChoiceResponse stale{240,505};session.HandleChoiceResponse(stale);assert(!acquired.casts);
 // Abandonment keeps a chosen flag and permits precisely that unfinished retry below102.
 Player retry;retry.classes=4;retry.level=98;retry.questStatuses[40618]=QUEST_STATUS_REWARDED;retry.questStatuses[40619]=QUEST_STATUS_REWARDED;
 retry.SendDisplayPlayerChoice(1,240);assert((retry.responses==std::vector<int>{505}));retry.SendDisplayPlayerChoice(1,240);assert(retry.responses.size()==1);
 // The old display consumer ignored Hunter conditions and offered a finished BM route.
 auto fixed=manager.rows;manager.rows.clear();acquired.responses.clear();acquired.SendDisplayPlayerChoice(1,240);assert(acquired.responses.size()==3);manager.rows=fixed;
 objects.rows[999]=objects.rows[240];Player other;other.SendDisplayPlayerChoice(1,999);assert(other.responses.size()==3);
 std::cout<<"PASS "<<cases<<" native condition/display/response lifecycle combinations; first/additional level, both menus, all completion masks, stale responses, no empty frame, abandonment retry, unrelated choice and controlled old unfiltered counterfactual\n";
}

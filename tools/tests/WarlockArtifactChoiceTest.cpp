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
using uint32=uint32_t;using int32=int32_t;
#define ASSERT assert
#define TC_LOG_DEBUG(...)
#include "Status.inc"
enum {CONDITION_CLASS=15,CONDITION_QUESTREWARDED=8,CONDITION_QUEST_COMPLETE=28,
 CONDITION_SOURCE_TYPE_PLAYER_CHOICE=32,CONDITION_SOURCE_TYPE_PLAYER_CHOICE_RESPONS=33,DEFAULT_LOCALE=0};
struct Player;
struct Unit {virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}virtual uint32 getClassMask()const{return 0;}};
struct Condition;
struct ConditionSourceInfo {Unit* object;Condition* mLastFailedCondition=nullptr;};
struct Scripts {int calls=0;bool OnConditionCheck(Condition*,ConditionSourceInfo&){return true;}void OnPlayerChoiceResponse(Player*,uint32,uint32){++calls;}} scripts;
auto sScriptMgr=&scripts;
using ObjectGuid=uint32;
struct Session {int GetSessionDbLocaleIndex(){return 0;}} session;
struct Player:Unit {
 uint32 classes=256;std::map<uint32,QuestStatus> quests;int packets=0,casts=0;
 std::vector<int> responses;
 Player* ToPlayer()override{return this;}uint32 getClassMask()const override{return classes;}
 QuestStatus GetQuestStatus(uint32 id){auto i=quests.find(id);return i==quests.end()?QUEST_STATUS_NONE:i->second;}
 bool GetQuestRewardStatus(uint32 id){return GetQuestStatus(id)==QUEST_STATUS_REWARDED;}
 Session* GetSession(){return &session;}void CastSpell(Player*,uint32,bool){++casts;}
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
int main(int argc,char** argv){
 assert(argc==2);std::ifstream file(argv[1]);assert(file);int type,group,entry;Condition c;
 while(file>>type>>group>>entry>>c.ElseGroup>>c.ConditionType>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition)manager.rows[{type,group,entry}].push_back(c);
 for(auto id:{523,524,525}){Response r;r.ResponseId=id;r.Reward.emplace();r.Reward->SpellID=id;objects.rows[245].Responses.push_back(r);}
 uint32 cases=0;
 for(auto status:{QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED})for(auto afflictionEnd:{40712,41156})for(int mask=0;mask<8;++mask){
  Player p;p.quests[40684]=QUEST_STATUS_REWARDED;
  int ends[]={afflictionEnd,42125,43254};for(int i=0;i<3;++i)if(mask&(1<<i))p.quests[ends[i]]=status;
  bool any=mask!=7;assert(manager.IsObjectMeetToConditions(&p,manager.List(15,19326,0))==any);
  p.SendDisplayPlayerChoice(1,245);assert(p.packets==int(any));
  for(int i=0;i<3;++i){bool offered=(mask&(1<<i))==0;int id=523+i;
   assert((std::find(p.responses.begin(),p.responses.end(),id)!=p.responses.end())==offered);
   int casts=p.casts,callbacks=scripts.calls;WorldSession s{&p};WorldPackets::Misc::ChoiceResponse packet{245,uint32(id)};
   s.HandleChoiceResponse(packet);assert(p.casts==casts+int(offered)&&scripts.calls==callbacks+int(offered));
  }++cases;
 }
 // Hidden selections survive abandonment; they must not hide an unfinished route.
 Player retry;retry.quests[40684]=QUEST_STATUS_REWARDED;for(auto q:{40686,40687,40688})retry.quests[q]=QUEST_STATUS_REWARDED;
 retry.SendDisplayPlayerChoice(1,245);assert(retry.responses.size()==3);
 // Initial quest accept opens the choice before the gossip introduction is rewarded.
 Player first;first.quests[40684]=QUEST_STATUS_INCOMPLETE;first.SendDisplayPlayerChoice(1,245);assert(first.responses.size()==3);
 assert(!manager.IsObjectMeetToConditions(&first,manager.List(15,19326,0)));
 Player foreign;foreign.classes=1;foreign.SendDisplayPlayerChoice(1,245);assert(!foreign.packets);
 WorldSession s{&foreign};WorldPackets::Misc::ChoiceResponse packet{245,523};s.HandleChoiceResponse(packet);assert(!foreign.casts);
 // Existing unrelated choice behavior is outside this content change.
 objects.rows[999]=objects.rows[245];Player unrelated;unrelated.SendDisplayPlayerChoice(1,999);assert(unrelated.responses.size()==3);
 std::cout<<"PASS: "<<cases<<" native completion/alternate-ending masks, actual display and response handlers, gossip AND/OR gates, stale responses, introduction and abandoned-route retry\n";
}

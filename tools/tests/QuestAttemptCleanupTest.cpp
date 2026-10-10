#include <cassert>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using QuestStatus=int;using InventoryResult=int;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_DELETE_SAVE_TYPE=2,EQUIP_ERR_OK=0,QUEST_OBJECTIVE_ITEM=1,QUEST_SPECIAL_FLAGS_NOT_REMOVE_SOURCE=4,QUEST_ITEM_COUNT=4};
#include "QuestStatusData.inc"
struct QuestObjective {unsigned Type=QUEST_OBJECTIVE_ITEM,ObjectID=139043;};
struct Quest {unsigned SourceItemId=139043,SourceItemIdCount=1,flags=0;unsigned ItemDrop[4]={139043},ItemDropQuantity[4]={1};std::vector<QuestObjective> objectives;auto const& GetObjectives()const{return objectives;}bool HasSpecialFlag(unsigned f)const{return flags&f;}};
struct QuestStore {std::map<unsigned,Quest> quests;Quest const* GetQuestTemplate(unsigned q){auto it=quests.find(q);return it==quests.end()?nullptr:&it->second;}} store;
auto sQuestDataStore=&store;
struct ItemTemplate {unsigned start=0;unsigned GetStartQuestID()const{return start;}} itemTemplate;
struct ObjectMgr {ItemTemplate const* GetItemTemplate(unsigned){return &itemTemplate;}} objects;
auto sObjectMgr=&objects;
struct PhaseUpdateData {unsigned quest=0;void AddQuestUpdate(unsigned q){quest=q;}};
struct PhaseMgr {unsigned notifications=0;void NotifyConditionChanged(PhaseUpdateData const&){++notifications;}};
struct Player {
 std::map<unsigned,QuestStatusData> m_QuestStatus;
 std::map<unsigned,QuestStatusData*> vector;
 decltype(vector)* m_QuestStatusVector=&vector;
 std::map<unsigned,int> m_QuestStatusSave;
 std::map<unsigned,unsigned> inventory;
 std::vector<std::function<void()>> events;
 PhaseMgr phase;bool equipped=false;unsigned errors=0,updates=0;
 QuestStatusData* getQuestStatus(unsigned q){auto it=m_QuestStatus.find(q);return it==m_QuestStatus.end()?nullptr:&it->second;}
 void Add(unsigned q){m_QuestStatus[q].Status=3;vector[q]=&m_QuestStatus[q];}
 void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}
 void Run(){while(!events.empty()){auto f=std::move(events.front());events.erase(events.begin());f();}}
 void SetQuestUpdate(unsigned){++updates;}PhaseMgr& GetPhaseMgr(){return phase;}
 int CanUnequipItems(unsigned,unsigned){return equipped?1:0;}
 void SendEquipError(int,void*,void*,unsigned){++errors;}
 void DestroyItemCount(unsigned id,unsigned count,bool,bool=false){inventory[id]-=std::min(inventory[id],count);}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void RemoveActiveQuest(uint32);
 bool TakeQuestSourceItem(uint32,bool);
};
#include "QuestDelayedEvent.inc"
#include "QuestCleanup.inc"
int main(){
 store.quests[38377]={};store.quests[38743]={};store.quests[40890]={};
 Player p;p.Add(38377);p.Add(38743);unsigned old=0,current=0,other=0;
 p.AddQuestDelayedEvent(38377,30000,[&](){++old;});p.AddQuestDelayedEvent(38743,500,[&](){++other;});
 p.RemoveActiveQuest(38377);assert(!p.getQuestStatus(38377)&&p.vector[38377]==nullptr&&p.m_QuestStatusSave[38377]==QUEST_DELETE_SAVE_TYPE);
 p.Add(38377);p.AddQuestDelayedEvent(38377,30000,[&](){++current;});p.Run();assert(old==0&&current==1&&other==1&&p.phase.notifications==1);
 // Retained status reset (repeatable acceptance/NONE) expires old callbacks too.
 p.AddQuestDelayedEvent(38377,1,[&](){++old;});p.getQuestStatus(38377)->ScriptLifetime.reset();p.Run();assert(!old);
 p.AddQuestDelayedEvent(999,1,[&](){++old;});assert(p.events.empty());
 p.inventory[139043]=1;p.inventory[42]=7;p.Add(40890);
 assert(p.TakeQuestSourceItem(40890,true));p.RemoveActiveQuest(40890);assert(!p.inventory[139043]&&p.inventory[42]==7);
 // Both reward and abandon call this production cleanup; repeated cleanup is harmless.
 assert(p.TakeQuestSourceItem(40890,true));p.inventory[139043]=1;assert(p.TakeQuestSourceItem(40890,true));assert(!p.inventory[139043]);
 p.inventory[139043]=1;p.equipped=true;assert(!p.TakeQuestSourceItem(40890,true)&&p.inventory[139043]==1&&p.errors==1);
 p.equipped=false;auto& q=store.quests[40890];q.SourceItemId=0;q.flags=QUEST_SPECIAL_FLAGS_NOT_REMOVE_SOURCE;
 assert(p.TakeQuestSourceItem(40890,true)&&p.inventory[139043]==1);
 std::cout<<"PASS: actual quest status deletion invalidates delayed callbacks before same-tick reaccept; unrelated quest events survive; source/drop item cleanup, repeated removal, inventory isolation and failure/preservation guards.\n";
}

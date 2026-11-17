#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint8=uint8_t; using uint32=uint32_t; using ObjectGuid=uint32;
#include "Enums.inc"
enum { INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15,EQUIPMENT_SLOT_OFFHAND=16,
 ITEM_FIELD_CREATOR,ITEM_FIELD_DYNAMIC_FLAGS,ITEM_FLAG_CHILD,PLAYER_FIELD_CURRENT_SPEC_ID,
 QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6 };
using QuestStatus=int;
struct ArtifactEntry { uint32 ChrSpecializationID; };
struct ChrSpecializationEntry { uint32 ID,ClassID; };
template<class T> struct Store { std::map<uint32,T> rows;
 T const* LookupEntry(uint32 id) const {auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;} };
Store<ArtifactEntry> sArtifactStore; Store<ChrSpecializationEntry> sChrSpecializationStore;
struct ItemTemplate { uint32 artifact; uint32 GetArtifactID()const{return artifact;} };
struct Item {uint32 entry=128862,owner=1,count=1;bool carried=true,equipped=false;
 ItemTemplate proto{1}; uint32 GetEntry()const{return entry;}uint32 GetOwnerGUID()const{return owner;}
 uint32 GetCount()const{return count;}bool IsEquipped()const{return equipped;}
 ItemTemplate const* GetTemplate()const{return &proto;}uint32 GetChildItem()const{return 0;}
 uint32 GetGUID()const{return 2;}uint32 GetGuidValue(int)const{return 0;}bool HasFlag(int,int)const{return false;}};
struct PhaseUpdateData {int type=0;void AddConditionType(int t){type=t;}};
struct PhaseMgr {int notifications=0;void NotifyConditionChanged(PhaseUpdateData u){assert(u.type==3);++notifications;}
 static bool IsConditionTypeSupported(ConditionTypes conditionType);};
#include "Supported.inc"
struct ObjectMgr {ItemTemplate artifact{1},ordinary{0};ItemTemplate const* GetItemTemplate(uint32 id)const
 {return id==128862?&artifact:id==1?&ordinary:nullptr;}} objectMgr;
ObjectMgr* sObjectMgr=&objectMgr;
struct Player;using Unit=Player;
struct Threat {Player* getHostilTarget(){return nullptr;}};
struct Player {uint32 spec=62;int status=QUEST_STATUS_INCOMPLETE;bool inWorld=true;Item item;PhaseMgr phase;
 uint32 GetGUID()const{return 1;}uint32 getClass()const{return 8;}uint32 GetSpecializationId()const{return spec;}
 Item* GetItemByEntry(uint32 id)const{return item.entry==id&&item.carried?const_cast<Item*>(&item):nullptr;}
 Item* GetItemByPos(uint8,uint8 slot)const{return item.carried&&item.equipped&&slot==15?const_cast<Item*>(&item):nullptr;}
 Item* GetChildItemByGuid(uint32)const{return nullptr;}
 bool HasItemOrGemWithIdEquipped(uint32 id,int)const{return id==item.entry&&item.carried&&item.equipped;}
 uint32 GetUInt32Value(int)const{return spec;}int GetQuestStatus(uint32 q)const{assert(q==42429);return status;}
 bool GetQuestRewardStatus(uint32)const{return status==QUEST_STATUS_REWARDED;}
 bool IsCreature()const{return false;}Player* ToPlayer(){return this;}Unit* ToUnit(){return this;}
 Threat threat;Threat& getThreatManager(){return threat;}bool isPet(){return false;}Player* GetOwner(){return nullptr;}
 bool IsInWorld()const{return inWorld;}PhaseMgr& GetPhaseMgr(){return phase;}
 void ItemAddedQuestCheck(uint32 entry,uint32 count);void ItemRemovedQuestCheck(uint32 entry,uint32 count);
};
#include "Inventory.inc"
#include "ItemAddedQuestCheck.inc"
#include "ItemRemovedQuestCheck.inc"
struct Condition;
struct SourceInfo {Condition* mLastFailedCondition=nullptr;};
struct ScriptMgr {bool OnConditionCheck(Condition*,SourceInfo&); } scriptMgr;
ScriptMgr* sScriptMgr=&scriptMgr;Player* active=nullptr;
struct Condition {ConditionTypes ConditionType;uint32 ConditionValue1;bool NegativeCondition;std::string script;
 bool Meets(Player* object) {SourceInfo sourceInfo;bool condMeets=false;
 switch(ConditionType){
#include "Conditions.inc"
 default:assert(false);}
#include "ConditionResult.inc"
 }};
bool ScriptMgr::OnConditionCheck(Condition* c,SourceInfo&)
 {if(c->script.empty())return true;assert(c->script=="condition_artifact_inventory_128862");
 return ArtifactAcquisitionInventory::Ready(active,128862,15);}
int main(int argc,char** argv) {
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
 sArtifactStore.rows[1]={64};sChrSpecializationStore.rows={{62,{62,8}},{63,{63,8}},{64,{64,8}}};
 std::map<int,std::vector<Condition>> groups;std::ifstream f(argv[1]);assert(f);std::string line;int rows=0;
 while(std::getline(f,line)){std::istringstream r(line);int g,t,v,n;r>>g>>t>>v>>n;std::string s;r>>s;
 assert(PhaseMgr::IsConditionTypeSupported(ConditionTypes(t)));groups[g].push_back({ConditionTypes(t),uint32(v),n!=0,s});++rows;}
 assert(rows==15&&groups.size()==6);assert(!PhaseMgr::IsConditionTypeSupported(CONDITION_TYPE_MASK));
 Player p;active=&p;auto meets=[&]{for(auto& g:groups){bool all=true;for(auto& c:g.second)all=c.Meets(&p)&&all;if(all)return true;}return false;};
 int cases=0;
 for(int q:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED})
 for(uint32 spec:{62u,63u,64u})for(bool equipped:{false,true})for(bool carried:{false,true}) {
  p.status=q;p.spec=spec;p.item.equipped=equipped;p.item.carried=carried;
  assert(meets()==(q!=QUEST_STATUS_NONE&&carried&&(equipped||spec!=64)));++cases;
 }
 p.status=QUEST_STATUS_INCOMPLETE;p.spec=62;p.item={};assert(meets());
 p.item.owner=2;assert(!meets());p.item.owner=1;p.item.count=0;assert(!meets());p.item.count=1;
 // Negation must never negate the script: absent/banked artifacts cannot pass.
 p.item.carried=false;assert(!meets());p.item.carried=true;assert(meets());
 for(bool added:{false,true})for(bool world:{false,true})for(uint32 count:{0u,1u})for(uint32 entry:{128862u,1u,999u}) {
  p.inWorld=world;int before=p.phase.notifications;
  if(added)p.ItemAddedQuestCheck(entry,count);else p.ItemRemovedQuestCheck(entry,count);
  assert(p.phase.notifications-before==int(world&&count&&entry==128862));++cases;
 }
 std::cout<<"PASS "<<cases<<" native phase admission/notification cases; quest and spec transitions, bag/bank, owner/count, native negation/script order; controlled world endpoints\n";
}

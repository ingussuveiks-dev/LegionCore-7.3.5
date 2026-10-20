#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
using uint16=uint16_t;
constexpr int QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6;
constexpr int GAMEOBJECT_TYPE_CHEST=3,UPDATETYPE_VALUES=0;
constexpr int GAMEOBJECT_FIELD_FLAGS=0,OBJECT_FIELD_DYNAMIC_FLAGS=1;
constexpr uint32 GO_FLAG_NOT_SELECTABLE=16,GO_FLAG_LOCKED=2,GO_DYNFLAG_LO_ACTIVATE=1,GO_DYNFLAG_LO_SPARKLE=8;
struct Quest { uint32 id; };
struct Player {
    std::map<uint32,int> quests;
    std::set<uint32> available;
    int GetQuestStatus(uint32 id) { auto it=quests.find(id);return it==quests.end()?0:it->second; }
    bool CanTakeQuest(Quest const* q,bool) { return available.count(q->id)!=0; }
};
struct Unit { Player* player; Player* ToPlayer(){return player;} };
using Relations=std::multimap<uint32,uint32>;
struct QuestStore {
    Relations starts{{245793,40606}},involved{{245793,40604},{245793,40835},{245793,40932}};
    Quest compass{40606};
    auto GetGOQuestRelationBounds(uint32 entry) { return starts.equal_range(entry); }
    auto GetGOQuestInvolvedRelationBounds(uint32 entry) { return involved.equal_range(entry); }
    Quest const* GetQuestTemplate(uint32 id) { return id==40606?&compass:nullptr; }
} store;
auto sQuestDataStore=&store;
struct Buffer { std::vector<uint32> values; Buffer& operator<<(uint32 value){values.push_back(value);return *this;} };
struct GameObject {
    uint32 entry=245788,map=0;
    uint32 m_uint32Values[2]={0,0};
    bool _changesMask[2]={false,false};
    uint32 _fieldNotifyFlags=0;
    int uses=0;
    struct {struct {bool usegrouplootrules=false;}chest;} info;
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return map;}
    int GetGoType() const{return GAMEOBJECT_TYPE_CHEST;}
    auto GetGOInfo() const{return &info;}
    bool HasLootRecipient() const{return false;}
    bool IsLootAllowedFor(Player*) const{return true;}
    bool IsQuestObjectExpired() const{return false;}
    void Use(Unit* user);
    uint32 Flags(Player* target) const;
    uint16 DynamicFlags(Player* target) const;
    bool SendsField(Player* target,int index) const;
};
#include "CampHelpers.inc"
void GameObject::Use(Unit* user) {
#include "UseAdmission.inc"
    ++uses;
}
uint32 GameObject::Flags(Player* target) const {
    auto output=Buffer{}; auto data=&output; int index=GAMEOBJECT_FIELD_FLAGS;
#include "UpdateSetup.inc"
    (void)forcedFlags;
    if(false){}
#include "FlagsBranch.inc"
    return output.values.at(0);
}
bool GameObject::SendsField(Player* target,int index) const {
    uint32 flags[2]={1,1},visibleFlag=1;int updateType=UPDATETYPE_VALUES;
#include "UpdateSetup.inc"
    (void)canUseCampObject;
    return
#include "UpdatePredicate.inc"
    ;
}
uint16 GameObject::DynamicFlags(Player* target) const {
#include "UpdateSetup.inc"
    (void)forcedFlags;
    uint16 dynFlags=GO_DYNFLAG_LO_ACTIVATE|GO_DYNFLAG_LO_SPARKLE|0x40;
#include "DynamicGate.inc"
    return dynFlags;
}
struct PhaseUpdateData { void AddQuestUpdate(uint32){} };
struct PhaseMgr { int notifications=0; void NotifyConditionChanged(PhaseUpdateData const&){++notifications;} };
struct RefreshPlayer {
    PhaseMgr phases; int updates=0;
    PhaseMgr& GetPhaseMgr(){return phases;}
    void UpdateForQuestWorldObjects(){++updates;}
    void Removed(uint32 quest_id) {
#include "RemovalRefresh.inc"
    }
};
int main() {
    Player player,other; GameObject object;
    for(uint32 quest:{40604u,40835u,40932u}) {
        for(int state:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}) {
            player.quests.clear();player.quests[quest]=state;
            for(uint32 entry:{245785u,245786u,245787u,245788u,245789u}) {
                object.entry=entry;bool allowed=state==QUEST_STATUS_INCOMPLETE;
                assert(CanUseAridenCampObject(&object,&player)==allowed);
                Unit user{&player};int before=object.uses;object.Use(&user);assert(object.uses==before+(allowed?1:0));
                assert(bool(object.Flags(&player)&GO_FLAG_NOT_SELECTABLE)==!allowed);
                assert(object.DynamicFlags(&player)==(allowed?(0x40|GO_DYNFLAG_LO_ACTIVATE|GO_DYNFLAG_LO_SPARKLE):0x40));
                assert(object.SendsField(&player,GAMEOBJECT_FIELD_FLAGS));
                assert(object.SendsField(&player,OBJECT_FIELD_DYNAMIC_FLAGS));
                GameObject journal; journal.entry=245793;
                assert(CanUseAridenCampObject(&journal,&player)==(state==QUEST_STATUS_COMPLETE));
            }
        }
    }
    // Another player keeps their own active investigation on the same object.
    player.quests={{40604,QUEST_STATUS_REWARDED}};other.quests={{40604,QUEST_STATUS_INCOMPLETE}};
    object.entry=245788;object.m_uint32Values[0]=0x80;
    assert(object.Flags(&player)==(0x80|GO_FLAG_NOT_SELECTABLE));assert(object.Flags(&other)==0x80);
    assert(object.m_uint32Values[0]==0x80);
    object.entry=245793;player.available.insert(40606);assert(CanUseAridenCampObject(&object,&player));
    player.available.clear();player.quests[40606]=QUEST_STATUS_INCOMPLETE;assert(CanUseAridenCampObject(&object,&player));
    player.quests[40606]=QUEST_STATUS_COMPLETE;assert(CanUseAridenCampObject(&object,&player));
    player.quests[40606]=QUEST_STATUS_REWARDED;assert(!CanUseAridenCampObject(&object,&player));
    // The follow-up compass remains usable until its own quest completes.
    object.entry=245794;player.quests[40606]=QUEST_STATUS_INCOMPLETE;assert(CanUseAridenCampObject(&object,&player));
    player.quests[40606]=QUEST_STATUS_COMPLETE;assert(!CanUseAridenCampObject(&object,&player));
    // An unrelated object or another map receives no new restriction or forced field.
    object.entry=123;assert(CanUseAridenCampObject(&object,&player));assert(!object.SendsField(&player,OBJECT_FIELD_DYNAMIC_FLAGS));
    object.entry=245788;object.map=1;assert(CanUseAridenCampObject(&object,&player));
    RefreshPlayer refresh;for(uint32 q:{40604u,40835u,40932u,40606u})refresh.Removed(q);
    assert(refresh.updates==4 && refresh.phases.notifications==4);refresh.Removed(123);assert(refresh.updates==4 && refresh.phases.notifications==5);
    std::cout<<"PASS: native camp eligibility, use admission, emitted per-player flags and zero-dirty field selection; three routes, hand-in/next quest, compass, two-player isolation and removal refresh\n";
}

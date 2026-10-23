#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum class HighGuid {Player};
enum {CONVERSATION_FIELD_LAST_LINE_END_TIME=7};
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid {
    uint64 low=0;
    template<HighGuid>static ObjectGuid Create(uint64 value){return {value};}
    uint64 GetLowPart()const{return low;}uint64 GetHighPart()const{return 1;}
};
struct Creature {uint32 identity=0;ObjectGuid GetGUID(){return {identity};}} vereesa{100397},orestes{100398};
struct WorldObject {
    std::map<uint32,Creature*> nearby{{100397,&vereesa},{100398,&orestes}};
    Creature* FindNearestCreature(uint32 identity,float){auto found=nearby.find(identity);return found==nearby.end()?nullptr:found->second;}
    float GetVisibilityRange(){return 100;}
};
using Unit=WorldObject;
#include "ConversationStructs.inc"
struct Store {
    std::vector<ConversationCreature> creatures;std::vector<ConversationData> lines;std::vector<ConversationActor> actors;
    auto GetConversationData(uint32){return &lines;}auto GetConversationCreature(uint32){return &creatures;}auto GetConversationActor(uint32){return &actors;}
} store;
auto sConversationDataStore=&store;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
uint32 currentTime=0;uint32 getMSTime(){return currentTime;}
struct Conversation {
    uint32 entry=1072,deadline=0,lifetime=0,casterGUID=42;bool questExpired=false,removed=false;
    std::map<uint32,uint32> playing{{42,100}};
    std::map<uint32,ConversationDynamicFieldCreatures> creatures;std::map<uint32,ConversationDynamicFieldLines> lines;
    void SetActor(ConversationDynamicFieldActors*,uint32,bool){assert(false);}
    void SetCreature(ConversationDynamicFieldCreatures* value,uint32 index,bool){creatures[index]=*value;}
    void SetLine(ConversationDynamicFieldLines* value,uint32 index,bool){lines[index]=*value;}
    void SetUInt32Value(uint32 field,uint32 value){assert(field==7);deadline=value;}
    uint32 GetUInt32Value(uint32 field){assert(field==7);return deadline;}
    void SetDuration(int32 value){assert(value>0);lifetime=value;}uint32 GetDuration(){return lifetime;}
    bool IsQuestObjectExpired(){return questExpired;}void Remove(){removed=true;}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
    bool Start(Unit* caster){uint32 triggerEntry=entry;
#include "Lifetime.inc"
        return true;
    }
    void Update(uint32);
};
#include "ConversationData.inc"
#include "Update.inc"
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);std::string text;
    while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
        if(kind=="creature"){ConversationCreature value{};assert(row>>value.entry>>value.id>>value.creatureId>>value.creatureGuid>>value.unk1>>value.unk2>>value.duration);store.creatures.push_back(value);}
        else if(kind=="line"){ConversationData value{};assert(row>>value.entry>>value.id>>value.idx>>value.textId>>value.unk1>>value.unk2);store.lines.push_back(value);}
        else assert(false);
    }
    assert(store.creatures.size()==3&&store.lines.size()==6);Unit finder;
    auto fixed=store.creatures;store.creatures[0].duration=2650;
    {Conversation baseline;assert(baseline.Start(&finder));assert(baseline.deadline==12950);currentTime=100+12951;baseline.Update(1);assert(baseline.removed);
        uint32 omitted=0;for(auto const& row:store.lines)if(row.textId>=baseline.deadline)++omitted;assert(omitted==3);
    }
    store.creatures=fixed;
    {Conversation repaired;assert(repaired.Start(&finder));assert(repaired.deadline==35300&&repaired.lifetime==35300&&repaired.lines.size()==6&&repaired.creatures.size()==3);
        assert(repaired.creatures[0].GuidLow==100397&&repaired.creatures[1].GuidLow==100398&&repaired.creatures[2].GuidLow==0xFFFFFFFFFF);
        for(auto const& row:store.lines){assert(row.textId<repaired.deadline);currentTime=100+row.textId;repaired.Update(1);assert(!repaired.removed);}
        currentTime=100+30028;repaired.Update(1);assert(!repaired.removed);currentTime=100+35300;repaired.Update(1);assert(!repaired.removed);
        currentTime=100+35301;repaired.Update(1);assert(repaired.removed);
    }
    for(uint32 missing:{100397u,100398u}){Unit unavailable;unavailable.nearby.erase(missing);Conversation rejected;assert(!rejected.Start(&unavailable));}
    {Conversation cleanup;assert(cleanup.Start(&finder));cleanup.questExpired=true;currentTime=101;cleanup.Update(1);assert(cleanup.removed);}
    {Conversation replay;assert(replay.Start(&finder)&&replay.deadline==35300);}
    std::cout<<"PASS: actual conversation payload/lifetime/update bodies reproduce12950ms expiry before3 lines; repaired35300ms lifetime retains all6 native slots, player sentinel and live actor GUIDs, then expires normally or on scope expiry. Missing actors reject creation; map/clock/visibility remain controlled hosts.\n";
}

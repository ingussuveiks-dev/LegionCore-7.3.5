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
    bool player=true;uint32 guid=42;bool IsPlayer()const{return player;}uint32 GetGUID()const{return guid;}
    std::map<uint32,Creature*> nearby{{100397,&vereesa},{100398,&orestes}};
    Creature* FindNearestCreature(uint32 identity,float){auto found=nearby.find(identity);return found==nearby.end()?nullptr:found->second;}
    float GetVisibilityRange(){return 100;}
};
using Unit=WorldObject;
#include "ConversationStructs.inc"
struct Store {
    std::vector<ConversationCreature> creatures;std::vector<ConversationData> lines;std::vector<ConversationActor> actors;
#ifdef MARKSMANSHIP_INTERROGATION_TEST
    uint32 identity=0;
    auto GetConversationData(uint32 entry){return identity==entry?&lines:nullptr;}
    auto GetConversationCreature(uint32 entry){return identity==entry?&creatures:nullptr;}
    auto GetConversationActor(uint32 entry){return identity==entry?&actors:nullptr;}
#else
    auto GetConversationData(uint32){return &lines;}auto GetConversationCreature(uint32){return &creatures;}auto GetConversationActor(uint32){return &actors;}
#endif
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
#ifdef MARKSMANSHIP_INTERROGATION_TEST
    std::map<uint32,ConversationDynamicFieldActors> actors;
    void SetActor(ConversationDynamicFieldActors* value,uint32 index,bool){actors[index]=*value;}
#else
    void SetActor(ConversationDynamicFieldActors*,uint32,bool){assert(false);}
#endif
    void SetCreature(ConversationDynamicFieldCreatures* value,uint32 index,bool){creatures[index]=*value;}
    void SetLine(ConversationDynamicFieldLines* value,uint32 index,bool){lines[index]=*value;}
    void SetUInt32Value(uint32 field,uint32 value){assert(field==7);deadline=value;}
    uint32 GetUInt32Value(uint32 field)const{assert(field==7);return deadline;}
    void SetDuration(int32 value){assert(value>0);lifetime=value;}uint32 GetDuration(){return lifetime;}
    bool IsQuestObjectExpired(){return questExpired;}void Remove(){removed=true;}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
    bool CanNeverSee2(WorldObject const*)const;
    bool Start(Unit* caster){uint32 triggerEntry=entry;
#include "Lifetime.inc"
        return true;
    }
    void Update(uint32);
};
#include "ConversationData.inc"
#include "Update.inc"
#include "Playback.inc"
#ifndef MARKSMANSHIP_INTERROGATION_TEST
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);std::string text;
    while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
        if(kind=="creature"){ConversationCreature value{};assert(row>>value.entry>>value.id>>value.creatureId>>value.creatureGuid>>value.unk1>>value.unk2>>value.duration);store.creatures.push_back(value);}
        else if(kind=="line"){ConversationData value{};assert(row>>value.entry>>value.id>>value.idx>>value.textId>>value.unk1>>value.unk2);store.lines.push_back(value);}
        else assert(false);
    }
    assert(store.creatures.size()==3&&store.lines.size()==6);Unit finder;
    uint32 fixedDeadline=5000;for(auto const& actor:store.creatures)fixedDeadline+=actor.duration;
    assert(fixedDeadline==35300||fixedDeadline==50300);
    auto fixed=store.creatures;store.creatures[0].duration=2650;
    {Conversation baseline;assert(baseline.Start(&finder));assert(baseline.deadline==12950);currentTime=100+12951;baseline.Update(1);assert(baseline.removed);
        uint32 omitted=0;for(auto const& row:store.lines)if(row.textId>=baseline.deadline)++omitted;assert(omitted==3);
    }
    store.creatures=fixed;
    if(fixedDeadline==50300)
    {
        store.creatures[0].duration=25000;
        Conversation previous;
        assert(previous.Start(&finder)&&previous.deadline==35300);
        currentTime=100+37173;
        previous.Update(1);
        assert(previous.removed);
        store.creatures=fixed;
    }
    {Conversation repaired;assert(repaired.Start(&finder));assert(repaired.deadline==fixedDeadline&&repaired.lifetime==fixedDeadline&&repaired.lines.size()==6&&repaired.creatures.size()==3);
        assert(repaired.creatures[0].GuidLow==100397&&repaired.creatures[1].GuidLow==100398&&repaired.creatures[2].GuidLow==0xFFFFFFFFFF);
        for(auto const& row:store.lines){assert(row.textId<repaired.deadline);currentTime=100+row.textId;repaired.Update(1);assert(!repaired.removed);}
        if(fixedDeadline==50300)
            for(uint32 checkpoint:{35301u,37173u,44151u,49151u}){currentTime=100+checkpoint;repaired.Update(1);assert(!repaired.removed);}
        currentTime=100+30028;repaired.Update(1);assert(!repaired.removed);currentTime=100+fixedDeadline;repaired.Update(1);assert(!repaired.removed);
        currentTime=100+fixedDeadline+1;repaired.Update(1);assert(repaired.removed);
    }
    for(uint32 missing:{100397u,100398u}){Unit unavailable;unavailable.nearby.erase(missing);Conversation rejected;assert(!rejected.Start(&unavailable));}
    {Conversation cleanup;assert(cleanup.Start(&finder));cleanup.questExpired=true;currentTime=101;cleanup.Update(1);assert(cleanup.removed);}
    {Conversation replay;assert(replay.Start(&finder)&&replay.deadline==fixedDeadline);}
    {WorldObject petCaster;petCaster.player=false;petCaster.guid=7;Conversation misowned;misowned.casterGUID=7;misowned.playing.clear();assert(misowned.Start(&petCaster));
        currentTime=100;assert(!misowned.CanNeverSee2(&finder)&&misowned.CanNeverSee2(&petCaster)&&!misowned.playing.count(7));
        currentTime=100+fixedDeadline+1;misowned.Update(1);assert(!misowned.removed);
        Conversation owned;owned.playing.clear();assert(owned.Start(&finder));currentTime=100;assert(!owned.CanNeverSee2(&finder)&&owned.playing.count(42));
        currentTime=100+fixedDeadline+1;owned.Update(1);assert(owned.removed);
    }
    std::cout<<"PASS: actual conversation payload/lifetime/update bodies reproduce12950ms expiry before3 lines; configured"<<fixedDeadline<<"ms lifetime retains all6 native slots, player sentinel and live actor GUIDs, then expires normally or on scope expiry. Missing actors reject creation; map/clock/visibility remain controlled hosts.\n";
}
#endif

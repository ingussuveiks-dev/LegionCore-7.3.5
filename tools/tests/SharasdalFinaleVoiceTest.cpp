#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <memory>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum class HighGuid {Player,Conversation};
enum {CONVERSATION_FIELD_LAST_LINE_END_TIME=7};
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid {
    uint64 low=0;
    template<HighGuid>static ObjectGuid Create(uint64 value){return {value};}
    uint64 GetLowPart()const{return low;}uint64 GetHighPart()const{return 1;}
};
struct Creature {uint32 identity=0;bool alive=true;ObjectGuid GetGUID(){return {identity};}};
struct WorldObject;using Player=WorldObject;
struct GroupReference{WorldObject* member=nullptr;GroupReference* nextMember=nullptr;WorldObject* getSource(){return member;}GroupReference* next(){return nextMember;}};
struct Group{GroupReference* first=nullptr;GroupReference* GetFirstMember(){return first;}};
struct Position{float x=0,y=0,z=0;};
struct WorldObject {
 bool player=true;uint32 guid=42;Group* group=nullptr;
 bool IsPlayer()const{return player;}uint32 GetGUID()const{return guid;}Player* ToPlayer(){return player?this:nullptr;}Group* GetGroup(){return group;}Position GetPosition()const{return {float(guid),2,3};}
 std::map<uint32,Creature*> nearby;
 Creature* FindNearestCreature(uint32 identity,float){auto found=nearby.find(identity);return found==nearby.end()||!found->second||!found->second->alive?nullptr:found->second;}
 float GetVisibilityRange(){return 100;}
};
using Unit=WorldObject;
#include "ConversationStructs.inc"
struct Store {
    std::vector<ConversationCreature> creatures;std::vector<ConversationData> lines;std::vector<ConversationActor> actors;
    uint32 identity=0;
    auto GetConversationData(uint32 entry){return identity==entry?&lines:nullptr;}
    auto GetConversationCreature(uint32 entry){return identity==entry?&creatures:nullptr;}
    auto GetConversationActor(uint32 entry){return identity==entry?&actors:nullptr;}
} store;
auto sConversationDataStore=&store;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
uint32 currentTime=0;uint32 getMSTime(){return currentTime;}
struct Conversation {
    uint32 entry=786,deadline=0,lifetime=0,casterGUID=42;bool questExpired=false,removed=false;
    std::map<uint32,uint32> playing;std::set<uint32> personal;Position createdAt;
    void AddPlayerInPersonnalVisibilityList(uint32 id){personal.insert(id);}
    bool CreateConversation(uint32,uint32,Unit*,void const*,Position const&);
    std::map<uint32,ConversationDynamicFieldCreatures> creatures;std::map<uint32,ConversationDynamicFieldLines> lines;
    std::map<uint32,ConversationDynamicFieldActors> actors;
    void SetActor(ConversationDynamicFieldActors* value,uint32 index,bool){actors[index]=*value;}
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
std::vector<std::unique_ptr<Conversation>> live;
bool Conversation::CreateConversation(uint32,uint32 triggerEntry,Unit* caster,void const*,Position const& pos){
 if(!caster)return false;entry=triggerEntry;casterGUID=caster->GetGUID();createdAt=pos;playing.clear();personal.clear();
#include "PlayerVisibility.inc"
 if(!Start(caster))return false;live.emplace_back(this);return true;
}
static void Load(char const* path,uint32 entry,uint32 duration){
 std::ifstream input(path);assert(input);std::string text;store={};store.identity=entry;
 while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
  if(kind=="line"){ConversationData value{};assert(row>>value.entry>>value.id>>value.textId>>value.unk1>>value.unk2>>value.idx);if(value.entry==entry)store.lines.push_back(value);}
  else if(kind=="creature"){ConversationCreature value{};assert(row>>value.entry>>value.id>>value.creatureId>>value.creatureGuid>>value.unk1>>value.unk2>>value.duration);if(value.entry==entry)store.creatures.push_back(value);}else assert(false);
 }
 uint32 sum=0;for(auto const& actor:store.creatures)sum+=actor.duration;
 assert(store.actors.empty()&&store.creatures.size()==3&&store.lines.size()==(entry==1906?5:4)&&sum==duration);
}
static Conversation* Create(WorldObject& caster,uint32 entry){
 live.clear();auto value=new Conversation;
 if(!value->CreateConversation(1,entry,&caster,nullptr,caster.GetPosition())){delete value;return nullptr;}return value;
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);unsigned cases=0;
 WorldObject owner,peer,unrelated;peer.guid=43;unrelated.guid=44;
 Creature erunak{102826},lady{104856},grash{105298},adelee{104853};
 owner.nearby={{102826,&erunak},{104856,&lady},{105298,&grash},{104853,&adelee}};
 for(uint32 entry:{1906u,1923u}){
  uint32 original=entry==1906?8433:13800,required=entry==1906?23788:23727;
  Load(argv[1],entry,original);currentTime=100;auto old=Create(owner,entry);assert(old&&old->lifetime==original+5000);
  assert(!old->CanNeverSee2(&owner));currentTime+=original+5001;old->Update(1);assert(old->removed&&original+5000<store.lines.back().textId);++cases;
  Load(argv[2],entry,required);currentTime=100;auto fixed=Create(owner,entry);
  assert(fixed&&fixed->lifetime==required+5000&&fixed->deadline==required+5000&&fixed->creatures.size()==3&&fixed->actors.empty()&&fixed->lines.size()==store.lines.size());
  assert(fixed->personal==std::set<uint32>{42}&&!fixed->CanNeverSee2(&owner));
  for(auto const& line:store.lines){auto const& wire=fixed->lines.at(line.idx);assert(wire.id==line.id&&wire.textId==line.textId&&wire.unk1==line.unk1&&wire.unk2==line.unk2);}
  currentTime=100+original+5001;fixed->Update(1);assert(!fixed->removed);currentTime=100+required;fixed->Update(1);assert(!fixed->removed);
  currentTime=100+required+5000;fixed->Update(1);assert(!fixed->removed);++currentTime;fixed->Update(1);assert(fixed->removed);++cases;
  GroupReference second{&peer,nullptr},first{&owner,&second};Group group{&first};owner.group=&group;
  currentTime=100;auto shared=Create(owner,entry);assert(shared&&shared->personal==std::set<uint32>({42,43})&&!shared->personal.count(unrelated.guid));
  assert(!shared->CanNeverSee2(&peer));currentTime+=100000;shared->Update(1);assert(!shared->removed);assert(!shared->CanNeverSee2(&owner));currentTime+=required+5001;shared->Update(1);assert(shared->removed);owner.group=nullptr;++cases;
  auto definition=store;
  for(auto const& actor:store.creatures){auto saved=owner.nearby[actor.creatureId];owner.nearby[actor.creatureId]=nullptr;assert(!Create(owner,entry));owner.nearby[actor.creatureId]=saved;++cases;}
  if(entry==1906){lady.alive=false;assert(!Create(owner,entry));lady.alive=true;++cases;/* Existing living-actor admission is NOT repaired by a duration change. */}
  store.lines.clear();assert(!Create(owner,entry));store=definition;++cases;
  currentTime=100;auto expired=Create(owner,entry);assert(expired);expired->questExpired=true;expired->Update(1);assert(expired->removed);++cases;
  currentTime=100;auto replay=Create(owner,entry);assert(replay&&!replay->CanNeverSee2(&owner)&&replay->deadline==required+5000);++cases;
 }
 std::cout<<"PASS "<<cases<<" native Conversation GenerateDynamicFieldData/creation lifetime/personal visibility/CanNeverSee2/Update contexts: original1906 13433 and1923 18800 expire before final-line start; repaired28788/28727 cover actual voice budgets plus native5000. Original9 line payloads/3+3 creature binding, strict deadline/quest cleanup/replay and group-first owner clock retained. Missing actors, including dead Lady, still reject creation: caller/defeat/pickup timing and client playback remain OPEN.\n";
}

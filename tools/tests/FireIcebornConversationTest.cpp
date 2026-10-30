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
struct Creature {uint32 identity=0;ObjectGuid GetGUID(){return {identity};}};
struct WorldObject;using Player=WorldObject;
struct GroupReference{WorldObject* member=nullptr;GroupReference* nextMember=nullptr;WorldObject* getSource(){return member;}GroupReference* next(){return nextMember;}};
struct Group{GroupReference* first=nullptr;GroupReference* GetFirstMember(){return first;}};
struct Position{float x=0,y=0,z=0;};
struct WorldObject {
 bool player=true;uint32 guid=42;Group* group=nullptr;
 bool IsPlayer()const{return player;}uint32 GetGUID()const{return guid;}Player* ToPlayer(){return player?this:nullptr;}Group* GetGroup(){return group;}Position GetPosition()const{return {float(guid),2,3};}
 std::map<uint32,Creature*> nearby;
 Creature* FindNearestCreature(uint32 identity,float){auto found=nearby.find(identity);return found==nearby.end()?nullptr:found->second;}
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
struct Generator{uint32 Generate(){return 1;}};struct ObjectMgr{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} objectManager;auto sObjectMgr=&objectManager;
using SpellEffIndex=uint32;enum{SPELL_EFFECT_HANDLE_HIT=1};struct EffectInfo{uint32 MiscValue=786;};struct SpellInfo{uint32 Id=197760;EffectInfo effect;EffectInfo* Effects[1]={&effect};};
struct Destination{Position position;Position GetPosition(){return position;}};
struct Spell{WorldObject* caster=nullptr;uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT;struct{bool destination=false;bool HasDst(){return destination;}}m_targets;Destination destination;Destination* destTarget=&destination;SpellInfo info;
 WorldObject* GetCaster(){return caster;}SpellInfo const* GetSpellInfo(){return &info;}void EffectSummonConversation(SpellEffIndex);
};
#include "SummonConversation.inc"
static Conversation* Cast(WorldObject& caster){live.clear();Spell spell;spell.caster=&caster;spell.EffectSummonConversation(0);assert(live.size()==1);return live.back().get();}
static void Load(char const* path,uint32 expectedDuration){
 std::ifstream input(path);assert(input);std::string text;store={};store.identity=786;
 while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
  if(kind=="actor"){ConversationActor value{};assert(row>>value.entry>>value.id>>value.actorId>>value.creatureId>>value.displayId>>value.unk1>>value.unk2>>value.unk3>>value.duration);store.actors.push_back(value);}
  else if(kind=="line"){ConversationData value{};assert(row>>value.entry>>value.id>>value.idx>>value.textId>>value.unk1>>value.unk2);store.lines.push_back(value);}else assert(false);
 }
 assert(store.actors.size()==1&&store.actors[0].entry==786&&store.actors[0].actorId==50151&&store.actors[0].creatureId==99615&&store.actors[0].displayId==66283&&store.actors[0].duration==expectedDuration&&store.lines.size()==4);
 uint32 starts[]{0,6250,12350,17750};for(unsigned i=0;i<4;++i)assert(store.lines[i].id==1870+i&&store.lines[i].idx==i&&store.lines[i].textId==starts[i]);
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);WorldObject owner,peer,unrelated,pet;peer.guid=43;unrelated.guid=44;pet.player=false;pet.guid=7;
 constexpr uint32 required=48827,repairedLifetime=53827;
 Load(argv[1],6000);currentTime=100;auto old=Cast(pet);assert(old->casterGUID==7&&old->personal.empty()&&old->deadline==11000&&old->actors.size()==1&&old->lines.size()==4);
 assert(!old->CanNeverSee2(&owner)&&old->CanNeverSee2(&pet)&&old->playing.count(42)&&!old->playing.count(7));currentTime+=100000;old->Update(1);assert(!old->removed);
 currentTime=100;auto shortPlayer=Cast(owner);assert(!shortPlayer->CanNeverSee2(&owner));currentTime+=11001;shortPlayer->Update(1);assert(shortPlayer->removed&&11000<14723&&11000<store.lines[2].textId&&11000<store.lines[3].textId);
 Load(argv[2],required);currentTime=100;auto fixed=Cast(owner);assert(fixed->casterGUID==42&&fixed->personal==std::set<uint32>{42}&&fixed->deadline==repairedLifetime&&fixed->lifetime==required+5000&&fixed->actors.size()==1&&fixed->lines.size()==4);
 assert(!fixed->CanNeverSee2(&owner));currentTime+=11001;fixed->Update(1);assert(!fixed->removed);currentTime=100+required;fixed->Update(1);assert(!fixed->removed);currentTime=100+repairedLifetime;fixed->Update(1);assert(!fixed->removed);++currentTime;fixed->Update(1);assert(fixed->removed);
 GroupReference second{&peer,nullptr},first{&owner,&second};Group group{&first};owner.group=&group;
 currentTime=100;auto shared=Cast(owner);assert(shared->personal==std::set<uint32>({42,43})&&!shared->personal.count(unrelated.guid));assert(!shared->CanNeverSee2(&peer));currentTime+=100000;shared->Update(1);assert(!shared->removed);assert(!shared->CanNeverSee2(&owner));currentTime+=repairedLifetime+1;shared->Update(1);assert(shared->removed);owner.group=nullptr;
 auto definition=store;for(bool actorMissing:{false,true}){live.clear();store=definition;if(actorMissing)store.actors.clear();else store.lines.clear();Spell spell;spell.caster=&owner;spell.EffectSummonConversation(0);assert(live.empty());}store=definition;
 currentTime=100;auto expired=Cast(owner);expired->questExpired=true;expired->Update(1);assert(expired->removed);
 live.clear();{Spell spell;spell.caster=&owner;spell.effectHandleMode=0;spell.EffectSummonConversation(0);assert(live.empty());spell.effectHandleMode=1;spell.info.effect.MiscValue=0;spell.EffectSummonConversation(0);assert(live.empty());}
 currentTime=100;auto replay=Cast(owner);assert(!replay->CanNeverSee2(&owner)&&replay->deadline==repairedLifetime);
 std::cout<<"PASS actual native219/786 payload/visibility/playback/update: original pet cannot start own expiry; original Player11000ms expires before first native voice end14723ms and last two offsets. Repaired Player/group personal list,original4 lines+Lyandra actor,48827ms audio/gap+5000ms lifetime,strict expiry,missing definitions/mode/scope cleanup/replay retained. Group-first still awaits owner visibility as native; map/outer delivery/client scheduling remain controlled or separate.\n";
}

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using uint8=uint8_t;
enum {SMART_ACTION_SUMMON_CONVERSATION=206,SMART_TARGET_ACTION_INVOKER=7,SMART_TARGET_PLAYER_DISTANCE=18,SMART_TARGET_CLOSEST_PLAYER=21,SMART_TARGET_ACTION_INVOKER_PLAYER=130};
enum class HighGuid {Player,Conversation};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) ((void)0)
struct Unit;struct Player;struct Creature;
struct ObjectGuid {
    using LowType=uint64;Unit* unit=nullptr;static ObjectGuid Empty;
    bool IsPlayer()const;uint64 GetLowPart()const{return 0;}uint64 GetHighPart()const{return 0;}
    template<HighGuid>static ObjectGuid Create(uint64){return {};}
};ObjectGuid ObjectGuid::Empty;
struct Position {float x=0;void Relocate(float first,float,float,float){x=first;}};
struct Map{} mapOne,mapTwo;
struct WorldObject {
    bool world=true,phase=true,scope=true;Map* map=&mapOne;float distance=1;
    virtual ~WorldObject()=default;virtual Unit* ToUnit(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}
    bool IsInWorld(){return world;}Map* GetMap(){return map;}bool InSamePhase(WorldObject*){return phase;}
    bool IsWithinDistInMap(WorldObject*,float range){return distance<=range;}
    bool HasQuestObjectScope(){return !scope;}bool CanTargetQuestObject(WorldObject*){return scope;}
    Creature* FindNearestCreature(uint32,float){return nullptr;}float GetVisibilityRange(){return 100;}
};
struct Unit:WorldObject {
    Unit* owner=nullptr;Unit* charmer=nullptr;bool available=true;Position position;
    Unit* ToUnit()override{return this;}Player* ToPlayer()override{return nullptr;}virtual Player const* ToPlayer()const{return nullptr;}
    ObjectGuid GetGUID(){return {this};}ObjectGuid GetCharmerOrOwnerGUID()const{return {charmer?charmer:owner};}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;Position GetPosition(){return position;}
};
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {GroupReference first;GroupReference* GetFirstMember(){return &first;}};
struct Player:Unit {Group* group=nullptr;Player* ToPlayer()override{return this;}Player const* ToPlayer()const override{return this;}Group* GetGroup(){return group;}};
struct Creature:Unit{};
bool ObjectGuid::IsPlayer()const{return unit&&unit->ToPlayer();}
struct ObjectAccessor {static Player* GetPlayer(Unit const&,ObjectGuid guid){return guid.unit&&guid.unit->available?guid.unit->ToPlayer():nullptr;}};
#include "Owner.inc"
#include "CreditPlayer.inc"
#include "Resolver.inc"
#include "ConversationStructs.inc"
struct DataStore {
    std::map<uint32,std::vector<ConversationData>> lines;std::map<uint32,std::vector<ConversationActor>> actors;
    auto GetConversationData(uint32 id){return &lines[id];}auto GetConversationActor(uint32 id){return &actors[id];}
    std::vector<ConversationCreature>const* GetConversationCreature(uint32){return nullptr;}
} dataStore;
auto sConversationDataStore=&dataStore;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
struct SpellInfo{};
struct Observation {Player* owner;uint32 entry,duration,line,display;std::set<Unit*> visibility;};
struct Conversation;std::vector<Conversation*> activeObjects;
std::vector<Observation> observations;uint32 attempts=0,objects=0;bool failCreation=false;
struct Conversation {
    std::map<uint8,ConversationDynamicFieldActors> actors;std::map<uint8,ConversationDynamicFieldLines> lines;std::set<Unit*> visibility;
    Conversation(){++objects;}~Conversation(){--objects;}
    float GetPositionX(){return 0;}float GetPositionY(){return 0;}
    void SetActor(ConversationDynamicFieldActors* value,uint8 index,bool){actors[index]=*value;}
    void SetLine(ConversationDynamicFieldLines* value,uint8 index,bool){lines[index]=*value;}
    void SetCreature(ConversationDynamicFieldCreatures*,uint8,bool){assert(false);}
    void AddPlayerInPersonnalVisibilityList(ObjectGuid guid){visibility.insert(guid.unit);}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
    bool CreateConversation(uint64,uint32 id,Unit* caster,SpellInfo const*,Position const&){
        ++attempts;
#include "Visibility.inc"
        uint32 duration=0;if(failCreation||!GenerateDynamicFieldData(id,caster,duration))return false;
        observations.push_back({caster->ToPlayer(),id,duration,lines.at(0).id,actors.at(0).displayId,visibility});activeObjects.push_back(this);return true;
    }
};
#include "ConversationData.inc"
struct Generator {uint64 Generate(){return 1;}} generator;
struct ObjectManager {template<HighGuid>Generator* GetGenerator(){return &generator;}} objectManager;
auto sObjectMgr=&objectManager;
struct Event {
    uint32 type=130;uint32 GetTargetType()const{return type;}
    struct{struct{uint32 dist=100;}playerDistance;}target;
    struct{struct{uint32 id=1123,targetX=0,targetY=0,targetZ=0;}conversation;}action;
};
using ObjectList=std::list<WorldObject*>;
struct Script {
    WorldObject* base=nullptr;Unit* last=nullptr;
    Unit* GetLastInvoker(){return last;}WorldObject* GetBaseObject(){return base;}
    ObjectList* GetTargets(Event const& event,Unit* invoker){
#include "Trigger.inc"
        auto* targets=new ObjectList;
        switch(event.GetTargetType()){
#include "PlayerTarget.inc"
            case 7:if(trigger)targets->push_back(trigger);break;
        }
#include "ScopeTail.inc"
    void Run(Event const& event,Unit* unit){if(unit)last=unit;uint32 before=objects;switch(SMART_ACTION_SUMMON_CONVERSATION){
#include "ConversationAction.inc"
        }for(auto* conversation:activeObjects)delete conversation;activeObjects.clear();assert(objects==before);}
};
bool Valid(Event const& event){switch(event.GetTargetType()){
#include "Validation.inc"
    default:return false;
}return true;}
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);std::string line;
    while(std::getline(input,line)){std::istringstream row(line);std::string type;row>>type;
        if(type=="actor"){ConversationActor actor{};row>>actor.entry>>actor.id>>actor.actorId>>actor.creatureId>>actor.displayId>>actor.unk1>>actor.unk2>>actor.unk3>>actor.duration;dataStore.actors[actor.entry].push_back(actor);}
        if(type=="line"){ConversationData speech{};row>>speech.entry>>speech.id>>speech.idx>>speech.textId>>speech.unk1>>speech.unk2;dataStore.lines[speech.entry].push_back(speech);}
    }
    assert(dataStore.lines.size()==4&&dataStore.actors.size()==4);
    std::map<uint32,std::vector<uint32>> expected{{1123,{2737,18345,3150}},{1009,{2464,4426,6050}},{1555,{3421,4426,4200}},{1395,{3183,67031,6350}}};
    for(auto const& pair:expected){
        Player owner,peer;Creature actor;Unit pet,guardian,charmed,ambient,npcOwner,npcOwned;pet.owner=&owner;guardian.owner=&owner;charmed.charmer=&owner;npcOwned.owner=&npcOwner;
        Script script;script.base=&actor;Event event;event.action.conversation.id=pair.first;
        auto baseline=event;baseline.type=7;auto previous=attempts;script.Run(baseline,&pet);assert(attempts==previous&&script.last==&pet);
        for(Unit* cause:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed}){
            previous=attempts;script.Run(event,cause);assert(attempts==previous+1&&script.last==cause);
            auto const& result=observations.back();assert(result.owner==&owner&&result.line==pair.second[0]&&result.display==pair.second[1]&&result.duration==pair.second[2]&&result.visibility==std::set<Unit*>{&owner});
        }
        Group group;GroupReference second{&peer,nullptr};group.first={&owner,&second};owner.group=&group;script.Run(event,&pet);assert(observations.back().visibility==std::set<Unit*>({&owner,&peer}));owner.group=nullptr;
        for(Unit* cause:{&ambient,&npcOwned,static_cast<Unit*>(nullptr)}){script.last=nullptr;previous=attempts;script.Run(event,cause);assert(attempts==previous&&script.last==cause);}
        script.last=&pet;previous=attempts;script.Run(event,nullptr);assert(attempts==previous+1&&script.last==&pet);
        for(uint32 invalid=0;invalid<9;++invalid){
            previous=attempts;switch(invalid){case 0:owner.world=false;break;case 1:actor.world=false;break;case 2:owner.map=&mapTwo;break;case 3:owner.phase=false;break;case 4:owner.distance=101;break;case 5:owner.available=false;break;case 6:script.base=nullptr;break;case 7:event.target.playerDistance.dist=0;break;case 8:actor.scope=false;break;}
            script.Run(event,&pet);assert(attempts==previous);owner.world=actor.world=true;owner.map=&mapOne;owner.phase=true;owner.distance=1;owner.available=true;actor.scope=true;script.base=&actor;event.target.playerDistance.dist=100;
        }
        previous=attempts;owner.distance=100;script.Run(event,&pet);assert(attempts==previous+1);owner.distance=1;
        failCreation=true;previous=attempts;script.Run(event,&pet);assert(attempts==previous+1&&!objects);failCreation=false;
        auto saved=dataStore.actors[pair.first];dataStore.actors[pair.first].clear();previous=attempts;script.Run(event,&pet);assert(attempts==previous+1&&!objects);dataStore.actors[pair.first]=saved;
        event.action.conversation.targetX=1;script.Run(event,&pet);assert(!observations.back().owner&&observations.back().visibility.empty());event.action.conversation.targetX=0;
        assert(Valid(event));event.target.playerDistance.dist=0;assert(!Valid(event));event.type=131;assert(!Valid(event));
    }
    assert(!objects);
    std::cout<<"PASS: native opt-in player/owner selector, owner lookup, target validation, action206, actual conversation fields and group visibility; original7 still drops pet requests,130 resolves player/pet/guardian/charmer without rewriting raw invoker, rejects remote/missing/unavailable owners, handles failed creation/data, and grants no credit. Creation/map/render/scheduler remain controlled boundaries.\n";
}

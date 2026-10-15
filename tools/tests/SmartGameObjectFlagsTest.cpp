#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using uint32=uint32_t;
#include "Constants.inc"
constexpr uint32 GAMEOBJECT_FIELD_FLAGS=7,GO_FLAG_NOT_SELECTABLE=16;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(false)
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) throw std::runtime_error("Invalid native link")
struct GameObject;
struct WorldObject {bool go=false;GameObject* ToGameObject();};
struct Unit:WorldObject {uint32 flags=0xF00D;};
struct GameObject:WorldObject {
    uint32 flags=0,entry=251879,updates=0,unrelated=0xAABBCCDD;
    GameObject(){go=true;}uint32 GetEntry(){return entry;}
    void SetUInt32Value(uint32 field,uint32 value){CHECK(field==GAMEOBJECT_FIELD_FLAGS);flags=value;++updates;}
    void SetFlag(uint32 field,uint32 value){CHECK(field==GAMEOBJECT_FIELD_FLAGS);flags|=value;++updates;}
    void RemoveFlag(uint32 field,uint32 value){CHECK(field==GAMEOBJECT_FIELD_FLAGS);flags&=~value;++updates;}
};
GameObject* WorldObject::ToGameObject(){return go?static_cast<GameObject*>(this):nullptr;}
struct ObjectList:std::vector<WorldObject*> {
    static int outstanding;ObjectList(){++outstanding;}~ObjectList(){--outstanding;}
};int ObjectList::outstanding=0;
struct SmartScriptHolder {
    uint32 type=0,event_id=1,link=0,entryOrGuid=251879,event=SMART_EVENT_LINK;
    struct {struct {uint32 flag=0;} goFlag;} action;
    uint32 GetActionType()const{return type;}uint32 GetEventType()const{return event;}uint32 GetScriptType()const{return 1;}
};
struct SmartScript {
    bool nullTargets=false;std::vector<WorldObject*> selected;std::map<uint32,SmartScriptHolder> linked;
    int links=0;bool bvar=false;uint32 var0=0,var1=0;void* spell=nullptr;void* gob=nullptr;
    ObjectList* GetTargets(SmartScriptHolder const&,Unit*){if(nullTargets)return nullptr;auto* l=new ObjectList;l->assign(selected.begin(),selected.end());return l;}
    SmartScriptHolder FindLinkedEvent(uint32 id){return linked.at(id);}
    void ProcessEvent(SmartScriptHolder&,Unit*,uint32,uint32,bool,void*,void*){++links;}
    void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr){
        switch(e.GetActionType()){
            #include "FlagActions.inc"
            default:throw std::runtime_error("Unhandled GO flag action");
        }
        #include "LinkTail.inc"
};
int main()try{
    SmartScript s;GameObject manifest,second;Unit actor;WorldObject other;
    SmartScriptHolder e;e.type=SMART_ACTION_REMOVE_GO_FLAG;e.action.goFlag.flag=GO_FLAG_NOT_SELECTABLE;
    manifest.flags=GO_FLAG_NOT_SELECTABLE|1|4;s.selected={&manifest};
    s.ProcessAction(e);CHECK(manifest.flags==5&&manifest.updates==1&&manifest.unrelated==0xAABBCCDD);
    s.ProcessAction(e);CHECK(manifest.flags==5); // replay preserves unrelated bits
    second.flags=0x80000001;s.selected={&manifest,&actor,&other,&second};
    e.type=SMART_ACTION_ADD_GO_FLAG;e.action.goFlag.flag=0x80000010;s.ProcessAction(e);
    CHECK(manifest.flags==0x80000015&&second.flags==0x80000011&&actor.flags==0xF00D);
    e.type=SMART_ACTION_REMOVE_GO_FLAG;e.action.goFlag.flag=0x80000000;s.ProcessAction(e);
    CHECK(manifest.flags==0x15&&second.flags==0x11);
    e.type=SMART_ACTION_SET_GO_FLAG;e.action.goFlag.flag=4;s.ProcessAction(e);CHECK(manifest.flags==4&&second.flags==4);
    for(auto action:{SMART_ACTION_ADD_GO_FLAG,SMART_ACTION_REMOVE_GO_FLAG}){e.type=action;e.action.goFlag.flag=0;s.ProcessAction(e);CHECK(manifest.flags==4&&second.flags==4);}
    e.type=SMART_ACTION_SET_GO_FLAG;
    e.action.goFlag.flag=0;s.ProcessAction(e);CHECK(manifest.flags==0&&second.flags==0);
    // Missing/empty or mixed selectors must not mutate the actor or leak a list.
    for(bool missing:{false,true}){s.nullTargets=missing;s.selected={};s.ProcessAction(e);CHECK(!ObjectList::outstanding);}
    s.nullTargets=false;s.selected={&manifest};e.type=SMART_ACTION_REMOVE_GO_FLAG;e.action.goFlag.flag=16;
    SmartScriptHolder next;next.type=45;next.event_id=2;s.linked[2]=next;e.link=2;
    manifest.flags=16|4;s.ProcessAction(e);CHECK(manifest.flags==4&&s.links==1&&!ObjectList::outstanding);
    s.nullTargets=true;s.ProcessAction(e);s.nullTargets=false;s.selected={};s.ProcessAction(e);
    CHECK(s.links==3&&!ObjectList::outstanding);
    CHECK(actor.flags==0xF00D&&manifest.unrelated==0xAABBCCDD);
    std::cout<<"PASS: production GO flag dispatcher and LINK continuation; manifest unlock, replace/add/remove, repeated/high/zero masks, mixed/multiple/null/empty targets and selector lifetime\n";
}catch(std::exception const& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}

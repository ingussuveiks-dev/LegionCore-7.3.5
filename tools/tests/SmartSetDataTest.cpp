#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <crtdbg.h>
using uint32=uint32_t;using int64=int64_t;
#include "Constants.inc"
#define SI64FMTD "%" PRId64
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(false)
#define ASSERT(x,...) CHECK(x)
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) throw std::runtime_error("Invalid native LINK")
std::vector<std::string> warnings;
template<class... A>void Warn(char const* category,char const* format,A... args){
 CHECK(std::string(category)=="scripts.ai");char text[2048];int n=snprintf(text,sizeof(text),format,args...);CHECK(n>0&&n<int(sizeof(text)));warnings.emplace_back(text);
}
#define TC_LOG_WARN(...) Warn(__VA_ARGS__)
struct Guid{std::string value;std::string ToString()const{return value;}};
struct Creature;struct GameObject;
struct WorldObject{
 enum Kind{OTHER,CREATURE,GAMEOBJECT}kind=OTHER;uint32 entry=0;Guid guid{"Object-Full-128bit-GUID"};
 bool IsCreature()const{return kind==CREATURE;}bool IsGameObject()const{return kind==GAMEOBJECT;}
 Creature* ToCreature();GameObject* ToGameObject();uint32 GetEntry()const{return entry;}Guid const& GetGUID()const{return guid;}
};
struct Unit:WorldObject{};
struct Recorder{uint32 calls=0,field=0,data=0,destroyed=0;std::function<void()> callback;};
struct CreatureAI{Recorder& r;explicit CreatureAI(Recorder& value):r(value){}virtual ~CreatureAI(){++r.destroyed;}virtual void SetData(uint32 f,uint32 d){++r.calls;r.field=f;r.data=d;if(r.callback)r.callback();}};
struct GameObjectAI{Recorder& r;explicit GameObjectAI(Recorder& value):r(value){}virtual ~GameObjectAI()=default;virtual void SetData(uint32 f,uint32 d){++r.calls;r.field=f;r.data=d;if(r.callback)r.callback();}};
struct Creature:Unit{
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;
 Creature(){kind=CREATURE;entry=97774;guid.value="Creature-0-1014-1234-97774-0000000586C5";}
 #include "CreatureAI.inc"
 bool AIM_Destroy();
};
struct GameObject:WorldObject{GameObjectAI* m_AI=nullptr;GameObject(){kind=GAMEOBJECT;entry=251879;guid.value="GameObject-0-1014-1234-251879-0000000586C6";}GameObjectAI* AI()const;};
Creature* WorldObject::ToCreature(){return IsCreature()?static_cast<Creature*>(this):nullptr;}
GameObject* WorldObject::ToGameObject(){return IsGameObject()?static_cast<GameObject*>(this):nullptr;}
struct ObjectList:std::list<WorldObject*>{static int outstanding;ObjectList(){++outstanding;}~ObjectList(){--outstanding;}};int ObjectList::outstanding=0;
struct SmartScriptHolder{
 uint32 type=SMART_ACTION_SET_DATA,event_id=3,link=0,event=SMART_EVENT_LINK;int64 entryOrGuid=-INT64_C(9223372036854775807);
 struct{struct{uint32 field=1,data=1;}setData;}action;
 uint32 GetActionType()const{return type;}uint32 GetEventType()const{return event;}uint32 GetScriptType()const{return 0;}
};
struct SmartScript{
 bool nullTargets=false;std::vector<WorldObject*> selected;std::map<uint32,SmartScriptHolder> linkedEvents;uint32 links=0,var0=0,var1=0;bool bvar=false;void* spell=nullptr;void* gob=nullptr;
 bool IsCreature(WorldObject*);bool IsGameObject(WorldObject*);
 ObjectList* GetTargets(SmartScriptHolder const&,Unit*){if(nullTargets)return nullptr;auto* result=new ObjectList;result->assign(selected.begin(),selected.end());return result;}
 SmartScriptHolder FindLinkedEvent(uint32 id){return linkedEvents.at(id);}
 void ProcessEvent(SmartScriptHolder&,Unit*,uint32,uint32,bool,void*,void*){++links;}
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr){
  switch(e.GetActionType()){
   #include "SetData.inc"
   default:throw std::runtime_error("Unexpected action");
  }
  #include "LinkTail.inc"
};
#include "NativeMembers.inc"
int main(int argc,char** argv)try{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 CHECK(argc==2);std::string mode=argv[1];SmartScript script;SmartScriptHolder event;
 Recorder cr,gr;Creature creature;GameObject go;creature.i_AI=new CreatureAI(cr);go.m_AI=new GameObjectAI(gr);
 if(mode=="destroyed-creature"){CHECK(creature.AIM_Destroy());CHECK(!creature.AI()&&!creature.IsAIEnabled&&cr.destroyed==1);script.selected={&creature};script.ProcessAction(event);return 0;}
 if(mode=="missing-gameobject"){delete go.m_AI;go.m_AI=nullptr;script.selected={&go};script.ProcessAction(event);return 0;}
 WorldObject other;Unit player;
 script.selected={&creature,&go,nullptr,&other,&player};event.action.setData.field=UINT32_MAX;event.action.setData.data=UINT32_C(0x80000001);script.ProcessAction(event);
 CHECK(cr.calls==1&&gr.calls==1&&cr.field==UINT32_MAX&&gr.data==UINT32_C(0x80000001));CHECK(!ObjectList::outstanding&&warnings.empty());
 if(mode=="control"){CHECK(creature.AIM_Destroy());delete go.m_AI;std::cout<<"PASS: pinned/current actual SET_DATA valid Creature+GameObject control, native getters and selector lifetime\n";return 0;}
 CHECK(mode=="regression");
 event.link=4;SmartScriptHolder next;next.event_id=4;script.linkedEvents[4]=next;
 // Native lifecycle: a real in-world object can have no current AI after destruction.
 CHECK(creature.AIM_Destroy());CHECK(!creature.AI()&&!creature.IsAIEnabled&&cr.destroyed==1);
 script.ProcessAction(event);CHECK(cr.calls==1&&gr.calls==2&&warnings.size()==1&&script.links==1);
 CHECK(warnings.back().find("-9223372036854775807")!=std::string::npos);
 CHECK(warnings.back().find("Event 3")!=std::string::npos&&warnings.back().find(creature.guid.value)!=std::string::npos);
 CHECK(warnings.back().find("4294967295")!=std::string::npos&&warnings.back().find("2147483649")!=std::string::npos);
 CHECK(warnings.back().find("not delivered")!=std::string::npos);
 delete go.m_AI;go.m_AI=nullptr;script.ProcessAction(event);
 CHECK(warnings.size()==3&&warnings.back().find(go.guid.value)!=std::string::npos&&script.links==2);
 // Do not send to suspended AI or fabricate a queued delivery after AI replacement.
 Recorder suspended;creature.i_disabledAI=new CreatureAI(suspended);script.selected={&creature};script.ProcessAction(event);
 CHECK(!suspended.calls&&warnings.size()==4);delete creature.i_disabledAI;creature.i_disabledAI=nullptr;
 creature.i_AI=new CreatureAI(cr);script.selected={&creature};CHECK(!creature.IsAIEnabled);script.ProcessAction(event);
 CHECK(cr.calls==2&&warnings.size()==4); // existing active-pointer semantics, no new enabled gate
 creature.m_AI_locked=true;CHECK(!creature.AIM_Destroy());CHECK(creature.AI());script.ProcessAction(event);CHECK(cr.calls==3);
 creature.m_AI_locked=false;CHECK(creature.AIM_Destroy());CHECK(cr.destroyed==2);
 // An earlier callback can retire another receiver's AI before its own list turn.
 Recorder firstRecord,secondRecord;Creature first,second;first.i_AI=new CreatureAI(firstRecord);second.i_AI=new CreatureAI(secondRecord);
 firstRecord.callback=[&]{CHECK(second.AIM_Destroy());};script.selected={&first,&second};script.ProcessAction(event);
 CHECK(firstRecord.calls==1&&!secondRecord.calls&&secondRecord.destroyed==1&&warnings.size()==5);
 CHECK(first.AIM_Destroy());
 // Native LINK policy is retained for missing/empty selectors and repeated events.
 auto links=script.links;script.selected={};script.ProcessAction(event);script.nullTargets=true;script.ProcessAction(event);
 script.nullTargets=false;script.selected={nullptr,&other,&player};script.ProcessAction(event);CHECK(script.links==links+3&&warnings.size()==5);
 script.selected={&creature,&go};for(int i=0;i<3;++i)script.ProcessAction(event);
 CHECK(warnings.size()==11&&script.links==links+6&&!ObjectList::outstanding&&cr.calls==3&&gr.calls==2&&!suspended.calls);
 std::cout<<"PASS: actual SET_DATA/native AIM_Destroy/getters/LINK; absent/current/suspended/locked/replaced AI, callback retires next receiver, mixed/null/empty/repeated selectors, exact diagnostics and full-width payload; absent AI never reported delivered\n";
}catch(std::exception const& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}

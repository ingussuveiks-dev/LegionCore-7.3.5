#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
using uint32=uint32_t;
int errors=0,debugs=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) (++debugs)
constexpr int SCRIPT_COMMAND_TEMP_SUMMON_CREATURE=10,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN=1;
struct Summon {bool personal=false,visible=false;void AddPlayerInPersonnalVisibilityList(int){personal=true;}
 void AddDelayedEvent(int delay,std::function<void()> f){assert(delay==100);f();}void SetVisible(bool b){visible=b;}};
struct WorldObject {bool fail=false,player=false;Summon result;WorldObject* ToPlayer(){return player?this:nullptr;}
 int GetGUID(){return 42;}Summon* SummonCreature(uint32,float,float,float,float,int,uint32){return fail?nullptr:&result;}};
struct Script {struct {uint32 CreatureEntry=106644,DespawnDelay=600000;bool PersonalVisible=false;float PosX=1,PosY=2,PosZ=3,Orientation=4;}TempSummonCreature;
 std::string GetDebugInfo(){return "event_scripts 50917";}};
WorldObject* _GetScriptWorldObject(WorldObject* s,bool,Script*){return s;}
void Run(WorldObject* source,Script& script){struct {Script* script;}step{&script};switch(SCRIPT_COMMAND_TEMP_SUMMON_CREATURE){
#include "Summon.inc"
}}
int main(){
 Script script;WorldObject source;Run(&source,script);assert(errors==0&&debugs==1);
 source.player=true;script.TempSummonCreature.PersonalVisible=true;Run(&source,script);
 assert(errors==0&&debugs==2&&source.result.personal&&source.result.visible);
 source.fail=true;Run(&source,script);assert(errors==1&&debugs==2);
 script.TempSummonCreature.CreatureEntry=0;Run(&source,script);assert(errors==2&&debugs==2);
 Run(nullptr,script);assert(errors==2&&debugs==2);
 std::cout<<"PASS: extracted summon command logs success at DEBUG; real failures remain ERROR; personal visibility and delayed display preserved\n";
}

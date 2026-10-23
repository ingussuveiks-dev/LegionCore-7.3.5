#include <cassert>
#include <cstdint>
#include <iostream>
enum SheathState { SHEATH_STATE_UNARMED=0,SHEATH_STATE_MELEE=1,SHEATH_STATE_RANGED=2 };
constexpr int UNIT_FIELD_BYTES_2=17,UNIT_BYTES_2_OFFSET_SHEATH_STATE=0,SMART_ACTION_SET_SHEATH=40;
#define TC_LOG_DEBUG(...) ((void)0)
struct Actor {
 uint32_t packed=0x12345601,items[3]={0,0,128861};
 void SetByteValue(int field,int offset,uint8_t value){assert(field==17&&offset==0);packed=(packed&0xffffff00)|value;}
#include "SheathSetter.inc"
};
struct Event {struct {struct {uint32_t sheath;}setSheath;}action;}e;
void Dispatch(Actor* me,int type){switch(type){
#include "SheathAction.inc"
default:assert(false);
}}
int main(){
 Actor prustaga;e.action.setSheath.sheath=2;Dispatch(&prustaga,40);
 assert(prustaga.packed==0x12345602&&prustaga.items[2]==128861);
 Dispatch(&prustaga,40);assert(prustaga.packed==0x12345602);
 Dispatch(nullptr,40); // a missing optional actor does not dereference it
 std::cout<<"PASS: native SmartAI ranged draw and packed Unit sheath byte; equipped rifle/other bytes retained, repeat and missing actor\n";
}

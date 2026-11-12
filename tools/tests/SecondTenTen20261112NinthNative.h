#define SECOND_NINTH_NATIVE_CLOCK
#include "SecondTenTen20261112NinthFixture.h"
#undef SECOND_NINTH_NATIVE_CLOCK
#include "NinthBindings.inc"
static void RunSecondNinthNative(){
 for(char const*name:secondNinthBindings){Require(sObjectMgr->GetScriptId(name),"Ninth actual script registry binding absent");auto rows=WorldDatabase.PQuery("SELECT entry FROM creature_template WHERE ScriptName='%s'",name);Require(bool(rows),"Ninth actual creature binding absent");std::cout<<"NINTH_BINDING "<<name;do{std::cout<<" entry="<<rows->Fetch()[0].GetUInt32();}while(rows->NextRow());std::cout<<"\n";}
 for(uint32 id:secondNinthSpells)Require(sSpellMgr->GetSpellInfo(id),"Ninth matching26972 authored spell absent");
 for(int i=0;i<10;++i)NinthClock::RunNinthCase(i);
 std::cout<<"PASS ninth10 actual native EventMap: eight reset/reentry queues discard old phase,flight,thaw,expiry and sentinel callbacks while preserving authored fresh roots;both sentinel variants;actual Unga callbacks and explicit Nefarian event-arm projections preserve independent repeating attacks. Native bindings/spells match;creature/instance/phase endpoints controlled,full encounters/client traversal separate\n";
}

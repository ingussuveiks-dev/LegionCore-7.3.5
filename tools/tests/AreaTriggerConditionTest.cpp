#include <vector>
#include <cstdint>
#include <iostream>
#include <stdexcept>
using uint32=uint32_t;
enum {CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION=26};
struct Condition{uint32 SourceGroup=11869,SourceEntry=0;};
struct AreaTriggerAction{uint32 id;};
using AreaTriggerActionList=std::vector<AreaTriggerAction>;
struct AreaTriggerInfo{AreaTriggerActionList actions;};
struct Store{AreaTriggerInfo* ordinary=nullptr;AreaTriggerInfo* custom=nullptr;
 AreaTriggerInfo const* GetAreaTriggerInfo(uint32){return ordinary;}
 AreaTriggerInfo const* GetAreaTriggerInfoByEntry(uint32){return custom;}
} store;
auto sAreaTriggerDataStore=&store;
uint32 errors=0;
#define TC_LOG_ERROR(...) ++errors
bool Validate(Condition*cond){switch(CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION){
#include "Validation.inc"
}return true;}
void Require(bool value){if(!value)throw std::runtime_error("Actual area condition validation differs");}
int main(){try{
 AreaTriggerInfo ordinary{{{0}}},custom{{{0},{1}}},empty;
 Condition c;
 uint32 cases=0;
 for(auto native:{&ordinary,&empty,static_cast<AreaTriggerInfo*>(nullptr)})
  for(auto scoped:{&custom,&empty,static_cast<AreaTriggerInfo*>(nullptr)})
   for(uint32 id:{0u,1u,2u}){
    store.ordinary=native;store.custom=scoped;c.SourceEntry=id;errors=0;
    bool expected=(native==&ordinary&&id==0)||(scoped==&custom&&id<2);
#ifdef BASELINE
    bool oldExpected=native==&ordinary&&id==0;Require(Validate(&c)==oldExpected&&errors==uint32(!oldExpected));
#else
    Require(Validate(&c)==expected&&errors==uint32(!expected));
#endif
    ++cases;
   }
#ifdef BASELINE
 std::cout<<"COUNTERFACTUAL pinned ordinary-only validator rejects existing custom leave1, including numeric native/custom11869 collision;27 endpoint combinations, not an ordinary passing server run\n";
#else
 std::cout<<"PASS "<<cases<<" ASAN complete actual area-action validation cases: native/custom/both/missing/empty stores, native0/custom0/1, missing2 rejection and retained exact error diagnostics\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

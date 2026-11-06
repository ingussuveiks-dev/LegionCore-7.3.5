// Actual native/custom area stores and complete core source validation.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "AreaTriggerData.h"
struct AreaValidationTag{using Type=bool(ConditionMgr::*)(Condition*);friend Type Access(AreaValidationTag);};
template struct FixtureAccess<AreaValidationTag,&ConditionMgr::isSourceTypeValid>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
 auto ordinary=sAreaTriggerDataStore->GetAreaTriggerInfo(11869),custom=sAreaTriggerDataStore->GetAreaTriggerInfoByEntry(11869);
 Require(ordinary&&custom,"Native/custom11869 collision witness missing");
 auto has=[](AreaTriggerInfo const*info,uint32 id){for(auto const&a:info->actions)if(a.id==id)return true;return false;};
 Require(has(ordinary,0)&&!has(ordinary,1)&&has(custom,0)&&has(custom,1),"Actual11869 action lists changed; reinspect collision");
 Condition condition;condition.SourceType=CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION;condition.SourceGroup=11869;
 for(uint32 id:{0u,1u}){condition.SourceEntry=id;Require((sConditionMgr->*Access(AreaValidationTag{}))(&condition),"Native/custom action rejected by actual core source validator");}
 std::cout<<"PASS actual native/custom11869 loaded from MariaDB: ordinary action0 retained; distinct custom enter0/leave1 admitted by complete core validator. No condition check/log bypass or persistent DB writes\n";
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

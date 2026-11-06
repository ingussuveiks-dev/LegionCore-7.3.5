#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "BonfireNative.inc"
#include "Spell.h"
struct LegacyBonfireDispatch:Spell{using Spell::Spell;SpellCastResult Open(GameObject* fire){gameObjTarget=fire;SkillType skill=SKILL_NONE;int32 required=0,value=0;auto result=CanOpenLock(0,2474,skill,required,value);if(result==SPELL_CAST_OK)SendLoot(fire->GetGUID(),LOOT_SKINNING);return result;}};
struct BonfireInstance:InstanceScript {using InstanceScript::InstanceScript;uint32 transitions=0;void onScenarionNextStep(uint32 step)override{if(step==1)++transitions;}};
struct BonfireInstanceFactory:InstanceMapScript {
 BonfireInstanceFactory():InstanceMapScript("instance_warswords",1511){}
 InstanceScript* GetInstanceScript(InstanceMap* map)const override{return new BonfireInstance(map);}
};
struct BonfireDbTag{using Type=uint64 GameObject::*;friend Type Access(BonfireDbTag);};
template struct FixtureAccess<BonfireDbTag,&GameObject::m_DBTableGuid>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29136");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Native bonfire criteria needs additional loader rules");
 auto info=sObjectMgr->GetGameObjectTemplate(245188);Require(info&&info->type==GAMEOBJECT_TYPE_GOOBER&&info->GetLockId()==2474&&info->goober.spell==198513,"Live bonfire template changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new BonfireInstanceFactory;new go_mystic_bonfire;sScriptMgr->SwapScriptContext(true);
 for(uint32 race:{RACE_HUMAN,RACE_ORC}){
 MapInstanced parent(1511,300);InstanceMap map(1511,300,9987100+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
 auto data=dynamic_cast<BonfireInstance*>(map.GetInstanceScript());Require(data,"Native bonfire instance factory missing");
 auto scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(0);auto session=Session();
 {
 NativePlayer player(session,map,901,{3402.01f,1475.4f,58.5142f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);
 player.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40043);
 auto fire=new GameObject;Require(fire->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(),245188,&map,1,{3402.01f,1477.4f,58.5142f,0},G3D::Quat(),255,GO_STATE_READY),"Native bonfire creation failed");
 fire->*Access(BonfireDbTag{})=134117;Require(map.AddToMap(fire),"Native bonfire world insertion failed");
 Require(dynamic_cast<go_mystic_bonfire::go_mystic_bonfireAI*>(fire->AI()),"Actual native bonfire AI binding missing");
 auto count=[&](){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto it=rows->find(45849);return it==rows->end()?uint64(0):it->second.Counter;};
 fire->AI()->GossipHello(&player,false);Require(!count()&&scenario->GetCurrentStep()==0,"Report-use credited ignition");
 player.CastSpell(fire,3365,true);player.m_Events.Update(1000);
 Require(!count()&&scenario->GetCurrentStep()==0&&!fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Wrong native Opening bypassed Lock2474");
 SpellCastResult result;
#ifdef BONFIRE_BASELINE
 TriggerCastData cast;cast.triggerFlags=TRIGGERED_FULL_MASK;LegacyBonfireDispatch legacy(&player,sSpellMgr->GetSpellInfo(198513),cast);result=legacy.Open(fire);
#else
 result=player.CastSpell(fire,198513,true);
#endif
 player.m_Events.Update(1000);
 std::cout<<"TRACE native bonfire race="<<race<<" cast="<<result<<" tree45849="<<count()<<" step="<<unsigned(scenario->GetCurrentStep())<<" flags="<<fire->GetUInt32Value(GAMEOBJECT_FIELD_FLAGS)<<std::endl;
#ifdef BONFIRE_BASELINE
 Require(result==SPELL_CAST_OK&&!count()&&scenario->GetCurrentStep()==0,"Pinned old goober dispatch no longer reproduces");
 std::cout<<"COUNTERFACTUAL actual native198513/lock2474/goober Use never dispatches GossipHello(true); stage0 remains uncredited. Not a passing ordinary run\n";
#else
 Require(result==SPELL_CAST_OK&&count()==1&&scenario->GetCurrentStep()==1&&data->transitions==1,"Actual native bonfire cast failed stage0->1");
 fire->AI()->GossipHello(&player,false);player.CastSpell(fire,198513,true);player.m_Events.Update(1000);
 Require(count()==1&&scenario->GetCurrentStep()==1&&data->transitions==1,"Repeated native/report use duplicated ignition");
 std::cout<<"PASS real native198513/Lock2474/GossipHello true -> CriteriaTree45849=1/scenario944 stage0->1 once; report-use and repeated native hit grant no extra progress. Native cast triggered for clock control, not rendered client opening\n";
#endif
 fire->RemoveFromWorld();fire->RemoveFromGrid();fire->ResetMap();delete fire;map.RemoveAllObjectsInRemoveList();CleanStories(map);
 }
 session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

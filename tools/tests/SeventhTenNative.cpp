// Exact selected owner assignments in real Unit::Kill dispatch; full callbacks
// are source-ASAN tested separately. Synthetic profiles; no character save.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ConditionMgr.h"
#include "Item.h"
#include <array>
struct SeventhItemsTag{using Type=Item*(Player::*)[PLAYER_SLOTS_COUNT];friend Type Access(SeventhItemsTag);};template struct FixtureAccess<SeventhItemsTag,&Player::m_items>;
#include "SeventhResolvers.inc"
struct SeventhDeathAI:ScriptedAI{
 using Resolver=Player*(*)(Unit*);Resolver resolve;uint32 calls=0;ObjectGuid owner,original;
 SeventhDeathAI(Creature*c,Resolver r):ScriptedAI(c),resolve(r){}
 void JustDied(Unit*killer)override{++calls;if(killer)original=killer->GetGUID();if(auto p=resolve(killer))owner=p->GetGUID();}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
 auto bindings=WorldDatabase.Query("SELECT entry,ScriptName FROM creature_template WHERE entry IN(22355,25284,25301,93221,96441,98986,96682,96783)");Require(bindings&&bindings->GetRowCount()==8,"Native selected bindings absent");do{auto f=bindings->Fetch();std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(bindings->NextRow());
 auto field=WorldDatabase.Query("SELECT CAST('4294967296' AS UNSIGNED),CAST(1099511627793 AS UNSIGNED)");std::cout<<"FIELD64 text="<<(field?field->Fetch()[0].GetString():"missing")<<" other="<<(field?field->Fetch()[1].GetUInt64():0)<<" present="<<bool(field)<<" value="<<(field?field->Fetch()[0].GetUInt64():0)<<" expected="<<(uint64(1)<<32)<<std::endl;Require(field&&field->Fetch()[0].GetUInt64()==(uint64(1)<<32),"Native high-only saved counter control failed");
 for(uint32 id:{45532u,210093u,210107u,210486u,195440u,210500u,195450u})Require(sSpellMgr->GetSpellInfo(id),"Native continuation spell absent");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999711,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
 NativePlayer p(session,map,995,{50,803,805,0});CreditPet pet(p,map);CreditGuardian guardian(p,map);
 auto artifact=std::make_unique<Item>();Require(artifact->Create(19999711,128943,nullptr),"Native artifact Item creation failed");artifact->SetOwnerGUID(p.GetGUID());artifact->SetSlot(EQUIPMENT_SLOT_MAINHAND);auto&slots=p.*Access(SeventhItemsTag{});slots[EQUIPMENT_SLOT_MAINHAND]=artifact.get();artifact->SetUInt64Value(ITEM_FIELD_ARTIFACT_XP,(uint64(1)<<32)+17);
 Condition c;c.ConditionType=CONDITION_ARTIFACT_POWER;c.ConditionValue1=0;c.ConditionValue2=0;c.ConditionValue3=100;ConditionSourceInfo info(&p);Require(!c.Meets(info),"Native high AP wrapped below maximum");c.ConditionValue1=128943;Require(!c.Meets(info),"Native named equipped high AP wrapped");c.ConditionValue3=0;c.ConditionValue2=100;Require(c.Meets(info),"Native high AP failed minimum-only condition");artifact->SetUInt64Value(ITEM_FIELD_ARTIFACT_XP,99);Require(!c.Meets(info),"Native minimum lost");slots[EQUIPMENT_SLOT_MAINHAND]=nullptr;
 uint32 guid=10100;
 for(auto const& selected:seventhResolvers){
  for(Unit*killer:{static_cast<Unit*>(&p),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)}){
   Actor victim(&map,selected.entry,guid++,{52,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&victim)),"Native victim admission failed");auto ai=new SeventhDeathAI(&victim,selected.resolve);Require(victim.AIM_Initialize(ai),"Native selected resolver AI missing");victim.SetMaxHealth(1000);victim.SetHealth(1000);killer->Kill(&victim,false);Require(ai->calls==1&&ai->owner==p.GetGUID()&&ai->original==killer->GetGUID(),"Native player/pet/guardian killing blow lost owner");killer->Kill(&victim,false);Require(ai->calls==1,"Native repeated Kill redispatched death");
  }
  Actor unowned(&map,299,guid++,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&unowned)),"Native unowned admission failed");Require(!selected.resolve(&unowned)&&!selected.resolve(nullptr),"Native unowned/null progressed");
  std::cout<<"CASE "<<selected.entry<<" actual Player/Pet/Guardian Kill and duplicate-Kill guard;null/unowned resolver refusal\n";
 }
 }session->SetPlayer(nullptr);delete session;CloseAltheaNative();std::cout<<"PASS native Condition57 real equipped artifact64bit bounds and minimum;actual Field64 high-only transport counter;8 exact owner assignments in24 real Unit::Kill dispatches/8null-unowned controls,original killer GUID and exactly-once native death. Full death callbacks/group/cast/outro endpoints source-modeled;not client/full-chain/full-core ASAN. No character saves/imports.\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}

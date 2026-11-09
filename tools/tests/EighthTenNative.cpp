// Controlled native boss death reproduction; no character writes.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Spell.h"
#include "EighthResolvers.inc"
struct EighthBoss:BossAI{uint32 deaths=0;explicit EighthBoss(Creature*c):BossAI(c,0){}void JustDied(Unit*killer)override{BossAI::JustDied(killer);++deaths;}};
struct EighthDeathAI:ScriptedAI{using Resolver=Player*(*)(Unit*);Resolver resolve;uint32 calls=0;ObjectGuid owner,original;EighthDeathAI(Creature*c,Resolver r):ScriptedAI(c),resolve(r){}void JustDied(Unit*killer)override{++calls;if(killer)original=killer->GetGUID();if(auto p=resolve(killer))owner=p->GetGUID();}};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 auto info=sSpellMgr->GetSpellInfo(7);Require(info,"Native Suicide7 missing");std::cout<<"CONTROL native7 effect="<<info->Effects[EFFECT_0]->Effect<<std::endl;
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999721,DIFFICULTY_N_SCENARIO,&parent);{
 Actor boss(&map,30020,10101,{52,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native boss admission");auto ai=new EighthBoss(&boss);Require(boss.AIM_Initialize(ai),"Native BossAI setup");boss.SetMaxHealth(1000);boss.SetHealth(1000);
 auto cast=boss.CastSpell(&boss,7,true);Require(cast==SPELL_CAST_OK&&!boss.IsAlive()&&ai->deaths==1,"Native Suicide7 did not dispatch BossAI death");boss.Unit::Update(1);boss.Unit::Update(1);Require(boss.m_Events.Size()==0,"Native aborted spell event retained after normal updates");std::cout<<"CASE native BossAI Suicide7 death handled once;normal updates released all spell events"<<std::endl;
 }
 for(auto id:eighthPossibleSpawns)Require(sObjectMgr->GetCreatureTemplate(id),"Actual Webbed pool template absent");Require(std::size(eighthPossibleSpawns)==32&&urand(31,31)==31,"Native full pool bound contract");std::cout<<"POOL32 last="<<eighthPossibleSpawns[31]<<'\n';
 for(auto id:eighthHyjalSpells)Require(sSpellMgr->GetSpellInfo(id),"Actual Hyjal companion spell absent");Require(sAreaTableStore.LookupEntry(5087),"Matching Hyjal5087 absent");std::cout<<"HYJAL actual unique spells="<<std::size(eighthHyjalSpells)<<" area5087\n";
 auto session=Session();{NativePlayer p(session,map,995,{50,803,805,0});CreditPet pet(p,map);CreditGuardian guardian(p,map);uint32 guid=10200;
 for(auto const& selected:eighthResolvers){
  auto info=sObjectMgr->GetCreatureTemplate(selected.entry);Require(info,"Selected actual NPC template absent");std::cout<<"BIND "<<selected.entry<<" "<<sObjectMgr->GetScriptName(info->ScriptID)<<'\n';
  for(Unit*killer:{static_cast<Unit*>(&p),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)}){
   Actor victim(&map,selected.entry,guid++,{52,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&victim)),"Native victim admission");auto ai=new EighthDeathAI(&victim,selected.resolve);Require(victim.AIM_Initialize(ai),"Native resolver dispatch AI");victim.SetMaxHealth(1000);victim.SetHealth(1000);killer->Kill(&victim,false);Require(ai->calls==1&&ai->owner==p.GetGUID()&&ai->original==killer->GetGUID(),"Real player/pet/guardian Kill lost owner");killer->Kill(&victim,false);Require(ai->calls==1,"Duplicate Kill replayed native death");
  }
  Actor other(&map,299,guid++,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&other)),"Native refusal actor admission");Require(!selected.resolve(nullptr)&&!selected.resolve(&other),"Null/unowned killer progressed");other.SetOwnerGUID(ObjectGuid::Create<HighGuid::Player>(9999999));Require(!selected.resolve(&other),"Missing owner progressed");other.SetOwnerGUID(ObjectGuid::Empty);other.SetGuidValue(UNIT_FIELD_CHARMED_BY,p.GetGUID());Require(!other.GetOwner()&&selected.resolve(&other)==&p,"Ownerless charmer attribution failed");other.SetGuidValue(UNIT_FIELD_CHARMED_BY,ObjectGuid::Empty);
  std::cout<<"CASE "<<selected.entry<<" player/Pet/Guardian Kill once,exact killer;null/unowned/lost owner refused,ownerless charmer resolved\n";
 }}session->SetPlayer(nullptr);delete session;CloseAltheaNative();std::cout<<"PASS native actual BossAI/Suicide7 spell death lifecycle,32real pool templates and Hyjal spell/area catalog,21actual Kill dispatches with selected attribution and28null/unowned/lost-owner/charmer controls. Full seven progression callbacks and Hyjal casts/state source-modeled;not full client/raid/quest traversal or full-core ASAN;no character saves.\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}

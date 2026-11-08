#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include <functional>
#include <memory>
namespace DevourerOutroFixture {
#include "DevourerOutroInstance.inc"
}
using OutroInstance=DevourerOutroFixture::instance_devourer::instance_devourer_InstanceMapScript;
struct ObservedAI : NullCreatureAI {
 unsigned hits=0;std::function<void()> callback;
 explicit ObservedAI(Creature* c):NullCreatureAI(c){}
 void SpellHit(Unit* caster,SpellInfo const* spell) override {
  Require(caster==me&&spell&&spell->Id==(me->GetEntry()==98308?209814u:209813u),"Unexpected native outro spell/target");
  ++hits;if(callback)callback();
 }
};
// Exercise the actual Creature callbacks; lightweight shared Actor bypasses these.
struct OutroActor : Actor {
 using Actor::Actor;
 void AddToWorld() override {Creature::AddToWorld();Require(AIM_Initialize(new ObservedAI(this)),"Native observer AI failed");SetReactState(REACT_PASSIVE);}
 void RemoveFromWorld() override {Creature::RemoveFromWorld();}
 ObservedAI* Observe(){return dynamic_cast<ObservedAI*>(AI());}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings absent");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 auto steps=sScenarioMgr->GetScenarioSteps(1078);Require(steps&&steps->size()==7,"Actual1078 steps absent");
 for(unsigned id:{209813u,209814u}){auto spell=sSpellMgr->GetSpellInfo(id);Require(spell&&spell->GetEffect(EFFECT_0)->Effect==SPELL_EFFECT_DUMMY&&spell->CalcCastTime()==0,"Native visual spell changed");}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1607,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new DevourerOutroFixture::instance_devourer;sScriptMgr->SwapScriptContext(true);unsigned cases=0,totalHits=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned mode=0;mode<5;++mode){
  auto parent=std::make_unique<MapInstanced>(1607,300);auto mapOwner=std::make_unique<InstanceMap>(1607,300,9999500+race*10+mode,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;map.CreateInstanceData(nullptr);auto script=dynamic_cast<OutroInstance*>(map.GetInstanceScript());Require(script,"Whole registered Devourer instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1078);
  {
   Position pose{836.16f,1035.4f,48.61f,0};auto p=std::make_unique<NativePlayer>(session,map,980,pose);p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetPhaseId({7003},false);p->SetQuestStatus(41924,mode%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);
   std::vector<std::unique_ptr<OutroActor>> actors;for(unsigned entry:{98308u,98307u,54638u}){actors.emplace_back(new OutroActor(&map,entry,981+unsigned(actors.size()),pose));actors.back()->SetPhaseId({7003},false);Require(map.AddToMap(static_cast<Creature*>(actors.back().get())),"Native actor map insertion failed");}
   auto a=actors[0].get(),b=actors[1].get(),c=actors[2].get();Require(script->trashGUIDconteiner.size()==3,"Real create callbacks not tracked");
   // No reactions are eligible at any of the seven incomplete stages.
   for(unsigned step=0;step<7;++step){scenario->SetCurrentStep(step);script->Update(4000);Require(a->Observe()->hits+b->Observe()->hits+c->Observe()->hits==0&&script->outrotimer==2000,"Incomplete stage admitted reactions");}
   p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50301);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==7&&script->getScenarionStep()==6,"Natural final event/cache contract changed");
   auto status=p->GetQuestStatus(41924);auto progress=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();script->Update(1999);Require(!a->Observe()->hits&&!b->Observe()->hits&&!c->Observe()->hits,"Reactions before two-second deadline");
#ifdef DEVOURER_OUTRO_BASELINE
   for(unsigned i=0;i<64;++i)script->Update(2000);Require(!a->Observe()->hits&&!b->Observe()->hits&&!c->Observe()->hits&&script->outrotimer==2000,"Pinned cached-step failure not reproduced");
#else
   Require(script->outrotimer==1,"Native deadline lost");script->Update(1);
   unsigned before=0;
   if(mode==1){map.RemoveFromMap(static_cast<Creature*>(b),false);Require(script->trashGUIDconteiner.size()==2,"Real removal callback failed");}
   if(mode==2)script->OnCreatureCreate(a);
   if(mode==3)a->Observe()->callback=[&]{if(b->IsInWorld())map.RemoveFromMap(static_cast<Creature*>(b),false);};
   if(mode==4)a->Observe()->callback=[&]{script->Initialize();};
   for(unsigned i=0;i<64;++i){auto prev=a->Observe()->hits;script->Update(2000);Require(a->Observe()->hits<=prev+1,"Duplicate actor callback cast twice in pulse");}
   Require(a->Observe()->hits>0&&(mode==4||c->Observe()->hits>0),"Native ordinary visual casts not delivered");
   if(mode==0||mode==2)Require(b->Observe()->hits>0,"Native98307 visual absent");
   if(mode==1)Require(b->Observe()->hits<=1,"Removed actor received later casts");
   if(mode==3){before=b->Observe()->hits;for(unsigned i=0;i<4;++i)script->Update(2000);Require(!b->IsInWorld()&&b->Observe()->hits==before,"Removed target recast from snapshot");}
   if(mode==4){before=a->Observe()->hits+b->Observe()->hits+c->Observe()->hits;Require(script->trashGUIDconteiner.empty(),"Callback reset did not clear tracking");script->Update(2000);Require(a->Observe()->hits+b->Observe()->hits+c->Observe()->hits==before,"Reset actors recast");}
   totalHits+=a->Observe()->hits+b->Observe()->hits+c->Observe()->hits;
#endif
   Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==7&&script->getScenarionStep()==6&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==progress&&p->GetQuestStatus(41924)==status&&!p->HasItemCount(128476,1,true),"Visual update changed quest/item/progress/cache");
   for(auto& actor:actors){if(actor->IsInWorld())map.RemoveFromMap(static_cast<Creature*>(actor.get()),false);}Require(script->trashGUIDconteiner.empty(),"Native remove left dangling tracked actor");actors.clear();++cases;
  }
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
#ifdef DEVOURER_OUTRO_BASELINE
 std::cout<<"COUNTERFACTUAL Devourer outro native:10 real completed7/cache6 cases,ordinary original Update admits zero reactions. Whole pinned e59f20dd3 instance/current native support,not historical EXE"<<std::endl;
#else
 std::cout<<"PASS Devourer outro native:10 Human/Orc + active quest status cases,real50301 complete7/cache6,ordinary209813/209814 SpellHit,three native templates,real create/remove,duplicate registration,reentrant removal/reset,no progression/item mutation; observed="<<totalHits<<". Earlier stages/actor poses controlled,98308 scripted-only fixture;actual client appearance/full chain separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}}

#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include "QuestObjectScope.h"
#include "MapInstanced.h"
#include <filesystem>
#include <cstdlib>
#include "AltheaActor.inc"
#ifdef ALTHEA_IDENTITY_BASELINE
using AltheaDeathConversation = AltheaIdentityBaselineConversation;
#endif
static LONG WINAPI AltheaTrace(EXCEPTION_POINTERS* error){
 std::cerr<<"ALTHEA NATIVE FAULT rva="<<std::hex<<(error->ContextRecord->Rip-reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)))<<" target="<<error->ExceptionRecord->ExceptionInformation[1]<<std::dec<<'\n';
 return CrashTrace(error);
}
int main(int argc,char**argv){try{
 SetUnhandledExceptionFilter(AltheaTrace);
 Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
 auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();
 std::string runtime=directory.parent_path().parent_path().string()+'/';char* args[]={argv[0],config,runtime.data()};InitializeNative(3,args);sConversationDataStore->LoadConversationData();
 bool repaired=std::getenv("GARONA_OUTRO_ORIGINAL")==nullptr;
 if(std::getenv("GARONA_OUTRO_SHADOW")){
  auto actors=const_cast<std::vector<ConversationCreature>*>(sConversationDataStore->GetConversationCreature(2432));
  auto lines=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(2432)));
  Require(actors&&actors->size()==2&&lines&&lines->size()==2,"Native Garona shadow context missing");
  for(auto& actor:*actors)if(actor.id==1){Require(actor.duration==2350||actor.duration==11814,"Custom actor budget");actor.duration=11814;}
  for(auto& line:*lines)if(line.id==5152){Require(line.textId==11800||line.textId==10693,"Custom line start");line.textId=10693;}
 }
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}}});
 {MapInstanced parent(1620,300);InstanceMap map(1620,300,9999996,DIFFICULTY_N_SCENARIO,&parent);Actor althea(&map,108218,12001,{-8320.0f,280.0f,156.8f,0.0f}),garona(&map,108214,12002,althea);
 map.LoadGrid(althea.GetPositionX(),althea.GetPositionY());Require(map.AddToMap(static_cast<Creature*>(&althea))&&map.AddToMap(static_cast<Creature*>(&garona)),"Native actor admission failed");
 auto count=objectCountInWorld[uint8(HighGuid::Conversation)];std::cout<<"TRACE native actors admitted"<<std::endl;
 auto live=new Conversation;Require(live->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona)&&live->IsInWorld(),"Living2431 control failed");live->Remove();map.RemoveAllObjectsInRemoveList();
 althea.Unit::setDeathState(JUST_DIED);Require(!althea.IsAlive(),"Controlled immediate-death endpoint failed");
 auto dead=new Conversation;Require(!dead->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona),"Current dead actor refusal changed");Require(!dead->IsInWorld()&&!dead->IsInGrid(),"Refused native actor retains ownership");delete dead;
 uint32 cases=0;ObjectGuid lastBound;std::cout<<"TRACE ordinary living/corpse controls passed"<<std::endl;
 // The real accessor uses a lower32-bit vector index. A captured GUID with a
 // different high/upper-low word must not admit the indexed actor by accident.
 bool actorBaseline=std::getenv("ALTHEA_ACTOR_BASELINE")!=nullptr;
 auto exact=althea.GetGUID(),otherHigh=exact,upperLow=exact;
 otherHigh.SetRawValue(exact.GetHighPart()^1,exact.GetLowPart());upperLow.SetRawValue(exact.GetHighPart(),exact.GetLowPart()^(uint64(1)<<40));
 for(auto wrong:{otherHigh,upperLow}){
  Conversation* value=new AltheaDeathConversation(wrong);bool created=value->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona);
  Require(created==actorBaseline,"Scoped Althea full-GUID collision contract differs");
  if(created){Require(value->GetCreatures().size()==1&&value->GetCreatures()[0]->GuidLow==exact.GetLowPart()&&value->GetCreatures()[0]->GuidHight==exact.GetHighPart(),"Pinned collision no longer binds actual conflicting actor");value->Remove();map.RemoveAllObjectsInRemoveList();}
  else{Require(!value->IsInWorld()&&!value->IsInGrid(),"Refused identity retained grid/world ownership");value->AddToWorld();Require(!value->IsInWorld(),"Publication accepted a mismatched source GUID");delete value;}
  Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Identity control leaked native conversation");
 }
 std::cout<<(actorBaseline?"COUNTERFACTUAL two captured GUIDs with different high/upper-low words admit the lower32-bit indexed corpse; controlled identity collision, not a client incident\n":"PASS two native high/upper-low same-index identity refusals at resolution and direct publication, no grid/world/allocation leak\n");
 auto bound=[&](ObjectGuid guid,bool expected){
  std::cout<<"TRACE bound case "<<cases<<" expected="<<expected<<std::endl;
  Conversation* result=new AltheaDeathConversation(guid);auto key=sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate();
  bool created=result->CreateConversation(key,2431,&garona,nullptr,garona);
  std::cout<<"TRACE factory returned "<<created<<std::endl;
  Require(created==expected,"Exact Althea actor admission differs");
  if(created){Require(result->IsInWorld()&&result->GetCreatures().size()==1,"Bound native world/payload missing");lastBound=result->GetGUID();Require(result->GetQuestObjectScope().lock()==althea.GetQuestObjectScope().lock(),"Native source cancellation scope not inherited");auto actor=result->GetCreatures()[0];Require(actor->GuidLow==guid.GetLowPart()&&actor->GuidHight==guid.GetHighPart()&&result->GetDuration()==6800&&result->GetLines()[0]->id==5150,"Exact actor/native original line/budget changed");std::cout<<"TRACE native payload verified"<<std::endl;result->Remove();map.RemoveAllObjectsInRemoveList();}
  else{Require(!result->IsInWorld()&&!result->IsInGrid(),"Bound refusal retained grid");delete result;}
  Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Bound actor allocation leak");++cases;
 };
 // A closer living duplicate must neither replace the corpse nor rescue a
 // missing exact source. The old ordinary alive-only resolver remains intact.
 Actor duplicate(&map,108218,12003,garona);Require(map.AddToMap(static_cast<Creature*>(&duplicate)),"Duplicate control admission failed");
 bound(althea.GetGUID(),true);althea.Unit::setDeathState(CORPSE);bound(althea.GetGUID(),true);bound(garona.GetGUID(),false);
 bound(ObjectGuid::Create<HighGuid::Creature>(1620,108218,999999),false);
 althea.SetPhaseMask(2,false);bound(althea.GetGUID(),false);althea.SetPhaseMask(1,false);
 auto home=althea.GetPosition();althea.Relocate(home.GetPositionX()+2000,home.GetPositionY(),home.GetPositionZ(),home.GetOrientation());bound(althea.GetGUID(),false);althea.Relocate(home);
 bound(althea.GetGUID(),true);
 auto scope=std::make_shared<QuestObjectScope>();scope->Owner=garona.GetGUID();althea.BindQuestObjectScope(scope);
 bound(althea.GetGUID(),true);auto cancelled=scope->Cancel();Require(std::find(cancelled.begin(),cancelled.end(),lastBound)!=cancelled.end(),"Source cancellation did not register the real conversation GUID");bound(althea.GetGUID(),false);
 std::cout<<"PASS "<<cases<<" native scoped exact-corpse2431 binding/refusals: full original GUID despite living duplicate; missing/wrong-entry/phase/range/cancelled scope fail without fallback or allocation/grid leak; original5150/6800 retained\n";
 auto outro=new Conversation;Require(outro->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2432,&garona,nullptr,garona)&&outro->IsInWorld(),"Native2432 control failed");
 Require(outro->GetLines().size()==2,"Native Garona lines changed");auto second=outro->GetLines()[1]->textId;
 if(repaired){
  Require(outro->GetDuration()==19164&&second==10693,"Repaired2432 native budget/start differs");
  auto guid=outro->GetGUID();map.RemoveFromMap(static_cast<Creature*>(&garona),false);outro->Update(0);
  Require(outro->IsInWorld()&&ObjectAccessor::GetConversation(althea,guid)==outro,"NPC departure retires live native story");
  std::cout<<"PASS repaired native2432 duration19164/start10693, original payload and real accessor survive original caster removal; no client/audio claim\n";
 }else{Require(outro->GetDuration()==9700&&second==11800,"Original2432 counterfactual changed");std::cout<<"COUNTERFACTUAL native2432 duration9700 before second line11800\n";}
 std::cout<<"COUNTERFACTUAL ordinary native2431 alive-only resolver refuses the corpse; scoped production override admits exact original actor\n";
 outro->Remove();map.RemoveAllObjectsInRemoveList();Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Native probe allocation leaked");}
 std::cout<<"PASS native scoped repair and original controls; no authenticated Player/client/full scenario claim\n";std::cout.flush();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

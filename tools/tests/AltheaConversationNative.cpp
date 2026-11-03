#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include <filesystem>
#include <cstdlib>
int main(int argc,char**argv){try{
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
 {Map map(1620,300,9999996,DIFFICULTY_N_SCENARIO);Actor althea(&map,108218,12001,{-8320.0f,280.0f,156.8f,0.0f}),garona(&map,108214,12002,althea);
 map.LoadGrid(althea.GetPositionX(),althea.GetPositionY());Require(map.AddToMap(static_cast<Creature*>(&althea))&&map.AddToMap(static_cast<Creature*>(&garona)),"Native actor admission failed");
 auto count=objectCountInWorld[uint8(HighGuid::Conversation)];
 auto live=new Conversation;Require(live->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona)&&live->IsInWorld(),"Living2431 control failed");live->Remove();map.RemoveAllObjectsInRemoveList();
 althea.Unit::setDeathState(CORPSE);Require(!althea.IsAlive(),"Controlled corpse endpoint failed");
 auto dead=new Conversation;Require(!dead->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona),"Current dead actor refusal changed");Require(!dead->IsInWorld()&&!dead->IsInGrid(),"Refused native actor retains ownership");delete dead;
 auto outro=new Conversation;Require(outro->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2432,&garona,nullptr,garona)&&outro->IsInWorld(),"Native2432 control failed");
 Require(outro->GetLines().size()==2,"Native Garona lines changed");auto second=outro->GetLines()[1]->textId;
 if(repaired){
  Require(outro->GetDuration()==19164&&second==10693,"Repaired2432 native budget/start differs");
  auto guid=outro->GetGUID();map.RemoveFromMap(static_cast<Creature*>(&garona),false);outro->Update(0);
  Require(outro->IsInWorld()&&ObjectAccessor::GetConversation(althea,guid)==outro,"NPC departure retires live native story");
  std::cout<<"PASS repaired native2432 duration19164/start10693, original payload and real accessor survive original caster removal; no client/audio claim\n";
 }else{Require(outro->GetDuration()==9700&&second==11800,"Original2432 counterfactual changed");std::cout<<"COUNTERFACTUAL native2432 duration9700 before second line11800\n";}
 std::cout<<"EXPECTED_OPEN native2431 alive control succeeds; same real actor made corpse refuses; this separate admission defect remains unresolved\n";
 outro->Remove();map.RemoveAllObjectsInRemoveList();Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Native probe allocation leaked");}
 std::cout<<"PASS native controls and explicitly expected-open corpse data finding; no authenticated Player/client/full scenario claim\n";std::cout.flush();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

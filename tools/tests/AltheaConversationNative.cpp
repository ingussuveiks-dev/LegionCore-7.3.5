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
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}}});
 {Map map(1620,300,9999996,DIFFICULTY_N_SCENARIO);Actor althea(&map,108218,12001,{-8320.0f,280.0f,156.8f,0.0f}),garona(&map,108214,12002,althea);
 map.LoadGrid(althea.GetPositionX(),althea.GetPositionY());Require(map.AddToMap(static_cast<Creature*>(&althea))&&map.AddToMap(static_cast<Creature*>(&garona)),"Native actor admission failed");
 auto count=objectCountInWorld[uint8(HighGuid::Conversation)];
 auto live=new Conversation;Require(live->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona)&&live->IsInWorld(),"Living2431 control failed");live->Remove();map.RemoveAllObjectsInRemoveList();
 althea.Unit::setDeathState(CORPSE);Require(!althea.IsAlive(),"Controlled corpse endpoint failed");
 auto dead=new Conversation;Require(!dead->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2431,&garona,nullptr,garona),"Current dead actor refusal changed");Require(!dead->IsInWorld()&&!dead->IsInGrid(),"Refused native actor retains ownership");delete dead;
 auto outro=new Conversation;Require(outro->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),2432,&garona,nullptr,garona)&&outro->IsInWorld(),"Native2432 control failed");
 Require(outro->GetLines().size()==2&&outro->GetDuration()==9700,"Original2432 baseline changed");auto second=outro->GetLines()[1]->textId;Require(second==11800&&second>uint32(outro->GetDuration()),"Current data counterfactual changed");
 std::cout<<"EXPECTED_OPEN native2431 alive actor control succeeds; same real actor made corpse refuses. Native2432 duration="<<outro->GetDuration()<<" second-line-start="<<second<<"; playback/actor repair not part of null/owner hook fix\n";
 outro->Remove();map.RemoveAllObjectsInRemoveList();Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Native probe allocation leaked");}
 std::cout<<"PASS native controls and explicitly expected-open corpse/2432 data findings; no authenticated Player/client/full scenario claim\n";std::cout.flush();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

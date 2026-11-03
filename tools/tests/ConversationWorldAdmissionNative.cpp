#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include <filesystem>
#include <cstdlib>
#include "PinnedConversationAdmission.inc"
int main(int argc,char**argv)
{
 try
 {
  Require(argc==2,"Expected native dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
  auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();
  std::string runtime=directory.parent_path().parent_path().string()+'/';char* arguments[]={argv[0],config,runtime.data()};InitializeNative(3,arguments);
  sConversationDataStore->LoadConversationData();
  dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1500,{}}});
  MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1500,{}}});
  {
   Map map(1500,300,9999994,DIFFICULTY_N_SCENARIO);
   Actor live(&map,97167,11001,{-1923.29f,-1218.93f,0.90809f,0.0f});
   Actor absent(&map,97167,11002,live);map.LoadGrid(live.GetPositionX(),live.GetPositionY());
   Require(map.AddToMap(static_cast<Creature*>(&live)),"Live caster admission failed");
   uint32 contexts=0;auto count=objectCountInWorld[uint8(HighGuid::Conversation)];
   for(bool baseline:{true,false})for(uint32 entry:{770u,1412u})for(bool registered:{false,true})
   {
    auto caster=registered?&live:&absent;Conversation* result=baseline?static_cast<Conversation*>(new PinnedConversation):new Conversation;
    auto key=sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate();
    bool created=baseline?static_cast<PinnedConversation*>(result)->CreateConversation(key,entry,caster,nullptr,*caster):result->CreateConversation(key,entry,caster,nullptr,*caster);
    bool world=result->IsInWorld(),grid=result->IsInGrid();auto guid=result->GetGUID();
    std::cout<<"OBSERVE\t"<<baseline<<'\t'<<entry<<'\t'<<registered<<'\t'<<created<<'\t'<<world<<'\t'<<grid<<'\n';
    if(registered){Require(created&&world&&grid,"Ordinary native admitted control failed");Require(ObjectAccessor::GetConversation(*caster,guid)==result,"Admitted native accessor receipt missing");}
    else if(!baseline)Require(!created&&!world&&!grid,"Repaired factory must refuse without retaining grid ownership");
    else if(entry==770)Require(created&&!world&&grid,"Pinned model-actor counterfactual no longer reproduces");
    else Require(!created&&!world&&!grid,"Pinned creature-actor negative control changed");
    if(created)
    {
     result->Remove();
     if(!world)
     {
      std::cout<<"COUNTERFACTUAL native factory accepted absent caster; Remove leaves grid="<<result->IsInGrid()<<"; cleanup explicitly uses real Map::RemoveFromMap\n";
      Require(result->IsInGrid(),"Absent-caster removal changed");map.RemoveFromMap(result,true);
     }
     else map.RemoveAllObjectsInRemoveList();
    }
    else{Require(!world&&!grid,"False factory leaves caller-owned allocation registered");delete result;}
    Require(!ObjectAccessor::GetConversation(live,guid),"Retired conversation remains in accessor");
    Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Native conversation allocation leaked");++contexts;
   }
   Require(contexts==8,"Native context count changed");
  }
  WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
  std::cout<<"PASS8 native pinned/repaired model+creature actor factories: actual absent-caster770 grid leak paired with repaired false/clean caller deletion; all admitted controls/world/accessor and allocation cleanup retained. Native whole factory/Map, no virtual refusal override or Player/client claim\n";
 }
 catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}

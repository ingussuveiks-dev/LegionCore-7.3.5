#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include <filesystem>
#include <cstdlib>
#include "CellImpl.h"
#include "NativeConversationGrid.inc"
int main(int argc,char**argv)
{
 try
 {
  Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
  auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();
  std::string runtime=directory.parent_path().parent_path().string()+'/';char* arguments[]={argv[0],config,runtime.data()};InitializeNative(3,arguments);
  sConversationDataStore->LoadConversationData();
  // Never invoke LoadConversations: its clone maintenance writes World data.
  auto rows=WorldDatabase.Query("SELECT guid,id,map,phaseMask,position_x,position_y,position_z,orientation FROM conversation WHERE guid IN(7,8) ORDER BY guid");
  Require(bool(rows)&&rows->GetRowCount()==2,"Original two model-actor spawns missing");
  dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1481,{}}});
  MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1481,{}}});
  uint32 cases=0;
  do
  {
   auto f=rows->Fetch();auto key=f[0].GetUInt64();auto& record=sConversationDataStore->NewOrExistConversationData(key);
   record.guid=key;record.id=f[1].GetUInt32();record.mapid=f[2].GetUInt16();record.phaseMask=f[3].GetUInt32();
   record.posX=f[4].GetFloat();record.posY=f[5].GetFloat();record.posZ=f[6].GetFloat();record.orientation=f[7].GetFloat();
   Map map(record.mapid,300,9999995,DIFFICULTY_NORMAL);map.LoadGrid(record.posX,record.posY);
   auto count=objectCountInWorld[uint8(HighGuid::Conversation)];
   for(uint32 mode:{0u,1u,2u})
   {
    auto result=new Conversation;bool loaded=mode==0?result->LoadConversationFromDB(key,&map,true):result->LoadFromDB(key,&map);
    Require(loaded,"Actual DB factory failed before admission");
    if(mode==1)Require(map.AddToMap(result),"Deferred native map insertion failed");
    if(mode==2){Cell cell(Trinity::ComputeCellCoord(record.posX,record.posY));AddObjectHelper(cell,&map,result);}
    std::cout<<"COUNTERFACTUAL_DBSPAWN\t"<<key<<'\t'<<record.id<<'\t'<<mode<<'\t'<<loaded<<'\t'<<result->IsInWorld()<<'\t'<<result->IsInGrid()<<'\t'<<result->GetCasterGUID().IsEmpty()<<'\t'<<result->GetDuration()<<'\t'<<result->GetActors().size()<<'\t'<<result->GetLines().size()<<'\n';
    Require(!result->IsInWorld()&&result->IsInGrid()&&result->GetCasterGUID().IsEmpty()&&!result->GetDuration(),"Expected current static admission defect changed");
    result->Remove();Require(result->IsInGrid(),"Current static Remove changed");
    map.RemoveFromMap(result,true);Require(objectCountInWorld[uint8(HighGuid::Conversation)]==count,"Explicit research cleanup leaked");++cases;
   }
  }while(rows->NextRow());
  Require(cases==6,"Research count changed");
  std::cout<<"EXPECTED_BROKEN6 actual native531/747 DB factory and direct/deferred/exact AddObjectHelper admission: reports success/grid-owned but never in world; no caster, ordinary Remove cannot retire it. Explicit native Map cleanup restored allocation count. Actual production AddObjectHelper body exported unchanged; full cell visitation and authenticated Player traversal remain separate; no client or repaired claim.\n";
  std::cout.flush();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
 }
 catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}

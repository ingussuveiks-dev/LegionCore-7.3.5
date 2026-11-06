// Actual native849 payload, real Player publication and original Conversation clock.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
namespace PublicationNative
{
#include "PickupPublication.inc"
}
struct ObservedPickup : PublicationNative::PickupConversation
{
    void WithoutViewer() { playing.erase(owner); }
    void AgeViewer(uint32 age) { playing[owner] = getMSTime() - age; }
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 sConversationDataStore->LoadConversationData();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 MapInstanced parent(1511,300);InstanceMap map(1511,300,9986800,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();
 {
  NativePlayer player(session,map,901,{3515.24f,1800.51f,0.538696f,0});
  player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
  for(uint32 mode=0;mode<3;++mode){
   auto receipt=std::make_shared<PublicationNative::Publication>();
   auto clip=new ObservedPickup;clip->owner=player.GetGUID();clip->publication=receipt;
   Require(clip->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),849,&player,nullptr,player),"Actual native849 creation failed");
   Require(clip->IsInWorld()&&clip->GetCasterGUID()==player.GetGUID()&&clip->GetLines().size(),"Native849 payload/player binding missing");
   auto guid=clip->GetGUID();uint32 budget=clip->GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME);
   Require(budget&&budget==uint32(clip->GetDuration()),"Native849 publication budget invalid");
   clip->WithoutViewer();clip->Update(300000);
   Require(!receipt->finished&&clip->IsInWorld(),"Creation/large diff forged player publication completion");
   Require(!clip->CanNeverSee2(&player),"Real Player native publication was rejected");
   if(mode==0){clip->AgeViewer(budget-1);clip->Update(0);Require(!receipt->finished,"Native849 receipt preceded full published budget");clip->AgeViewer(budget+1);clip->Update(0);Require(receipt->finished&&!clip->IsInWorld(),"Full native849 publication did not acknowledge/retire");}
   if(mode==1){clip->Remove();Require(!receipt->finished,"Early native conversation removal forged acknowledgement");}
   if(mode==2){receipt.reset();clip->AgeViewer(budget+1);clip->Update(0);Require(!clip->IsInWorld(),"Expired consumer retained native conversation");}
   Require(!ObjectAccessor::GetConversation(player,guid),"Native849 removed object remains addressable");
   map.RemoveAllObjectsInRemoveList();
   std::cout<<"CASE native849 mode="<<mode<<" budget_ms="<<budget<<" actual Player/native payload/publication/admission/removal;protected viewer age controlled,not real audio playback\n";
  }
 }
 session->SetPlayer(nullptr);delete session;
 std::cout<<"PASS3 actual native849/derived production publication receipt: no viewer or large diff cannot finish; full actual viewer budget acknowledges, early removal cannot acknowledge,expired consumer safely retires. No character save or persistent DB write;not a rendered client traversal\n";
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

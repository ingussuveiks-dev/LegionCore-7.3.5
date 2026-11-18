// Actual Map/DynamicMapTree/GameObject/Player/TempSummon and matching 2230 mesh.
// Player publication and accepted-scope setup are controlled; no client/quest grant.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameObjectModel.h"
#include "QuestCollisionQuery.h"
#include <filesystem>
#include <cstdlib>
struct CollisionPlayer:Player {
 CollisionPlayer(WorldSession* session, Map& map, uint64 guid):Player(session){
  _Create(ObjectGuid::Create<HighGuid::Player>(guid));SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_DRAENEI);SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);
  SetLevel(110);setFaction(1);SetMaxHealth(1000);SetHealth(1000);Relocate(880,1103,44,0);SetMap(&map);session->SetPlayer(this);sWorld->IncreasePlayerCount();
  sObjectAccessor->AddObject(static_cast<Player*>(this));map.LoadGrid(GetPositionX(),GetPositionY());
  map.AddToGrid(static_cast<Player*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));Unit::AddToWorld();
 }
 ~CollisionPlayer(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);Unit::RemoveFromWorld();RemoveFromGrid();sObjectAccessor->RemoveObject(static_cast<Player*>(this));ResetMap();}
};
struct CollisionDoor:GameObject {
 ~CollisionDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
struct CollisionGuardian:TempSummon {
 CollisionGuardian(Map& map, Unit& summoner, uint32 guid):TempSummon(sSummonPropertiesStore.LookupEntry(2912),&summoner,false){
  Require(Create(guid,&map,1,115132,0,ALLIANCE,880,1103,44,0),"Native guardian create failed");SetSummonerGUID(summoner.GetGUID());
 }
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Guardian AI failed");}
 void RemoveFromWorld()override{Unit::RemoveFromWorld();sObjectAccessor->RemoveObject(static_cast<Creature*>(this));}
 ~CollisionGuardian(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static unsigned cases;
static void Check(bool value,char const* message){Require(value,message);++cases;}
int main(int argc,char**argv){try{
 SetUnhandledExceptionFilter(CrashTrace);Require(argc==2,"Expected dbc path");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Missing private config");
 auto dir=std::filesystem::path(argv[1]).lexically_normal();if(dir.filename().empty())dir=dir.parent_path();std::string runtime=dir.parent_path().parent_path().string()+'/';char* args[]={argv[0],config,runtime.data()};InitializeNative(3,args);
 sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);HashMapHolder<Player>::SetSize(1024);opcodeTable.Initialize();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1469,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1469,{}}});
 LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Login pool failed");
 LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto ro=LoginDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Login not read-only");
 auto session=[](){return new WorldSession(0,std::string("Quest collision native probe"),{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});};
 { Map map(1469,300,9996999,DIFFICULTY_NONE);CollisionPlayer owner(session(),map,100),other(session(),map,101);
  auto a=std::make_shared<QuestObjectScope>(),b=std::make_shared<QuestObjectScope>();a->Owner=owner.GetGUID();b->Owner=other.GetGUID();
  CollisionDoor first,second,shared;
  auto create=[&](CollisionDoor& door,uint32 guid,float offset,std::shared_ptr<QuestObjectScope> const& scope,bool restrictTargets=true){
   Require(door.Create(guid,245122,&map,1,{887.451f+offset,1106.41f,49.4598f,.237051f},G3D::Quat(0,0,.118248f,.992984f),255,GO_STATE_READY),"Actual rockslide creation failed");
   Check(!door.EnableQuestScopedCollision(),"Unbound object opted into private collision");
   if(scope){door.BindQuestObjectScope(scope,restrictTargets);Check(door.EnableQuestScopedCollision(),"Bound unpublished door rejected private collision");}
   Require(map.AddToMap(static_cast<GameObject*>(&door)),"Rockslide publication failed");door.Update(5000);
   Check(!door.EnableQuestScopedCollision(),"Published door changed collision scope");Check(door.m_model&&door.m_model->isCollisionEnabled(),"Native closed rock has no mesh");
  };
  // The second scope deliberately permits ordinary combat targets. Its physical
  // audience must still remain its actual owner, not every target in the map.
  create(first,2501,0,a);create(second,2502,0,b,false);
  Check(first.CanCollideWith(&owner)&&!first.CanCollideWith(&other)&&!first.CanCollideWith(nullptr),"Native owner/foreign/no-context collision audience differs");
  Check(second.CanCollideWith(&other)&&!second.CanCollideWith(&owner),"Unrestricted target scope leaked private collision");
  Actor pet(&map,115132,2510,{880,1103,44,0});Require(map.AddToMap(static_cast<Creature*>(&pet)),"Pet actor publication failed");
  // Pet::Create uses CREATED_BY. This core's SetOwnerGUID writes the distinct
  // SUMMONED_BY field; it alone is not GetOwnerGUID/GetAnyOwner proof.
  pet.SetCreatorGUID(owner.GetGUID());pet.SetOwnerGUID(owner.GetGUID());
  Check(first.CanCollideWith(&pet)&&!second.CanCollideWith(&pet),"Actual owner-guid pet audience differs");
  CollisionGuardian guardian(map,owner,2511);Require(map.AddToMap(static_cast<Creature*>(&guardian)),"Guardian publication failed");
  Check(guardian.GetOwnerGUID().IsEmpty()&&guardian.GetSummonerGUID()==owner.GetGUID(),"Guardian fixture accidentally tests ordinary owner GUID");
  Check(first.CanCollideWith(&guardian)&&!second.CanCollideWith(&guardian),"Actual TempSummon summoner audience differs");
  CollisionGuardian nested(map,guardian,2512);Require(map.AddToMap(static_cast<Creature*>(&nested)),"Nested guardian publication failed");
  Check(first.CanCollideWith(&nested)&&!second.CanCollideWith(&nested),"Nested native guardian audience differs");
  Actor companion(&map,115132,2513,{880,1103,44,0});companion.BindQuestObjectScope(a);Require(map.AddToMap(static_cast<Creature*>(&companion)),"Scoped companion publication failed");
  Check(first.CanCollideWith(&companion)&&!second.CanCollideWith(&companion),"Same/foreign accepted scope audience differs");
  Actor forged(&map,115132,2514,{880,1103,44,0});Require(map.AddToMap(static_cast<Creature*>(&forged)),"Decoy publication failed");
  Check(!first.CanCollideWith(&forged),"Unowned actor acquired private collision");
  guardian.SetSummonerGUID(nested.GetGUID());Check(!first.CanCollideWith(&nested),"Cyclic ownership did not terminate");guardian.SetSummonerGUID(owner.GetGUID());
  auto lo=first.m_model->getBounds().low(),hi=first.m_model->getBounds().high();float x1=lo.x-2,x2=hi.x+2,y=(lo.y+hi.y)/2,z=(lo.z+hi.z)/2;
  auto los=[&](WorldObject const* who,DynamicTreeCallback* out=nullptr){return map.isInLineOfSight(x1,y,z,x2,y,z,{},VMAP::ModelIgnoreFlags::Nothing,out,who);};
  for(auto who:{static_cast<WorldObject const*>(&owner),static_cast<WorldObject const*>(&other),static_cast<WorldObject const*>(&pet),static_cast<WorldObject const*>(&guardian),static_cast<WorldObject const*>(&nested),static_cast<WorldObject const*>(&companion)}){
   DynamicTreeCallback result;Check(!los(who,&result),"Owner's native closed-rock LOS is clear");Check(result.go==(who==&other?&second:&first),"LOS hit receipt belongs to another player's hidden rock");
  }
  Check(los(nullptr)&&los(&forged),"Private mesh blocks an unrelated/context-free ray");
  float rx=0,ry=0,rz=0;DynamicTreeCallback contact;
  Check(map.getObjectHitPos({},true,x1,y,z,x2,y,z,rx,ry,rz,0,&contact,&owner)&&contact.go==&first&&rx<x2,"Native contact ray lost owner/nearest hit");
  Check(!map.getObjectHitPos({},true,x1,y,z,x2,y,z,rx,ry,rz,0,nullptr,&forged)&&rx==x2,"Hidden mesh clipped unrelated contact ray");
  DynamicTreeCallback floor;float nativeFloor=map.GetHeight({},887.451f,1106.41f,70,true,50,nullptr,nullptr);
  float privateFloor=map.GetHeight({},887.451f,1106.41f,70,true,50,&floor,&owner);
  Check(privateFloor>nativeFloor+1&&floor.go==&first,"Native height ignored owner mesh or returned foreign support");
  floor.filter=[](GameObject const*){return false;};Check(map.GetHeight({},887.451f,1106.41f,70,true,50,&floor,&owner)==nativeFloor&&!floor.go,"Caller filter/support receipt not composed");
  DynamicTreeCallback rejected;rejected.filter=[](GameObject const*){return false;};rejected.go=&first;Check(los(&owner,&rejected)&&!rejected.go,"Rejected LOS retained stale supporting-object receipt");
  first.UseDoorOrButton();Check(first.GetGoState()==GO_STATE_ACTIVE&&!first.m_model->isCollisionEnabled(),"Native opening left own collision active");
  Check(los(&owner)&&!los(&other),"Opening owner's door changed another player's closed door");
  Check(map.GetHeight({},887.451f,1106.41f,70,true,50,nullptr,&owner)==nativeFloor,"Foreign closed door became owner's floor after own opening");
  first.ResetDoorOrButton();Check(!los(&owner)&&!los(&other),"Native reset failed to restore each owner's collision");
  a->Cancel();Check(los(&owner)&&!los(&other)&&!first.CanCollideWith(&guardian),"Canceled scope still collides or cancels another owner");
  create(shared,2503,0,{});Check(shared.CanCollideWith(nullptr)&&shared.CanCollideWith(&owner)&&shared.CanCollideWith(&other),"Ordinary shared collision changed");
  Check(!los(&owner)&&!los(&forged)&&!los(nullptr),"Shared closed door stopped blocking ordinary rays");
  shared.UseDoorOrButton();Check(los(&owner)&&los(&forged)&&los(nullptr)&&!los(&other),"Shared opening/private closed-rock separation differs");
  shared.ResetDoorOrButton();shared.SetGoState(GO_STATE_ACTIVE);b->Cancel();
  Check(los(&other),"Canceled peer retained closed model collision");
  pet.SetOwnerGUID(ObjectGuid::Empty);pet.SetCreatorGUID(ObjectGuid::Empty); // Sessions intentionally process lifetime: no destructor/account UPDATE.
 }
 std::cout<<"PASS "<<cases<<" actual native private/shared rock mesh, Map LOS/contact/height receipts, caller-filter composition, two owners, owner-guid pet, TempSummon/nested/cyclic ownership, accepted-scope cancellation, native open/reset; controlled player publication/scope, no whole quest/client claim\n";std::cout.flush();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

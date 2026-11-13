// Actual owned journals/26972 stores and logout endpoints, with controlled
// session logout admission. Durable full-session save is checked separately.
#define main RetainedFinalHealthMain
#include "PetBattleFinalHealthNative.cpp"
#undef main
#include "PetBattleSystem.h"
struct LogoutFlagTag { using Type=bool WorldSession::*;friend Type Access(LogoutFlagTag); };
template struct FixtureAccess<LogoutFlagTag,&WorldSession::m_playerLogout>;
struct LogoutQueueTag { using Type=std::map<ObjectGuid,LFBTicket*> PetBattleSystem::*;friend Type Access(LogoutQueueTag); };
template struct FixtureAccess<LogoutQueueTag,&PetBattleSystem::_LFBRequests>;
static uint32 logoutCases;
static bool reenterFinish;
static uint32 reentryReceipts;
class LogoutReentryProbe : public PlayerScript {
public:
 LogoutReentryProbe():PlayerScript("NativeLogoutReentryProbe"){}
 void OnPetBattleFinish(Player* player) override {
  if(!reenterFinish)return;auto b=sPetBattleSystem->GetBattle(player->_petBattleId);Require(b&&b->BattleStatus==PETBATTLE_STATUS_FINISHING,"Actual callback must see a nondeletable finishing battle");auto id=b->ID;auto hp=b->Pets[0]->Health;
  b->Finish(1,true,false);Require(b->Pets[0]->Health==hp,"Callback reentry cannot repeat completion side effects");sPetBattleSystem->Update(30000);Require(sPetBattleSystem->GetBattle(id)==b&&b->BattleStatus==PETBATTLE_STATUS_FINISHING,"Manager update must neither queue nor delete a battle still inside Finish");++reentryReceipts;
 }
};
static void LogoutCases(Player& player,std::string const& choice) {
 for(uint32 team=0;team<2;++team)for(uint32 mode=0;mode<17;++mode){
  if(!choice.empty()&&((choice=="detached"&&mode!=1)||(choice=="webbed"&&mode!=2)||(choice=="offline"&&mode!=8)))continue;
  auto b=sPetBattleSystem->CreateBattle();HealthFill(*b,player,team);auto id=b->ID;player._petBattleId=id;auto p=b->Pets[team*3];
  auto session=player.GetSession();session->*Access(LogoutFlagTag{})=true;
  if(mode==0){sPetBattleSystem->OnPlayerLogout(&player);Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&p->Health==90&&p->OriginalBattlePet->Health==90&&p->OriginalBattlePet->needSave&&!player._petBattleId,"Bound online logout must forfeit and persist exactly once");}
  else if(mode==1){player.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&player);Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&p->Health==90&&p->OriginalBattlePet->Health==90&&p->OriginalBattlePet->needSave&&!player._petBattleId,"Bound detached logout must preserve native penalty and actual journal before save");}
  else if(mode==2){p->States[BATTLEPET_STATE_Mechanic_IsWebbed]=1;sPetBattleSystem->ForfeitBattle(id,player.GetGUID(),false);Require(b->BattleStatus==PETBATTLE_STATUS_RUNNING,"Voluntary webbed forfeit must remain blocked outside scoped logout dispatch");player.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&player);Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&p->OriginalBattlePet->Health==90&&!player._petBattleId,"Genuine detached logout must settle a webbed battle instead of orphaning it");}
  else if(mode==3){b->BattleStatus=PETBATTLE_STATUS_CREATION;for(auto t:b->Teams)t->ActivePetID=0x7fffffff;sPetBattleSystem->OnPlayerLogout(&player);Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&b->WinnerTeamId==-1&&p->Health==100&&!p->OriginalBattlePet->needSave&&!player._petBattleId,"Unstarted cancellation must not read active pets,award a win or penalize/save combat state");}
  else if(mode==4){sPetBattleSystem->OnPlayerLogout(&player);sPetBattleSystem->OnPlayerLogout(&player);b->Finish(!team,true,false);Require(p->Health==90&&p->OriginalBattlePet->Health==90,"Repeated logout/Finish must not apply a second penalty");}
  else if(mode==5){session->*Access(LogoutFlagTag{})=false;sPetBattleSystem->OnPlayerLogout(&player);Require(b->BattleStatus==PETBATTLE_STATUS_RUNNING&&player._petBattleId==id&&!p->OriginalBattlePet->needSave,"An ordinary caller cannot invoke forced logout settlement");}
  else if(mode==6){b->Teams[team]->OwnerGuid=ObjectGuid::Create<HighGuid::Player>(999999);sPetBattleSystem->OnPlayerLogout(&player);Require(b->BattleStatus==PETBATTLE_STATUS_RUNNING&&!p->OriginalBattlePet->needSave,"A foreign owner cannot terminate or persist another battle");}
  else if(mode==7){player._petBattleId=ObjectGuid::Create<HighGuid::PetBattle>(999999);sPetBattleSystem->OnPlayerLogout(&player);Require(!player._petBattleId&&b->BattleStatus==PETBATTLE_STATUS_RUNNING,"Missing battle clears only the player's stale binding");}
  else if(mode==8){player.Unit::RemoveFromWorld();b->Finish(!team,true,false);Require(p->Health==90&&p->OriginalBattlePet->Health==90&&p->OriginalBattlePet->needSave,"Detached ordinary Finish must save owned journal without a filtered Player lookup");sPetBattleSystem->OnPlayerLogout(&player);Require(p->Health==90&&!player._petBattleId,"Later logout cannot repeat a prior detached Finish penalty");}
  else if(mode==9){Sacrifice(*b,p->ID);player.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&player);Require(!p->IsAlive()&&!p->Health&&!p->OriginalBattlePet->Health&&p->OriginalBattlePet->needSave,"Actual positive-health final death must persist zero through detached logout");}
  else if(mode==10){p->OriginalBattlePet.reset();player.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&player);Require(p->Health==90&&!player._petBattleId,"Missing journal cannot prevent battle release");}
  else if(mode==11){b->BattleType=PETBATTLE_TYPE_PVP_MATCHMAKING;player.m_recallLoc=WorldLocation(1220,5478,470,150,1);player.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&player);Require(player.GetMapId()==1220&&player.GetPositionX()==5478&&!player.IsBeingTeleportedFar(),"Logout recall must prepare the safe saved pose without teleporting a deleting player");}
  else if(mode==12){auto request=sPetBattleSystem->CreateRequest(player.GetGUID());request->RequesterGuid=player.GetGUID();player._petBattleId.Clear();sPetBattleSystem->OnPlayerLogout(&player);Require(!sPetBattleSystem->GetRequest(player.GetGUID()),"Logout must remove unanswered own challenge even without a battle");}
  else if(mode==13){p->XP=11;p->UpdateOriginalInstance(nullptr);Require(p->OriginalBattlePet->XP==11&&p->OriginalBattlePet->needSave,"Offline original-instance copy must not dereference a null notification receiver");}
  else if(mode==14){b->BattleStatus=PETBATTLE_STATUS_CREATION;for(auto t:b->Teams)t->ActivePetID=0x7fffffff;session->*Access(LogoutFlagTag{})=false;sPetBattleSystem->ForfeitBattle(id,player.GetGUID(),false);Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&b->WinnerTeamId==-1&&p->Health==100&&!p->OriginalBattlePet->needSave&&!player._petBattleId,"Ordinary cancel before Begin must share safe unstarted cancellation without active-pet access or penalty");}
  else if(mode==15){b->BattleStatus=PETBATTLE_STATUS_CREATION;for(auto t:b->Teams)t->ActivePetID=0x7fffffff;b->Finish(!team,true,false);Require(b->BattleStatus==PETBATTLE_STATUS_CREATION&&p->Health==100&&!p->OriginalBattlePet->needSave,"Direct Finish cannot treat an unstarted battle as combat");sPetBattleSystem->OnPlayerLogout(&player);}
  else{b->BattleType=PETBATTLE_TYPE_PVP_MATCHMAKING;player.m_recallLoc=WorldLocation(1220,5478,470,150,1);player.GetTeleportDest().WorldRelocate(0,1,2,3,0);player.SetSemaphoreTeleportFar(true);player.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&player);Require(player.IsBeingTeleportedFar()&&player.GetTeleportDest().GetMapId()==1220&&player.GetTeleportDest().GetPositionX()==5478,"Pending matchmaking transfer must retain loading state and save the real recall destination");player.SetSemaphoreTeleportFar(false);}
  Require(!sPetBattleSystem->GetLoggingOutPlayer(id),"Synchronous logout binding must not escape its scope");
  if(!player.IsInWorld())player.Unit::AddToWorld();player.WorldRelocate(WorldLocation(player.GetMap()->GetId(),52,803,805,0));player._petBattleId.Clear();player.m_Events.KillAllEvents(true);session->*Access(LogoutFlagTag{})=false;sPetBattleSystem->RemoveBattle(id);++logoutCases;
 }
}
static void SharedCases(Player& first,Player& second,Map& map) {
 {
  auto b=sPetBattleSystem->CreateBattle();HealthFill(*b,first,0);auto id=b->ID;first._petBattleId=id;first.GetSession()->*Access(LogoutFlagTag{})=true;reenterFinish=true;sPetBattleSystem->OnPlayerLogout(&first);reenterFinish=false;
  Require(reentryReceipts==1&&b->BattleStatus==PETBATTLE_STATUS_FINISHED&&b->Pets[0]->Health==90,"Native callback/manager reentry must finish once with one penalty");first.GetSession()->*Access(LogoutFlagTag{})=false;first.m_Events.KillAllEvents(true);sPetBattleSystem->RemoveBattle(id);++logoutCases;
 }
 {
  sPetBattleSystem->JoinQueue(&first);sPetBattleSystem->JoinQueue(&second);auto&queue=sPetBattleSystem->*Access(LogoutQueueTag{});auto a=queue.at(first.GetGUID()),b=queue.at(second.GetGUID());Require(a&&b,"Actual native queue tickets missing");a->State=b->State=LFBState::LFB_STATE_PROPOSAL;a->MatchingOpponent=b;b->MatchingOpponent=a;sPetBattleSystem->CreateRequest(first.GetGUID());first.GetSession()->*Access(LogoutFlagTag{})=true;sPetBattleSystem->OnPlayerLogout(&first);
  Require(!queue.at(first.GetGUID())&&b->State==LFBState::LFB_STATE_QUEUED&&!b->MatchingOpponent&&!sPetBattleSystem->GetRequest(first.GetGUID()),"Actual logout must cancel its request/proposal and return the connected opponent to queue");first.GetSession()->*Access(LogoutFlagTag{})=false;sPetBattleSystem->LeaveQueue(&second);++logoutCases;
 }
 for(uint32 team=0;team<2;++team)for(bool creation:{false,true}){
  auto leaving=team?&second:&first;auto other=team?&first:&second;auto b=sPetBattleSystem->CreateBattle();HealthFill(*b,*leaving,team);b->BattleType=PETBATTLE_TYPE_PVP_DUEL;b->Teams[!team]->PlayerGuid=b->Teams[!team]->OwnerGuid=other->GetGUID();auto id=b->ID;leaving->_petBattleId=other->_petBattleId=id;
  for(auto p:{leaving,other}){p->SetControlled(true,UNIT_STATE_ROOT);p->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_NPC);}
  if(creation){b->BattleStatus=PETBATTLE_STATUS_CREATION;for(auto t:b->Teams)t->ActivePetID=0x7fffffff;}
  leaving->GetSession()->*Access(LogoutFlagTag{})=true;leaving->Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(leaving);
  Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&!leaving->_petBattleId&&!other->_petBattleId&&!other->HasUnitState(UNIT_STATE_ROOT)&&!other->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_NPC),"Connected opponent must be released from running and unstarted battles for either leaving team");
  Require(creation?b->WinnerTeamId==-1:uint32(b->WinnerTeamId)==!team,"Unstarted cancellation has no winner;running logout preserves the real opponent winner");
  for(auto p:b->Teams[!team]->TeamPets)Require(!p->OriginalBattlePet->needSave==creation&&p->Health==100,"Online opponent persistence cannot invent a PvP health penalty or pre-start journal changes");
  leaving->Unit::AddToWorld();leaving->GetSession()->*Access(LogoutFlagTag{})=false;for(auto p:{leaving,other})p->m_Events.KillAllEvents(true);sPetBattleSystem->RemoveBattle(id);++logoutCases;
 }
 for(bool creation:{false,true})for(bool rebound:{false,true}){
  Actor wild(&map,98572,31000+logoutCases,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&wild)),"Native wild binding actor admission failed");auto b=sPetBattleSystem->CreateBattle();HealthFill(*b,first,0);b->PveBattleType=PVE_PETBATTLE_WILD;auto id=b->ID;first._petBattleId=id;b->Pets[3]->OriginalCreature=wild.GetGUID();wild._petBattleId=rebound?ObjectGuid::Create<HighGuid::PetBattle>(999999):id;wild.SetControlled(true,UNIT_STATE_ROOT);wild.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_PC);
  if(creation){b->BattleStatus=PETBATTLE_STATUS_CREATION;for(auto t:b->Teams)t->ActivePetID=0x7fffffff;}
  first.GetSession()->*Access(LogoutFlagTag{})=true;first.Unit::RemoveFromWorld();sPetBattleSystem->OnPlayerLogout(&first);
  Require(rebound?bool(wild._petBattleId):!wild._petBattleId,"Only the actual battle-bound wild creature may be released");Require(rebound?wild.HasUnitState(UNIT_STATE_ROOT):!wild.HasUnitState(UNIT_STATE_ROOT),"Do not unroot a wild creature rebound to another battle");
  first.Unit::AddToWorld();first.GetSession()->*Access(LogoutFlagTag{})=false;first.m_Events.KillAllEvents(true);sPetBattleSystem->RemoveBattle(id);++logoutCases;
 }
}
#ifndef PET_LOGOUT_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeAltheaNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999831,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();auto selected=std::getenv("LEGION_PET_LOGOUT_CASE");std::string choice=selected?selected:"";
 if(choice.empty()){sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new LogoutReentryProbe;sScriptMgr->SwapScriptContext(true);}
 {NativePlayer player(session,map,28302,{52,803,805,0});player.setFactionForRace(RACE_HUMAN);LogoutCases(player,choice);if(choice.empty()){auto otherSession=Session();{NativePlayer other(otherSession,map,28303,{54,803,805,0});other.setFactionForRace(RACE_HUMAN);SharedCases(player,other,map);}otherSession->SetPlayer(nullptr);delete otherSession;}}session->SetPlayer(nullptr);delete session;CloseAltheaNative();Require(!choice.empty()||logoutCases==44,"Logout endpoint inventory incomplete");std::cout<<"PASS actual native logout cases="<<logoutCases<<";selected="<<(choice.empty()?"all44":choice)<<";controlled session admission,no persistent character writes\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif

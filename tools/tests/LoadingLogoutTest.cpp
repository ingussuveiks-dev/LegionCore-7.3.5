// Extracted production map iteration and logout scheduler. Socket/DB/native
// removal endpoints are controlled; this is not a full client logout test.
#include <cassert>
#include <cstdint>
#include <ctime>
#include <exception>
#include <iostream>
#include <vector>
using uint32=uint32_t;
struct Player;struct Map;
struct ObjectGuid{bool IsEmpty(){return true;}};
constexpr unsigned MOVEMENTFLAG_FALLING=1,MOVEMENTFLAG_FALLING_FAR=2,PLAYER_FIELD_PLAYER_FLAGS=3,PLAYER_FLAGS_RESTING=4,CONFIG_INSTANT_LOGOUT=5,UNIT_STAND_STATE_SIT=6,UNIT_FIELD_FLAGS=7,UNIT_FLAG_STUNNED=8;
using AccountTypes=unsigned;
struct World{unsigned getIntConfig(unsigned value){assert(value==CONFIG_INSTANT_LOGOUT);return 3;}} world;auto sWorld=&world;
namespace WorldPackets {namespace Character {struct LogoutRequest{};struct LogoutResponse{uint32 LogoutResult=0;bool Instant=false;LogoutResponse* Write(){return this;}};}}
uint32 getMSTime(){return 1;}uint32 GetMSTimeDiffToNow(uint32){return 0;}
namespace GameTime {time_t now=100;time_t GetGameTime(){return now;}}
struct Log {template<class... T>void outDiff(T...){assert(false);}template<class... T>void outTryCatch(T...){assert(false);}} logObject;
auto sLog=&logObject;
struct Loading {bool pending=true;bool IsEmpty(){return !pending;}};
struct WorldSession {
 Player* player=nullptr;Map* map=nullptr;bool canLogout=false,loggingOut=false,kicked=false;unsigned completed=0;
 time_t _logoutTime=0;Loading m_playerLoading;
 WorldPackets::Character::LogoutResponse response;unsigned sent=0;
 bool PlayerLoading(){return !m_playerLoading.IsEmpty();}bool PlayerLogout(){return loggingOut;}
 Player* GetPlayer(){return player;}Map* GetMap(){return map;}void SetCanLogout(){canLogout=true;}
 void KickPlayer(){kicked=true;}void LogoutPlayer(bool save){assert(save);++completed;player=nullptr;m_playerLoading.pending=false;_logoutTime=0;}
 unsigned GetSecurity(){return 0;}void DoLootRelease(ObjectGuid){}void SendPacket(WorldPackets::Character::LogoutResponse* value){response=*value;++sent;}
 void LogoutRequest(time_t time){_logoutTime=time;}void HandleLogoutRequest(WorldPackets::Character::LogoutRequest&);
 bool ShouldLogOut(time_t currTime)const{
#include "ShouldLogOut.inc"
 }
 void Schedule();
};
struct Player {
 WorldSession* session=nullptr;Map* map=nullptr;bool deleted=false,preDelete=false,changing=false,world=true,remove=false,removeDuringUpdate=false;
 unsigned updates=0;uint32 _castCount=0,_targetCount=0;
 bool resting=false,combat=false,falling=false,frozen=false,duel=false,flight=false,gm=false,free=true,rooted=false,stunned=false;uint32 stand=0;
 struct Movement{bool falling=false;bool HasMovementFlag(unsigned){return falling;}}m_movementInfo;
 struct Instance{bool active=false;bool IsEncounterInProgress(){return active;}} instance;
 bool hasInstance=false;
 ObjectGuid GetLootGUID(){return {};}bool isInCombat(){return combat;}bool HasAura(unsigned value){assert(value==9454);return frozen;}
 Instance* GetInstanceScript(){return hasInstance?&instance:nullptr;}bool isGameMaster(){return gm;}
 bool HasFlag(unsigned field,unsigned value){assert(field==PLAYER_FIELD_PLAYER_FLAGS&&value==PLAYER_FLAGS_RESTING);return resting;}
 bool isInFlight(){return flight;}uint32 GetMapId(){return 1;}bool CanFreeMove(){return free;}
 void SetStandState(uint32 value){stand=value;}void SetRooted(bool value){rooted=value;}void SetFlag(unsigned field,unsigned value){assert(field==UNIT_FIELD_FLAGS&&value==UNIT_FLAG_STUNNED);stunned=true;}
 bool IsDelete(){return deleted;}bool IsPreDelete(){return preDelete;}WorldSession* GetSession(){return session;}
 bool IsChangeMap(){return changing;}Map* GetMap(){return map;}bool IsInWorld(){return world;}
 bool IsRemoveFromMap(){return remove;}void SetRemoveFromMap(bool value){remove=value;}
 uint32 GetGUIDLow(){return 1;}void Update(uint32){++updates;if(removeDuringUpdate)remove=true;}
};
struct Reference {Player* player=nullptr;Player* getSource(){return player;}};
struct Map {
 std::vector<Reference> m_mapRefManager;std::vector<Reference>::iterator m_mapRefIter;
 bool b_isMapUnload=false;WorldSession* m_currentSession=nullptr;
 uint32 i_InstanceId=0,m_activeEntry=0;unsigned removed=0,visits=0;
 uint32 GetId(){return 1;}void VisitNearbyCellsOf(Player*){++visits;}
 void RemovePlayerFromMap(Player* player,bool destroy){assert(!destroy&&player->map==this);++removed;player->world=false;}
 void Tick(){uint32 t_diff=1,_mapId=1,_instanceId=0;
#include "MapIteration.inc"
 }
};
void WorldSession::Schedule(){
#include "Schedule.inc"
}
#include "LogoutRequest.inc"
struct Fixture {
 Map map,other;WorldSession session;Player player;
 Fixture(){session.player=&player;session.map=&map;player.session=&session;player.map=&map;map.m_mapRefManager.push_back({&player});}
 void Requested(bool instant){player.resting=instant;WorldPackets::Character::LogoutRequest request;session.HandleLogoutRequest(request);assert(session.sent==1&&session.response.LogoutResult==0&&session.response.Instant==instant);}
};
int main(){
 // Existing ordinary loading protection is retained, with no fake loading
 // acknowledgement, progression, actor update or initialization callback.
 {Fixture f;f.map.Tick();assert(!f.player.updates&&!f.map.removed&&f.session.PlayerLoading());}
 {Fixture f;f.session.m_playerLoading.pending=false;f.map.Tick();assert(f.player.updates==1&&f.map.visits==1&&!f.map.removed);}
 for(bool instant:{false,true}){
  GameTime::now=100;Fixture f;f.Requested(instant);
  if(!instant){f.session.Schedule();f.map.Tick();assert(!f.player.remove&&!f.map.removed);GameTime::now=119;f.session.Schedule();assert(!f.player.remove);GameTime::now=120;}
  f.session.Schedule();f.map.Tick();
#ifdef COUNTERFACTUAL
  assert(!f.map.removed&&!f.session.canLogout&&!f.session.completed&&f.session.PlayerLoading());
  assert(f.player.remove==instant);
#else
  assert(f.map.removed==1&&!f.player.remove&&f.session.canLogout&&f.session.PlayerLoading()&&!f.player.updates&&!f.map.visits);
  f.session.Schedule();assert(f.session.completed==1&&!f.session.player&&!f.session.PlayerLoading());
  f.session.Schedule();assert(f.session.completed==1);
#endif
 }
 // Fully admitted logout, player updates requesting removal, and cancellation
 // keep the original behavior and strict20-second timeout boundary.
 for(bool instant:{false,true}){GameTime::now=100;Fixture f;f.session.m_playerLoading.pending=false;f.Requested(instant);GameTime::now=120;f.session.Schedule();f.map.Tick();assert(f.map.removed==1&&f.session.canLogout);f.session.Schedule();assert(f.session.completed==1);}
 {Fixture f;f.session.m_playerLoading.pending=false;f.player.removeDuringUpdate=true;f.map.Tick();assert(f.player.updates==1&&f.map.removed==1&&f.session.canLogout);}
 {GameTime::now=100;Fixture f;f.Requested(false);f.session.LogoutRequest(0);GameTime::now=1000;f.session.Schedule();f.map.Tick();assert(!f.map.removed&&!f.player.remove&&!f.session.completed);}
 for(unsigned mode=0;mode<4;++mode){GameTime::now=100;Fixture f;f.player.resting=true;
  if(mode==0)f.player.combat=true;if(mode==1)f.player.m_movementInfo.falling=true;if(mode==2)f.player.duel=true;if(mode==3){f.player.hasInstance=true;f.player.instance.active=true;}
  WorldPackets::Character::LogoutRequest request;f.session.HandleLogoutRequest(request);assert(f.session.response.LogoutResult==(mode==1?3u:mode==2?2u:1u)&&!f.player.remove&&!f.session._logoutTime);
  f.map.Tick();assert(!f.map.removed&&!f.session.canLogout);
 }
 // The source/destination handoff and ownership/deletion guards must still
 // block removal; never mutate a Player from the wrong map or mid-transfer.
 for(unsigned mode=0;mode<7;++mode){Fixture f;f.Requested(true);
  if(mode==0)f.player.changing=true;if(mode==1)f.player.map=&f.other;if(mode==2)f.session.map=&f.other;
  if(mode==3)f.player.deleted=true;if(mode==4)f.player.preDelete=true;if(mode==5)f.session.loggingOut=true;if(mode==6)f.map.b_isMapUnload=true;
  f.map.Tick();assert(!f.map.removed&&!f.player.updates&&!f.session.canLogout&&f.player.remove);
 }
 {GameTime::now=1000;Fixture f;f.session.player=nullptr;f.session._logoutTime=100;f.session.Schedule();assert(!f.session.canLogout&&!f.session.completed);f.session.m_playerLoading.pending=false;f.session.Schedule();assert(f.session.canLogout);f.session.Schedule();assert(f.session.completed==1);}
#ifdef COUNTERFACTUAL
 std::cout<<"COUNTERFACTUAL: pending loading receipt blocks both instant map removal and expired20s logout scheduling until another loading notification. Ordinary admitted paths still work.\n";
#else
 std::cout<<"PASS actual map iteration/logout scheduler: pending-loading instant and expired20s requests detach once,then native logout dispatch; no Player update/visibility initialization before loading. Ordinary/cancel/no-Player/predelete/wrong-map/transfer/timeout guards retained. Socket,DB,cleanup endpoints and client timing remain separate.\n";
#endif
}

#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
namespace TenthQueue {
using uint32=uint32_t;using ObjectGuid=uint64_t;
#include "TenthQueueLabels.inc"
using eBattlePetRequests=int;using LFBState=int;using LFBAnswer=int;using LFBUpdateStatus=int;
// Scoped names in the actual methods are enum qualifiers; matching named constants
// are supplied by the selected source label table, with aliases below generated.
struct BattlePet{int Health=100;};struct BattlePetInstance:BattlePet{uint32 Slot=0;std::shared_ptr<BattlePet>OriginalBattlePet;void CloneFrom(std::shared_ptr<BattlePet>p){Health=p->Health;}};
struct Player;struct LFBTicket{ObjectGuid RequesterGUID=0;int State=LFB_STATE_QUEUED,ProposalAnswer=LFB_ANSWER_PENDING;LFBTicket*MatchingOpponent=nullptr;uint32 JoinTime=0,TicketID=0;};
namespace WorldPackets{namespace BattlePet{struct NullCmsg{};}}
struct World{bool enabled=true;bool getBoolConfig(uint32){return enabled;}};static World world;static World*sWorld=&world;
struct WorldSession{Player*_player=nullptr;int failed=0;void HandleJoinPetBattleQueue(WorldPackets::BattlePet::NullCmsg&);void SendPetBattleRequestFailed(int r){failed=r;}void SendPetBattleQueueStatus(uint32,uint32,int,uint32){}void SendBattlePetJournalLockDenied(){}};
struct Player{ObjectGuid guid;bool live=true,dead=false,combat=false;uint64_t _petBattleId=0;uint32 unlocked=1;WorldSession session;std::array<std::shared_ptr<BattlePet>,3>pets;explicit Player(ObjectGuid id):guid(id){session._player=this;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return live;}bool isDead(){return dead;}bool IsAlive(){return !dead;}bool isInCombat(){return combat;}void UpdateBattlePetCombatTeam(){}auto GetBattlePetCombatTeam(){return pets;}uint32 GetUnlockedPetBattleSlot(){return unlocked;}WorldSession*GetSession(){return &session;}};
static std::map<ObjectGuid,Player*>players;namespace ObjectAccessor{static Player*FindPlayer(ObjectGuid id){auto i=players.find(id);return i==players.end()?nullptr:i->second;}}
struct PetBattleSystem{std::mutex _LFBRequestsMutex;std::map<ObjectGuid,LFBTicket*>_LFBRequests;uint32 _LFBAvgWaitTime=0;~PetBattleSystem(){for(auto&p:_LFBRequests)delete p.second;}eBattlePetRequests CheckQueueEntry(Player*);void JoinQueue(Player*p){auto&t=_LFBRequests[p->guid];if(!t){t=new LFBTicket;t->RequesterGUID=p->guid;}}void LeaveQueue(Player*);void Decline(LFBTicket*,LFBTicket*,Player*,Player*,std::vector<ObjectGuid>&);};static PetBattleSystem*sPetBattleSystem=nullptr;
#include "TenthQueueSelected.inc"
static void Require(bool b,char const*s){if(!b)throw std::runtime_error(s);}
static void Run(int n){PetBattleSystem sys;sPetBattleSystem=&sys;Player a(1),b(2),c(3);players={{1,&a},{2,&b},{3,&c}};WorldPackets::BattlePet::NullCmsg packet;
 if(n==8){a.pets[0]=std::make_shared<BattlePet>();a.dead=true;a.session.HandleJoinPetBattleQueue(packet);Require(!sys._LFBRequests.count(1),"Dead player cannot enter the pet matchmaking queue");a.dead=false;Require(sys.CheckQueueEntry(&a)==PETBATTLE_REQUEST_OK,"Living player and living unlocked pet remain eligible");a.pets[0]->Health=0;Require(sys.CheckQueueEntry(&a)==PETBATTLE_REQUEST_ALL_PETS_DEAD,"Dead roster is refused");a.unlocked=0;Require(sys.CheckQueueEntry(&a)==PETBATTLE_REQUEST_NO_PETS_IN_SLOT,"Locked slots cannot admit saved pets");a.unlocked=1;a.pets[0]->Health=100;a.combat=true;Require(sys.CheckQueueEntry(&a)==PETBATTLE_REQUEST_NOT_WHILE_IN_COMBAT,"Combat admission refused");a.combat=false;a._petBattleId=1;Require(sys.CheckQueueEntry(&a)==PETBATTLE_REQUEST_IN_BATTLE,"Existing battle refused");a._petBattleId=0;a.live=false;Require(sys.CheckQueueEntry(&a)==PETBATTLE_REQUEST_CREATE_FAILED,"Detached player refused");}
 else{sys.JoinQueue(&a);sys.JoinQueue(&b);sys.JoinQueue(&c);auto left=sys._LFBRequests[1],right=sys._LFBRequests[2],third=sys._LFBRequests[3];left->MatchingOpponent=right;right->MatchingOpponent=left;left->State=right->State=LFB_STATE_PROPOSAL;right->ProposalAnswer=LFB_ANSWER_DENY;std::vector<ObjectGuid>remove;sys.Decline(left,right,&a,&b,remove);left->MatchingOpponent=third;third->MatchingOpponent=left;left->State=third->State=LFB_STATE_PROPOSAL;sys.LeaveQueue(&b);Require(left->MatchingOpponent==third&&third->MatchingOpponent==left&&left->State==LFB_STATE_PROPOSAL,"Old declined-ticket cleanup cannot dismantle a newly matched third-player pair");}players.clear();sPetBattleSystem=nullptr;}
}

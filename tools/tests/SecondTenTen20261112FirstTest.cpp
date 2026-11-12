#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using uint8=uint8_t;
#include "Labels.inc"
using eBattlePetRequests=int;
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
struct Guid{int value=0;Guid()=default;Guid(int v):value(v){}explicit operator bool()const{return value!=0;}bool IsPlayer()const{return value>0&&value<100;}bool IsCreature()const{return value>=100;}int GetEntry()const{return value;}void Clear(){value=0;}friend bool operator<(Guid a,Guid b){return a.value<b.value;}friend bool operator==(Guid a,Guid b){return a.value==b.value;}};
using ObjectGuid=Guid;
struct Pos{};struct Geometry{bool getObjectHitPos(int,bool,Pos const&,Pos const&,float){return false;}};
struct PetBattle;struct Player;struct Creature;struct BattlePetInstance;struct PetBattleRequest;
struct Session{int finalRounds=0;void SendPetBattleFinished(){}void SendBattlePetUpdates(BattlePetInstance*){}void SendPetBattleFinalRound(PetBattle*){++finalRounds;}void SendBattlePetJournal(){}};
struct Player{Guid guid{1},_petBattleId{1};int m_recallLoc=0,bonus=0,levels=0,wins=0;bool dead=false,combat=false,world=true,range=true,los=true;Geometry map;Session session;std::array<std::shared_ptr<BattlePetInstance>,3>pets{};bool IsInWorld(){return world;}bool isDead(){return dead;}bool isInCombat(){return combat;}Guid GetGUID(){return guid;}int GetPhases(){return 0;}Geometry*GetMap(){return &map;}Session*GetSession(){return &session;}auto GetBattlePetCombatTeam(){return pets;}int GetUnlockedPetBattleSlot(){return 3;}bool IsWithinDist3d(Player*p,int){return p==this||range;}bool IsWithinLOSInMap(Player*p){return p==this||los;}bool IsWithinDist3d(Creature*,int){return range;}bool IsWithinLOSInMap(Creature*){return los;}Creature*GetNPCIfCanInteractWith(Guid,int){return nullptr;}int GetTotalAuraModifier(int){return bonus;}void UpdateBattlePetCombatTeam(){}template<class...T>void UpdateAchievementCriteria(int kind,T...){if(kind==CRITERIA_TYPE_BATTLEPET_LEVEL_UP)++levels;if(kind==CRITERIA_TYPE_BATTLEPET_WIN)++wins;}template<class...T>void QuestObjectiveSatisfy(T...){}void KilledMonsterCredit(int){}void CompletedAchievement(int const*){}void RemoveFlag(int,int){}void SetControlled(bool,int){}template<class F>void AddDelayedEvent(int,F){}void SummonLastSummonedBattlePet(){}void TeleportTo(int){}std::map<Guid,std::shared_ptr<BattlePetInstance>>_battlePets;};
struct Instance{void SetGuidData(int,Guid){}};using InstanceScript=Instance;struct Creature{Guid _petBattleId{1};int released=0;void RemoveFlag(int,int){++released;}void SetControlled(bool,int){}int GetEntry(){return 100;}Guid GetGUID(){return 100;}Instance*GetInstanceScript(){return nullptr;}};
static Player*players[2]{};static Creature*wild=nullptr;
namespace ObjectAccessor{static Player*FindPlayer(Guid g){for(auto p:players)if(p&&p->guid==g)return p;return nullptr;}static Player*GetObjectInWorld(Guid g,Player*){return FindPlayer(g);}static Creature*GetObjectInOrOutOfWorld(Guid,Creature*){return wild;}}
struct Accessor{Creature*GetCreature(Player&,Guid){return wild;}};static Accessor accessor,*sObjectAccessor=&accessor;
struct Scripts{int finishes=0;void OnPetBattleFinish(Player*){++finishes;}};static Scripts scripts,*sScriptMgr=&scripts;
struct Species{int ID=1,PetTypeEnum=1,CreatureID=1;};struct SpeciesStore{Species data;Species*LookupEntry(int id){return id?&data:nullptr;}};static SpeciesStore sBattlePetSpeciesStore;
struct Manager{Species data;Species*GetSpeciesByCreatureID(int id){return id?&data:nullptr;}};static Manager sDB2Manager;
struct Achievements{int data=0;int*LookupEntry(int){return &data;}};static Achievements sAchievementStore;
struct Wilds{int leaves=0;void LeaveBattle(Creature*,bool){++leaves;}};static Wilds wilds,*sWildBattlePetMgr=&wilds;
template<class T>static T AddPct(T n,int pct){return T(n+n*pct/100);}static int CalculatePct(int n,int pct){return n*pct/100;}
struct BattlePetInstance{Guid OriginalCreature,JournalID;int Health=100,InfoMaxHealth=100,ID=0,Species=1,Quality=0,Slot=0;uint32 Level=1,XP=0;std::array<int,1024>States{};uint32 earned=100;bool IsAlive(){return !States[0];}uint32 GetXPEarn(uint32){return earned;}uint32 GetMaxXPForCurrentLevel(){return Level<25?Level*100:0;}void UpdateStats(){InfoMaxHealth=100+Level;Health=InfoMaxHealth;}void UpdateAbilities(){}void UpdateOriginalInstance(Player*){}void AddToPlayer(Player*){}};
struct Team{Guid PlayerGuid,OwnerGuid;uint32 ActivePetID=0,TeamPetCount=0,CapturedPet=0;bool locked=false,isRun=false,Ready=false;std::array<std::shared_ptr<BattlePetInstance>,3>TeamPets{};std::vector<int>GetAvailablesPets(){std::vector<int>v;if(!locked)for(auto p:TeamPets)if(p&&p->IsAlive())v.push_back(p->ID);return v;}bool CanSwap(int){return true;}int CanCatchOpponentTeamFrontPet(){return PETBATTLE_TEAM_CATCH_FLAG_ENABLE_TRAP;}uint32 GetCatchAbilityID(){return 1;}};
struct PetBattle{Guid ID{1},InitialWildPetGUID;uint32 BattleStatus=PETBATTLE_STATUS_RUNNING,RoundStatus=0,WinnerTeamId=0,BattleType=PETBATTLE_TYPE_PVE,PveBattleType=PVE_PETBATTLE_TRAINER,CombatResult=0,Turn=1;std::array<std::shared_ptr<BattlePetInstance>,6>Pets{};std::array<Team*,2>Teams;std::map<int,bool>FightedPets;PetBattle(){Teams={new Team,new Team};Teams[1]->ActivePetID=3;}~PetBattle(){for(auto t:Teams)delete t;}int GetForfeitHealthPenalityPct(){return 10;}void Finish(uint32,bool,bool);bool CanCast(int,int){return true;}void PrepareCast(int,int){}void SwapPet(int,int){}};
struct PetBattleRequest{static inline int alive=0;Guid RequesterGuid,OpponentGuid;std::array<Pos,2>TeamPosition{};Pos PetBattleCenterPosition;PetBattleRequest(){++alive;}~PetBattleRequest(){--alive;}};
struct PetBattleSystem{PetBattle*b=nullptr;int forfeits=0;std::map<Guid,PetBattleRequest*>_battleRequests;~PetBattleSystem(){for(auto row:_battleRequests)delete row.second;}PetBattle*GetBattle(Guid){return b;}void ForfeitBattle(Guid,Guid,bool){++forfeits;}PetBattleRequest*CreateRequest(Guid);void RemoveRequest(Guid);PetBattleRequest*GetRequest(Guid g){auto i=_battleRequests.find(g);return i==_battleRequests.end()?nullptr:i->second;}eBattlePetRequests CanPlayerEnterInPetBattle(Player*,PetBattleRequest*);};static PetBattleSystem systemFixture,*sPetBattleSystem=&systemFixture;
namespace WorldPackets{namespace BattlePet{struct PetBattleInput{uint32 Round=0,MoveType=0,AbilityID=1;int NewFrontPet=0;bool IgnoreAbandonPenalty=false;};}}
struct WorldSession:Session{Player*_player;void HandlePetBattleInput(WorldPackets::BattlePet::PetBattleInput&);};
enum class LFBAnswer{LFB_ANSWER_PENDING,LFB_ANSWER_AGREE,LFB_ANSWER_DENY};struct LFBTicket{uint32 Weight=1;LFBAnswer ProposalAnswer=LFBAnswer::LFB_ANSWER_AGREE;};
#include "Selected.inc"
static void Fill(PetBattle&b,Player&p){players[0]=&p;b.Teams[0]->PlayerGuid=p.guid;for(int t=0;t<2;++t){auto pet=std::make_shared<BattlePetInstance>();pet->ID=t*3;b.Pets[t*3]=pet;b.Teams[t]->TeamPets[0]=pet;b.Teams[t]->TeamPetCount=1;b.FightedPets[t*3]=true;}p.pets[0]=b.Pets[0];}
int main(int argc,char**argv){try{Require(argc==2,"case");int test=std::stoi(argv[1]);Player p;PetBattle b;Fill(b,p);auto pet=b.Pets[0];
switch(test){
case 0:b.Finish(1,true,false);Require(pet->Health==90,"Ordinary forfeit applies health penalty");break;
case 1:{Creature c;wild=&c;b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_WILD;b.Pets[3]->OriginalCreature=100;b.Finish(0,false,false);Require(c.released==1&&wilds.leaves==1&&!c._petBattleId,"Actual wild GUID is released exactly once");break;}
case 2:b.Finish(0,false,false);{auto xp=pet->XP;auto finishes=scripts.finishes;b.Finish(0,false,false);Require(pet->XP==xp&&scripts.finishes==finishes,"Repeated finish cannot replay XP or callbacks");}break;
case 3:b.Teams[0]->locked=true;b.Finish(0,false,false);Require(pet->Level==2,"A temporary swap lock cannot cancel earned experience");break;
case 4:pet->earned=1000;b.Finish(0,false,false);Require(pet->Level==5&&pet->XP==0&&p.levels==4,"All crossed native thresholds trigger their level side effects");{PetBattle cap;Fill(cap,p);auto c=cap.Pets[0];c->Level=24;c->XP=2399;c->earned=1000;cap.Finish(0,false,false);Require(c->Level==25&&c->XP==0,"Maximum level discards remainder and stops leveling");}break;
case 5:{systemFixture.b=&b;WorldSession s;s._player=&p;WorldPackets::BattlePet::PetBattleInput in;s.HandlePetBattleInput(in);Require(systemFixture.forfeits==1,"MoveType zero must reach its leave switch arm");break;}
case 6:{LFBTicket a,b;Require(!QueueBefore(&a,&a)&&!QueueBefore(&a,&b)&&!QueueBefore(&b,&a),"Equal-weight ordering must be irreflexive");break;}
case 7:{LFBTicket a,b;NewConsent(&a,&b);Require(a.ProposalAnswer==LFBAnswer::LFB_ANSWER_PENDING&&b.ProposalAnswer==LFBAnswer::LFB_ANSWER_PENDING,"A fresh proposal requires fresh answers from both sides");break;}
case 8:{PetBattleSystem requests;requests.CreateRequest(1);requests.CreateRequest(1);Require(PetBattleRequest::alive==1,"Replacing a pending request must release its earlier allocation");break;}
case 9:{Player other;other.guid=2;other._petBattleId.Clear();p._petBattleId.Clear();players[1]=&other;p.range=false;PetBattleRequest request;request.OpponentGuid=other.guid;auto result=systemFixture.CanPlayerEnterInPetBattle(&p,&request);Require(result==PETBATTLE_REQUEST_TARGET_OUT_OF_RANGE,"Admission must measure the actual two participants");p.range=true;p.los=false;Require(systemFixture.CanPlayerEnterInPetBattle(&p,&request)==PETBATTLE_REQUEST_NOT_HERE_OBSTRUCTED,"Admission must test actual participant LOS");p.los=true;Require(systemFixture.CanPlayerEnterInPetBattle(&p,&request)==PETBATTLE_REQUEST_OK,"Eligible nearby visible participants remain admitted");break;}
default:throw std::runtime_error("unknown case");}
std::cout<<"PASS complete production methods or explicitly identified queue projections; actual native linkage/client traversal separate\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

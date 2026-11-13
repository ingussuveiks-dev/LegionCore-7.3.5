// Exact production arrival class with actual Player/PetBattle/Begin and 26972
// stores. Only the clock is controlled; private journals stay in memory.
#define main RetainedFinalHealthMain
#include "PetBattleFinalHealthNative.cpp"
#undef main
#include "PetBattleSystem.h"
static uint32 arrivalClock;
static uint32 ArrivalClock() { return arrivalClock; }
#define getMSTime ArrivalClock
#define PlayerScriptPetBattle NativeArrivalScript
#include "PetBattleArrivalScript.inc"
#undef PlayerScriptPetBattle
#undef getMSTime
int main(int argc,char** argv) { try {
    InitializeAltheaNative(argc,argv);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    auto& script=*new NativeArrivalScript; // Native registry owns its scripts.
    sScriptMgr->SwapScriptContext(true);
    MapInstanced parent(1600,300); InstanceMap map(1600,300,9999831,DIFFICULTY_N_SCENARIO,&parent); auto session=Session();uint32 count=0;
    { NativePlayer player(session,map,28302,{52,803,805,0});player.setFactionForRace(RACE_HUMAN);
      for(uint32 team=0;team<2;++team)for(uint32 mode=0;mode<3;++mode) {
        auto b=sPetBattleSystem->CreateBattle();HealthFill(*b,player,team);b->BattleStatus=PETBATTLE_STATUS_CREATION;b->Turn=0;player._petBattleId=b->ID;
        arrivalClock=mode==2?0xffffff00u:100u;uint32 start=arrivalClock;
        script.OnMovementInform(&player,POINT_MOTION_TYPE,PETBATTLE_ENTER_MOVE_SPLINE_ID);
        script.OnUpdate(&player,0);Require(b->BattleStatus==PETBATTLE_STATUS_CREATION&&b->Turn==0,"Actual Begin must wait for the original arrival delay");
        arrivalClock=start+999;script.OnUpdate(&player,0);Require(b->BattleStatus==PETBATTLE_STATUS_CREATION,"Actual Begin must not run at999ms");
        arrivalClock=start+(mode==1?5000u:1000u);script.OnUpdate(&player,0);
        Require(b->BattleStatus==PETBATTLE_STATUS_RUNNING&&b->Turn==1&&script.m_DelayedPetBattleStart.empty(),"Actual native arrival must reach one initialized running battle");
        Require(player.HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_NPC)&&player.HasUnitState(UNIT_STATE_ROOT),"Actual native arrival must retain root and combat protection");
        script.OnMovementInform(&player,POINT_MOTION_TYPE,PETBATTLE_ENTER_MOVE_SPLINE_ID);script.OnUpdate(&player,0);
        Require(b->Turn==1&&script.m_DelayedPetBattleStart.empty(),"Repeated actual arrival cannot reset the running round");
        player._petBattleId.Clear();player.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_NPC);player.SetControlled(false,UNIT_STATE_ROOT);sPetBattleSystem->RemoveBattle(b->ID);++count;
      }
    }
    session->SetPlayer(nullptr);delete session;CloseAltheaNative();Require(count==6,"Native arrival inventory incomplete");
    std::cout<<"PASS actual native arrival cases=6; both teams,exact/late/wrapped deadlines,real Begin/root/flags/round and repeated notification; no persistent journal/account writes\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;} }

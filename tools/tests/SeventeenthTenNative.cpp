#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SeventeenthNativeScripts.inc"
using namespace SeventeenthNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);
dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999776,DIFFICULTY_N_SCENARIO,&parent);Actor a(&map,299,10621,{52,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&a)),"Actor insertion");
#define CHECK(T,F,V) {Require(a.AIM_Initialize(new N##T(&a)),"Native AI initialization");auto ai=static_cast<N##T*>(a.AI());Require(ai->F==V,"Actual native InitializeAI dispatch selected production reset");ai->F=0xCDCDCDCD;ai->Reset();Require(ai->F==V,"Native repeated reset");}
CHECK(KarshTimer,CheckTimer,2000);CHECK(IvanyrTimer,CheckTimer,1000);CHECK(MillificentTimer,CheckTimer,2000);CHECK(ElderTimers,CheckTimer,2000);CHECK(ElderTimers,EvadeTimer,5000);CHECK(NightmotherTimer,jumpTimer,0);CHECK(ChampionTimer,uiDefendTimer,10000);CHECK(BlackHoleState,NeedForAHack,0);CHECK(VortexState,bStart,0);CHECK(VortexState,startTimer,2000);CHECK(FocusState,active,1);CHECK(SapperTimer,moveTimer,0);
CloseAltheaNative();std::cout<<"PASS native ten selected cases:actual native actor and InitializeAI/Reset dispatch with extracted production state writes;12 fields across10 controllers. Full production Reset side effects,UpdateAI,combat,spell/death/credit/client paths not executed;no character writes/full-core ASAN\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}

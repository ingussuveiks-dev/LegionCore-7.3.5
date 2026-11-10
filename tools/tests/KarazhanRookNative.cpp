// Complete production AI, native spline finalization, spell hit and area triggers.
// Controlled actor poses/pulses; no client, damage recipient or character saves.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "AreaTrigger.h"
#include "AreaTriggerData.h"
#include "../../src/server/scripts/Legion/ReturnToKarazhan/return_to_karazhan.h"
#include "ChessConstants.inc"
namespace RookUnderTest {
#include "ChessAI.inc"
}
struct RookSplineUpdateTag{using Type=void(Unit::*)(uint32);friend Type Access(RookSplineUpdateTag);};
template struct FixtureAccess<RookSplineUpdateTag,&Unit::UpdateSplineMovement>;
static void FinishRook(Creature& c){(c.*Access(RookSplineUpdateTag{}))(10000);c.GetMotionMaster()->UpdateMotion(10000);c.GetMap()->MoveAllCreaturesInMoveList();c.AI()->UpdateAI(1);}
static void RookCase(InstanceMap& map,Position const& position,uint32 mode,uint32 guid){
    Actor rook(&map,115407,guid,position);
    Require(map.AddToMap(static_cast<Creature*>(&rook)),"Native rook insertion");
    Require(rook.AIM_Initialize(new RookUnderTest::npc_karazhan_chessAI(&rook)),"Install complete source AI");
    auto pulse=sSpellMgr->GetSpellInfo(229468);Require(pulse,"Native move spell missing");
    auto start=rook.GetPosition();rook.AI()->SpellHit(&rook,pulse);
    Require(!rook.movespline->Finalized()&&rook.GetMotionMaster()->GetCurrentMovementGeneratorType()==EFFECT_MOTION_TYPE,"Native rook jump not installed");
    auto id=rook.movespline->GetId();rook.AI()->SpellHit(&rook,pulse);Require(rook.movespline->GetId()==id,"Repeated native pulse replaced movement");
    if(mode==1)rook.GetMotionMaster()->Clear();
    if(mode==2)rook.AI()->Reset();
    if(mode==3)rook.Unit::setDeathState(CORPSE);
    if(mode==4)Require(rook.AIM_Initialize(new NullCreatureAI(&rook)),"Actual AI replacement");
    FinishRook(rook);
    if(!mode||mode==5){
        Require(rook.movespline->Finalized()&&rook.GetExactDist(&start)>1.0f,"Actual native completed movement");
        Require(rook.GetExactDist(&rook.GetHomePosition())<0.1f,"Native rook home must use committed landing pose");
        Require(rook.GetCurrentSpell(CURRENT_GENERIC_SPELL)&&rook.GetCurrentSpell(CURRENT_GENERIC_SPELL)->GetSpellInfo()->Id==229567,"Arrival did not start native500ms Rook Move");
        if(mode==5)rook.InterruptNonMeleeSpells(false,229567);
    }
    rook.m_Events.Update(1);rook.m_Events.Update(600);rook.m_Events.Update(1);
    std::list<AreaTrigger*> tiles;rook.GetAreaObjectList(tiles,229570);
    Require(tiles.size()==(mode?0u:12u),"Native success/cancellation cardinal fields differ");
    for(auto tile:tiles)Require(tile->GetDuration()==2000,"Native warning field duration changed");
    std::set<std::pair<int,int>> cells;
    for(auto tile:tiles){
        int dx=int(std::lround((tile->GetPositionX()-rook.GetPositionX())/5.9f));
        int dy=int(std::lround((tile->GetPositionY()-rook.GetPositionY())/5.9f));
        Require((!dx||!dy)&&cells.emplace(dx,dy).second,"Native field destination duplicated/non-cardinal");
    }
    if(!mode){
        auto count=tiles.size();rook.AI()->SpellHit(&rook,sSpellMgr->GetSpellInfo(229567));
        tiles.clear();rook.GetAreaObjectList(tiles,229570);Require(tiles.size()==count,"Native repeated hit duplicated fields");
    }
    if(mode==5){
        rook.AI()->SpellHit(&rook,pulse);FinishRook(rook);
        rook.m_Events.Update(1);rook.m_Events.Update(600);rook.m_Events.Update(1);
        tiles.clear();rook.GetAreaObjectList(tiles,229570);Require(tiles.size()==12,"Native interrupted cast retry blocked");
    }
    rook.AI()->Reset();rook.InterruptNonMeleeSpells(true);rook.GetMotionMaster()->Clear();
    rook.RemoveAllAreaObjects();map.RemoveAllObjectsInRemoveList();
    map.RemoveFromMap(static_cast<Creature*>(&rook),false);
}
int main(int argc,char** argv){try{
    InitializeAltheaGuardsNative(argc,argv);
    sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1651,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1651,{}}});
    MapInstanced parent(1651,300);InstanceMap map(1651,300,9999771,DIFFICULTY_MYTHIC_DUNGEON,&parent);
    uint32 guid=19401;
    for(int x=0;x<7;++x)for(int y=0;y<6;++y)RookCase(map,{3903.55f+x*5.9f,-2254.64f+y*5.9f,779.351f,0},0,guid++);
    for(uint32 mode=1;mode<=5;++mode)RookCase(map,{3915.8f,-2236.9f,779.351f,0},mode,guid++);
    std::cout<<"PASS native complete rook AI:42 board poses including cross-cell relocation; native spline/500ms cast/12 fields with2000ms duration,duplicate hits,interrupted/reset/dead/replaced AI/failed cast controls. Client combat and floor/rendering separate\n";
    CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

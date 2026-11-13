// Whole production script, actual map departure dispatch and real root auras.
// Controlled accepted-quest/ceremony state; no client traversal or DB writes.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "GossipDef.h"
#include "BlessingProduction.inc"

static uint32 departureCases;
static void DepartureCase(Map& map, Player& player, Player& other, uint32 mode)
{
    player.CancelQuestObjects(39597);
    auto kind=mode/2;
    Actor actor(&map,96257,41000+departureCases,{5478,470,150,0});
    Actor foreign(&map,96257,42000+departureCases,{5479,470,150,0});
    Actor third(&map,96257,43000+departureCases,{5480,470,150,0});
    Require(map.AddToMap(static_cast<Creature*>(&actor)) && map.AddToMap(static_cast<Creature*>(&foreign)) && map.AddToMap(static_cast<Creature*>(&third)),"Native ceremony actors missing");
    auto ai=new npc_stormheim_blessing_actor(&actor);
    Require(actor.AIM_Initialize(ai),"Whole production ceremony AI missing");
    player.SetQuestStatus(39597,QUEST_STATUS_INCOMPLETE);
    player.getQuestStatus(39597)->ScriptLifetime=std::make_shared<uint8>();
    player.BindQuestObject(39597,&actor);
    ai->scoped=true;ai->owner=player.GetGUID();
    ai->token=player.getQuestStatus(39597)->ScriptLifetime;
    ai->scope=player.getQuestStatus(39597)->ScriptObjects;
    auto scope=ai->scope.lock();
    StormheimBlessing::ceremonies[player.GetGUID()]=actor.GetGUID();
    StormheimBlessing::offers[player.GetGUID()]={actor.GetGUID(),ai->token};
    auto spell=mode%2 ? 190905u : 190904u;
    // Native190905 replaces a second copy on the same recipient. Keep an
    // independent190904 root as the real foreign-caster preservation control.
    Require(actor.AddAura(spell,&player) && foreign.AddAura(190904,&player),"Actual native different-caster roots missing");
    Require(third.AddAura(190904,&other),"Other participant root missing");
    Require(player.GetAura(spell,actor.GetGUID()) && player.GetAura(190904,foreign.GetGUID()),"Both real native caster identities must exist before departure");
    if(kind==2)ai->scoped=false;
    if(kind==3)StormheimBlessing::ceremonies.erase(player.GetGUID());
    npc_stormheim_blessing_actor* otherAI=nullptr;
    if(kind==4)
    {
        otherAI=new npc_stormheim_blessing_actor(&foreign);
        Require(foreign.AIM_Initialize(otherAI),"Actual foreign-owner ceremony AI missing");
        otherAI->scoped=true;otherAI->owner=other.GetGUID();
        StormheimBlessing::ceremonies[player.GetGUID()]=foreign.GetGUID();
    }
    if(kind==0)map.RemovePlayerFromMap(&player,false);
    else sScriptMgr->OnPlayerLeaveMap(&map,&player);
    if(kind<2)
    {
        Require(ai->stopped && scope->Cancelled && !player.GetAura(spell,actor.GetGUID()),"Map departure must stop the actual ceremony and remove its root before detaching the owner");
        Require(!StormheimBlessing::offers.count(player.GetGUID()),"Map departure must discard the readiness offer");
        sScriptMgr->OnPlayerLeaveMap(&map,&player);
        Require(!player.GetAura(spell,actor.GetGUID()),"Repeated departure must remain idempotent");
    }
    else Require(player.GetAura(spell,actor.GetGUID()),"Missing/stale ceremony records cannot remove an unrelated root");
    Require(!otherAI || !otherAI->stopped,"A stale registry cannot stop another player's ceremony");
    if(kind!=0)map.RemovePlayerFromMap(&player,false);
    Require(player.GetAura(190904,foreign.GetGUID()) && other.GetAura(190904,third.GetGUID()),"Departure must preserve other casters and other players");
    Require(!player.GetQuestObjectiveData(39597,96285),"Departure cannot award ceremony completion");
    // Simulate the actual detached interval: an old actor's late cleanup must
    // not reacquire an owner through a global lookup or touch the other aura.
    ai->Cleanup();
    Require(player.GetAura(190904,foreign.GetGUID()),"Late detached cleanup cannot remove a different caster's root");
    map.AddToGrid(&player,Cell(Trinity::ComputeCellCoord(player.GetPositionX(),player.GetPositionY())));
    player.Unit::AddToWorld();
    player.RemoveAllAuras();other.RemoveAllAuras();
    StormheimBlessing::ceremonies.erase(player.GetGUID());StormheimBlessing::offers.erase(player.GetGUID());
    ai->scoped=false;player.m_Events.KillAllEvents(true);other.m_Events.KillAllEvents(true);
    ++departureCases;
}

int main(int argc,char** argv)
{
    try
    {
        InitializeAltheaNative(argc,argv);
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}}});
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_stormheim_blessing();sScriptMgr->SwapScriptContext(true);
        Map map(1220,300,0,DIFFICULTY_NONE);
        auto session=Session(),otherSession=Session();
        {
            NativePlayer player(session,map,28311,{5478,470,150,0});
            NativePlayer other(otherSession,map,28312,{5479,470,150,0});
            for(uint32 mode=0;mode<10;++mode)DepartureCase(map,player,other,mode);
        }
        session->SetPlayer(nullptr);otherSession->SetPlayer(nullptr);delete session;delete otherSession;
        CloseAltheaNative();Require(departureCases==10,"Departure inventory incomplete");
        std::cout<<"PASS actual native Stormheim departure cases="<<departureCases<<";map hook/root casters/idempotence/stale records/no quest credit;readonly schemas\n";
        return 0;
    }
    catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}

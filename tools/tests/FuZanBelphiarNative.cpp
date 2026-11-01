// Actual native attack admission, scaled damage, death, original SmartAI
// rescue/door prerequisites,52443 and manual maiden summon. No character save.
#define FUZAN_TEMPLE_NATIVE_SUPPORT_ONLY
#include "FuZanTempleIntroNative.cpp"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_belphiar.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_sanctuary_door.h"

struct BelphiarDataTag{using Type=CreatureData const* Creature::*;friend Type Access(BelphiarDataTag);};
template struct FixtureAccess<BelphiarDataTag,&Creature::m_creatureData>;
struct BelphiarDisabledTag{using Type=UnitAI* Unit::*;friend Type Access(BelphiarDisabledTag);};
template struct FixtureAccess<BelphiarDisabledTag,&Unit::i_disabledAI>;
struct BelphiarDoorTag{using Type=uint64 GameObject::*;friend Type Access(BelphiarDoorTag);};
template struct FixtureAccess<BelphiarDoorTag,&GameObject::m_DBTableGuid>;
static Position const bossPosition{854.562f,-2465.03f,175.044f,4.39329f};
static Position const doorPosition{917.288f,-2496.43f,180.524f,2.83113f};
static uint32 actorGuid=10000;
static uint32 level98XP=0;
struct BelphiarActor : Creature
{
    CreatureData privateSpawn;
    BelphiarActor(Map& map,uint32 entry,uint32 db,Position const& position,uint32 flags=0,ReactStates reaction=REACT_AGGRESSIVE)
    {
        Require(Create(++actorGuid,&map,1,entry,0,ALLIANCE,position.GetPositionX(),position.GetPositionY(),position.GetPositionZ(),position.GetOrientation()),"Native Belphiar actor creation failed");
        this->*Access(IntroDbGuidTag{})=db;
        privateSpawn.id=entry;privateSpawn.guid=db;privateSpawn.mapid=1625;privateSpawn.dbData=false;
        this->*Access(BelphiarDataTag{})=&privateSpawn; // Native source identity, explicitly nonpersistent fixture.
        SetHomePosition(position);SetFlag(UNIT_FIELD_FLAGS,flags);SetReactState(reaction);
        Require(map.AddToMap(static_cast<Creature*>(this)),"Native actor map admission failed");
        Require(dynamic_cast<SmartAI*>(AI()),"Actual native SmartAI factory missing");
    }
    ~BelphiarActor()
    {
        CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);
        if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);
        ResetMap();
    }
};
struct BelphiarGuardian : Guardian
{
    BelphiarGuardian(LootPlayer& owner,Map& map) : Guardian(nullptr,&owner,false)
    {
        auto p=owner.GetPosition();
        Require(Create(++actorGuid,&map,1,299,0,ALLIANCE,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ(),0.f),"Native guardian creation failed");
        SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());setFaction(owner.getFaction());
        SetLevel(owner.getLevel());SetMaxHealth(1000000);SetHealth(1000000);
        Require(map.AddToMap(static_cast<Creature*>(this)),"Native guardian map admission failed");
        Require(GetCharmerOrOwnerPlayerOrPlayerItself()==&owner,"Actual guardian owner resolution missing");
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)),"Native idle guardian AI failed");
    }
    ~BelphiarGuardian()
    {
        CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);
        if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);
        ResetMap();
    }
};
struct BelphiarDoor : GameObject
{
    ~BelphiarDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
static uint64 BelphiarCounter(Scenario* scenario,uint32 id)
{
    auto progress=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto it=progress->find(id);
    return it==progress->end()?0:it->second.Counter;
}
static std::list<Creature*> Maidens(Creature& source)
{
    std::list<Creature*> result;source.GetCreatureListWithEntryInGrid(result,109355,100.f);return result;
}
static void LoadBelphiarLoot()
{
    auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(49140,49356)");
    Require(rules&&!rules->Fetch()[0].GetUInt32(),"Additional actual boss/maiden criteria World rules must be loaded");
    sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
    auto& name=LootTemplates_Creature.*Access(LootTableNameTag{});auto original=name;
    name+=" WHERE Entry IN(109395,109397) ORDER BY Entry,Item";auto count=(LootTemplates_Creature.*Access(LoadLootTableTag{}))();name=original;
    Require(count==3&&LootTemplates_Creature.HaveLootFor(109395)&&LootTemplates_Creature.HaveLootFor(109397),"Actual native guard/Belphiar loot rows missing");
    Require(sXpGameTable.GetRow(98),"Matching native XP98 record missing");level98XP=uint32(sXpGameTable.GetRow(98)->Total);
    if(auto xp=WorldDatabase.Query("SELECT Experience FROM player_xp_for_level WHERE Level=98"))level98XP=xp->Fetch()[0].GetUInt32();
    Require(level98XP,"Actual XP98 threshold missing");
}
static void NativeRescues(Map& map,LootPlayer& player,Unit& killer)
{
    BelphiarActor scribe0(map,109393,370386,{866.043f,-2441.94f,192.647f,4.24205f});
    BelphiarActor scribe1(map,109393,370387,{830.497f,-2477.61f,192.631f,2.89358f});
    BelphiarActor scribe2(map,109393,370388,{863.568f,-2491.88f,192.629f,5.46246f});
    uint32 n=0;
    for(auto data:{std::make_pair(109394u,370389u),std::make_pair(109395u,370390u),std::make_pair(109395u,370391u)})
    {
        Position p=n==0?Position{826.257f,-2482.77f,192.636f,0.849705f}:n==1?
            Position{862.188f,-2448.71f,192.644f,1.07145f}:Position{868.684f,-2498.49f,192.634f,2.08828f};
        player.Relocate(p);killer.Relocate(p);
        BelphiarActor guard(map,data.first,data.second,p);
        Require(player.IsValidAttackTarget(&guard),"Original guard attack admission changed");
        killer.DealDamage(&guard,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(!guard.IsAlive(),"Actual native one-hit guard kill failed");++n;
    }
}
static void NativeDoor(Map& map,LootPlayer& player)
{
    player.Relocate(doorPosition);BelphiarDoor door;
    Require(door.Create(++actorGuid,251591,&map,1,doorPosition,G3D::Quat(0,0,0,1),255,GO_STATE_READY),"Native prerequisite door creation failed");
    door.*Access(BelphiarDoorTag{})=138378;Require(map.AddToMap(static_cast<GameObject*>(&door)),"Native prerequisite door map admission failed");
    door.Update(5000);Require(player.CastSpell(&door,218174,false)==SPELL_CAST_OK,"Actual native prerequisite Ox Kick failed");
    for(uint32 t=0;t<1000;t+=10)player.Unit::Update(10);
    Require(FuZanTempleDoor::Opened(sScenarioMgr->GetScenario(map.GetInstanceId())),"Actual native prerequisite door49141 missing");
}
#ifndef FUZAN_BELPHIAR_NATIVE_SUPPORT_ONLY
static void NativeMaidenSequence(Map& map,LootPlayer& player,Creature& boss,Scenario* scenario)
{
    BelphiarActor king(map,109610,373384,{826.786f,-2549.49f,183.12f,1.2733f});
    bool waypointReached=false;
    for(uint32 elapsed=0;elapsed<16000;elapsed+=10)
    {
        player.Unit::Update(10);king.Unit::Update(10);king.AI()->UpdateAI(10);
        for(auto maiden:Maidens(boss))
        {
            maiden->Unit::Update(10);
            if(maiden->IsInWorld()&&maiden->IsAIEnabled)maiden->AI()->UpdateAI(10);
            if(maiden->GetExactDist(850.834f,-2482.59f,176.315f)<0.5f)waypointReached=true;
        }
        map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();
    }
    auto maidens=Maidens(boss);
    Require(scenario->GetCurrentStep()==4&&BelphiarCounter(scenario,49356)==1&&maidens.size()==1,
        "Actual native maiden three text timers did not acknowledge52609/49356 once");
    Require(waypointReached&&maidens.front()->GetMotionMaster()->GetCurrentMovementGeneratorType()==FOLLOW_MOTION_TYPE&&
        dynamic_cast<SmartAI*>(king.AI())->GetScript()->HasRunEvent(0,SMART_EVENT_DATA_SET),
        "Actual maiden waypoint/follow/Monkey King data transition missing");
}
static void CheckBelphiar(uint8 race,uint32 kind,uint32 mode,uint32 number)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999800+number,DIFFICULTY_N_SCENARIO,&parent);
    map.*Access(IntroInstanceTag{})=new InstanceScript(&map);
    Scenario* scenario=mode==5?nullptr:sScenarioMgr->AddScenario(&map,1137);
    if(scenario)scenario->SetCurrentStep(mode==0||mode==15?2:mode==2?0:mode==4?3:1);
    auto session=new WorldSession(0,"Fu Zan native Belphiar fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,3000+number,bossPosition);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);
        player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        if(mode==14){player.SetLevel(98);player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP,level98XP);}
        std::unique_ptr<LootPet> pet;std::unique_ptr<BelphiarGuardian> guardian;
        if(kind==1){pet=std::make_unique<LootPet>(player,map);pet->setFaction(player.getFaction());pet->SetLevel(player.getLevel());}
        if(kind==2)guardian=std::make_unique<BelphiarGuardian>(player,map);
        Unit& killer=kind==1?static_cast<Unit&>(*pet):kind==2?static_cast<Unit&>(*guardian):static_cast<Unit&>(player);
        BelphiarActor boss(map,109397,mode==6?373406:373405,bossPosition,
            mode==3?UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE:0,mode==3?REACT_DEFENSIVE:REACT_AGGRESSIVE);
        auto smart=dynamic_cast<SmartAI*>(boss.AI());Require(smart,"Native boss SmartAI missing");
#ifdef FUZAN_BELPHIAR_BASELINE
        Require(!dynamic_cast<npc_fuzan_belphiar*>(boss.AI()),"Counterfactual still uses repaired boss factory");
        Require(player.IsValidAttackTarget(&boss),"Original native boss unexpectedly protected");
        killer.DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(!boss.IsAlive()&&Maidens(boss).size()==1,"Original native one-hit death/summon not reproduced");
        if(mode==1){Require(scenario->GetCurrentStep()==1&&!BelphiarCounter(scenario,49140),"Future-stage loss not reproduced");std::cout<<"EXPECTED_BROKEN early native Belphiar death consumes52443 without49140, summons maiden while stage1 remains; race="<<unsigned(race)<<" killer="<<kind<<'\n';}
        else Require(scenario->GetCurrentStep()==3&&BelphiarCounter(scenario,49140)==1,"Original active-stage positive control failed");
#else
        Require(dynamic_cast<npc_fuzan_belphiar*>(boss.AI()),"Actual repaired factory missing");
        if(mode==6)
        {
            Require(player.IsValidAttackTarget(&boss),"Other-source native attack admission changed");
            killer.DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
            Require(!boss.IsAlive()&&!BelphiarCounter(scenario,49140)&&Maidens(boss).size()==1,"Other-source original native death behavior changed");
        }
        else
        {
            if(mode!=0&&mode!=15)
            {
                Require(!player.IsValidAttackTarget(&boss)&&boss.GetReactState()==REACT_PASSIVE,"Pre-stage native attack admission not protected");
                Require(!boss.AI()->CanAIAttack(&player),"Pre-stage native AI attack admission not protected");
                boss.AI()->AttackStart(&player);
                Require(!boss.getVictim()&&!boss.isInCombat(),"Rejected native AttackStart entered combat");
                if(mode==7)boss.IsAIEnabled=false;
                CreatureAI* original=nullptr;
                if(mode==8||mode==9)
                {
                    original=boss.AI();boss.SetDisabledCurrentAI();
                    if(mode==8)boss.SetAI(new NullCreatureAI(&boss));
                    else boss.IsAIEnabled=false; // Native no-current-AI invariant, as in AIM_Destroy.
                    boss.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE); // Explicit lost-flag fault; final native fallback must protect.
                }
                if(mode==11)player.SetQuestStatus(42765,QUEST_STATUS_NONE);
                uint64 health=boss.GetHealth();
                killer.DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
                Require(boss.IsAlive()&&boss.GetHealth()==health&&Maidens(boss).empty()&&
                    !smart->GetScript()->HasRunEvent(4,SMART_EVENT_DEATH),"Rejected native scaled hit consumed health/death/root/summon");
                if(original){delete boss.AI();boss.SetAI(original);boss.*Access(BelphiarDisabledTag{})=nullptr;}
                if(mode==7||mode==9)boss.IsAIEnabled=true;
                if(mode==11)player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);
                if(mode==12)for(uint32 n=0;n<10;++n)boss.AI()->Reset();
                if(mode==13)
                {
                    Require(boss.AIM_Destroy(),"Actual native AI destruction refused");
                    Require(!boss.HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE)&&boss.GetReactState()==REACT_AGGRESSIVE,"Destroyed AI retained owned gate");
                    Require(boss.AIM_Initialize(),"Actual native factory recreation failed");smart=dynamic_cast<SmartAI*>(boss.AI());
                    Require(!player.IsValidAttackTarget(&boss),"Recreated native AI lost closed gate");
                }
                if(!scenario){scenario=sScenarioMgr->AddScenario(&map,1137);Require(scenario,"Late native scenario missing");}
                scenario->SetCurrentStep(1);
                // Both actual prerequisite orders, rather than directly
                // seeding49140 or setting the boss's active stage.
                if(mode%2)NativeDoor(map,player);
                NativeRescues(map,player,killer);
                Require(BelphiarCounter(scenario,49138)==3,"Actual native three rescues missing");
                if(!(mode%2))NativeDoor(map,player);
                Require(scenario->GetCurrentStep()==2,"Actual native rescue AND door did not admit stage2");
                boss.AI()->UpdateAI(10);
                if(mode==3)
                {
                    Require((boss.GetUInt32Value(UNIT_FIELD_FLAGS)&(UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE))==
                        (UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE)&&boss.GetReactState()==REACT_DEFENSIVE,"Foreign flags/reaction changed");
                    boss.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE); // Original owner releases its fixture flags.
                }
            }
            player.Relocate(bossPosition);killer.Relocate(bossPosition);
            auto peerSession=mode==10?new WorldSession(0,"Fu Zan native eligible group fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{}):nullptr;
            std::unique_ptr<LootPlayer> peer;std::unique_ptr<ProbeParty> group;
            if(peerSession)
            {
                peer=std::make_unique<LootPlayer>(peerSession,map,6000+number,bossPosition);
                peer->SetByteValue(UNIT_FIELD_BYTES_0,0,race);peer->setFactionForRace(race);peer->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
                peer->SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);peer->SetQuestSlot(0,42765);group=std::make_unique<ProbeParty>(player,*peer);
            }
            Require(player.IsValidAttackTarget(&boss)&&killer.GetCharmerOrOwnerPlayerOrPlayerItself()==&player,"Native active-stage/owner admission failed");
            Require(boss.AI()->CanAIAttack(&player),"Native active-stage AI combat was not restored");
            if(mode==14)Require(boss.GetHealthMultiplierForTarget(&killer)!=1.f,"Native level-scaling branch not exercised");
            if(mode==15){uint64 health=boss.GetHealth();killer.DealDamage(&boss,50,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(boss.IsAlive()&&boss.GetHealth()<health,"Ordinary active-stage damage was changed");}
            killer.DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
            Require(!boss.IsAlive()&&scenario->GetCurrentStep()==3&&BelphiarCounter(scenario,49140)==1&&Maidens(boss).size()==1,"Native real death did not give one49140 and one maiden");
            boss.AI()->JustDied(&killer); // Explicit duplicated notification, after the actual native kill.
            Require(BelphiarCounter(scenario,49140)==1&&Maidens(boss).size()==1,"Repeated death replayed native credit/summon");
            if(mode==0)NativeMaidenSequence(map,player,boss,scenario);
            group.reset();peer.reset();if(peerSession)peerSession->SetPlayer(nullptr);
        }
#endif
        for(auto maiden:Maidens(boss))maiden->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
        std::cout<<"CASE Belphiar race="<<unsigned(race)<<" killer="<<kind<<" mode="<<mode<<" native admission/scaling/death/prerequisite205/cleanup PASS\n";
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true,true);LoadBelphiarLoot();HashMapHolder<GameObject>::SetSize(20000);
        auto info=const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(109397));Require(info,"Native Belphiar template missing");
        uint32 id=sObjectMgr->GetScriptId("npc_fuzan_belphiar");Require(id,"Native Belphiar descriptor missing");
#ifdef FUZAN_BELPHIAR_BASELINE
        Require(!info->ScriptID||info->ScriptID==id,"Counterfactual refuses custom script");info->ScriptID=0;
#elif defined(FUZAN_BELPHIAR_LIVE)
        Require(info->ScriptID==id,"Actual live Belphiar binding missing");
#else
        Require(!info->ScriptID,"Private pending binding refuses custom/existing script");info->ScriptID=id;
#endif
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        uint32 number=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 kind=0;kind<3;++kind)
#ifdef FUZAN_BELPHIAR_BASELINE
            for(uint32 mode:{0u,1u})CheckBelphiar(race,kind,mode,++number);
#else
            for(uint32 mode=0;mode<16;++mode)CheckBelphiar(race,kind,mode,++number);
#endif
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
#ifdef FUZAN_BELPHIAR_BASELINE
        std::cout<<"EXPECTED_BROKEN6 early real player/pet/guardian kills paired with6 actual active-stage positive controls; not an old whole binary/client claim.\n";
#else
        std::cout<<"PASS96 actual native Belphiar cases,2 races/3 player-pet-guardian killers, actual3 rescue deaths AND native Ox Kick door in both orders, scaled one-hit/ordinary damage, suspended/no-current-AI final fallback, owned cleanup/recreation and exact original52443/one maiden; six actual maiden text/waypoint/52609/King-data/follow continuations. Private earlier preconditions, no character save or full client/Monkey King route claim.\n";
#endif
        return 0;
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
#endif

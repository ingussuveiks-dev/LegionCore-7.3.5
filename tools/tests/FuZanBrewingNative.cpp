// Production AI/SpellScripts, native packet decoder and actual build26972 data.
// No authenticated player, saved character mutation or direct recipe credit.
#define FUZAN_BREWING_NATIVE
#define FUZAN_TEMPLE_NATIVE_SUPPORT_ONLY
#include "FuZanTempleIntroNative.cpp"
#include "GameObjectPackets.h"

struct BrewingGoGuidTag { using Type=uint64 GameObject::*;friend Type Access(BrewingGoGuidTag); };
template struct FixtureAccess<BrewingGoGuidTag,&GameObject::m_DBTableGuid>;
struct BrewPhaseTag {using Type=FuZanBrewing::Actor::Phase FuZanBrewing::Actor::*;friend Type Access(BrewPhaseTag);};
template struct FixtureAccess<BrewPhaseTag,&FuZanBrewing::Actor::_phase>;
struct BrewIssuedTag {using Type=uint64 FuZanBrewing::Actor::*;friend Type Access(BrewIssuedTag);};
template struct FixtureAccess<BrewIssuedTag,&FuZanBrewing::Actor::_issued>;
struct BrewCreditTag {using Type=uint64 FuZanBrewing::Actor::*;friend Type Access(BrewCreditTag);};
template struct FixtureAccess<BrewCreditTag,&FuZanBrewing::Actor::_credit>;
struct BrewOwnerDeathTag {using Type=DeathState Unit::*;friend Type Access(BrewOwnerDeathTag);};
template struct FixtureAccess<BrewOwnerDeathTag,&Unit::m_deathState>;
struct BrewingGo : GameObject
{
    ~BrewingGo(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
    void Load(uint32 spawn,Map& map)
    {
        auto row=WorldDatabase.PQuery("SELECT id,position_x,position_y,position_z,orientation FROM gameobject WHERE guid=%u",spawn);
        Require(bool(row),"Actual brewing GO spawn missing");auto f=row->Fetch();
        Require(Create(spawn,f[0].GetUInt32(),&map,1,Position(f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()),G3D::Quat(0,0,0,1),255,GO_STATE_READY),"Native brewing GO create failed");
        this->*Access(BrewingGoGuidTag{})=spawn;
        Require(map.AddToMap(static_cast<GameObject*>(this)),"Native brewing GO admission failed");Update(5000);
    }
};
static void Use(WorldSession* session,GameObject& source)
{
    WorldPacket wire(CMSG_GAME_OBJ_USE);wire<<source.GetGUID();WorldPackets::GameObject::GameObjectUse packet(std::move(wire));packet.Read();session->HandleGameObjectUse(packet);
}
static Quest* LoadBrewingQuest()
{
    auto row=WorldDatabase.Query(
#include "NativeBrewingQuest.inc"
    );Require(bool(row),"Actual brewing quest missing");auto quest=new Quest(row->Fetch());
    (sQuestDataStore->*Access(QuestVectorTag{}))[42868]=quest;const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[42868]=quest;
    auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=42868 ORDER BY StorageIndex");
    Require(bool(objectives),"Actual brewing objectives missing");do{quest->LoadQuestObjective(objectives->Fetch());}while(objectives->NextRow());
    Require(quest->GetObjectives().size()==2 && quest->GetObjectives()[0].ObjectID==251490 && quest->GetObjectives()[1].ObjectID==109191,"Native brewing objectives differ");
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST);return quest;
}
static void Pulse(LootPlayer& player,uint32 duration,LootPlayer* other=nullptr)
{
    auto map=player.GetMap();
    for(uint32 elapsed=0;elapsed<duration;elapsed+=10)
    {
        player.Unit::Update(10);
        if(other)other->Unit::Update(10);
        std::list<Creature*> actors;player.GetCreatureListWithEntryInGrid(actors,109224,100);
        for(auto actor:actors){actor->Unit::Update(10);if(actor->IsAIEnabled)actor->AI()->UpdateAI(10);}
        map->MoveAllCreaturesInMoveList();map->RemoveAllObjectsInRemoveList();
    }
}
static void Incomplete(LootPlayer& player,Quest* quest)
{
    player.SetQuestStatus(42868,QUEST_STATUS_INCOMPLETE);
    player.SetQuestObjectiveData(quest,&quest->GetObjectives()[0],0);
    player.SetQuestObjectiveData(quest,&quest->GetObjectives()[1],0);
    player.AddQuestDelayedEvent(42868,0,[]{});
}
static FuZanBrewing::Actor* Start(LootPlayer& player,WorldSession* session,BrewingGo& fire)
{
    player.Relocate(fire.GetPosition());Use(session,fire);auto actor=FuZanBrewing::Find(&player);
    Require(actor && actor->Current(&player),"Production retry did not begin");Pulse(player,11500);
    Require(player.HasAura(217245),"Production retry did not request first ingredient");return actor;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeIntro(argc,argv);
        // This chain procs on energize. Initialize the real proc vector/table
        // just as world startup does; an empty, uninitialized vector makes a
        // non-instrumented happy path undefined even when it appears to pass.
        sSpellMgr->LoadSpellProcEvents();
        HashMapHolder<Player>::SetSize(20000);HashMapHolder<GameObject>::SetSize(20000);
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1625,{}},{870,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1625,{}},{870,{}}});
        auto quest=LoadBrewingQuest();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
        for(auto pair:{std::make_pair(251490u,"go_fuzan_brewing_fire"),{251505u,"go_fuzan_brewing_ingredient"},{251501u,"go_fuzan_brewing_ingredient"},{251503u,"go_fuzan_brewing_ingredient"},{251510u,"go_fuzan_brewing_ingredient"}})
        {
            auto value=const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(pair.first));auto id=sObjectMgr->GetScriptId(pair.second);
            Require(value && value->AIName.empty(),"Brewing fixture refuses custom GO AI");
#ifdef FUZAN_BREWING_LIVE
            Require(value->ScriptId==id,"Actual live brewing GO registration differs");
#else
            Require(!value->ScriptId,"Private fixture refuses custom/partial GO binding");value->ScriptId=id;
#endif
        }
        for(uint8 race:{RACE_HUMAN,RACE_ORC})
        {
            Map map(870,300,9999700+race,DIFFICULTY_NONE);
            auto session=new WorldSession(0,"Fu Zan isolated native brewing",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
            {
                LootPlayer player(session,map,9000+race,{924.45f,-2409.51f,168.598f,1.57153f});
                player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
                player.m_zoneId=5785;player.m_areaId=5974;player.SetQuestStatus(42868,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42868);
                auto row=WorldDatabase.Query("SELECT id,position_x,position_y,position_z,orientation FROM creature WHERE guid=366254");Require(bool(row),"Actual public Monkey King missing");auto f=row->Fetch();
                auto original=new Creature();Require(original->Create(366254,&map,1,f[0].GetUInt32(),0,0,f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()),"Actual public king create failed");
                original->*Access(IntroDbGuidTag{})=366254;Require(map.AddToMap(original),"Actual public king admission failed");
                BrewingGo fire;fire.Load(128074,map);std::array<BrewingGo,4> ingredients;
                for(uint32 i=0;i<4;++i)ingredients[i].Load(FuZanBrewing::Recipe[i].spawn,map);
                Require(dynamic_cast<go_fuzan_brewing_fire*>(fire.AI()),"Native GO factory did not select production fire controller");
                fire.AI()->GossipHello(&player,false);Require(!player.HasAura(217302)&&player.GetQuestObjectiveData(quest,0)==0,"ReportUse issued a recipe start");
                auto denied=[&](){Use(session,fire);Require(!player.HasAura(217302)&&!player.GetQuestObjectiveData(quest,0)&&!player.GetQuestObjectiveData(quest,1),"Invalid native source consumed first/final credit");};
                player.Relocate(1200,-2409.51f,168.598f);denied();player.Relocate(fire.GetPosition());
                player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);denied();player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
                fire.SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);denied();fire.RemoveFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
                original->SetPhaseMask(2,false);denied();original->SetPhaseMask(1,false);
                Require(player.CastSpell(&player,217321,true)==SPELL_CAST_OK&&player.HasAura(217321),"Native existing alternate bar fixture failed");denied();player.RemoveAurasDueToSpell(217321);
                auto templateData=const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(109224));auto nativeScript=templateData->ScriptID;
                templateData->ScriptID=0;denied();templateData->ScriptID=nativeScript;map.RemoveAllObjectsInRemoveList();
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217321)&&!player.HasAura(217332),"Missing production actor did not roll back its own partial native main");
                auto otherSession=new WorldSession(0,"Fu Zan unscoped native delegate",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
                {
                    LootPlayer other(otherSession,map,9600+race,fire.GetPosition());
                    other.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);other.m_zoneId=5785;other.m_areaId=5974;
                    Require(other.CastSpell(&other,217297,true)==SPELL_CAST_OK,"Unscoped native main fixture failed");Pulse(other,6200);
                    Require(other.HasAura(217302)&&FuZanBrewing::Find(&other),"New binding changed an unrelated native controller/summon lifetime");
                    sScriptMgr->OnPlayerBeforeLogout(&other);sScriptMgr->OnPlayerLeaveMap(&map,&other);
                    Require(other.HasAura(217302)&&FuZanBrewing::Find(&other),"Lifecycle cleanup claimed an unrelated class's native controller");
                    original->CastSpell(&other,217245,true);other.CastSpell(&other,217247,true);
                    Require(other.HasAura(217245)&&other.HasAura(217247),"Unscoped native request fixture failed");
                    other.CastSpell(&other,217251,true);Pulse(other,200);
                    Require(!other.HasAura(217245)&&!other.HasAura(217247)&&other.GetPower(POWER_ALTERNATE)==0,"Unrelated native Added/Credit effects were blocked or awarded new progress");
                    other.RemoveAllAuras();map.RemoveAllObjectsInRemoveList();
                }
                otherSession->SetPlayer(nullptr);
                Require(player.CastSpell(&player,217304,true)==SPELL_CAST_OK,"Other-source owned summon fixture failed");
                auto rawState=FuZanBrewing::Find(&player);Require(rawState,"Other-source native summon factory fixture missing");
                auto rawCreature=map.GetCreature(rawState->Guid());rawCreature->SetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL,0);
                // Controlled other-source identity, native owner/list/type/AI:
                // the brew finder must ignore it and native Aura428 cleanup
                // must preserve it while the real recipe runs beside it.
                Require(!FuZanBrewing::Find(&player),"Brew finder claimed another spell's summon");
                sScriptMgr->OnPlayerLeaveMap(&map,&player);Require(rawCreature->IsInWorld(),"Map leave removed an unrelated raw summon");
                Use(session,fire);auto actor=FuZanBrewing::Find(&player);
                Require(actor && actor->Current(&player) && player.GetQuestObjectiveData(quest,0)==1 && !player.GetQuestObjectiveData(quest,1),"Real fire packet failed production native start");
                Require(original->ShouldHideFor(player.GetGUID()),"Original king was not hidden for its owner");
                Pulse(player,11500);
                for(uint32 i=0;i<4;++i)
                {
                    std::cout<<"OBSERVATION production recipe request race="<<unsigned(race)<<" step="<<i<<" aura="<<player.HasAura(FuZanBrewing::Recipe[i].request)<<" meter="<<player.GetPower(POWER_ALTERNATE)<<'\n';
                    Require(player.HasAura(FuZanBrewing::Recipe[i].request),"Production movement/intro did not reach actual native request");
                    player.Relocate(ingredients[i].GetPosition());
                    Use(session,ingredients[i]);Use(session,ingredients[i]); // Same packet twice must not double-credit.
                    std::cout<<"OBSERVATION click phase="<<(actor->*Access(BrewPhaseTag{}))<<" issued="<<(actor->*Access(BrewIssuedTag{}))<<" pending="<<(actor->*Access(BrewCreditTag{}))<<" failure="<<player.HasAura(217247)<<'\n';
                    Pulse(player,1500);
                    std::cout<<"OBSERVATION delivery phase="<<(actor->*Access(BrewPhaseTag{}))<<" issued="<<(actor->*Access(BrewIssuedTag{}))<<" pending="<<(actor->*Access(BrewCreditTag{}))<<" failure="<<player.HasAura(217247)<<" request="<<player.HasAura(FuZanBrewing::Recipe[i].request)<<" power="<<player.GetPower(POWER_ALTERNATE)<<'\n';
                    Require(player.GetPower(POWER_ALTERNATE)==int32(i+1),"Actual ingredient packet/native delivery did not increment exactly once");
                    Require(player.GetQuestObjectiveData(quest,1)==(i==3?1:0),"Completion occurred outside real fourth native receipt");
                    if(i!=3)Pulse(player,3600);
                }
                Require(player.GetQuestStatus(42868)==QUEST_STATUS_COMPLETE,"Native four-unit completion proc did not finish quest");
                Pulse(player,12000);
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!player.HasAura(217321)&&!player.HasAura(217332)&&!original->ShouldHideFor(player.GetGUID()),"Production return/controller cleanup incomplete");
                Require(rawCreature->IsInWorld(),"Native recipe completion removed another source's owned summon");
                rawCreature->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
                std::cout<<"PASS production brewing race="<<unsigned(race)<<" actualCampfire=1 actualIngredients=4 nativeReceiptPower=4 nativeQuestReady=1 duplicateSafe=1 personalReturnCleanup=1\n";
                // All following changes concern only this unauthenticated,
                // never-saved in-memory fixture. No real character is reset.
                Incomplete(player,quest);
                player.Relocate(ingredients[0].GetPosition());Use(session,ingredients[0]);Pulse(player,50);
                Require(!player.HasAura(217302)&&player.GetPower(POWER_ALTERNATE)==0&&!player.GetQuestObjectiveData(quest,1),"Ingredient admitted without controller");
                player.Relocate(fire.GetPosition());Use(session,fire);auto stalled=FuZanBrewing::Find(&player);
                Require(stalled&&(stalled->*Access(BrewPhaseTag{}))==FuZanBrewing::Actor::Approach,"Native arrival deadline fixture failed");
                stalled->UpdateAI(15001);map.RemoveAllObjectsInRemoveList(); // Hold native movement; no arrival acknowledgement.
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!original->ShouldHideFor(player.GetGUID())&&player.GetQuestObjectiveData(quest,0)==1&&!player.GetQuestObjectiveData(quest,1),"Missing arrival receipt progressed or failed finite cleanup");
                Require(player.CastSpell(&player,217297,true)==SPELL_CAST_OK,"Native orphan reapplication endpoint failed");Pulse(player,600);
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!player.HasAura(217321)&&player.GetQuestObjectiveData(quest,0)==1&&!player.GetQuestObjectiveData(quest,1),"Orphan controller rebound a recipe or changed existing objectives");
                Start(player,session,fire);player.SetQuestStatus(42868,QUEST_STATUS_NONE);Pulse(player,600);
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!original->ShouldHideFor(player.GetGUID()),"Abandonment lease did not clean native personal state");
                Incomplete(player,quest);Start(player,session,fire);
                player.SetQuestStatus(42868,QUEST_STATUS_NONE);Incomplete(player,quest);Pulse(player,600);
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!original->ShouldHideFor(player.GetGUID())&&!player.GetQuestObjectiveData(quest,0)&&!player.GetQuestObjectiveData(quest,1),"Old controller rebound to a reaccepted quest lifetime");
                player.Relocate(fire.GetPosition());original->AddToHideList(player.GetGUID());
                actor=Start(player,session,fire); // Already-hidden public actor: this attempt owns no hide lease.
                player.RemoveAurasDueToSpell(217302);Pulse(player,50);
                Require(!FuZanBrewing::Find(&player)&&original->ShouldHideFor(player.GetGUID()),"Cleanup removed another owner's hide lease");
                original->RemoveFromHideList(player.GetGUID());
                actor=Start(player,session,fire);
                auto identity=actor->Guid();Use(session,fire);Require(FuZanBrewing::Find(&player)->Guid()==identity&&player.GetPower(POWER_ALTERNATE)==0,"Repeated fire reset active attempt");
                // Wrong first ingredient uses native lower-one and finite feedback.
                player.Relocate(ingredients[1].GetPosition());Use(session,ingredients[1]);Pulse(player,200);
                Require(player.GetPower(POWER_ALTERNATE)==0&&!player.HasAura(217245)&&!player.HasAura(217247)&&!player.GetQuestObjectiveData(quest,1),"Wrong ingredient advanced or retained old request");
                Pulse(player,6200);Require(player.HasAura(217245),"Wrong ingredient did not offer a finite retry");
                // Expired native failure window clears actual meter and retries.
                Pulse(player,9000);Require(player.GetPower(POWER_ALTERNATE)==0&&!player.GetQuestObjectiveData(quest,1),"Timeout granted progress");
                Pulse(player,6000);Require(player.HasAura(217245),"Timeout did not recover");
                player.Relocate(ingredients[0].GetPosition());
                auto unissued=player.CastSpell(&player,217213,true);Pulse(player,100);
                std::cout<<"OBSERVATION unissued native dummy result="<<unissued<<" (cast return is not a delivery acknowledgement)\n";
                Require(player.GetPower(POWER_ALTERNATE)==0&&player.HasAura(217245)&&player.HasAura(217247),"Unissued dummy progressed or consumed request");
                auto power=player.GetPower(POWER_ALTERNATE);
                Require(player.CastSpell(&player,217251,true)==SPELL_CAST_OK,"Native forged Added fixture did not execute");Pulse(player,200);
                Require(player.GetPower(POWER_ALTERNATE)==power&&player.HasAura(217245)&&player.HasAura(217247),"Unissued native Added removed current request or progressed");
                // A Credit already scheduled by a real Added must not clean a
                // reaccepted attempt or award its newer personal actor credit.
                Use(session,ingredients[0]);Pulse(player,10);
                Require((actor->*Access(BrewCreditTag{}))!=0,"Native delayed-credit race fixture did not reach pending state");
                player.RemoveAurasDueToSpell(217302);map.RemoveAllObjectsInRemoveList();
                player.SetQuestStatus(42868,QUEST_STATUS_NONE);Incomplete(player,quest);
                player.Relocate(fire.GetPosition());Use(session,fire);auto replacement=FuZanBrewing::Find(&player);
                Require(replacement&&replacement->Guid()!=identity,"Reaccepted quest reused old actor");Pulse(player,11500);
                Require(player.GetPower(POWER_ALTERNATE)==0&&player.HasAura(217245)&&!player.GetQuestObjectiveData(quest,1),"Late native delivery contaminated a new quest attempt");
                sScriptMgr->OnPlayerBeforeLogout(&player);map.RemoveAllObjectsInRemoveList();
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!original->ShouldHideFor(player.GetGUID()),"Native pre-save logout dispatch did not release personal state");
                Start(player,session,fire);
                sScriptMgr->OnPlayerLeaveMap(&map,&player);map.RemoveAllObjectsInRemoveList();
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!original->ShouldHideFor(player.GetGUID()),"Native old-map leave dispatch did not release personal state");
                Start(player,session,fire);
                // Two simultaneous owners share the real fire/surfaces. Keep
                // native effect90 personal credit, while their requests,
                // private actors and alternate-power bars remain independent.
                auto peerSession=new WorldSession(0,"Fu Zan isolated brewing group",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
                {
                    LootPlayer peer(peerSession,map,9500+race,fire.GetPosition());
                    peer.SetByteValue(UNIT_FIELD_BYTES_0,0,race);peer.setFactionForRace(race);peer.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
                    peer.m_zoneId=5785;peer.m_areaId=5974;Incomplete(peer,quest);peer.SetQuestSlot(0,42868);
                    FuZanBrewing::Leave(&player);map.RemoveAllObjectsInRemoveList();
                    ProbeParty party(player,peer);
                    player.Relocate(fire.GetPosition());Use(session,fire);Use(peerSession,fire);
                    auto ours=FuZanBrewing::Find(&player),theirs=FuZanBrewing::Find(&peer);
                    Require(ours&&theirs&&ours->Guid()!=theirs->Guid(),"Shared fire did not admit two independent owners");
                    auto ownCreature=map.GetCreature(ours->Guid()),peerCreature=map.GetCreature(theirs->Guid());
                    Require(player.canSeeOrDetect(ownCreature)&&!player.canSeeOrDetect(peerCreature)&&peer.canSeeOrDetect(peerCreature)&&!peer.canSeeOrDetect(ownCreature),"Native3331 personal visibility leaked across owners");
                    Pulse(player,11500,&peer);
                    for(uint32 i=0;i<4;++i)
                    {
                        player.Relocate(ingredients[i].GetPosition());Use(session,ingredients[i]);Pulse(player,1500,&peer);
                        Require(player.GetPower(POWER_ALTERNATE)==int32(i+1)&&peer.GetPower(POWER_ALTERNATE)==0,"Owned native receipt changed a group member's power bar");
                        if(i!=3){Require(!peer.GetQuestObjectiveData(quest,1),"Group final credit appeared before actual completed brew");Pulse(player,3600,&peer);}
                    }
                    Require(player.GetQuestObjectiveData(quest,1)==1&&peer.GetQuestObjectiveData(quest,1)==0,"Native217331 effect90 personal credit leaked to a group member");
                    Pulse(player,12000,&peer);
                    Require(FuZanBrewing::Find(&peer)&&peer.HasAura(217302)&&original->ShouldHideFor(peer.GetGUID())&&!original->ShouldHideFor(player.GetGUID()),"Owner completion disturbed the other group member's live attempt");
                    FuZanBrewing::Leave(&peer);map.RemoveAllObjectsInRemoveList();
                    Require(!FuZanBrewing::Find(&peer)&&!peer.HasAura(217302)&&!original->ShouldHideFor(peer.GetGUID()),"Second owner cleanup left its personal visibility lease");
                }
                peerSession->SetPlayer(nullptr);
                Incomplete(player,quest);auto pendingOwner=Start(player,session,fire);
                player.Relocate(ingredients[0].GetPosition());Use(session,ingredients[0]);Pulse(player,10);
                auto liveTicket=pendingOwner->*Access(BrewCreditTag{});
                Require(liveTicket&&pendingOwner->Delivery(&player,liveTicket),"Native pending-owner admission fixture failed");
                // Controlled interleaving: death state is already set but the
                // ordinary death aura teardown has not run yet. Exercise the
                // real final-receipt predicate, separately from native death.
                player.*Access(BrewOwnerDeathTag{})=JUST_DIED;
                Require(!pendingOwner->Delivery(&player,liveTicket),"Pending receipt admitted an already dead owner");Pulse(player,100);
                Require(!player.GetQuestObjectiveData(quest,1)&&!FuZanBrewing::Find(&player)&&!original->ShouldHideFor(player.GetGUID()),"Dead-owner interleaving progressed or retained private state");
                player.*Access(BrewOwnerDeathTag{})=ALIVE;
                Start(player,session,fire);
                player.Unit::setDeathState(JUST_DIED);Pulse(player,600);
                std::cout<<"OBSERVATION death alive="<<player.IsAlive()<<" actor="<<bool(FuZanBrewing::Find(&player))<<" parent="<<player.HasAura(217302)<<" meter="<<player.HasAura(217321)<<" hidden="<<original->ShouldHideFor(player.GetGUID())<<'\n';
                Require(!FuZanBrewing::Find(&player)&&!player.HasAura(217302)&&!player.HasAura(217321)&&!original->ShouldHideFor(player.GetGUID()),"Death did not clean personal actor, bar and hide lease");
                std::cout<<"PASS production brewing alternatives race="<<unsigned(race)<<" reportOnly=1 rangeAdmission=1 classAdmission=1 sourceFlags=1 missingPublicActor=1 existingBar=1 failedActorRollback=1 unrelatedNativeDelegation=1 rawOtherSummon=1 noController=1 arrivalDeadline=1 orphanReapplication=1 abandonmentLease=1 reacceptNoRebind=1 retainedHideLease=1 repeatedFire=1 wrongRetry=1 timeoutRetry=1 unissuedDummy=1 forgedAdded=1 staleDeliveryReaccept=1 preSaveLogout=1 oldMapLeave=1 concurrentPersonalOwners=1 nativePersonalCreditInGroup=1 pendingDeadOwner=1 deathCleanup=1\n";
                original->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
            }
            session->SetPlayer(nullptr); // Unauthenticated fixture: never run account logout writes.
        }
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"EOF actual native brewing2\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}

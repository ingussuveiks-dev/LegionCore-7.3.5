// Full production Lady AI/controller, actual native spell/vehicle/conversation
// and instance victory. Earlier quest/stage/spawn poses and playback clock are
// controlled; no live character saves or client/audio claims.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ScriptedGossip.h"
#include "GossipData.h"
#include "Item.h"
#include "FinalePolicy.inc"
#include "sharasdal_living_finale.h"
#include "FinaleLady.inc"
#include "FinaleInstance.inc"

struct FinaleActor : Actor
{
    using Actor::Actor;
    void Bind(uint64 spawn) { this->*Access(AltheaGuardDbTag{}) = spawn; }
    void AddToWorld() override
    {
        Actor::AddToWorld();
        if (GetEntry() == 104856)
            Require(AIM_Initialize(new npc_lady_zitrin::npc_lady_zitrinAI(this)), "Actual Lady AI missing");
        if (GetEntry() == 105298)
            Require(AIM_Initialize(new npc_grash::npc_grashAI(this)), "Actual Grash AI missing");
    }
};
struct PlaybackTag { using Type = std::map<ObjectGuid,uint32> Conversation::*; friend Type Access(PlaybackTag); };
template struct FixtureAccess<PlaybackTag, &Conversation::playing>;
struct FinaleGoDbTag { using Type = uint64 GameObject::*; friend Type Access(FinaleGoDbTag); };
template struct FixtureAccess<FinaleGoDbTag, &GameObject::m_DBTableGuid>;
struct FinaleWeapon : GameObject
{
    explicit FinaleWeapon(Map& map)
    {
        Require(Create(8006,249342,&map,1,Position{52,803,805,0},G3D::Quat(0,0,0,1),0,GO_STATE_READY),"Native weapon GO creation failed");
        this->*Access(FinaleGoDbTag{})=134299;SetPhaseId({6120},false);
        Require(map.AddToMap(static_cast<GameObject*>(this)),"Native weapon GO admission failed");
    }
    ~FinaleWeapon(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
struct FinalePlayer : NativePlayer
{
    using NativePlayer::NativePlayer;
    ~FinalePlayer(){for(uint8 slot=PLAYER_SLOT_START;slot<PLAYER_SLOT_END;++slot)if(auto item=GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->IsInWorld())item->RemoveFromWorld();}
};
namespace FinaleFault
{
    uint32 carry = 0, strike = 0;
    class zz_probe_sharasdal_finale_cast_refusal : public SpellScript
    {
        PrepareSpellScript(zz_probe_sharasdal_finale_cast_refusal);
        SpellCastResult Check()
        {
            auto& count = GetSpellInfo()->Id == 209457 ? carry : strike;
            if (count) { --count; return SPELL_FAILED_NOT_READY; }
            return SPELL_CAST_OK;
        }
        void Register() override { OnCheckCast += SpellCheckCastFn(zz_probe_sharasdal_finale_cast_refusal::Check); }
    };
    void Register() { RegisterSpellScript(zz_probe_sharasdal_finale_cast_refusal); }
}
struct FinaleBindingsTag { using Type = SpellScriptsContainer ScriptDataStoreMgr::*; friend Type Access(FinaleBindingsTag); };
template struct FixtureAccess<FinaleBindingsTag, &ScriptDataStoreMgr::_spellScriptsStore>;

int main(int argc, char** argv)
{
    try
    {
        InitializeAltheaGuardsNative(argc, argv);
        Require(sObjectMgr->LoadTrinityStrings(), "Native strings absent");
        sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();HashMapHolder<GameObject>::SetSize(100000);
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        Shadow("spell_proc_event", "1");
        Shadow("conditions", "SourceTypeOrReferenceId=13 AND SourceEntry=209457");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        sSpellMgr->LoadSpellProcEvents(); sConditionMgr->LoadConditions();
        // Native artifact reads and respawn writes stay connection-local.
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
        for (auto table : {"item_instance_artifact","creature_respawn"})
            for (auto sql : {std::string("CREATE TEMPORARY TABLE finale_seed_")+table+" LIKE "+table,
                            std::string("CREATE TEMPORARY TABLE ")+table+" LIKE finale_seed_"+table})
            {CharacterDatabase.DirectExecute(sql.c_str());CharacterDatabase.Execute(sql.c_str());}
        for(auto table:{"item_instance_artifact","creature_respawn"})
        {auto ddl=CharacterDatabase.Query((std::string("SHOW CREATE TABLE ")+table).c_str());Require(ddl&&ddl->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Native cleanup TEMP scope absent");}
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto& names = const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
        std::string cleanupName="spell_sharasdal_follower_cleanup";
#ifdef SHARASDAL_LIVING_LIVE
        Require(std::binary_search(names.begin(),names.end(),cleanupName),"Live cleanup script name missing");
        for(uint32 spell:{210247u,210045u})
        {
            auto range=(sScriptDataStore->*Access(FinaleBindingsTag{})).equal_range(spell);uint32 count=0;
            for(auto it=range.first;it!=range.second;++it)if(it->second.first==sObjectMgr->GetScriptId(cleanupName.c_str()))++count;
            Require(count==1,"Live native cleanup binding missing/duplicated");
        }
#else
        if(!std::binary_search(names.begin(),names.end(),cleanupName))names.insert(std::lower_bound(names.begin(),names.end(),cleanupName),cleanupName);
        // Test-only registry insertion is before the later fault-name suffix.
        // Instance/NPC names sort before spell_* and retain their native IDs.
        for(uint32 spell:{210247u,210045u})(sScriptDataStore->*Access(FinaleBindingsTag{})).emplace(spell,std::make_pair(sObjectMgr->GetScriptId(cleanupName.c_str()),true));
#endif
        std::string fault = "zz_probe_sharasdal_finale_cast_refusal";
        Require(names.back() < fault, "Fault name must not shift original IDs"); names.push_back(fault);
        auto scriptId = sObjectMgr->GetScriptId(fault.c_str());
        for (uint32 spell : {209457u,209487u}) (sScriptDataStore->*Access(FinaleBindingsTag{})).emplace(spell,std::make_pair(scriptId,true));
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        new instance_sharasdal; FinaleFault::Register();RegisterSpellScript(spell_sharasdal_follower_cleanup);
        sScriptMgr->SwapScriptContext(true);
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
        unsigned cases = 0;
        for (uint32 race : {uint32(RACE_DRAENEI), uint32(RACE_ORC)})
        for (uint32 mode = 0; mode < 34; ++mode)
        {
#ifdef SHARASDAL_LIVING_BASELINE
            if(mode)continue;
#endif
            FinaleFault::carry = mode == 9 ? 1 : 0; FinaleFault::strike = mode == 10 ? 1 : 0;
            MapInstanced parent(1600, 300);
            InstanceMap map(1600, 300, 9999200 + cases, DIFFICULTY_N_SCENARIO, &parent);
            map.CreateInstanceData(nullptr);
            Require(dynamic_cast<instance_sharasdal::instance_sharasdal_InstanceMapScript*>(map.GetInstanceScript()), "Actual instance missing");
            auto scenario = sScenarioMgr->AddScenario(&map, 1066); scenario->SetCurrentStep(4);
            auto session = Session(), visitorSession = Session();
            struct Cleanup
            {
                WorldSession* first; WorldSession* second; uint32 instance;
                ~Cleanup(){first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(instance);}
            } cleanup{session,visitorSession,map.GetInstanceId()};
            {
                FinalePlayer owner(session, map, 995, {40,803,805,0});
                owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race); owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);
                owner.setFactionForRace(race); owner.SetPhaseId({6120},false);
                owner.SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE); owner.SetQuestSlot(0,40341);
                NativePlayer visitor(visitorSession,map,996,{49,803,805,0});
                visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN); visitor.setFactionForRace(race);
                visitor.SetPhaseId({6120},false); visitor.SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE); visitor.SetQuestSlot(0,40341);
                auto model=sChrRacesStore.LookupEntry(race);Require(model,"Native race model absent");
                for(auto player:{static_cast<Player*>(&owner),static_cast<Player*>(&visitor)}){player->SetNativeDisplayId(model->MaleDisplayID);player->SetDisplayId(model->MaleDisplayID);}
                visitor.SetGameMaster(mode != 16 && mode != 22);
                std::unique_ptr<CreditParty> party;
                if (mode == 22) party = std::make_unique<CreditParty>(&owner,&visitor);
                FinaleActor lady(&map,104856,8001,{50,803,805,0}), grash(&map,105298,8002,{55,803,805,0}),
                    erunak(&map,102826,8003,{56,803,805,0}), adelee(&map,104853,8004,{57,803,805,0});
                lady.Bind(362095); grash.Bind(362092); erunak.Bind(362093); adelee.Bind(362094);
                for (auto actor : {&lady,&grash,&erunak,&adelee})
                {
                    actor->SetPhaseId({6120},false);
                    Require(map.AddToMap(static_cast<Creature*>(actor)), "Actual finale actor admission failed");
                }
                Require(grash.GetVehicleKit(), "Native Grash vehicle missing");
                FinaleWeapon weapon(map);
                owner.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,49819);
                auto rows = scenario->GetAchievementMgr().GetCriteriaProgressMap();
                Require(rows->count(47133) && rows->at(47133).Counter && scenario->GetCurrentStep()==4, "Native wall prerequisite missing");
                auto ai = dynamic_cast<npc_lady_zitrin::npc_lady_zitrinAI*>(lady.AI());
                Require(ai, "Whole production Lady AI absent");
                lady.SetMaxHealth(1000); lady.SetHealth(1000); lady.SetLootRecipient(&owner);
                std::unique_ptr<CreditPet> pet; std::unique_ptr<CreditGuardian> guardian;
                if (mode == 1 || mode == 24) pet = std::make_unique<CreditPet>(owner,map);
                if (mode == 2 || mode == 25) guardian = std::make_unique<CreditGuardian>(owner,map);
                Unit* killer = pet ? static_cast<Unit*>(pet.get()) : guardian ? static_cast<Unit*>(guardian.get()) :
                    mode == 3 || mode == 16 || mode == 22 ? &grash : mode == 4 ? &erunak : mode == 5 ? &adelee : static_cast<Unit*>(&owner);
                if (mode == 16 || mode == 22) lady.SetLootRecipient(nullptr);
                if (mode == 17) lady.Bind(362091);
                if (mode == 18) owner.SetQuestStatus(40341,QUEST_STATUS_NONE);
                uint32 damage = mode == 6 ? UINT32_MAX : uint32(lady.GetHealth(killer));
#ifdef SHARASDAL_LIVING_BASELINE
                owner.DealDamage(&lady,UINT32_MAX,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
                Require(!lady.IsAlive()&&scenario->GetCurrentStep()==5&&!Stories(map,1906)&&!weapon.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Pinned pre-finale counterfactual changed");
                std::cout<<"COUNTERFACTUAL race="<<race<<" actual scaled lethal damage immediately kills Lady and unlocks acquisition without living carry/1906/Grash finish\n";
#else
                if(mode>=23&&mode<=25){killer->DealDamage(&lady,UINT32_MAX,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);damage=0;}
                else ai->DamageTakenAfterScaling(killer,damage,DIRECT_DAMAGE);
                if (mode >= 16 && mode <= 18)
                {
                    Require(damage && !ai->finale.Active() && !rows->count(47380), "Foreign/illusion/questless hit admitted");
                    ++cases; continue;
                }
                Require(!damage && lady.IsAlive() && lady.GetHealth()==1 && ai->finale.Active(), "Actual lethal hook failed living surrender");
                if (mode == 7) { damage=UINT32_MAX;ai->DamageTakenAfterScaling(killer,damage,DIRECT_DAMAGE);Require(!damage,"Repeated lethal hit escaped"); }
                if (mode == 8)
                {
                    grash.SetMaxHealth(1000);grash.SetHealth(100);
                    grash.AddAura(208093,&grash);grash.AddUnitState(UNIT_STATE_STUNNED);
                    ai->UpdateLifecycle(1000); Require(!lady.GetVehicleBase()&&lady.IsAlive(),"Wounded Grash carried Lady");
                    auto guide=dynamic_cast<npc_grash::npc_grashAI*>(grash.AI());Require(guide,"Whole Grash AI absent");guide->wait_heal=true;
                    auto gain=owner.DealHeal(&grash,grash.GetMaxHealth(&owner),nullptr);grash.Unit::Update(1);guide->UpdateLifecycle(1);
                    if(grash.HasAura(208093)||grash.HasUnitState(UNIT_STATE_STUNNED))
                        std::cerr<<"HEAL health="<<grash.GetHealth()<<" max="<<grash.GetMaxHealth()<<" gain="<<gain<<" pending="<<guide->_pendingHealRecovery<<" wait="<<guide->wait_heal<<" ai="<<grash.IsAIEnabled<<std::endl;
                    Require(!grash.HasAura(208093)&&!grash.HasUnitState(UNIT_STATE_STUNNED),"Actual native heal/lifecycle did not recover Grash");
                }
                if (mode == 12 || mode == 13 || mode == 14)
                {
                    auto lease=owner.getQuestStatus(40341)->ScriptLifetime;
                    if(mode==12)owner.SetQuestStatus(40341,QUEST_STATUS_NONE);
                    if(mode==13)owner.getQuestStatus(40341)->ScriptLifetime=std::make_shared<uint8>();
                    if(mode==14)map.PlayerRelocation(&owner,300,803,805,0);
                    ai->UpdateLifecycle(1000);lady.m_Events.Update(1);
                    Require(lady.IsAlive()&&!lady.GetVehicleBase()&&!ai->finale.Active()&&!rows->count(47380),"Ended attempt completed finale");
                    ++cases; continue;
                }
                std::unique_ptr<FinaleActor> lower;
                if(mode==15){lower=std::make_unique<FinaleActor>(&map,105298,8005,Position{51,803,805,0});lower->Bind(362066);lower->SetPhaseId({6120},false);Require(map.AddToMap(static_cast<Creature*>(lower.get())),"Duplicate control absent");}
                if(mode==19){ai->SpellHit(&owner,sSpellMgr->GetSpellInfo(209487));Require(lady.IsAlive(),"Foreign early strike killed Lady");}
                if(mode==21)
                {
                    erunak.EnterVehicle(&grash,0);adelee.EnterVehicle(&grash,1);erunak.m_Events.Update(1);adelee.m_Events.Update(1);
                    ai->UpdateLifecycle(1000);lady.m_Events.Update(1);
                    Require(lady.IsAlive()&&lady.GetVehicleBase()!=&grash&&erunak.GetVehicleBase()==&grash&&adelee.GetVehicleBase()==&grash,"Full seats ejected legitimate passengers");
                    erunak.ExitVehicle();adelee.ExitVehicle();
                }
                ai->UpdateLifecycle(1000); lady.m_Events.Update(1);
                if(mode==9||mode==15)
                {
                    Require(!lady.GetVehicleBase()&&lady.IsAlive(),"Refused/foreign carry progressed");
                    if(lower){map.RemoveFromMap(static_cast<Creature*>(lower.get()),false);lower.reset();}
                    ai->UpdateLifecycle(1000);lady.m_Events.Update(1);
                }
                Require(lady.GetVehicleBase()==&grash, "Actual native carry did not board");
                if(mode==11)
                {
                    map.RemoveFromMap(static_cast<Creature*>(&erunak),false);ai->UpdateLifecycle(1000);
                    Require(ai->GetGUID(1906).IsEmpty()&&lady.IsAlive(),"Missing exact living speaker admitted dialogue");
                    erunak.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&erunak)),"Speaker reentry failed");
                }
                ai->UpdateLifecycle(1000);
                auto story = dynamic_cast<SharasdalFinale::DefeatConversation*>(ObjectAccessor::GetConversation(lady,ai->GetGUID(1906)));
                Require(story && story->IsInWorld() && story->GetLines().size()==5 && lady.GetHealth()==500, "Actual living1906 missing");
                Require(weapon.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Interrogation did not lock real pickup GO");
                if(mode==26)
                {
                    owner.SetQuestStatus(40341,QUEST_STATUS_NONE);ai->UpdateLifecycle(1000);map.RemoveAllObjectsInRemoveList();
                    Require(!ai->finale.Active()&&!lady.GetVehicleBase()&&lady.IsAlive()&&!weapon.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE)&&!rows->count(47380),"Mid-carry abandonment stranded passenger/pickup or granted victory");
                    ++cases;continue;
                }
                ai->UpdateLifecycle(1000);
                Require(lady.IsAlive(), "Unpublished conversation killed boss");
                Require(!story->CanNeverSee2(&owner), "Native owner publication refused");
                auto claimant=ObjectAccessor::GetPlayer(lady,story->GetCasterGUID());Require(claimant,"Captured eligible claimant missing");
                Require(!story->CanNeverSee2(claimant),"Native claimant publication refused");
                if(mode==20){for(int i=0;i<8;++i)ai->UpdateLifecycle(1000);Require(lady.IsAlive(),"Unelapsed line killed Lady");}
                story->*Access(PlaybackTag{}) = {{claimant->GetGUID(),getMSTime()-story->GetLine(2)->textId}};
                ai->UpdateLifecycle(1000);
                if(mode==10){Require(lady.IsAlive()&&!grash.HasUnitState(UNIT_STATE_CASTING),"Rejected strike killed Lady");ai->UpdateLifecycle(1000);}
                Require(lady.IsAlive() && grash.HasUnitState(UNIT_STATE_CASTING), "Native800ms strike not pending");
                grash.Unit::Update(799); Require(lady.IsAlive(), "Strike killed before native cast completion");
                grash.Unit::Update(1);
                Require(!lady.IsAlive() && !lady.GetVehicleBase(), "Actual209487 hit did not finish/eject Lady");
                Require(rows->count(47380) && rows->at(47380).Counter==1 && scenario->GetCurrentStep()==5, "Native real boss victory missing");
                Require(weapon.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Death unlocked weapon before last authored line");
                story->*Access(PlaybackTag{})={{claimant->GetGUID(),getMSTime()-story->GetLine(4)->textId}};
                story->Update(0);Require(!weapon.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Actual last-line handoff did not unlock pickup");
                Require(!owner.HasItemCount(128911,1,true), "Finale granted artifact before loot");
                if(mode==19){ai->SpellHit(&grash,sSpellMgr->GetSpellInfo(209487));Require(rows->at(47380).Counter==1,"Repeated strike duplicated victory");}
                if(mode>=27)
                {
                    bool abandon=mode==28||mode==29||mode==32||mode==33;
                    bool returned=mode==30;
                    if(mode==29)visitor.SetGameMaster(false);
                    if(returned)
                    {
                        owner.SetInventorySlotCount(16);
                        auto item=Item::CreateItem(128911,1,&owner);Require(item,"Native owned artifact absent");
                        ItemPosCountVec destination;destination.emplace_back((INVENTORY_SLOT_BAG_0<<8)|INVENTORY_SLOT_ITEM_START,1);
                        Require(owner.StoreItem(destination,item,true)==item,"Native owned artifact storage failed");
                    }
                    if(returned||mode==31){owner.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,49820);Require(scenario->GetCurrentStep()==6,"Native final-stage completion absent");}
                    std::unique_ptr<FinaleActor> foreign;
                    if(mode==32){foreign=std::make_unique<FinaleActor>(&map,105298,8007,Position{54,803,805,0});foreign->Bind(999999);foreign->SetPhaseId({6120},false);Require(map.AddToMap(static_cast<Creature*>(foreign.get())),"Foreign cleanup witness absent");}
                    if(mode==33){visitor.EnterVehicle(&grash,0);visitor.m_Events.Update(1);Require(visitor.GetVehicleBase()==&grash,"Player passenger witness failed");}
                    if(abandon)owner.SetQuestStatus(40341,QUEST_STATUS_NONE);
                    auto spell=abandon?210045u:210247u;
                    Require(owner.CastSpell(&owner,spell,true)==SPELL_CAST_OK,"Native cleanup cast failed");
                    bool remove=(abandon&&mode!=29)||returned;
                    if(erunak.IsDespawn()!=remove||adelee.IsDespawn()!=remove)
                        std::cerr<<"CLEANUP mode="<<mode<<" aura="<<owner.HasAura(210045)<<" erunakDespawn="<<erunak.IsDespawn()<<" alive="<<erunak.IsAlive()<<" adeleeDespawn="<<adelee.IsDespawn()<<" grashDespawn="<<grash.IsDespawn()<<std::endl;
                    Require(erunak.IsDespawn()==remove&&adelee.IsDespawn()==remove,"Native AfterCast cleanup eligibility mismatch");
                    Require(grash.IsDespawn()==(remove&&mode!=33),"Cleanup removed a player carrier or retained an eligible Grash");
                    if(foreign)Require(!foreign->IsDespawn(),"Cleanup removed unrelated NPC copy");
                    if(mode==33)visitor.ExitVehicle();
                }
                std::cout << "CASE race=" << race << " mode=" << mode << " living carry1906/native209487 cast+hit/instance victory once; earlier context and playback clock controlled\n";
                CleanStories(map);
#endif
            }
            ++cases;
        }
#ifdef SHARASDAL_LIVING_BASELINE
        std::cout << "PASS " << cases << " expected-broken counterfactual contexts; not repaired gameplay verification\n";
#else
        std::cout << "PASS " << cases << " native living Lady reconstruction contexts\n";
#endif
#ifdef SHARASDAL_LIVING_LIVE
        std::cout<<"PASS actual live210045/210247 native loader bindings, no registry substitution\n";
#endif
        CloseAltheaNative(); return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}

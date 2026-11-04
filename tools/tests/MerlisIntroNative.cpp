// Whole production actor, native criteria/scene/near travel and actual ACK.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "MovementPackets.h"
#include "MerlisIntro.inc"
void AddSC_Kingslayers();
static uint64 MerlisCounter(Scenario* scenario)
{
    auto rows = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto found = rows->find(48587);
    return found == rows->end() ? 0 : found->second.Counter;
}
static void AckMerlis(Player& player)
{
    Require(player.IsBeingTeleportedNear(), "Actual native near transfer missing");
    WorldPacket wire(CMSG_MOVE_TELEPORT_ACK); wire << player.GetGUID() << int32(0) << int32(0);
    WorldPackets::Movement::MoveTeleportAck ack(std::move(wire)); ack.Read();
    player.GetSession()->HandleMoveTeleportAck(ack);
    Require(!player.HasPendingTeleport(), "Native near ACK did not finish transfer");
}
struct MerlisExpansionTag{using Type=uint8 WorldSession::*;friend Type Access(MerlisExpansionTag);};
template struct FixtureAccess<MerlisExpansionTag,&WorldSession::m_expansion>;
int main(int argc, char** argv)
{
    try
    {
        InitializeAltheaGuardsNative(argc, argv); sSpellMgr->LoadSpellScene();
        Require(sObjectMgr->LoadTrinityStrings(), "Actual native server strings missing");
        // Horde's real arrival ACK applies2479 in this hostile zone; its proc
        // dispatch requires the same populated stores as ordinary startup.
        sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellProcs();
        Require(sSpellMgr->GetSpellScene(1313), "Actual native214942/1313 scene mapping missing");
        auto reveal=sSpellMgr->GetSpellInfo(214942);auto effect=reveal?reveal->GetEffect(0):nullptr;
        Require(effect&&effect->Effect==SPELL_EFFECT_APPLY_AURA&&effect->ApplyAuraName==SPELL_AURA_ACTIVATE_SCENE&&effect->MiscValue==1313,
            "Matching native214942 scene effect differs");
        Require(sObjectMgr->GetCreatureTemplate(107831)->lootid==0,"Actual Merlis loot now requires its native scoped loader");
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_Kingslayers();sScriptMgr->SwapScriptContext(true);
        uint32 cases = 0;
#ifdef MERLIS_NATIVE_FAULTS
        std::vector<uint32> modes{30,31,32};
#else
        std::vector<uint32> modes;for(uint32 i=0;i<26;++i)modes.push_back(i);
#endif
        for (uint32 quest : {42504u, 42627u})
            for (uint32 mode : modes)
            {
                MapInstanced parent(1620, 300);
                InstanceMap map(1620, 300, 9996000 + cases, DIFFICULTY_N_SCENARIO, &parent);
                map.CreateInstanceData(nullptr); auto scenario = sScenarioMgr->AddScenario(&map, 1123);
                scenario->SetCurrentStep(6); auto first = Session(), second = Session();
                {
                    Position center{-8321.93f, 287.852f, 156.916f, 0.7101f};
                    NativePlayer player(first, map, 101, {center.GetPositionX()+10,center.GetPositionY(),center.GetPositionZ(),0});
                    NativePlayer visitor(second, map, 102, {center.GetPositionX()+5,center.GetPositionY(),center.GetPositionZ(),0});
                    player.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);
                    player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE); player.setFactionForRace(player.getRace());
                    player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE); player.SetQuestSlot(0,quest);
                    visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);
                    Actor source(&map,107831,23001,center),speaker(&map,108397,23002,{-8360.25f,252.679f,155.43f,0.858926f});
                    Actor garona(&map,108333,23003,{-8343.12f,270.484f,155.43f,3.76786f});
                    Actor boat(&map,64367,23004,{-8350.7f,264.816f,155.43f,3.81693f});
                    for(auto actor:{&source,&speaker,&garona,&boat})Require(map.AddToMap(static_cast<Creature*>(actor)),"Native original actor admission failed");
                    auto ai=new npc_merlis_malagan::npc_merlis_malaganAI(&source);
                    Require(source.AIM_Initialize(ai),"Whole production Merlis AI failed");
                    std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;
                    if(mode==21){pet=std::make_unique<CreditPet>(player,map);guardian=std::make_unique<CreditGuardian>(player,map);}
                    if(mode==20||mode==21)player.Relocate(center.GetPositionX()+250,center.GetPositionY(),center.GetPositionZ(),0);
                    if(mode==20)visitor.SetGameMaster(true);
                    if(mode==30)map.RemoveFromMap(static_cast<Creature*>(&speaker),false);
                    if(mode==1)visitor.SetGameMaster(true);
                    if(mode==3)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
                    if(mode==4)player.Unit::setDeathState(CORPSE);
                    if(mode==5)player.SetPhaseMask(2,false);
                    if(mode==6)player.Relocate(center.GetPositionX()+71,center.GetPositionY(),center.GetPositionZ(),0);
                    if(mode==7)scenario->SetCurrentStep(5);
                    if(mode==8)player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51614);
                    if(mode==9)player.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);
                    if(mode==2)player.SetQuestStatus(quest,QUEST_STATUS_NONE);
                    ai->UpdateIntro(1000);
                    if(mode==20||mode==21){Require(!ai->introRecipient&&!ai->introDone&&!MerlisCounter(scenario),"GM/pet/guardian first arrival consumed intro without eligible owner");player.Relocate(center.GetPositionX()+10,center.GetPositionY(),center.GetPositionZ(),0);ai->UpdateIntro(1000);}
                    if(mode==8){Require(ai->introDone&&!ai->introRecipient&&!player.HasPendingTeleport()&&MerlisCounter(scenario)==1,"Existing native receipt replayed intro/travel");CleanStories(map);}
                    else
                    {
                        if(mode>=2&&mode<=7)
                        {
                            Require(!ai->introDone&&!ai->introRecipient&&!MerlisCounter(scenario),"Ineligible first visit consumed intro");
                            player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);
                            player.Unit::setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());player.SetPhaseMask(1,false);
                            player.Relocate(center.GetPositionX()+10,center.GetPositionY(),center.GetPositionZ(),0);scenario->SetCurrentStep(6);
                            ai->UpdateIntro(1000);
                        }
                        Require(ai->introRecipient==player.GetGUID()&&!ai->introDone&&ai->introDelay==5000,"Eligible Rogue not selected over ineligible visitor");
                        Require(Stories(map,2484)==(mode==30?0u:1u),"Original2484 native conversation missing/replayed");
                        if(mode==30){speaker.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&speaker)),"Original2484 actor recovery failed");}
                        std::unique_ptr<CreditParty> party;
                        if(mode==22){visitor.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);visitor.SetQuestSlot(0,quest);party=std::make_unique<CreditParty>(&player,&visitor);}
                        if(mode==10||mode==11||mode==18)
                        {
                            if(mode==10)player.SetQuestStatus(quest,QUEST_STATUS_NONE);
                            if(mode==11)player.SetPhaseMask(2,false);
                            if(mode==18)player.Relocate(center.GetPositionX()+10,center.GetPositionY(),center.GetPositionZ()+80,0);
                            ai->UpdateIntro(5000);
                            Require(!ai->introRecipient&&!player.HasPendingTeleport()&&!MerlisCounter(scenario)&&garona.IsAlive()&&boat.IsAlive(),"Invalid delayed context teleported/credited/cleaned up");
                            player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);player.SetPhaseMask(1,false);
                            player.Relocate(center.GetPositionX()+10,center.GetPositionY(),center.GetPositionZ(),0);
                            ai->UpdateIntro(1000);Require(ai->introRecipient==player.GetGUID(),"Native delayed context recovery failed");
                        }
                        if(mode==12)ai->Reset();
                        if(mode==14)
                        {
                            player.TeleportTo(1620,center.GetPositionX()+5,center.GetPositionY(),center.GetPositionZ(),0);ai->UpdateIntro(1000);
                            Require(!ai->introRecipient&&!MerlisCounter(scenario)&&garona.IsAlive()&&boat.IsAlive(),"Foreign pending transfer consumed intro/progress/cleanup");
                            AckMerlis(player);ai->UpdateIntro(1000);Require(ai->introRecipient==player.GetGUID(),"Recovery after foreign transfer failed");
                        }
                        if(mode==13)
                        {
                            player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51614);ai->UpdateIntro(5000);
                            Require(ai->introDone&&!player.HasPendingTeleport()&&garona.IsAlive()&&boat.IsAlive(),"Other legitimate native delivery replayed pending travel/cleanup");
                        }
                        else
                        {
                            if(mode==16)map.RemoveFromMap(static_cast<Creature*>(&garona),false);
                            if(mode==17)boat.SetPhaseMask(2,false);
                            if(mode==31)first->*Access(MerlisExpansionTag{})=0;
                            ai->UpdateIntro(4999);Require(!player.HasPendingTeleport()&&!MerlisCounter(scenario),"Original5s delay shortened");
                            auto criterion=mode==32?const_cast<CriteriaEntry*>(sCriteriaStore.LookupEntry(30474)):nullptr;
                            uint32 asset=criterion?criterion->Asset:0;if(criterion)criterion->Asset=UINT32_MAX;
                            ai->UpdateIntro(1);
                            if(criterion){Require(!ai->introDone&&!MerlisCounter(scenario)&&player.IsBeingTeleportedNear()&&garona.IsAlive()&&boat.IsAlive(),"Rejected native51614 consumed progress/cleanup");criterion->Asset=asset;ai->UpdateIntro(1000);}
                            if(mode==31){Require(!ai->introArriving&&!player.HasPendingTeleport()&&!MerlisCounter(scenario)&&garona.IsAlive()&&boat.IsAlive(),"Refused native SafeTeleport received progress/cleanup");first->*Access(MerlisExpansionTag{})=6;ai->UpdateIntro(1000);}
                            Require(ai->introDone&&player.IsBeingTeleportedNear()&&ai->IntroDestination(&player)&&MerlisCounter(scenario)==1&&scenario->CanUpdateCriteria(48589),"Verified native queue did not preserve original51614 before pet/guardian victory window");
                            Require(mode==16||!garona.IsAlive(),"Original Garona cleanup missing after acknowledged event");
                            Require(mode==17?boat.IsAlive():!boat.IsAlive(),"Original boat cleanup affected unrelated phase or missed actor");
                            Require(player.HasSceneStatus(1313,SCENE_LAUNCH),"Original native214942 scene was not launched on actual player");
                            if(mode==15)for(uint32 i=0;i<10;++i){ai->UpdateIntro(1000);Require(MerlisCounter(scenario)==1&&scenario->CanUpdateCriteria(48589)&&player.IsBeingTeleportedNear(),"Client ACK delay consumed/replayed native progression");}
                            if(mode>=23&&mode<=25)
                            {
                                source.SetLootRecipient(&player);source.LowerPlayerDamageReq(uint32(source.GetMaxHealth()));
                                std::unique_ptr<CreditPet> finalPet;std::unique_ptr<CreditGuardian> finalGuardian;
                                if(mode==23)finalPet=std::make_unique<CreditPet>(player,map);
                                if(mode==24)finalGuardian=std::make_unique<CreditGuardian>(player,map);
                                Unit* killer=finalPet?static_cast<Unit*>(finalPet.get()):finalGuardian?static_cast<Unit*>(finalGuardian.get()):static_cast<Unit*>(&player);
                                if(mode==25)killer->DealDamage(&source,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);else killer->Kill(&source,false);
                                Require(!source.IsAlive()&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48589).Counter==1&&scenario->GetCurrentStep()==8&&player.IsBeingTeleportedNear(),"Actual owner/pet/guardian killing blow before client ACK lost native51615");
                            }
                            AckMerlis(player);
                            Require(player.GetExactDist(-8342.62f,271.15f,155.34f)<0.1f&&MerlisCounter(scenario)==1,"Actual ACK changed earned native event/position");
                            ai->UpdateIntro(1000);
                            Require(ai->introDone&&scenario->GetCurrentStep()==(mode>=23&&mode<=25?8u:7u)&&!player.HasPendingTeleport(),"Actual ACK changed native transition stage");
                            ai->UpdateIntro(10000);ai->Reset();ai->UpdateIntro(10000);
                            Require(MerlisCounter(scenario)==1&&!player.HasPendingTeleport()&&((mode>=23&&mode<=25)||!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48589))&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48591),"Repeated intro granted boss/pickup or replayed travel");
                        }
                        CleanStories(map);
                    }
                    player.KillAllDelayedEvents();visitor.KillAllDelayedEvents();
                    std::cout<<"CASE native Merlis quest="<<quest<<" mode="<<mode<<" qualified5s intro, actual queued travel/51614 before independent native ACK retained\n";
                }
                first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;
                sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
            }
#ifdef MERLIS_NATIVE_FAULTS
        std::cout<<"PASS "<<cases<<" explicit native failure/recovery contexts:2484 actor refusal, actual expansion-refused SafeTeleport behind successful TeleportTo return, actual51614 rejection. Reviewed WARNs expected; separate from ordinary clean run\n";
#else
        std::cout<<"PASS "<<cases<<" native whole Merlis cases: ineligible/GM/pet/guardian first visits, group shared progress, original5s/2484/214942 scene launch, real accepted near travel/51614 and independent ACK, actual player/pet/guardian boss kills before ACK, abandonment/phase/range/retry/Reset, foreign transfer, missing actor and exactly-once native51614; earlier quest/stage/arrival endpoints controlled, no authenticated client/full combat claim\n";
#endif
        std::cout.flush();CloseAltheaNative();
    }
    catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}

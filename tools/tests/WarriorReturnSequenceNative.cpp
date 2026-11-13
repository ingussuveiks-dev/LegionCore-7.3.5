// Real pickup controller -> finite scene -> original conversation publication ->
// personal Val'kyr -> native area/visual -> native override/button -> jump spline.
// Earlier quest objectives and already-owned items are in-memory prerequisites.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Item.h"
#include "AreaTrigger.h"
#include "AreaTriggerData.h"
#include "QuestData.h"
#include <atomic>
#include <map>
#include <mutex>
namespace ReturnSequence {
#include "WarriorReturnController.inc"
#include "WarriorReturnFactory.inc"
}
struct ReturnPlayer : NativePlayer {
    using NativePlayer::NativePlayer;
    void NativeDefaults() {
        GetMap()->GetZoneAndAreaId(m_zoneId,m_areaId,GetPositionX(),GetPositionY(),GetPositionZ());
        for(auto field:{UNIT_FIELD_MOD_CASTING_SPEED,UNIT_FIELD_MOD_SPELL_HASTE,UNIT_FIELD_MOD_HASTE,UNIT_FIELD_MOD_HASTE_REGEN})
            SetFloatValue(field,1.0f);
    }
    ~ReturnPlayer(){for(uint8 slot=PLAYER_SLOT_START;slot<PLAYER_SLOT_END;++slot)if(auto item=GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->IsInWorld())item->RemoveFromWorld();}
};
static void OwnedPair(ReturnPlayer& player,ReturnSequence::WarriorPickupReturn::Mode const& mode,bool offspec){
    player.SetInventorySlotCount(16);player.AddWeaponProficiency(UINT32_MAX);player.AddArmorProficiency(UINT32_MAX);
    player.SetCanDualWield(true);player.SetCanTitanGrip(true);
    player.SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID,offspec?71:mode.map==1511?72:73);
    auto parent=Item::CreateItem(mode.item,1,&player),child=Item::CreateItem(mode.child,1,&player);
    Require(parent&&child,"Native owned pair fixture failed");
    parent->SetChildItem(child->GetGUID());child->SetGuidValue(ITEM_FIELD_CREATOR,parent->GetGUID());child->SetFlag(ITEM_FIELD_DYNAMIC_FLAGS,ITEM_FLAG_CHILD);
    ItemPosCountVec hidden;hidden.emplace_back((INVENTORY_SLOT_BAG_0<<8)|CHILD_EQUIPMENT_SLOT_START,1);
    Require(player.StoreItem(hidden,child,true)==child,"Native child storage failed");
    if(offspec){ItemPosCountVec stored;stored.emplace_back((INVENTORY_SLOT_BAG_0<<8)|INVENTORY_SLOT_ITEM_START,1);Require(player.StoreItem(stored,parent,true)==parent,"Native offspec parent storage failed");}
    else player.QuickEquipItem((INVENTORY_SLOT_BAG_0<<8)|mode.slot,parent);
}
int main(int argc,char**argv){try{
    InitializeAltheaGuardsNative(argc,argv);
    // Native Item creation reads artifact XP; isolate even that read from saved items.
    CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE return_artifact_seed LIKE item_instance_artifact");
    CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE item_instance_artifact LIKE return_artifact_seed");
    auto table=CharacterDatabase.Query("SHOW CREATE TABLE item_instance_artifact");Require(table&&table->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Artifact read shadow missing");
    CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto rows=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(40043,39191,41105) ORDER BY StorageIndex");
    Require(bool(rows),"Native Warrior objectives absent");
    do{auto fields=rows->Fetch();auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(fields[1].GetUInt32()));Require(quest,"Native Warrior quest absent");quest->LoadQuestObjective(fields);}while(rows->NextRow());
    sSpellMgr->LoadSpellScene();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    new ReturnSequence::npc_stromkar_valkyr;new ReturnSequence::condition_warrior_pickup_return;new ReturnSequence::condition_stromkar_return;
    sScriptMgr->SwapScriptContext(true);
    // Preserve every actual shared11869 branch and relevant spell condition.
    // This scoped fixture does not load unrelated loot/gossip/quest stores;
    // feeding their conditions to the native validator would be an invalid test.
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    Shadow("conditions","(SourceTypeOrReferenceId=26 AND SourceGroup=11869) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(215997,192085,216016))");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sConditionMgr->LoadConditions();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}},{1495,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}},{1495,{}}});
    uint32 cases=0;
    for(uint32 variant=0;variant<3;++variant){
        auto const& mode=variant==2?ReturnSequence::WarriorPickupReturn::protection:ReturnSequence::WarriorPickupReturn::fury;
        MapInstanced parent(mode.map,300);InstanceMap map(mode.map,300,9987100+variant,DIFFICULTY_N_SCENARIO,&parent);
        auto scenario=sScenarioMgr->AddScenario(&map,mode.scenario);scenario->SetCurrentStep(scenario->GetStepCount(false)-1);
        auto session=Session();auto otherSession=Session();{
            Position pose=mode.map==1511?Position{3515.24f,1800.51f,0.538696f,0}:Position{4846.0f,220.0f,-30.0f,0};
            ReturnPlayer player(session,map,931,pose),other(otherSession,map,932,pose);
            player.NativeDefaults();other.NativeDefaults();
            player.SetQuestStatus(mode.quest,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,mode.quest);
            auto quest=sQuestDataStore->GetQuestTemplate(mode.quest);Require(quest&&quest->GetObjectives().size()==3,"Actual quest graph changed");
            for(auto const& objective:quest->GetObjectives())player.SetQuestObjectiveData(quest,&objective,objective.ObjectID==103739?0:objective.Amount,true);
            OwnedPair(player,mode,variant==1);
            Require(ReturnSequence::WarriorPickupReturn::Earned(&player,mode),"Real native prerequisite admission failed");
            auto state=ReturnSequence::WarriorPickupReturn::Current(&player,&mode);
            state->Tick(&player,1000);player.Unit::Update(100);
            Require(player.HasAura(mode.sceneSpell)&&player.HasSceneStatus(mode.scene,SCENE_LAUNCH)&&state->valkyr.IsEmpty(),"Scene launch exposed exit early");
            state->Tick(&player,1000);
            for(uint32 t=0;t<9100;t+=10)player.Unit::Update(10);
            Require(!player.HasAura(mode.sceneSpell)&&player.HasSceneStatus(mode.scene,SCENE_COMPLETE),"Real finite scene did not acknowledge");
            state->Tick(&player,1000);player.Unit::Update(100);
            if(mode.conversation){
                auto clip=ObjectAccessor::GetConversation(player,state->conversation);Require(clip&&!state->publication->finished&&state->valkyr.IsEmpty(),"Exit did not wait for849 publication");
                Require(!clip->CanNeverSee2(&player),"Original849 viewer rejected");
                uint32 budget=clip->GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME);
                auto started=GetTickCount64();while(GetTickCount64()-started<=budget+20){clip->Update(0);Sleep(5);}
                Require(state->publication->finished,"Actual849 publication clock failed");map.RemoveAllObjectsInRemoveList();state->Tick(&player,1000);
            }
            auto valkyr=ObjectAccessor::GetCreature(player,state->valkyr);
            Require(valkyr&&ReturnSequence::WarriorPickupReturn::Owner(valkyr)==&player,"Real scoped Val'kyr did not appear");
            auto ai=dynamic_cast<ReturnSequence::WarriorPickupReturn::ValkyrAI*>(valkyr->AI());Require(ai,"Production shared factory did not select scoped AI");ai->UpdateAI(1000);
            auto area=map.GetAreaTrigger(ai->area);Require(area&&area->GetDuration()==-1,"Real owned return area missing");
            Require(area->GetUInt32Value(AREATRIGGER_FIELD_SPELL_FOR_VISUALS)==230829&&area->GetUInt32Value(AREATRIGGER_FIELD_SPELL_XSPELL_VISUAL_ID)==111461,"Native glow fields missing");
            map.PlayerRelocation(&player,area->GetPositionX(),area->GetPositionY(),area->GetPositionZ(),0);
            map.PlayerRelocation(&other,area->GetPositionX(),area->GetPositionY(),area->GetPositionZ(),0);
            auto scan=[&]{for(uint32 t=0;t<1000;t+=100)area->Update(100);player.Unit::Update(100);other.Unit::Update(100);};
            scan();Require(area->IsUnitAffected(player.GetGUID()),"Native spatial scanner missed owner inside real area");
            Require(!other.HasAura(215997),"Foreign visitor received personal button");
            Require(player.HasAura(215997)&&player.GetUInt16Value(PLAYER_FIELD_BYTES_5,PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET)==730&&player.HasSpell(192085),"Native aura did not publish actual jump override/button");
            map.PlayerRelocation(&other,area->GetPositionX()+20,area->GetPositionY(),area->GetPositionZ(),0);scan();Require(player.HasAura(215997),"Foreign leave removed owner button");
            map.PlayerRelocation(&player,area->GetPositionX()+20,area->GetPositionY(),area->GetPositionZ(),0);scan();Require(!player.HasAura(215997)&&!player.HasSpell(192085),"Native spatial leave retained button");
            map.PlayerRelocation(&player,area->GetPositionX(),area->GetPositionY(),area->GetPositionZ(),0);scan();Require(player.HasAura(215997)&&player.HasSpell(192085),"Native spatial reentry did not rearm button");
            player.RemoveAurasDueToSpell(215997);ai->UpdateAI(1000);player.Unit::Update(100);Require(player.HasAura(215997),"Tracked owner inside area lost native enter-action retry");
            float z=player.GetPositionZ();Require(player.CastSpell(&player,192085,false)==SPELL_CAST_OK,"Ordinary manual jump request rejected");player.Unit::Update(100);
            Require(player.movespline->Initialized()&&!player.movespline->Finalized()&&std::abs(player.movespline->FinalDestination().z-z-20.0f)<0.1f,"Actual manual button did not launch native20-yard lift");
            Require(player.GetQuestObjectiveData(mode.quest,103739)==0,"Launch forged Skyhold arrival credit");
            std::cout<<"CASE map="<<mode.map<<" offspec="<<(variant==1)<<" finite-scene/full849/native-Val'kyr/visual230829:111461/owner215997/override730/spell192085/native-Z+20; no arrival credit before arrival"<<std::endl;
            player.StopMoving();player.GetMotionMaster()->Clear();state->Clear(&player);ReturnSequence::WarriorPickupReturn::states.clear();map.RemoveAllObjectsInRemoveList();CleanStories(map);
        }session->SetPlayer(nullptr);otherSession->SetPlayer(nullptr);delete session;delete otherSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
    std::cout<<"PASS "<<cases<<" integrated actual-core return producer/visual fields/spatial enter-leave-reentry/aura/button/jump launch; earlier objectives/inventory prepared in memory. No client rendering/input, completed teleport/landing or full chain claim; no character save or persistent write\n";
    CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

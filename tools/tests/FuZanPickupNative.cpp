// Actual defense/finale -> native opening3365 -> loot packet -> release ->
// scene/equipment -> native gossip and far transfer/ACK. Earlier stage2 and
// its entry objective are controlled prerequisites, never a full-entry claim.
#define FUZAN_DEFENSE_NATIVE_SUPPORT_ONLY
#include "FuZanDefenseNative.cpp"
#include "ScenePackets.h"
#include "NPCPackets.h"
#include "GossipDef.h"

// The constructor below is extracted from production. Its admission method
// links the actual artifact_acquisition.obj, rather than a modeled guard.
namespace MonkArtifactScene {bool Denied(Player*,SpellScene const*,std::string const&);}
#include "NativeMonkScene.inc"
// Same isolated, unauthenticated native map/session registration as Sheilun.
#include "NativeReturnSession.inc"

struct PickupScenesTag{using Type=std::unordered_map<uint32,uint32> Player::*;friend Type Access(PickupScenesTag);};
template struct FixtureAccess<PickupScenesTag,&Player::m_sceneInstanceID>;

static void PickupStore(Player& player,Loot& loot)
{
    Require(loot.items.size()==1&&loot.items[0].item.ItemID==128938,"Actual Fu Zan source loot differs");
    WorldPacket wire(CMSG_LOOT_ITEM);wire<<uint32(1)<<loot.GetGUID()<<uint8(1);
    WorldPackets::Loot::AutoStoreLootItem packet(std::move(wire));packet.Read();
    Require(packet.Loot.size()==1&&packet.Loot[0].Object==loot.GetGUID(),"Actual loot packet decoding differs");
    player.GetSession()->HandleAutostoreLootItemOpcode(packet);
}
static void PickupScene(Player& player,uint32 instance,unsigned callback)
{
    WorldPackets::Scene::SceneInstance packet{WorldPacket(callback==1?CMSG_SCENE_PLAYBACK_CANCELED:CMSG_SCENE_PLAYBACK_COMPLETE)};
    packet.SceneInstanceID=instance;
    if(callback==1)player.GetSession()->HandleScenePlaybackCanceled(packet);
    else player.GetSession()->HandleScenePlaybackComplete(packet);
}
static void CheckPickup(uint8 race,unsigned killer,unsigned callback,bool fullBags,bool entered,bool abandon,bool otherSpec,uint32 number)
{
    DefenseFixture d(race,killer,number);auto& f=d.wave;auto& player=*f.player;
    auto quest=sQuestDataStore->GetQuestTemplate(42765);
    // Explicit earlier-stage boundary. Native217618 has TELEPORT227/misc1286
    // plus entry credit109329; its actual admission/travel is not replayed
    // inside this already running stage2 fixture. The missing-entry control
    // below must remain INCOMPLETE after otherwise successful pickup/return.
    Require(quest->GetObjectives()[0].ObjectID==109329&&quest->GetObjectives()[0].StorageIndex==0,"Earlier entry objective changed");
    if(entered)player.SetQuestObjectiveData(quest,&quest->GetObjectives()[0],1,true);
    player.SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID,otherSpec?SPEC_MONK_WINDWALKER:SPEC_MONK_BREWMASTER);
    player.SetSkill(SKILL_STAVES,1,1,1);
    for(unsigned wave=1;wave<=4;++wave)
    {
        auto actors=DefenseActors(*f.yulon);actors.remove_if([](Creature* actor){return !actor->IsAlive();});
        Require(actors.size()==8,"Actual pickup prerequisite wave differs");for(auto actor:actors)d.Kill(*actor,true);d.Pump(100);
    }
    d.Pump(60000);Require(f.scenario->GetCurrentStep()==6,"Actual pickup prerequisite confrontation missing");
    f.map.PlayerRelocation(&player,886,-2710,192.165f,0);d.Kill(*d.korithis,true);
    std::list<GameObject*> weapons;d.korithis->GetGameObjectListWithEntryInGrid(weapons,251605,100);
    Require(weapons.size()==1&&f.scenario->GetCurrentStep()==7&&!d.corrupted->IsAlive(),"Actual finale weapon/cleanup missing");auto weapon=weapons.front();
    f.map.PlayerRelocation(&player,weapon->GetPositionX(),weapon->GetPositionY(),weapon->GetPositionZ(),0);
    weapon->Update(5000);Require(weapon->getLootState()==GO_READY&&!weapon->IsPersonal(),"Actual native chest initialization differs");
    Require(player.CastSpell(weapon,3365,false)==SPELL_CAST_OK,"Native opening cast failed");d.Pump(5000);
    Require(weapon->getLootState()==GO_ACTIVATED&&!player.GetItemCount(128938)&&f.scenario->GetCurrentStep()==7,"Opening alone awarded pickup");
    auto& loot=weapon->loot;
    if(fullBags)
    {
        for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<player.GetInventoryEndSlot();++slot)if(!player.GetItemByPos(INVENTORY_SLOT_BAG_0,slot))
        {
            ItemPosCountVec dest;Require(player.CanStoreNewItem(INVENTORY_SLOT_BAG_0,slot,dest,2589,200)==EQUIP_ERR_OK&&player.StoreNewItem(dest,2589,true),"Native full-bag prerequisite failed");
        }
        ItemPosCountVec dest;Require(player.CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,128938,1)==EQUIP_ERR_INV_FULL,"Native inventory not full");
        PickupStore(player,loot);player.GetSession()->DoLootRelease(weapon->GetGUID());d.Pump(100);
        Require(!player.GetItemCount(128938)&&!loot.items[0].is_looted&&!player.HasAura(217814)&&!player.HasSceneStatus(1342,SCENE_LAUNCH)&&f.scenario->GetCurrentStep()==7,"Full bags consumed source or advanced pickup");
        player.DestroyItem(INVENTORY_SLOT_BAG_0,INVENTORY_SLOT_ITEM_START,true);
        Require(player.CastSpell(weapon,3365,false)==SPELL_CAST_OK,"Native original chest reopen failed");d.Pump(5000);
    }
    PickupStore(player,loot);PickupStore(player,loot);
    Require(player.GetItemCount(128938)==1&&loot.items[0].is_looted&&f.scenario->GetCurrentStep()==7,"Native packet storage/duplicate admission differs");
    ObjectGuid artifact=player.GetItemByEntry(128938)->GetGUID();
    player.GetSession()->DoLootRelease(weapon->GetGUID());d.Pump(1000);
    Require(f.scenario->GetCurrentStep()==8&&player.HasAura(217814)&&player.HasSceneStatus(1342,SCENE_LAUNCH)&&player.GetQuestObjectiveData(quest,1)==1&&!player.HasAura(217659)&&!player.HasAura(217667),"Actual release/retrieval/credit/controller cleanup missing");
    Require(!player.GetItemByPos(INVENTORY_SLOT_BAG_0,EQUIPMENT_SLOT_MAINHAND),"Native retrieval equipped before its scene endpoint");
    uint32 instance=0;for(auto const& scene:player.*Access(PickupScenesTag{}))if(scene.second==1342)instance=std::max(instance,scene.first);
    Require(instance,"Actual pickup scene instance missing");
    if(abandon){player.SetQuestStatus(42765,QUEST_STATUS_NONE);d.Pump(1000);}
    if(callback==2){d.Pump(10000);PickupScene(player,instance,0);}else{PickupScene(player,instance,callback);d.Pump(1000);PickupScene(player,instance,callback);}
    auto equipped=player.GetItemByPos(INVENTORY_SLOT_BAG_0,EQUIPMENT_SLOT_MAINHAND);
    Require(player.GetItemCount(128938)==1&&player.GetItemByEntry(128938)->GetGUID()==artifact&&!player.HasAura(217814),"Actual scene/expiry changed item identity or failed aura cleanup");
    // Native SCENE_SEEN requires COMPLETE. Expiry without a terminal packet
    // must not invent that receipt or auto-equip; the late packet is stale.
    Require(bool(equipped)==(!abandon&&!otherSpec&&callback!=2)&&(!equipped||equipped->GetGUID()==artifact),"Native scene/expiry equipment admission differs");
    Require(player.HasSceneStatus(1342,SCENE_COMPLETE)==(callback!=2),"Actual callback/expiry scene status differs");
    std::list<Creature*> returns;d.korithis->GetCreatureListWithEntryInGrid(returns,109440,200);Require(returns.size()==1,"Actual return actor missing");auto receiver=returns.front();
    f.map.PlayerRelocation(&player,receiver->GetPositionX()+1,receiver->GetPositionY(),receiver->GetPositionZ(),0);
    WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TALK_TO_GOSSIP)};hello.Unit=receiver->GetGUID();
    if(abandon)Require(!player.HasPhaseId(6905)&&!player.HasPhaseId(6906)&&!player.GetQuestObjectiveData(quest,2)&&player.GetQuestStatus(42765)==QUEST_STATUS_NONE,"Native abandonment retained phases or active quest progress");
    // The original postpickup actor has no quest/phase gate and retains an
    // exit after abandonment. Verify that native escape without inventing a
    // new admission restriction or awarding abandoned quest objectives.
    {
        NativeReturnSession nativeSession(player);
        f.session->HandleGossipHelloOpcode(hello);
        Require(player.PlayerTalkClass->GetGossipMenu().GetMenuId()==19932&&player.PlayerTalkClass->GetGossipMenu().GetMenuItemCount()==1,"Actual return offer differs");
        WorldPackets::NPC::GossipSelectOption select{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};select.GossipUnit=receiver->GetGUID();select.GossipID=19932;select.GossipIndex=0;f.session->HandleGossipSelectOption(select);d.Pump(100);
        Require(player.GetQuestObjectiveData(quest,2)==(abandon?0:1)&&f.scenario->IsCompleted(false)&&player.GetQuestStatus(42765)==(abandon?QUEST_STATUS_NONE:entered?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE),"Native final receipt/completion, abandoned exit or missing-entry control differs");
        Require(player.IsBeingTeleportedFar()&&player.GetTeleportDest().GetMapId()==1514&&!player.IsInWorld(),"Actual return transfer missing");
        f.session->HandleWorldPortAck();Require(player.IsInWorld()&&player.GetMapId()==1514&&!player.HasPendingTeleport()&&player.GetItemCount(128938)==1&&player.GetItemByEntry(128938)->GetGUID()==artifact,"Actual return ACK/item persistence failed");
    }
    for(auto entry:{109432u,109979u,109980u,109440u}){std::list<Creature*> actors;d.korithis->GetCreatureListWithEntryInGrid(actors,entry,200);Require(actors.size()==1,"Native repeated pickup duplicated or lost finale actor");for(auto actor:actors)actor->DespawnOrUnsummon();}
    weapon->Delete();f.map.RemoveAllObjectsInRemoveList();
    std::cout<<"CASE native pickup race="<<unsigned(race)<<" killer="<<killer<<" callback="<<callback<<" fullBags="<<fullBags<<" earlierEntry="<<entered<<" abandon="<<abandon<<" otherSpec="<<otherSpec<<" PASS\n";
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true,true,true,true,true,true,true);HashMapHolder<EventObject>::SetSize(20000);HashMapHolder<GameObject>::SetSize(20000);
        auto& table=LootTemplates_Creature.*Access(LootTableNameTag{});auto original=table;table+=" WHERE Entry IN(109395,109397,109438,109933,109821) ORDER BY Entry,Item";
        auto count=(LootTemplates_Creature.*Access(LoadLootTableTag{}))();table=original;Require(count==12,"Actual defense loot differs");
        level98XP=uint32(sXpGameTable.GetRow(98)->Total);if(auto xp=WorldDatabase.Query("SELECT Experience FROM player_xp_for_level WHERE Level=98"))level98XP=xp->Fetch()[0].GetUInt32();
        auto entrance=sSpellMgr->GetSpellInfo(217618);Require(entrance,"Native entry spell missing");unsigned teleports=0,credits=0,entryEffects=0;
        for(auto effect:entrance->Effects)if(effect&&effect->Effect){++entryEffects;if(effect->Effect==SPELL_EFFECT_TELEPORT&&effect->MiscValue==1286)++teleports;if(effect->Effect==SPELL_EFFECT_KILL_CREDIT&&effect->MiscValue==109329)++credits;}
        Require(entryEffects==2&&teleports==1&&credits==1,"Native earlier entry boundary changed");
        std::string ruleQuery="SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(49567,49568,49620,49177,49351,49179";unsigned endpoints=0;
        for(auto entry:sCriteriaStore)if(entry->Type==CRITERIA_TYPE_SCRIPT_EVENT_2&&(entry->Asset==52483||entry->Asset==52751)){ruleQuery+=","+std::to_string(entry->ID);++endpoints;}
        ruleQuery+=")";Require(endpoints==2,"Native pickup/return criteria changed");
        auto rules=WorldDatabase.Query(ruleQuery.c_str());Require(rules&&!rules->Fetch()[0].GetUInt32(),"Additional native defense/pickup/return rules must be loaded");
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);new scene_monk_artifact_equipment();sScriptMgr->SwapScriptContext(true);
        uint32 number=1200;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
        {
            for(unsigned killer=0;killer<5;++killer)CheckPickup(race,killer,0,false,true,false,false,++number);
            CheckPickup(race,3,1,false,true,false,false,++number);CheckPickup(race,2,2,false,true,false,false,++number);
            CheckPickup(race,2,0,true,true,false,false,++number);CheckPickup(race,0,0,false,false,false,false,++number);
            CheckPickup(race,0,0,false,true,true,false,++number);CheckPickup(race,4,0,false,true,false,true,++number);
        }
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
        std::cout<<"PASS22 native pickup/scene/equipment/return cases. Earlier entry/stage2 controlled, wave combat AI not ticked, corrupted actor uses template initialization, no character saves or client rendering/flight/turn-in claim.\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}

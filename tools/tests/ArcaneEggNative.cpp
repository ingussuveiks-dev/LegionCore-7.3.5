// Real Release SpellScript, Creature AI and Player::SendLoot/StoreLootItem.
// Socketless accepted-quest player and synthetic source placements only.
#define main ChestSupportMain
#include "LegionChestLootLinksNative.cpp"
#undef main
#include "arcane_infused_egg.h"
struct EggControlledTag {using Type=bool Unit::*;friend Type Access(EggControlledTag);};
template struct FixtureAccess<EggControlledTag,&Unit::m_ControlledByPlayer>;

struct EggBird : Creature
{
    EggBird(Map& map,uint32 guid,Position pos)
    {
        Require(Create(guid,&map,1,89652,0,ALLIANCE,pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),pos.GetOrientation()),"Heron native Create failed");
        SetHomePosition(*this);
        Require(map.AddToMap(static_cast<Creature*>(this)),"Heron native map admission failed");
    }
    ~EggBird(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static Quest* eggQuest;
static void LoadEggQuest()
{
    auto rows=WorldDatabase.Query(
#include "NativeEggQuest.inc"
    );
    Require(bool(rows),"Native egg quest missing");eggQuest=new Quest(rows->Fetch());
    auto id=eggQuest->GetQuestId();Require(id==37727,"Wrong egg quest");
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[id]=eggQuest;
    (sQuestDataStore->*Access(QuestVectorTag{}))[id]=eggQuest;
    auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=37727 ORDER BY StorageIndex");
    Require(bool(objectives),"Native egg objective missing");
    do{auto f=objectives->Fetch();eggQuest->LoadQuestObjective(f);Require(f[2].GetUInt8()==QUEST_OBJECTIVE_ITEM&&f[4].GetUInt32()==120939&&f[5].GetUInt32()==6,"Native six-egg objective differs");eggQuest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);}while(objectives->NextRow());
}
static void ClearOpen(LootPlayer& player,GameObject& egg)
{
    player.SetLootGUID(ObjectGuid::Empty);player.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_LOOTING);player.RemoveLoot(egg.GetGUID());egg.loot.clear();
}
static void CheckEgg(Map& map,LootPlayer& player,uint32& guid)
{
    player.SetQuestStatus(37727,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,37727);
    // Native Player::Create sets this player identity flag. It is distinct
    // from enabling PvP and is required by neutral creature attack admission.
    player.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PVP_ATTACKABLE);
    auto base=player.GetPosition();auto nearPose=base;nearPose.m_positionX+=3;auto farPose=base;farPose.m_positionX+=12;
    Require(!ArcaneEggLoot::Guard(nullptr)&&!ArcaneEggLoot::Guard(&player),"No actual egg source accepted");
    {
        EggBird guard(map,++guid,nearPose),other(map,++guid,farPose);
        Require(guard.getFaction()==190&&guard.IsAIEnabled&&guard.AI(),"Actual heron faction/default AI differs");
        ChestLootObject source;Require(source.Create(++guid,239692,&map,1,base,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&source)),"Guard fixture source failed");source.Update(5000);player.SetLootGUID(source.GetGUID());
        Require(ArcaneEggLoot::Guard(&player)==&guard,"Actual closest guard admission failed");
        player.SetPhaseMask(2,false);Require(!ArcaneEggLoot::Guard(&player),"Cross-phase egg accepted");player.SetPhaseMask(1,false);
        auto away=base;away.m_positionX+=100;player.Relocate(away);Require(!ArcaneEggLoot::Guard(&player),"Out-of-range source accepted");player.Relocate(base);
        guard.SetPhaseMask(2,false);other.SetPhaseMask(2,false);Require(!ArcaneEggLoot::Guard(&player),"Cross-phase guard accepted");guard.SetPhaseMask(1,false);other.SetPhaseMask(1,false);
        guard.Relocate(away);other.Relocate(away);Require(!ArcaneEggLoot::Guard(&player),"Distant guards accepted");guard.Relocate(nearPose);other.Relocate(farPose);
        guard.*Access(EggControlledTag{})=true;other.*Access(EggControlledTag{})=true;Require(!ArcaneEggLoot::Guard(&player),"Player-controlled bird accepted");guard.*Access(EggControlledTag{})=false;other.*Access(EggControlledTag{})=false;
        guard.AI()->AttackStart(&player);Require(!ArcaneEggLoot::Guard(&player),"Engaged guard was redirected");guard.AI()->EnterEvadeMode();guard.CombatStop(true);player.CombatStop(true);
        source.SetPhaseMask(2,false);Require(!ArcaneEggLoot::Guard(&player),"Invisible source accepted");source.SetPhaseMask(1,false);
        player.SetLootGUID(ObjectGuid::Empty);
    }
    for(unsigned n=0;n<6;++n)
    {
        Tick(player,2000); // Independent ordinary clicks, not same-tick casts.
        EggBird guard(map,++guid,nearPose),other(map,++guid,farPose);
        ChestLootObject egg;Require(egg.Create(++guid,239692,&map,1,base,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&egg)),"Native egg Create failed");egg.Update(5000);
        Require(sObjectMgr->GetGameObjectTemplate(239692)->chest.spell==179109&&egg.GetGOInfo()->GetLootId()==239692,"Native egg metadata missing");
        player.SendLoot(egg.GetGUID(),LOOT_CORPSE);
        player.Unit::Update(1);
        Require(guard.getVictim()==&player&&guard.isInCombat()&&!other.getVictim(),"Native postloot spell did not engage nearest actual guard");
        Require(guard.getFaction()==190,"Reaction mutated authored faction");
        Require(!ArcaneEggLoot::Guard(&player),"Repeated open redirects engaged guard");
        auto loot=egg.IsPersonal()?&player.personalLoot[egg.GetGUID()]:&egg.loot;
        WorldPackets::Loot::LootResponse packet;loot->BuildLootResponse(packet,&player,ALL_PERMISSION);
        Require(packet.Items.size()==1&&packet.Items[0].Loot.ItemID==120939&&packet.Items[0].Quantity==1,"Native egg loot differs from one authored item");
        auto before=player.GetItemCount(120939,true);
        player.StoreLootItem(uint8(loot->items.size()),loot);
        Require(player.GetItemCount(120939,true)==before+1,"Native quest egg inventory storage missing");
        player.StoreLootItem(uint8(loot->items.size()),loot);
        Require(player.GetItemCount(120939,true)==before+1,"Repeat physical slot duplicated egg");
        ClearOpen(player,egg);guard.AI()->EnterEvadeMode();guard.CombatStop(true);player.CombatStop(true);
        Require(guard.getFaction()==190&&!guard.getVictim(),"Native guard evade failed to restore idle neutrality");
    }
    Require(player.GetItemCount(120939,true)==6&&player.GetQuestStatus(37727)==QUEST_STATUS_COMPLETE,"Actual six-item collection did not complete native quest");
    // The helper itself never grants inventory or quest progress.
    EggBird guard(map,++guid,nearPose);
    auto before=player.GetItemCount(120939,true);
    Require(player.CastSpell(&player,179109,false)==SPELL_CAST_OK&&!guard.getVictim()&&player.GetItemCount(120939,true)==before,"Source-free helper granted progress or attacked");
    player.Unit::Update(1);Require(!guard.getVictim()&&player.GetItemCount(120939,true)==before,"Deferred source-free cast granted progress");
    std::cout<<"CASE native egg six independent loot sources, exactly-once physical slots, neutral heron derived vehicle AI combat and evade PASS\n";
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeNative(argc,argv);opcodeTable.Initialize();LoadLootInputs();LoadEggQuest();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();sAchievementMgr->LoadCriteriaList();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());ArcaneEggLoot::Register();sScriptMgr->SwapScriptContext(true);
        unsigned enabled=0;auto bounds=sScriptDataStore->GetSpellScriptsBounds(179109);for(auto i=bounds.first;i!=bounds.second;++i)if(i->second.second)++enabled;Require(enabled==1,"Actual egg spell binding not enabled");
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Read-only login pool failed");LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(20000);HashMapHolder<GameObject>::SetSize(20000);
        auto& table=LootTemplates_Gameobject.*Access(LootTableNameTag{});auto original=table;table+=" WHERE Entry=239692";auto rows=(LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();table=original;Require(rows==1,"Actual egg loot row missing");
        for(bool horde:{false,true}){
            auto session=new WorldSession(0,"Native egg regression",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
            Map map(1220,300,9998773,DIFFICULTY_NONE);LootPlayer player(session,map,horde?890012:890011,{5478.f,470.f,150.f,0.f});player.m_mailsLoaded=true;
            if(horde){player.SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_ORC);player.setFaction(2);}
            uint32 guid=horde?840000:830000;CheckEgg(map,player,guid);session->SetPlayer(nullptr);
        }
        LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS native egg reaction and loot admission\n";return 0;
    }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}

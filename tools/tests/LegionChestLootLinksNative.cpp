// Actual matching World templates and authored loot, native GetLootId,
// Player::SendLoot and FillLoot. Only source placement/player context and the
// zero-link counterfactual/data delta are controlled; no item storage or save.
#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "GameObject.h"
#include <fstream>
#include "NativeChestEntries.inc"
struct ChestLootObject : GameObject
{
    ~ChestLootObject(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
static void CheckChest(Player& player,Map& map,uint32 entry,uint32& guid)
{
    auto money=player.GetMoney();
    auto info=const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(entry));
    Require(info&&info->type==GAMEOBJECT_TYPE_CHEST&&info->ScriptId==0&&info->AIName.empty(),"Selected chest acquired custom script/type");
#ifdef CHEST_LOOT_LIVE
    Require(info->GetLootId()==entry,"Actual live template loot link missing");
#else
    Require(info->GetLootId()==0||info->GetLootId()==entry,"Pending chest fixture refuses foreign loot link");
#endif
    Require(LootTemplates_Gameobject.HaveLootFor(entry),"Actual authored chest loot table missing");
    for(bool repaired:{false,true})
    {
        info->chest.chestLoot=repaired?entry:0; // Exact declared data delta/counterfactual, private memory only.
        ChestLootObject chest;auto pos=player.GetPosition();
        Require(chest.Create(++guid,entry,&map,1,pos,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&chest)),"Native chest creation/map admission failed");
        chest.Update(5000);Require(chest.getLootState()==GO_READY,"Native chest initialization differs");
        player.SendLoot(chest.GetGUID(),LOOT_CORPSE);
        Loot* loot=chest.IsPersonal()?&player.personalLoot[chest.GetGUID()]:&chest.loot;
        Require((loot->GetLootOwner()==&player)==repaired,"Actual native SendLoot/FillLoot did not distinguish zero link from repaired link");
        if(!repaired)Require(loot->items.empty()&&loot->quest_items.empty(),"Zero link unexpectedly generated authored loot");
        // Test teardown only: a real DoLootRelease intentionally autostores
        // personal rewards. This unauthenticated generation test must not
        // invoke inventory/mail/character paths or pretend that consuming a
        // personal container is an idempotent reopen.
        player.SetLootGUID(ObjectGuid::Empty);player.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_LOOTING);
        player.RemoveLoot(chest.GetGUID());chest.loot.clear();
    }
    info->chest.chestLoot=entry;
    Require(player.GetMoney()==money,"Generation test unexpectedly granted money");
    for(uint8 slot=EQUIPMENT_SLOT_START;slot<player.GetInventoryEndSlot();++slot)
        Require(!player.GetItemByPos(INVENTORY_SLOT_BAG_0,slot),"Generation test unexpectedly stored an inventory item");
    std::cout<<"CASE native chest="<<entry<<" actual zero-link skips FillLoot; repaired authored link generates native loot; native generation PASS\n";
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeNative(argc,argv);opcodeTable.Initialize();LoadLootInputs();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Read-only login pool failed");LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(20000);HashMapHolder<GameObject>::SetSize(20000);
        std::string ids;for(auto entry:ChestEntries){if(!ids.empty())ids+=",";ids+=std::to_string(entry);}
        auto& table=LootTemplates_Gameobject.*Access(LootTableNameTag{});auto original=table;table+=" WHERE Entry IN("+ids+") ORDER BY Entry,Item";auto rows=(LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();table=original;Require(rows>=std::size(ChestEntries),"Actual authored loot rows missing");
        // Socketless private session: never run its account-write destructor.
        auto session=new WorldSession(0,"Native chest loot-link regression",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        {
            Map map(1220,300,9998771,DIFFICULTY_NONE);LootPlayer player(session,map,890001,{5478.f,470.f,150.f,0.f});
            player.m_mailsLoaded=true; // Declared synthetic login state: empty mailbox, no real character/mail query.
            uint32 guid=820000;
            for(auto entry:ChestEntries)CheckChest(player,map,entry,guid);
            session->SetPlayer(nullptr);
        }
        LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
        std::cout<<"PASS"<<std::size(ChestEntries)<<" native distinct chest loot links, paired zero-link counterfactuals and full native SendLoot/FillLoot generation; positions/admission context controlled, no probability/item/quest grants or character saves/client traversal\n";return 0;
    }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}

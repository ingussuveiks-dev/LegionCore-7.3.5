#include "CharacterService.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

namespace
{
constexpr uint32 ItemHearthstone = 6948;
constexpr uint32 ItemBoostBag = 142075;
constexpr uint8 BoostBagCount = 4;
}

CharacterService* CharacterService::instance()
{
    static CharacterService instance;
    return &instance;
}

void CharacterService::SetRename(Player* player)
{
    player->SetAtLoginFlag(AT_LOGIN_RENAME);

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
    stmt->setUInt16(0, AT_LOGIN_RENAME);
    stmt->setUInt64(1, player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

void CharacterService::ChangeFaction(Player* player)
{
    player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
    stmt->setUInt16(0, AT_LOGIN_CHANGE_FACTION);
    stmt->setUInt64(1, player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

void CharacterService::ChangeRace(Player* player)
{
    player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
    stmt->setUInt16(0, AT_LOGIN_CHANGE_RACE);
    stmt->setUInt64(1, player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

void CharacterService::Customize(Player* player)
{
    player->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
    stmt->setUInt16(0, AT_LOGIN_CUSTOMIZE);
    stmt->setUInt64(1, player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

std::vector<uint32> CharacterService::GetBoostItems(Player const* player, uint16 specializationId, uint8 targetLevel) const
{
    if (!player)
        return { };

    std::vector<uint32> result = GetBoostItems(player->getClass(), specializationId, targetLevel);
    if (player->HasItemCount(ItemHearthstone, 1, true))
        result.erase(std::remove(result.begin(), result.end(), ItemHearthstone), result.end());

    return result;
}

std::vector<uint32> CharacterService::GetBoostItems(uint8 classId, uint16 specializationId, uint8 targetLevel) const
{
    std::vector<uint32> result;

    // Purpose 3 contains the old level-90 loadout; purpose 6 is the Legion
    // level-100 boost loadout. ItemSpec data removes weapons/trinkets intended
    // for another specialization while keeping shared armor and utility items.
    uint8 loadoutPurpose = targetLevel < 100 ? 3 : 6;
    for (uint32 itemId : sDB2Manager.GetLowestIdItemLoadOutItemsBy(classId, loadoutPurpose))
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate || !itemTemplate->IsUsableBySpecialization(specializationId, targetLevel, false))
            continue;

        result.push_back(itemId);
    }

    return result;
}

bool CharacterService::Boost(Player* player, uint16 specializationId, uint8 targetLevel)
{
    if (!player || !player->GetSession())
        return false;

    ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(specializationId);
    if (!specialization || specialization->ClassID != player->getClass())
        return false;

    uint8 maxLevel = uint8(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    if (!targetLevel || targetLevel > maxLevel || player->getLevel() >= targetLevel)
        return false;

    std::vector<uint32> boostItems = GetBoostItems(player, specializationId, targetLevel);
    if (boostItems.empty())
    {
        TC_LOG_ERROR("battlepay", "Character boost has no loadout for class %u, specialization %u and level %u",
            player->getClass(), specializationId, targetLevel);
        return false;
    }

    player->SetPrimarySpecialization(specializationId);
    player->ForceChangeTalentGroup(specializationId);
    player->GiveLevel(targetLevel);
    player->SetUInt32Value(PLAYER_FIELD_XP, 0);

    // Riding and Cold Weather Flying are part of the legacy boost service.
    player->CastSpell(player, 34092, true);
    player->CastSpell(player, 54198, true);
    player->ModifyMoney(5000000);

    std::vector<Item*> oldEquipmentToMail;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (Item* equippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemPosCountVec destination;
            if (player->CanStoreItem(NULL_BAG, NULL_SLOT, destination, equippedItem, false) == EQUIP_ERR_OK)
            {
                player->RemoveItem(INVENTORY_SLOT_BAG_0, slot, true);
                player->StoreItem(destination, equippedItem, true);
            }
            else
                oldEquipmentToMail.push_back(equippedItem);
        }
    }

    if (!oldEquipmentToMail.empty())
    {
        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        MailDraft draft("Inventory Full: Old Equipment",
            "Your old equipment did not fit in your bags and was mailed to you before the character boost set was equipped.");

        for (Item* equippedItem : oldEquipmentToMail)
        {
            player->MoveItemFromInventory(equippedItem, true);
            equippedItem->DeleteFromInventoryDB(transaction);
            equippedItem->SaveToDB(transaction);
            draft.AddItem(equippedItem);
        }

        draft.SendMailTo(transaction, player, MailSender(player, MAIL_STATIONERY_GM),
            MailCheckMask(MAIL_CHECK_MASK_COPIED | MAIL_CHECK_MASK_RETURNED));
        CharacterDatabase.CommitTransaction(transaction);
    }

    std::vector<uint32> newItemsToMail;
    for (uint32 itemId : boostItems)
        if (!player->StoreNewItemInBestSlots(itemId, 1))
            newItemsToMail.push_back(itemId);

    if (!newItemsToMail.empty())
    {
        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        MailDraft draft("Inventory Full: Character Boost Items",
            "Some items from your specialization-specific character boost set did not fit in your bags and were mailed to you.");

        for (uint32 itemId : newItemsToMail)
        {
            if (Item* item = Item::CreateItem(itemId, 1, player))
            {
                item->SaveToDB(transaction);
                draft.AddItem(item);
            }
        }

        draft.SendMailTo(transaction, player, MailSender(player, MAIL_STATIONERY_GM),
            MailCheckMask(MAIL_CHECK_MASK_COPIED | MAIL_CHECK_MASK_RETURNED));
        CharacterDatabase.CommitTransaction(transaction);
    }

    // Bags are mailed separately so all four are guaranteed to arrive even
    // when the character's current inventory is completely full.
    {
        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        MailDraft draft("Level 100 Character Boost Bags",
            "Four 30-slot Imbued Silkweave Bags are included with your character boost.");
        for (uint8 count = 0; count < BoostBagCount; ++count)
        {
            if (Item* bag = Item::CreateItem(ItemBoostBag, 1, player))
            {
                bag->SaveToDB(transaction);
                draft.AddItem(bag);
            }
        }

        draft.SendMailTo(transaction, player, MailSender(player, MAIL_STATIONERY_GM),
            MailCheckMask(MAIL_CHECK_MASK_COPIED | MAIL_CHECK_MASK_RETURNED));
        CharacterDatabase.CommitTransaction(transaction);
    }

    player->SaveToDB();
    TC_LOG_INFO("battlepay", "Boosted character %s (%s) to level %u with specialization %u and %zu loadout items",
        player->GetName(), player->GetGUID().ToString().c_str(), targetLevel, specializationId, boostItems.size());
    return true;
}

bool CharacterService::BoostCharacter(WorldSession* session, ObjectGuid targetCharGuid, uint16 specializationId,
    uint8 targetLevel, std::vector<uint32>& boostItems)
{
    if (!session || targetCharGuid.IsEmpty())
        return false;

    CharacterInfo const* charInfo = sWorld->GetCharacterInfo(targetCharGuid);
    ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(specializationId);
    uint8 maxLevel = uint8(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    if (!charInfo || charInfo->AccountId != session->GetAccountId() || charInfo->Level >= targetLevel ||
        !targetLevel || targetLevel > maxLevel || !specialization || specialization->ClassID != charInfo->Class)
        return false;

    boostItems = GetBoostItems(charInfo->Class, specializationId, targetLevel);
    boostItems.erase(std::remove(boostItems.begin(), boostItems.end(), ItemHearthstone), boostItems.end());
    if (boostItems.empty())
    {
        TC_LOG_ERROR("battlepay", "Character boost has no loadout for class %u, specialization %u and level %u",
            charInfo->Class, specializationId, targetLevel);
        return false;
    }

    ObjectGuid::LowType guid = targetCharGuid.GetCounter();
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    transaction->PAppend("UPDATE characters SET level = %u, xp = 0, specialization = %u, "
        "lootspecialization = %u, money = money + 5000000, health = 4294967295, mana = 4294967295, "
        "at_login = at_login | %u WHERE guid = " UI64FMTD,
        targetLevel, specializationId, specializationId, uint16(AT_LOGIN_RESET_SPELLS | AT_LOGIN_RESET_TALENTS), guid);

    for (uint32 spellId : { 34092u, 54198u })
    {
        CharacterDatabasePreparedStatement* statement = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SPELL);
        statement->setUInt64(0, guid);
        statement->setUInt32(1, spellId);
        statement->setUInt8(2, 1);
        statement->setUInt8(3, 0);
        transaction->Append(statement);
    }

    // Offline characters cannot safely be re-equipped through Player APIs.
    // Deliver the specialization-filtered starter set by system mail instead
    // of destroying or orphaning the character's existing equipment.
    for (std::size_t begin = 0; begin < boostItems.size(); begin += MAX_MAIL_ITEMS)
    {
        MailDraft draft("Level 100 Character Boost",
            "Your specialization-specific Legion starter equipment is attached. Your previous equipment was left unchanged.");
        std::size_t end = std::min(begin + std::size_t(MAX_MAIL_ITEMS), boostItems.size());
        for (std::size_t index = begin; index < end; ++index)
        {
            if (Item* item = Item::CreateItem(boostItems[index], 1, nullptr))
            {
                item->SaveToDB(transaction);
                draft.AddItem(item);
            }
        }

        draft.SendMailTo(transaction, MailReceiver(guid),
            MailSender(MAIL_NORMAL, ObjectGuid::LowType(0), MAIL_STATIONERY_GM),
            MailCheckMask(MAIL_CHECK_MASK_COPIED | MAIL_CHECK_MASK_RETURNED));
    }

    MailDraft bagDraft("Level 100 Character Boost Bags",
        "Four 30-slot Imbued Silkweave Bags are included with your character boost.");
    for (uint8 count = 0; count < BoostBagCount; ++count)
    {
        if (Item* bag = Item::CreateItem(ItemBoostBag, 1, nullptr))
        {
            bag->SaveToDB(transaction);
            bagDraft.AddItem(bag);
        }
    }
    bagDraft.SendMailTo(transaction, MailReceiver(guid),
        MailSender(MAIL_NORMAL, ObjectGuid::LowType(0), MAIL_STATIONERY_GM),
        MailCheckMask(MAIL_CHECK_MASK_COPIED | MAIL_CHECK_MASK_RETURNED));

    CharacterDatabase.CommitTransaction(transaction);
    sWorld->UpdateCharacterInfoLevel(targetCharGuid, targetLevel);

    TC_LOG_INFO("battlepay", "Boosted offline character %s (%s) to level %u with specialization %u; mailed %zu items",
        charInfo->Name.c_str(), targetCharGuid.ToString().c_str(), targetLevel, specializationId, boostItems.size());
    return true;
}

void CharacterService::RestoreDeletedCharacter(WorldSession* session)
{
    session->AddAuthFlag(AT_AUTH_FLAG_RESTORE_DELETED_CHARACTER);
}


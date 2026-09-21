#include "CharacterService.h"
#include "CollectionMgr.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestData.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <set>

namespace
{
constexpr uint32 ItemHearthstone = 6948;
constexpr uint32 ItemBoostBag = 142075;
constexpr uint8 BoostBagCount = 4;
constexpr uint16 BoostFactionHorde = 1;
constexpr uint16 BoostFactionAlliance = 2;
constexpr uint16 BoostProfessionSkill = 700;

struct ProfessionDefinition
{
    uint16 SkillId;
    uint32 FirstRankSpell;
};

ProfessionDefinition const BoostProfessions[] =
{
    { SKILL_ALCHEMY,        2259  },
    { SKILL_BLACKSMITHING,  2018  },
    { SKILL_ENCHANTING,     7411  },
    { SKILL_ENGINEERING,    4036  },
    { SKILL_HERBALISM,      2366  },
    { SKILL_INSCRIPTION,    45357 },
    { SKILL_JEWELCRAFTING,  25229 },
    { SKILL_LEATHERWORKING, 2108  },
    { SKILL_MINING,         2575  },
    { SKILL_SKINNING,       8613  },
    { SKILL_TAILORING,      3908  }
};

struct BoostDestination
{
    WorldLocation Location;
    uint16 ZoneId;
};

bool ResolveBoostFaction(uint8 currentRace, uint16 requestedFaction, uint8& finalRace, uint16& faction)
{
    finalRace = currentRace;
    faction = requestedFaction;

    if (currentRace == RACE_PANDAREN_NEUTRAL)
    {
        if (requestedFaction == BoostFactionHorde)
            finalRace = RACE_PANDAREN_HORDE;
        else if (requestedFaction == BoostFactionAlliance)
            finalRace = RACE_PANDAREN_ALLIANCE;
        else
            return false;

        return true;
    }

    uint16 expectedFaction = Player::TeamForRace(currentRace) == HORDE ? BoostFactionHorde : BoostFactionAlliance;
    if (requestedFaction && requestedFaction != expectedFaction)
        return false;

    faction = expectedFaction;
    return true;
}

BoostDestination GetBoostDestination(uint16 faction)
{
    // The retail boost tutorial used faction gunships, but that tutorial is
    // not scripted in this core. Place the player at the first working Legion
    // introduction hand-in instead, ready to continue the Broken Shore chain.
    if (faction == BoostFactionHorde)
        return { WorldLocation(1, 1352.49f, -4396.55f, 29.2122f, 2.28638f), 14 };

    return { WorldLocation(0, -8495.11f, 1078.70f, 18.0276f, 1.56232f), 1519 };
}

void LearnBoostFactionLanguages(Player* player, uint16 faction)
{
    player->learnSpell(faction == BoostFactionHorde ? 669 : 668, false);
    player->learnSpell(108127, false);
}

void GrantBoostTravel(Player* player, uint16 faction)
{
    // Level 100 boosts received Artisan Riding and the regional flying
    // licences available before achievement-gated Pathfinder flying.
    player->CastSpell(player, 34093, true);  // Artisan Riding
    player->CastSpell(player, 54198, true);  // Cold Weather Flying
    player->CastSpell(player, 90269, true);  // Flight Master's License
    player->CastSpell(player, 115916, true); // Wisdom of the Four Winds

    uint32 factionMount = faction == BoostFactionHorde ? 32243 : 32235;
    if (!player->GetCollectionMgr()->HasMount(factionMount))
        player->GetCollectionMgr()->AddMount(factionMount);
}

void GrantSkippedStartingZoneAbilities(Player* player)
{
    switch (player->getClass())
    {
        case CLASS_DEATH_KNIGHT:
            player->learnSpell(50977, false); // Death Gate
            player->CastSpell(player, 53431, true); // Runeforging
            if (!player->GetCollectionMgr()->HasMount(48778))
                player->GetCollectionMgr()->AddMount(48778); // Acherus Deathcharger
            break;
        default:
            break;
    }

    switch (player->getRace())
    {
        case RACE_WORGEN:
            player->learnSpell(68996, false); // Two Forms
            if (!player->GetCollectionMgr()->HasMount(87840))
                player->GetCollectionMgr()->AddMount(87840); // Running Wild
            break;
        case RACE_GOBLIN:
            player->learnSpell(69046, false); // Pack Hobgoblin
            break;
        default:
            break;
    }
}

void AddLegionIntroductionQuest(Player* player, uint16 faction)
{
    uint32 questId = faction == BoostFactionHorde ? 43926 : 40519;
    if (player->GetQuestStatus(questId) != QUEST_STATUS_NONE)
        return;

    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(questId))
        if (player->CanTakeQuest(quest, false) && player->CanAddQuest(quest, false))
            player->AddQuestAndCheckCompletion(quest, nullptr);
}

uint32 GetHighestBoostProfessionRankSpell(uint16 skillId, uint16& skillStep)
{
    uint32 bestSpell = 0;
    uint16 bestMaximum = 0;
    skillStep = 0;

    for (SkillLineAbilityEntry const* ability : sSkillLineAbilityStore)
    {
        if (ability->SkillLine != skillId)
            continue;

        SpellLearnSkillNode const* learnedSkill = sSpellMgr->GetSpellLearnSkill(ability->Spell);
        if (!learnedSkill || learnedSkill->skill != skillId || !learnedSkill->maxvalue ||
            learnedSkill->maxvalue > BoostProfessionSkill || learnedSkill->maxvalue <= bestMaximum)
            continue;

        bestSpell = ability->Spell;
        bestMaximum = learnedSkill->maxvalue;
        skillStep = learnedSkill->step;
    }

    return bestSpell;
}

void SetBoostProfession(Player* player, ProfessionDefinition const& profession)
{
    if (!player->HasSkill(profession.SkillId))
        player->learnSpell(profession.FirstRankSpell, false);

    uint16 skillStep = player->GetSkillStep(profession.SkillId);
    if (uint32 rankSpell = GetHighestBoostProfessionRankSpell(profession.SkillId, skillStep))
        player->learnSpell(rankSpell, false);

    player->SetSkill(profession.SkillId, skillStep, BoostProfessionSkill, BoostProfessionSkill);
}

void GrantVeteranProfessionBoost(Player* player)
{
    std::vector<ProfessionDefinition const*> professions;
    for (ProfessionDefinition const& profession : BoostProfessions)
        if (player->HasSkill(profession.SkillId))
            professions.push_back(&profession);

    // Retail selected an armor-appropriate pair when an eligible veteran
    // character had no primary professions; it did not show a profession picker.
    if (professions.empty())
    {
        switch (player->getClass())
        {
            case CLASS_PRIEST:
            case CLASS_MAGE:
            case CLASS_WARLOCK:
                professions = { &BoostProfessions[10], &BoostProfessions[2] }; // Tailoring, Enchanting
                break;
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
                professions = { &BoostProfessions[1], &BoostProfessions[8] }; // Blacksmithing, Mining
                break;
            default:
                professions = { &BoostProfessions[7], &BoostProfessions[9] }; // Leatherworking, Skinning
                break;
        }
    }

    for (ProfessionDefinition const* profession : professions)
        SetBoostProfession(player, *profession);

    ProfessionDefinition firstAid = { SKILL_FIRST_AID, 3273 };
    SetBoostProfession(player, firstAid);

    uint32 maxPrimaryProfessions = sWorld->getIntConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL);
    player->SetFreePrimaryProfessions(professions.size() >= maxPrimaryProfessions ?
        0 : uint16(maxPrimaryProfessions - professions.size()));
}

void EquipBoostLoadout(Player* player, std::vector<uint32> const& boostItems)
{
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
}

void MailBoostBags(Player* player)
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

void PopulateBoostActionBar(Player* player)
{
    std::vector<uint32> spells;
    std::set<uint32> addedSpells;

    if (std::vector<SpecializationSpellsEntry const*> const* specializationSpells =
        sDB2Manager.GetSpecializationSpells(player->GetSpecializationId()))
    {
        for (SpecializationSpellsEntry const* specializationSpell : *specializationSpells)
            if (player->HasActiveSpell(specializationSpell->SpellID) && addedSpells.insert(specializationSpell->SpellID).second)
                spells.push_back(specializationSpell->SpellID);
    }

    std::sort(spells.begin(), spells.end(), [](uint32 left, uint32 right)
    {
        SpellInfo const* leftInfo = sSpellMgr->GetSpellInfo(left);
        SpellInfo const* rightInfo = sSpellMgr->GetSpellInfo(right);
        uint32 leftLevel = leftInfo ? leftInfo->SpellLevel : 0;
        uint32 rightLevel = rightInfo ? rightInfo->SpellLevel : 0;
        return leftLevel != rightLevel ? leftLevel < rightLevel : left < right;
    });

    std::vector<uint32> remainingSpells;
    for (auto const& knownSpell : player->GetSpellMapConst())
        if (player->HasActiveSpell(knownSpell.first) && addedSpells.insert(knownSpell.first).second)
            remainingSpells.push_back(knownSpell.first);

    std::sort(remainingSpells.begin(), remainingSpells.end(), [](uint32 left, uint32 right)
    {
        SpellInfo const* leftInfo = sSpellMgr->GetSpellInfo(left);
        SpellInfo const* rightInfo = sSpellMgr->GetSpellInfo(right);
        uint32 leftLevel = leftInfo ? leftInfo->SpellLevel : 0;
        uint32 rightLevel = rightInfo ? rightInfo->SpellLevel : 0;
        return leftLevel != rightLevel ? leftLevel < rightLevel : left < right;
    });
    spells.insert(spells.end(), remainingSpells.begin(), remainingSpells.end());

    bool changed = false;
    for (uint32 spellId : spells)
        changed |= player->AddSpellToActionBarIfAppropriate(spellId, false);

    if (changed)
        player->SendActionButtons(1);
}
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

bool CharacterService::Boost(Player* player, uint16 specializationId, uint8 targetLevel, uint16 factionChoice)
{
    if (!player || !player->GetSession())
        return false;

    uint8 finalRace = player->getRace();
    uint16 faction = 0;
    if (!ResolveBoostFaction(player->getRace(), factionChoice, finalRace, faction))
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

    if (finalRace != player->getRace())
    {
        player->SetRace(finalRace);
        player->setFactionForRace(finalRace);
        sWorld->UpdateCharacterInfo(player->GetGUID(), player->GetName(), GENDER_NONE, finalRace);
    }

    uint8 previousLevel = player->getLevel();
    LearnBoostFactionLanguages(player, faction);
    player->SetPrimarySpecialization(specializationId);
    player->SetLootSpecID(specializationId);
    player->ForceChangeTalentGroup(specializationId);
    player->GiveLevel(targetLevel);
    player->SetUInt32Value(PLAYER_FIELD_XP, 0);

    GrantBoostTravel(player, faction);
    GrantSkippedStartingZoneAbilities(player);
    if (previousLevel >= 60)
        GrantVeteranProfessionBoost(player);
    player->ModifyMoney(5000000);

    EquipBoostLoadout(player, boostItems);
    MailBoostBags(player);
    PopulateBoostActionBar(player);

    BoostDestination destination = GetBoostDestination(faction);
    player->SetHomebind(destination.Location, destination.ZoneId);
    player->TeleportTo(destination.Location);
    AddLegionIntroductionQuest(player, faction);
    player->RemoveAtLoginFlag(AT_LOGIN_FIRST);

    player->SaveToDB();
    TC_LOG_INFO("battlepay", "Boosted character %s (%s) to level %u with specialization %u, faction %u and %zu loadout items",
        player->GetName(), player->GetGUID().ToString().c_str(), targetLevel, specializationId, faction, boostItems.size());
    return true;
}

bool CharacterService::BoostCharacter(WorldSession* session, ObjectGuid targetCharGuid, uint16 specializationId,
    uint8 targetLevel, uint16 factionChoice, std::vector<uint32>& boostItems)
{
    if (!session || targetCharGuid.IsEmpty())
        return false;

    CharacterInfo const* charInfo = sWorld->GetCharacterInfo(targetCharGuid);
    ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(specializationId);
    uint8 maxLevel = uint8(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    if (!charInfo || charInfo->AccountId != session->GetAccountId() || charInfo->Level >= targetLevel ||
        !targetLevel || targetLevel > maxLevel || !specialization || specialization->ClassID != charInfo->Class)
        return false;

    uint8 finalRace = charInfo->Race;
    uint16 faction = 0;
    if (!ResolveBoostFaction(charInfo->Race, factionChoice, finalRace, faction))
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
    BoostDestination destination = GetBoostDestination(faction);
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    AtLoginFlags boostFlags = AtLoginFlags(AT_LOGIN_RESET_SPELLS | AT_LOGIN_RESET_TALENTS | AT_LOGIN_CHARACTER_BOOST);
    if (charInfo->Level >= 60)
        boostFlags = AtLoginFlags(boostFlags | AT_LOGIN_BOOST_PROFESSIONS);

    transaction->PAppend("UPDATE characters SET race = %u, level = %u, xp = 0, activespec = %u, specialization = %u, "
        "lootspecialization = %u, money = money + 5000000, health = 4294967295, mana = 4294967295, instance_id = 0, "
        "map = %u, zone = %u, position_x = %f, position_y = %f, position_z = %f, orientation = %f, "
        "at_login = (at_login & ~%u) | %u WHERE guid = " UI64FMTD,
        finalRace, targetLevel, specialization->OrderIndex, specializationId, specializationId,
        destination.Location.GetMapId(), destination.ZoneId, destination.Location.GetPositionX(),
        destination.Location.GetPositionY(), destination.Location.GetPositionZ(), destination.Location.GetOrientation(),
        uint16(AT_LOGIN_FIRST), uint16(boostFlags), guid);

    transaction->PAppend("REPLACE INTO character_homebind (guid, mapId, zoneId, posX, posY, posZ) "
        "VALUES (" UI64FMTD ", %u, %u, %f, %f, %f)", guid, destination.Location.GetMapId(), destination.ZoneId,
        destination.Location.GetPositionX(), destination.Location.GetPositionY(), destination.Location.GetPositionZ());

    std::vector<uint32> boostSpells = { 108127u };
    boostSpells.push_back(faction == BoostFactionHorde ? 669u : 668u);
    for (uint32 spellId : boostSpells)
    {
        CharacterDatabasePreparedStatement* statement = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SPELL);
        statement->setUInt64(0, guid);
        statement->setUInt32(1, spellId);
        statement->setUInt8(2, 1);
        statement->setUInt8(3, 0);
        transaction->Append(statement);
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
    if (finalRace != charInfo->Race)
        sWorld->UpdateCharacterInfo(targetCharGuid, charInfo->Name, GENDER_NONE, finalRace);

    TC_LOG_INFO("battlepay", "Prepared offline character %s (%s) for level %u boost with specialization %u, faction %u and %zu loadout items",
        charInfo->Name.c_str(), targetCharGuid.ToString().c_str(), targetLevel, specializationId, faction, boostItems.size());
    return true;
}

void CharacterService::FinalizeBoost(Player* player)
{
    if (!player || !player->HasAtLoginFlag(AT_LOGIN_CHARACTER_BOOST))
        return;

    std::vector<uint32> boostItems = GetBoostItems(player, player->GetSpecializationId(), player->getLevel());
    if (boostItems.empty())
    {
        TC_LOG_ERROR("battlepay", "Cannot finalize boost for character %s (%s): no loadout for specialization %u",
            player->GetName(), player->GetGUID().ToString().c_str(), player->GetSpecializationId());
        return;
    }

    uint16 faction = Player::TeamForRace(player->getRace()) == HORDE ? BoostFactionHorde : BoostFactionAlliance;
    LearnBoostFactionLanguages(player, faction);
    GrantBoostTravel(player, faction);
    GrantSkippedStartingZoneAbilities(player);
    if (player->HasAtLoginFlag(AT_LOGIN_BOOST_PROFESSIONS))
        GrantVeteranProfessionBoost(player);
    AddLegionIntroductionQuest(player, faction);

    EquipBoostLoadout(player, boostItems);
    PopulateBoostActionBar(player);
    player->RemoveAtLoginFlag(AtLoginFlags(AT_LOGIN_CHARACTER_BOOST | AT_LOGIN_BOOST_PROFESSIONS));
    player->SaveToDB();

    TC_LOG_INFO("battlepay", "Finalized level %u boost for character %s (%s): equipped %zu items and populated action bars",
        player->getLevel(), player->GetName(), player->GetGUID().ToString().c_str(), boostItems.size());
}

void CharacterService::RestoreDeletedCharacter(WorldSession* session)
{
    session->AddAuthFlag(AT_AUTH_FLAG_RESTORE_DELETED_CHARACTER);
}


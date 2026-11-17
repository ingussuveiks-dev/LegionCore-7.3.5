#pragma once

#include "Player.h"
#include "Item.h"
#include "DB2Stores.h"

// Inventory outcome only. Callers must still authenticate their real pickup,
// quest attempt, encounter and presentation; ownership cannot replace them.
namespace ArtifactAcquisitionInventory
{
inline bool OtherSpecialization(Player const* player, ItemTemplate const* item)
{
    ArtifactEntry const* artifact = item ? sArtifactStore.LookupEntry(item->GetArtifactID()) : nullptr;
    ChrSpecializationEntry const* spec = artifact ? sChrSpecializationStore.LookupEntry(artifact->ChrSpecializationID) : nullptr;
    ChrSpecializationEntry const* current = player ? sChrSpecializationStore.LookupEntry(player->GetSpecializationId()) : nullptr;
    return player && spec && current && current->ClassID == player->getClass() && spec->ClassID == player->getClass() &&
        spec->ID != player->GetSpecializationId();
}

inline bool StoredForOtherSpecialization(Player const* player, Item* parent, uint32 childEntry = 0)
{
    if (!player || !parent || !parent->GetCount() || parent->GetOwnerGUID() != player->GetGUID() ||
        player->GetItemByEntry(parent->GetEntry()) != parent || parent->IsEquipped() ||
        !OtherSpecialization(player, parent->GetTemplate()))
        return false;
    if (!childEntry)
        return true;
    Item* child = player->GetChildItemByGuid(parent->GetChildItem());
    return child && child->GetEntry() == childEntry && child->GetCount() &&
        child->GetOwnerGUID() == player->GetGUID() && child->GetGuidValue(ITEM_FIELD_CREATOR) == parent->GetGUID() &&
        child->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_CHILD);
}

inline bool Ready(Player const* player, uint32 entry, uint8 slot = EQUIPMENT_SLOT_MAINHAND,
    uint32 childEntry = 0, uint8 childSlot = EQUIPMENT_SLOT_OFFHAND)
{
    Item* parent = player ? player->GetItemByEntry(entry) : nullptr;
    if (!parent || !parent->GetCount() || parent->GetOwnerGUID() != player->GetGUID())
        return false;
    if (StoredForOtherSpecialization(player, parent, childEntry))
        return true;
    if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) != parent)
        return false;
    Item* child = childEntry ? player->GetItemByPos(INVENTORY_SLOT_BAG_0, childSlot) : nullptr;
    return !childEntry || (child && child->GetEntry() == childEntry && child->GetOwnerGUID() == player->GetGUID() &&
        parent->GetChildItem() == child->GetGUID());
}
}

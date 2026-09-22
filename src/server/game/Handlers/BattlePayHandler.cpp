/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BattlePayPackets.h"
#include "BattlePayMgr.h"
#include "BattlePayData.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "DatabaseEnv.h"
#include <set>

auto GetBagsFreeSlots = [](Player* player) -> uint32
{
    uint32 freeBagSlots = 0;
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
        if (auto bag = player->GetBagByPos(i))
            freeBagSlots += bag->GetFreeSlots();

    uint8 inventoryEnd = INVENTORY_SLOT_ITEM_START + player->GetInventorySlotCount();
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < inventoryEnd; i++)
        if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            ++freeBagSlots;

    return freeBagSlots;
};

auto CharacterCanReceiveProduct = [](CharacterInfo const* characterInfo, Battlepay::Product const* product) -> bool
{
    if (!characterInfo || !product)
        return false;

    // A neutral Pandaren has no Alliance/Horde identity yet. Services whose
    // result depends on that identity must wait until the Wandering Isle
    // faction choice has been completed.
    if (characterInfo->Race == RACE_PANDAREN_NEUTRAL &&
        (product->WebsiteType == Battlepay::Faction ||
         product->WebsiteType == Battlepay::Race))
        return false;

    uint32 classMask = 1u << (characterInfo->Class - 1);
    uint64 raceMask = UI64LIT(1) << (characterInfo->Race - 1);
    if (product->ClassMask && !(product->ClassMask & classMask))
        return false;

    for (Battlepay::ProductItem const& item : product->Items)
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item.ItemID);
        if (!itemTemplate)
            return false;

        if (itemTemplate->AllowableClass && !(itemTemplate->AllowableClass & classMask))
            return false;
        if (itemTemplate->AllowableRace && !(itemTemplate->AllowableRace & raceMask))
            return false;

        std::set<uint32> learnedSpells;
        for (ItemEffectEntry const* itemEffect : itemTemplate->Effects)
        {
            if (!itemEffect || itemEffect->TriggerType != ITEM_SPELLTRIGGER_LEARN_SPELL_ID || !itemEffect->SpellID)
                continue;

            learnedSpells.insert(itemEffect->SpellID);
            SpellLearnSpellMapBounds bounds = sSpellMgr->GetSpellLearnSpellMapBounds(itemEffect->SpellID);
            for (SpellLearnSpellMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
                learnedSpells.insert(itr->second.spell);
        }

        for (uint32 spellId : learnedSpells)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;

            if (spellInfo->HasAttribute(SPELL_ATTR7_HORDE_ONLY) && !(raceMask & RACEMASK_HORDE))
                return false;
            if (spellInfo->HasAttribute(SPELL_ATTR7_ALLIANCE_ONLY) && !(raceMask & RACEMASK_ALLIANCE))
                return false;
        }
    }

    return true;
};

auto SendStoreFailureMessage = [](WorldSession* session, char const* message) -> void
{
    if (Player* player = session->GetPlayer())
    {
        std::ostringstream data;
        data << message;
        player->SendCustomMessage("Store purchase failed ", data);
    }
};

auto SendStartPurchaseResponse = [](WorldSession* session, Battlepay::Purchase const& purchase, Battlepay::Error const& result) -> void
{
    WorldPackets::BattlePay::StartPurchaseResponse response;
    response.PurchaseID = purchase.PurchaseID;
    response.ClientToken = purchase.ClientToken;
    response.PurchaseResult = result;
    session->SendPacket(response.Write());
};

auto SendPurchaseUpdate = [](WorldSession* session, Battlepay::Purchase const& purchase, uint32 result) -> void
{
    WorldPackets::BattlePay::PurchaseUpdate packet;
    WorldPackets::BattlePay::BattlePayPurchase data;
    data.PurchaseID = purchase.PurchaseID;
    data.UnkLong = 0;
    data.UnkLong2 = 0;
    data.Status = purchase.Status;
    data.ResultCode = result;
    data.ProductID = purchase.ProductID;
    data.UnkInt = purchase.ServerToken;
    data.WalletName = session->GetBattlePayMgr()->GetDefaultWalletName();
    packet.Purchase.emplace_back(data);
    session->SendPacket(packet.Write());
};

void WorldSession::HandleGetPurchaseListQuery(WorldPackets::BattlePay::GetPurchaseListQuery& /*packet*/)
{
    WorldPackets::BattlePay::PurchaseListResponse packet;
    //uint32 Result = 0;
    //std::vector<WorldPackets::BattlePay::BattlePayPurchase> Purchase;
    SendPacket(packet.Write());
}

void WorldSession::HandleUpdateVasPurchaseStates(WorldPackets::BattlePay::UpdateVasPurchaseStates& /*packet*/)
{
    if (!GetBattlePayMgr()->IsAvailable())
        return;

    std::vector<WorldPackets::BattlePay::BattlePayDistributionObject> distributions =
        GetBattlePayMgr()->BuildPendingBoostDistributions();
    WorldPackets::BattlePay::DistributionListResponse response;
    response.DistributionObject = distributions;
    SendPacket(response.Write());

    for (WorldPackets::BattlePay::BattlePayDistributionObject const& distribution : distributions)
    {
        WorldPackets::BattlePay::DistributionUpdate update;
        update.DistributionObject = distribution;
        SendPacket(update.Write());
    }
}

void WorldSession::HandleBattlePayDistributionAssign(WorldPackets::BattlePay::DistributionAssignToTarget& packet)
{
    if (!GetBattlePayMgr()->IsAvailable())
        return;

    GetBattlePayMgr()->AssignDistributionToCharacter(packet.TargetCharacter, packet.DistributionID, packet.ProductID, packet.SpecializationID, packet.ChoiceID);
}

void WorldSession::HandleGetProductList(WorldPackets::BattlePay::GetProductList& /*packet*/)
{
    if (!GetBattlePayMgr()->IsAvailable())
        return;

    GetBattlePayMgr()->SendProductList();
    GetBattlePayMgr()->SendPointsBalance();

    std::vector<WorldPackets::BattlePay::BattlePayDistributionObject> distributions =
        GetBattlePayMgr()->BuildPendingBoostDistributions();
    WorldPackets::BattlePay::DistributionListResponse response;
    response.DistributionObject = distributions;
    SendPacket(response.Write());

    for (WorldPackets::BattlePay::BattlePayDistributionObject const& distribution : distributions)
    {
        WorldPackets::BattlePay::DistributionUpdate update;
        update.DistributionObject = distribution;
        SendPacket(update.Write());
    }
}

auto MakePurchase = [](ObjectGuid targetCharacter, uint32 clientToken , uint32 productID, WorldSession* session) -> void
{
    if (!session || !session->GetBattlePayMgr()->IsAvailable())
        return;

    auto mgr = session->GetBattlePayMgr();

    Player* player = session->GetPlayer();

    auto accountID = session->GetAccountId();

    Battlepay::Purchase purchase;
    purchase.ProductID = productID;
    purchase.ClientToken = clientToken;
    purchase.TargetCharacter = targetCharacter;
    purchase.Status = Battlepay::UpdateStatus::Loading;
    purchase.DistributionId = mgr->GenerateNewDistributionId();

    auto characterInfo = sWorld->GetCharacterInfo(targetCharacter);
    if (!characterInfo)
    {
        SendStartPurchaseResponse(session, purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    if (characterInfo->AccountId != accountID)
    {
        SendStartPurchaseResponse(session, purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    if (!sBattlePayDataStore->ProductExist(productID))
    {
        SendStartPurchaseResponse(session, purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    Battlepay::ProductGroup* group = sBattlePayDataStore->GetProductGroupForProductId(purchase.ProductID);
    if (!group)
    {
        SendStartPurchaseResponse(session, purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    auto const* product = sBattlePayDataStore->GetProduct(purchase.ProductID);
    if (!product)
    {
        SendStartPurchaseResponse(session, purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    if (!CharacterCanReceiveProduct(characterInfo, product))
    {
        SendStartPurchaseResponse(session, purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    purchase.CurrentPrice = product->CurrentPriceFixedPoint;

    mgr->RegisterStartPurchase(purchase);

    auto accountBalance = session->GetTokenBalance(group->TokenType);
    auto purchaseData = mgr->GetPurchase();

    if (!accountBalance)
    {
        SendStartPurchaseResponse(session, *purchaseData, Battlepay::Error::InsufficientBalance);
        return;
    }

    if (accountBalance < static_cast<int64>(purchaseData->CurrentPrice))
    {
        SendStartPurchaseResponse(session, *purchaseData, Battlepay::Error::InsufficientBalance);
        return;
    }

    if (player && player->GetGUID() == targetCharacter && !product->Items.empty())
    {
        uint32 requiredSlots = 0;
        for (Battlepay::ProductItem const& item : product->Items)
            requiredSlots += item.Quantity;

        if (requiredSlots > GetBagsFreeSlots(player))
        {
            SendStoreFailureMessage(session,
                sObjectMgr->GetTrinityString(Battlepay::String::NotEnoughFreeBagSlots, session->GetSessionDbLocaleIndex()));
            SendStartPurchaseResponse(session, *purchaseData, Battlepay::Error::PurchaseDenied);
            return;
        }
    }

    if (!product->ScriptName.empty())
    {
        std::string reason;
        if (!sScriptMgr->BattlePayCanBuy(session, product, reason))
        {
            SendStoreFailureMessage(session, reason.c_str());
            SendStartPurchaseResponse(session, *purchaseData, Battlepay::Error::PurchaseDenied);
            return;
        }
    }

    for (auto itr : product->Items)
    {
        if (mgr->AlreadyOwnProduct(itr.ItemID, targetCharacter))
        {
            SendStoreFailureMessage(session,
                sObjectMgr->GetTrinityString(Battlepay::String::YouAlreadyOwnThat, session->GetSessionDbLocaleIndex()));
            SendStartPurchaseResponse(session, *purchaseData, Battlepay::Error::PurchaseDenied);
            return;
        }
    }

    purchaseData->PurchaseID = mgr->GenerateNewPurchaseID();
    purchaseData->ServerToken = urand(0, 0xFFFFFFF);
    //purchaseData->Status = Battlepay::UpdateStatus::Ready; ?

    SendStartPurchaseResponse(session, *purchaseData, Battlepay::Error::Ok);
    SendPurchaseUpdate(session, *purchaseData, Battlepay::Error::Ok);

    WorldPackets::BattlePay::ConfirmPurchase confirmPurchase;
    confirmPurchase.PurchaseID = purchaseData->PurchaseID;
    confirmPurchase.ServerToken = purchaseData->ServerToken;
    session->SendPacket(confirmPurchase.Write());
};

void WorldSession::HandleBattlePayStartPurchase(WorldPackets::BattlePay::StartPurchase& packet)
{
    MakePurchase(packet.TargetCharacter, packet.ClientToken, packet.ProductID, this);
}

void WorldSession::HandleBattlePayPurchaseProduct(WorldPackets::BattlePay::PurchaseProduct& packet)
{
    MakePurchase(packet.TargetCharacter, packet.ClientToken, packet.ProductID, this);
}

void WorldSession::HandleBattlePayConfirmPurchase(WorldPackets::BattlePay::ConfirmPurchaseResponse& packet)
{
    if (!GetBattlePayMgr()->IsAvailable())
        return;

    packet.ClientCurrentPriceFixedPoint /= Battlepay::g_CurrencyPrecision;

    auto purchase = GetBattlePayMgr()->GetPurchase();
    if (!purchase)
        return;

    if (purchase->Lock)
    {
        SendPurchaseUpdate(this, *purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    if (purchase->ServerToken != packet.ServerToken || !packet.ConfirmPurchase || purchase->CurrentPrice != packet.ClientCurrentPriceFixedPoint)
    {
        SendPurchaseUpdate(this, *purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    Player* player = GetPlayer();

    Battlepay::ProductGroup* group = sBattlePayDataStore->GetProductGroupForProductId(purchase->ProductID);
    if (!group)
    {
        SendPurchaseUpdate(this, *purchase, Battlepay::Error::PurchaseDenied);
        return;
    }
    
    if (GetTokenBalance(group->TokenType) < static_cast<int64>(purchase->CurrentPrice))
    {
        SendPurchaseUpdate(this, *purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    auto const* product = sBattlePayDataStore->GetProduct(purchase->ProductID);
    if (!product)
    {
        SendPurchaseUpdate(this, *purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    CharacterInfo const* characterInfo = sWorld->GetCharacterInfo(purchase->TargetCharacter);
    if (!characterInfo || characterInfo->AccountId != GetAccountId() || !CharacterCanReceiveProduct(characterInfo, product))
    {
        SendPurchaseUpdate(this, *purchase, Battlepay::Error::PurchaseDenied);
        return;
    }

    purchase->Lock = true;
    purchase->Status = Battlepay::UpdateStatus::Finish;

    if (!product->ScriptName.empty())
    {
        std::string reason;
        if (!sScriptMgr->BattlePayCanBuy(this, product, reason))
        {
            SendStoreFailureMessage(this, reason.c_str());
            SendPurchaseUpdate(this, *purchase, Battlepay::Error::PaymentFailed);
            return;
        }
    }

    if (player && player->GetGUID() == purchase->TargetCharacter && !product->Items.empty())
    {
        uint32 requiredSlots = 0;
        for (Battlepay::ProductItem const& item : product->Items)
            requiredSlots += item.Quantity;

        if (requiredSlots > GetBagsFreeSlots(player))
        {
            SendStoreFailureMessage(this,
                sObjectMgr->GetTrinityString(Battlepay::String::NotEnoughFreeBagSlots, GetSessionDbLocaleIndex()));
            SendStartPurchaseResponse(this, *purchase, Battlepay::Error::PurchaseDenied);
            return;
        }
    }

    for (auto itr : product->Items)
    {
        if (GetBattlePayMgr()->AlreadyOwnProduct(itr.ItemID, purchase->TargetCharacter))
        {
            SendStoreFailureMessage(this,
                sObjectMgr->GetTrinityString(Battlepay::String::YouAlreadyOwnThat, GetSessionDbLocaleIndex()));
            SendStartPurchaseResponse(this, *purchase, Battlepay::Error::PurchaseDenied);
            return;
        }
    }

    SendPurchaseUpdate(this, *purchase, Battlepay::Error::Other);

    if (ChangeTokenBalanceAndSave(group->TokenType, -int64(purchase->CurrentPrice)))
        GetBattlePayMgr()->ProcessDelivery(purchase);
}

void WorldSession::HandleBattlePayAckFailedResponse(WorldPackets::BattlePay::BattlePayAckFailedResponse& /*packet*/)
{
}

void WorldSession::HandleBattlePayQueryClassTrialResult(WorldPackets::BattlePay::BattlePayQueryClassTrialResult& /*packet*/)
{
}

void WorldSession::HandleBattlePayTrialBoostCharacter(WorldPackets::BattlePay::BattlePayTrialBoostCharacter& packet)
{
    // Some 7.3.5 UI paths submit the final character/spec selection through
    // the class-trial opcode even for a paid boost.
    if (!HasAuthFlag(AT_AUTH_FLAG_100_LVL_UP) || !GetBattlePayMgr()->IsAvailable())
        return;

    std::vector<WorldPackets::BattlePay::BattlePayDistributionObject> distributions =
        GetBattlePayMgr()->BuildPendingBoostDistributions();
    if (distributions.empty())
        return;

    GetBattlePayMgr()->AssignDistributionToCharacter(packet.Character,
        distributions.front().DistributionID, Battlepay::Level100BoostProductId, uint16(packet.SpecializationID), 0);
}

void WorldSession::HandleBattlePayPurchaseDetailsResponse(WorldPackets::BattlePay::BattlePayPurchaseDetailsResponse& packet)
{
    WorldPackets::BattlePay::BattlePayPurchaseUnk response;
    response.UnkInt = 0;
    response.Key = "";
    response.UnkByte = packet.UnkByte;
    SendPacket(response.Write());
}

void WorldSession::HandleBattlePayPurchaseUnkResponse(WorldPackets::BattlePay::BattlePayPurchaseUnkResponse& /*packet*/)
{
    auto purchaseData = GetBattlePayMgr()->GetPurchase();
    SendPurchaseUpdate(this, *purchaseData, Battlepay::Error::Ok);
}

void WorldSession::SendDisplayPromo(int32 promotionID /*= 0*/)
{
    SendPacket(WorldPackets::BattlePay::DisplayPromotion(promotionID).Write());

    if (!GetBattlePayMgr()->IsAvailable())
        return;

    //SendPacket(WorldPackets::BattlePay::BattlepayUnk(2).Write());

    /*
    auto player = GetPlayer();
    auto const& product = sBattlePayDataStore->GetProduct(109);
    WorldPackets::BattlePay::DistributionListResponse packet;
    packet.Result = Battlepay::Error::Ok;

    WorldPackets::BattlePay::BattlePayDistributionObject data;
    data.TargetPlayer;
    data.DistributionID = GetBattlePayMgr()->GenerateNewDistributionId();
    data.PurchaseID = GetBattlePayMgr()->GenerateNewPurchaseID();
    data.Status = Battlepay::DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE;
    data.ProductID = 109;
    data.TargetVirtualRealm = 0;
    data.TargetNativeRealm = 0;
    data.Revoked = false;

    WorldPackets::BattlePay::BattlePayProduct pProduct;
    pProduct.ProductID = product.ProductID;
    pProduct.Flags = product.Flags;
    pProduct.Type = product.Type;
    //pProduct.UnkBits Optional<uint16> ;
    //pProduct.UnkInt1 = 0;
    //pProduct.DisplayId = 0;
    //pProduct.ItemId = 0;
    //pProduct.UnkInt4 = 0;
    //pProduct.UnkInt5 = 0;
    //pProduct.UnkString = "";
    //pProduct.UnkBit = false;

    for (auto& itr : product.Items)
    {
        WorldPackets::BattlePay::ProductItem pItem;
        pItem.ID = itr.ID;
        pItem.ItemID = product.Items.size() > 1 ? 0 : itr.ItemID; ///< Disable tooltip for packs (client handle only one tooltip).
        pItem.Quantity = itr.Quantity;
        //pItem.UnkInt1 = 0;
        //pItem.UnkInt2 = 0;
        //pItem.UnkByte = 0;
        pItem.HasPet = GetBattlePayMgr()->AlreadyOwnProduct(itr.ItemID);
        pItem.PetResult = itr.PetResult;

        auto dataP = GetBattlePayMgr()->WriteDisplayInfo(itr.DisplayInfoID, GetSessionDbLocaleIndex());
        if (std::get<0>(dataP))
        {
            pItem.DisplayInfo.emplace();
            pItem.DisplayInfo = std::get<1>(dataP);
        }

        pProduct.Items.emplace_back(pItem);
    }

    auto dataP = GetBattlePayMgr()->WriteDisplayInfo(product.DisplayInfoID, GetSessionDbLocaleIndex());
    if (std::get<0>(dataP))
    {
        pProduct.DisplayInfo.emplace();
        pProduct.DisplayInfo = std::get<1>(dataP);
    }

    data.Product.emplace();
    data.Product = pProduct;

    packet.DistributionObject.emplace_back(data);

    SendPacket(packet.Write());
    */
}

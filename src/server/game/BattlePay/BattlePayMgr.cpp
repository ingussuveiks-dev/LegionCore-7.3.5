/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "ObjectMgr.h"
#include "BattlePayMgr.h"
#include "WorldSession.h"
#include "Player.h"
#include "BattlePayData.h"
#include "DatabaseEnv.h"
#include "QuestData.h"
#include "LoginQueryHolder.h"
#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "PetBattle.h"
#include "BattlePetData.h"
#include "CharacterService.h"
#include "CollectionMgr.h"
#include "Chat.h"
#include "Item.h"
#include "Mail.h"
#include "SpellMgr.h"
#include <set>

using namespace Battlepay;

BattlepayManager::BattlepayManager(WorldSession* session)
{
    _session = session;
    _walletName = "Donation points";
    _purchaseIDCount = 0;
    _distributionIDCount = 0;
}

BattlepayManager::~BattlepayManager() = default;

void BattlepayManager::RegisterStartPurchase(Purchase purchase)
{
    _actualTransaction = purchase;
}

uint64 BattlepayManager::GenerateNewPurchaseID()
{
    return uint64(0x1E77800000000000 | ++_purchaseIDCount);
}

uint64 BattlepayManager::GenerateNewDistributionId()
{
    return uint64(0x1E77800000000000 | ++_distributionIDCount);
}

Purchase* BattlepayManager::GetPurchase()
{
    return &_actualTransaction;
}

std::string const& BattlepayManager::GetDefaultWalletName() const
{
    return _walletName;
}

BattlePayCurrency BattlepayManager::GetShopCurrency() const
{
    /// @TODO: Move that to config files
    return Krw;
}

bool BattlepayManager::IsAvailable() const
{
    if (AccountMgr::IsModeratorAccount(_session->GetSecurity()))
        return true;

    return sWorld->getBoolConfig(CONFIG_FEATURE_SYSTEM_BPAY_STORE_ENABLED);
}

std::string Product::Serialize() const
{
    std::string res;
    res += "Expansion: " + std::to_string(CURRENT_EXPANSION);
    res += ", Type: " + std::to_string(WebsiteType);
    res += ", Quantity: " + std::to_string(1);
    res += ", IngameShop: " + std::to_string(1);
    res += ", CustomData: " + sScriptMgr->BattlePayGetCustomData(*this);

    uint32 idx = 0;
    for (auto const& itr : Items)
    {
        std::string iconName;
        if (auto itemTemplate = sObjectMgr->GetItemTemplate(itr.ItemID))
        {
            if (auto fileDataId = sDB2Manager.GetItemDIconFileDataId(itr.ItemID))
                iconName = std::to_string(fileDataId);

            switch (WebsiteType)
            {
            case Item:
                res += ", Item(ItemID: " + std::to_string(itr.ItemID) + ", Quality: " + std::to_string(itemTemplate->GetQuality()) + ", Icon: " + iconName + ")";
                break;
            case PackItems:
                res += ", Pack(Icon: " + iconName + ", ItemsEntry: " + std::to_string(itr.ItemID) + ", Num: " + std::to_string(idx) + ")";
                break;
            default:
                break;
            }
        }

        idx++;
    }

    return res;
}

void BattlepayManager::ProcessDelivery(Purchase* purchase)
{
    Player* player = _session->GetPlayer();

    auto const* product = sBattlePayDataStore->GetProduct(purchase->ProductID);
    if (!product)
        return;

    switch (product->WebsiteType)
    {
    case Battlepay::BattlePet:
    case Battlepay::Item:
    case Battlepay::PackItems:
    case Battlepay::ItemMount:
    {
        bool deliverDirectly = player && player->GetGUID() == purchase->TargetCharacter;
        if (deliverDirectly)
        {
            for (Battlepay::ProductItem const& productItem : product->Items)
                player->AddItem(productItem.ItemID, productItem.Quantity);
            break;
        }

        CharacterInfo const* characterInfo = sWorld->GetCharacterInfo(purchase->TargetCharacter);
        if (!characterInfo || characterInfo->AccountId != _session->GetAccountId())
            break;

        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        std::vector<::Item*> items;
        for (Battlepay::ProductItem const& productItem : product->Items)
        {
            for (uint32 count = 0; count < productItem.Quantity; ++count)
            {
                if (::Item* item = ::Item::CreateItem(productItem.ItemID, 1, nullptr))
                {
                    item->SaveToDB(transaction);
                    items.push_back(item);
                }
            }
        }

        for (std::size_t begin = 0; begin < items.size(); begin += MAX_MAIL_ITEMS)
        {
            MailDraft draft("BattlePay Shop Delivery",
                "Your purchase for this character is attached. Thank you for using the in-game shop.");
            std::size_t end = std::min(begin + std::size_t(MAX_MAIL_ITEMS), items.size());
            for (std::size_t index = begin; index < end; ++index)
                draft.AddItem(items[index]);

            draft.SendMailTo(transaction, MailReceiver(purchase->TargetCharacter.GetCounter()),
                MailSender(MAIL_NORMAL, ObjectGuid::LowType(0), MAIL_STATIONERY_GM),
                MailCheckMask(MAIL_CHECK_MASK_COPIED | MAIL_CHECK_MASK_RETURNED));
        }

        CharacterDatabase.CommitTransaction(transaction);
        break;
    }
    case Rename:
        if (player)
            sCharacterService->SetRename(player);
        break;
    case Faction:
        if (player)
            sCharacterService->ChangeFaction(player);
        break;
    case DeletedCharacter:
        sCharacterService->RestoreDeletedCharacter(_session);
        break;
    case Customization:
        if (player)
            sCharacterService->Customize(player);
        break;
    case Race:
        if (player)
            sCharacterService->ChangeRace(player);
        break;
    case CharacterBoost:
    {
        _session->AddAuthFlag(AT_AUTH_FLAG_100_LVL_UP);
        purchase->Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE;
        std::vector<WorldPackets::BattlePay::BattlePayDistributionObject> distributions = BuildPendingBoostDistributions();

        WorldPackets::BattlePay::DistributionListResponse listResponse;
        listResponse.DistributionObject = distributions;
        _session->SendPacket(listResponse.Write());
        for (WorldPackets::BattlePay::BattlePayDistributionObject const& distribution : distributions)
        {
            WorldPackets::BattlePay::DistributionUpdate update;
            update.DistributionObject = distribution;
            _session->SendPacket(update.Write());
        }

        TC_LOG_INFO("battlepay", "Account %u purchased level 100 boost product %u; entitlement saved for character selection",
            _session->GetAccountId(), purchase->ProductID);
        return; // Delivery continues after the client chooses a character and specialization.
    }

    //case Category:
    //    break;
    //case Battlepay::Spell:
    //    break;
    //case Currency:
    //    break;
    //case GuildRename:
    //    break;
    //case Gold:
    //    break;
    //case Level:
    //    break;
    //case PremadeCharacter:
    //    break;
    //case RealmTransfer:
    //    break;
    //case ExpansionTransfer:
    //    break;
    //case Premium:
    //    break;
    //case PackItems:
    //    break;
    //case ItemProfession:
    //    break;
    //case Transmogrification:
    //    break;
    //case CategoryProfession:
    //    break;
    //case CategoryPremade:
    //    break;
    //case ItemMount:
    //    break;
    //case CategoryCharacterManagement:
    //    break;
    //case CategoryRealmTransfer:
    //    break;
    //case CategoryExpansionTransfer:
    //    break;
    //case CategoryGold:
    //    break;
    default:
        break;
    }

    if (!product->ScriptName.empty())
        sScriptMgr->OnBattlePayProductDelivery(_session, product);
}

bool BattlepayManager::AlreadyOwnProduct(uint32 itemId, ObjectGuid targetCharacter) const
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return true;

    Player* player = _session->GetPlayer();
    if (targetCharacter.IsEmpty() && player)
        targetCharacter = player->GetGUID();

    if (player && (targetCharacter.IsEmpty() || player->GetGUID() == targetCharacter))
    {
        if (player->GetCollectionMgr()->HasToy(itemId))
            return true;

        if (itemTemplate->GetMaxCount() > 0 && player->HasItemCount(itemId, 1, true))
            return true;
    }

    if (CharacterDatabase.PQuery("SELECT 1 FROM `account_toys` WHERE `accountId` = %u AND `itemId` = %u LIMIT 1",
        _session->GetAccountId(), itemId))
        return true;

    std::set<uint32> learnedSpells;
    std::set<uint32> battlePetSpecies;
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
        if (sDB2Manager.GetMount(spellId))
        {
            if (player && player->GetCollectionMgr()->HasMount(spellId))
                return true;

            if (CharacterDatabase.PQuery("SELECT 1 FROM `account_mounts` WHERE `account` = %u AND `spell` = %u LIMIT 1",
                _session->GetAccountId(), spellId))
                return true;
        }

        if (BattlePetSpeciesEntry const* species = sDB2Manager.GetSpeciesBySpell(spellId))
            battlePetSpecies.insert(species->ID);

        if (!targetCharacter.IsEmpty() && CharacterDatabase.PQuery(
            "SELECT 1 FROM `character_spell` WHERE `guid` = " UI64FMTD " AND `spell` = %u LIMIT 1",
            targetCharacter.GetCounter(), spellId))
            return true;
    }

    for (uint32 speciesId : battlePetSpecies)
    {
        if (player && player->GetBattlePetCountForSpecies(speciesId))
            return true;

        if (CharacterDatabase.PQuery("SELECT 1 FROM `account_battlepet` WHERE `account` = %u AND `species` = %u LIMIT 1",
            _session->GetAccountId(), speciesId))
            return true;
    }

    if (!targetCharacter.IsEmpty() && itemTemplate->GetMaxCount() > 0 && CharacterDatabase.PQuery(
        "SELECT 1 FROM `item_instance` WHERE `owner_guid` = " UI64FMTD " AND `itemEntry` = %u LIMIT 1",
        targetCharacter.GetCounter(), itemId))
        return true;

    return false;
}

auto BattlepayManager::ProductFilter(Product product) -> bool
{
    auto player = _session->GetPlayer();
    if (!player)
    {
        switch (product.WebsiteType)
        {
        case Battlepay::Item:
        case PackItems:
        case ItemMount:
        case Battlepay::BattlePet:
        case CharacterBoost:
            return true;
        default:
            return false;
        }
    }

    if (product.ClassMask && (player->getClassMask() & product.ClassMask) == 0)
        return false;

    for (auto& itr : product.Items)
    {
        if (AlreadyOwnProduct(itr.ItemID))
            return false;

        if (auto itemTemplate = sObjectMgr->GetItemTemplate(itr.ItemID))
        {
            if (itemTemplate->AllowableClass && (itemTemplate->AllowableClass & player->getClassMask()) == 0)
                return false;

            if (itemTemplate->AllowableRace && (itemTemplate->AllowableRace & player->getRaceMask()) == 0)
                return false;

            if (itemTemplate->GetMinFactionID() && uint32(player->GetReputationRank(itemTemplate->GetMinFactionID())) < itemTemplate->GetMinReputation())
                return false;

            for (auto effectData : itemTemplate->Effects)
            {
                if (effectData->SpellID != 0 && effectData->TriggerType == ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
                {
                    if (auto spellInfo = sSpellMgr->GetSpellInfo(effectData->SpellID))
                    {
                        if (spellInfo->HasAttribute(SPELL_ATTR7_HORDE_ONLY) && (player->getRaceMask() & RACEMASK_HORDE) == 0)
                            return false;

                        if (spellInfo->HasAttribute(SPELL_ATTR7_ALLIANCE_ONLY) && (player->getRaceMask() & RACEMASK_ALLIANCE) == 0)
                            return false;
                    }
                }
            }
        }
    }

    return true;
};

void BattlepayManager::SendProductList()
{
    WorldPackets::BattlePay::ProductListResponse response;
    if (!IsAvailable())
    {
        response.Result = ProductListResult::LockUnk1;
        _session->SendPacket(response.Write());
        return;
    }

    auto const& player = _session->GetPlayer();
    auto const& localeIndex = _session->GetSessionDbLocaleIndex();

    response.Result = ProductListResult::Available;
    response.ProductList.CurrencyID = GetShopCurrency();

    for (auto& itr : sBattlePayDataStore->GetProductGroups())
    {
        if (!player && itr.IngameOnly)
            continue;

        if (itr.OwnsTokensOnly && _session->GetTokenBalance(itr.TokenType) <= 0)
            continue;

        WorldPackets::BattlePay::BattlePayProductGroup pGroup;
        pGroup.GroupID = itr.GroupID;
        pGroup.IconFileDataID = itr.IconFileDataID;
        pGroup.Ordering = itr.Ordering;
        pGroup.Flags = itr.Flags;
        pGroup.IsAvailableDescription = "";
        pGroup.DisplayType = itr.DisplayType;

        auto name = itr.Name;
        if (auto productLocale = sBattlePayDataStore->GetProductGroupLocale(itr.GroupID))
            ObjectMgr::GetLocaleString(productLocale->Name, localeIndex, name);
        pGroup.Name = name;
        response.ProductList.ProductGroup.emplace_back(pGroup);
    }

    for (auto const& itr : sBattlePayDataStore->GetShopEntries())
    {
        Battlepay::ProductGroup* productGroup = sBattlePayDataStore->GetProductGroup(itr.GroupID);
        if (!productGroup)
            continue;

        Product const* shopProduct = sBattlePayDataStore->GetProduct(itr.ProductID);
        if (!shopProduct || !ProductFilter(*shopProduct))
            continue;

        if (!player && productGroup->IngameOnly)
            continue;

        if (productGroup->OwnsTokensOnly && _session->GetTokenBalance(productGroup->TokenType) <= 0)
            continue;

        WorldPackets::BattlePay::BattlePayShopEntry sEntry;
        sEntry.EntryID = itr.EntryID;
        sEntry.GroupID = itr.GroupID;
        sEntry.ProductID = itr.ProductID;
        sEntry.Ordering = itr.Ordering;
        sEntry.VasServiceType = itr.Flags;
        sEntry.StoreDeliveryType = itr.BannerType;

        auto data = WriteDisplayInfo(itr.DisplayInfoID, localeIndex);
        if (std::get<0>(data))
        {
            sEntry.DisplayInfo.emplace();
            sEntry.DisplayInfo = std::get<1>(data);
        }

        response.ProductList.Shop.emplace_back(sEntry);
    }

    for (auto const& itr : sBattlePayDataStore->GetProducts())
    {
        auto const& product = itr.second;
        if (!ProductFilter(product))
            continue;

        Battlepay::ProductGroup* productGroup = sBattlePayDataStore->GetProductGroupForProductId(product.ProductID);
        if (!productGroup)
            continue;

        if (!player && productGroup->IngameOnly)
            continue;

        int64 tokenBalance = _session->GetTokenBalance(productGroup->TokenType);
        if (productGroup->OwnsTokensOnly && tokenBalance <= 0)
            continue;

        WorldPackets::BattlePay::ProductInfoStruct pInfo;
        pInfo.NormalPriceFixedPoint = product.NormalPriceFixedPoint * g_CurrencyPrecision;
        pInfo.CurrentPriceFixedPoint = product.CurrentPriceFixedPoint * g_CurrencyPrecision;
        pInfo.ProductID = product.ProductID;
        pInfo.ChoiceType = product.ChoiceType;
        pInfo.ProductIDs.emplace_back(product.ProductID);
        //std::vector<uint32> UnkInts;
        pInfo.UnkInt2 = 47; // 2 ?

        auto dataPI = WriteDisplayInfo(product.DisplayInfoID, localeIndex);
        if (std::get<0>(dataPI))
        {
            pInfo.DisplayInfo.emplace();
            pInfo.DisplayInfo = std::get<1>(dataPI);
        }

        response.ProductList.ProductInfo.emplace_back(pInfo);

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

        bool hideProductPrice = false;
        if (pInfo.DisplayInfo.has_value() && pInfo.DisplayInfo->Flags.has_value())
            hideProductPrice = *pInfo.DisplayInfo->Flags & BattlepayDisplayInfoFlag::HidePrice;
        bool hasEnoughTokens = tokenBalance >= static_cast<int64>(product.CurrentPriceFixedPoint);

        for (auto& item : product.Items)
        {
            WorldPackets::BattlePay::ProductItem pItem;
            pItem.ID = item.ID;
            pItem.ItemID = product.Items.size() > 1 ? 0 : item.ItemID; ///< Disable tooltip for packs (client handle only one tooltip).
            pItem.Quantity = item.Quantity;

            // Disable the buy button for products already owned, or when a
            // hidden-price product cannot be afforded.
            pItem.HasPet = AlreadyOwnProduct(item.ItemID) || (hideProductPrice && !hasEnoughTokens);
            pItem.PetResult = item.PetResult;

            auto dataP = WriteDisplayInfo(item.DisplayInfoID, localeIndex);
            if (std::get<0>(dataP))
                pItem.DisplayInfo = std::get<1>(dataP);

            pProduct.Items.emplace_back(pItem);
        }

        auto dataP = WriteDisplayInfo(product.DisplayInfoID, localeIndex);
        if (std::get<0>(dataP))
        {
            pProduct.DisplayInfo.emplace();
            pProduct.DisplayInfo = std::get<1>(dataP);
        }

        response.ProductList.Product.emplace_back(pProduct);
    }

    _session->SendPacket(response.Write());
}

std::tuple<bool, WorldPackets::BattlePay::ProductDisplayInfo> BattlepayManager::WriteDisplayInfo(uint32 displayInfoID, LocaleConstant localeIndex, uint32 productId /*= 0*/)
{
    auto GeneratePackDescription = [localeIndex](Product const* product) -> std::string
    {
        auto getQualityColor = [](uint32 quality) -> std::string
        {
            switch (quality)
            {
            case ITEM_QUALITY_POOR:
                return "|cff9d9d9d";
            case ITEM_QUALITY_NORMAL:
                return "|cffffffff";
            case ITEM_QUALITY_UNCOMMON:
                return "|cff1eff00";
            case ITEM_QUALITY_RARE:
                return "|cff0070dd";
            case ITEM_QUALITY_EPIC:
                return "|cffa335ee";
            case ITEM_QUALITY_LEGENDARY:
                return "|cffff8000";
            case ITEM_QUALITY_ARTIFACT:
                return "|cffe5cc80";
            case ITEM_QUALITY_HEIRLOOM:
                return "|cffe5cc80";
            default:
                return "|cffe5cc80";
            }
        };

        std::string res;
        for (auto itr : product->Items)
            if (auto itemTemplate = sObjectMgr->GetItemTemplate(itr.ItemID))
                res += getQualityColor(itemTemplate->GetQuality()) + itemTemplate->GetName()->Get(localeIndex) + "\n";
        return res;
    };

    auto info = WorldPackets::BattlePay::ProductDisplayInfo();
    if (!displayInfoID)
        return std::make_tuple(false, info);

    auto displayInfo = sBattlePayDataStore->GetDisplayInfo(displayInfoID);
    if (!displayInfo)
        return std::make_tuple(false, info);

    auto displayLocale = sBattlePayDataStore->GetDisplayInfoLocale(displayInfoID);

    info.Name1 = displayInfo->Name1;
    if (displayLocale)
        ObjectMgr::GetLocaleString(displayLocale->Name1, localeIndex, info.Name1);

    info.Name2 = displayInfo->Name2;
    if (displayLocale)
        ObjectMgr::GetLocaleString(displayLocale->Name2, localeIndex, info.Name2);

    info.Name3 = displayInfo->Name3;
    if (productId)
    {
        auto product = sBattlePayDataStore->GetProduct(productId);
        if (!product->Items.empty())
            info.Name3 = GeneratePackDescription(product);
    }
    else if (displayLocale)
        ObjectMgr::GetLocaleString(displayLocale->Name3, localeIndex, info.Name3);

    info.Name4 = displayInfo->Name4;
    if (displayLocale)
        ObjectMgr::GetLocaleString(displayLocale->Name4, localeIndex, info.Name4);

    if (displayInfo->CreatureDisplayInfoID != 0)
        info.CreatureDisplayInfoID = displayInfo->CreatureDisplayInfoID;

    if (auto visualsId = displayInfo->VisualsId)
    {
        if (auto visuals = sBattlePayDataStore->GetDisplayInfoVisuals(displayInfoID))
        {
            info.VisualsId = displayInfo->VisualsId;

            for (auto const& itr : *visuals)
            {
                WorldPackets::BattlePay::ProductDisplayVisualData visual;
                visual.DisplayId = itr.DisplayId;
                visual.VisualId = itr.VisualId;
                visual.ProductName = itr.ProductName;
                info.Visuals.emplace_back(visual);
            }
        }
    }

    if (displayInfo->Flags != 0)
        info.Flags = displayInfo->Flags;

    //Optional<uint32> UnkInt1;
    //Optional<uint32> UnkInt2;
    //Optional<uint32> UnkInt3;

    return std::make_tuple(true, info);
}

void BattlepayManager::SendPointsBalance()
{
    ChatHandler chatHandler(_session);
    if (!_session->GetPlayer())
        return;

    chatHandler.PSendSysMessage("Account name: %s", _session->GetAccountName());

    for (auto& tokenType : sBattlePayDataStore->GetTokenTypes())
    {
        int64 balance = _session->GetTokenBalance(tokenType.first);
        if (balance || tokenType.second.listIfNone)
            chatHandler.PSendSysMessage("%s: %d", tokenType.second.name, balance);
    }
}

void BattlepayManager::SendBattlePayDistribution(uint32 productId, uint8 status, uint64 distributionId, ObjectGuid targetGuid)
{
    WorldPackets::BattlePay::DistributionUpdate distributionBattlePay;
    auto product = sBattlePayDataStore->GetProduct(productId);
    if (!product || !product->ProductID)
        return;

    auto const& localeIndex = _session->GetSessionDbLocaleIndex();
    distributionBattlePay.DistributionObject.DistributionID = distributionId;
    distributionBattlePay.DistributionObject.Status = status;
    distributionBattlePay.DistributionObject.ProductID = productId;
    distributionBattlePay.DistributionObject.PurchaseID = _actualTransaction.PurchaseID;
    distributionBattlePay.DistributionObject.Revoked = false; // not needed for us

    if (!targetGuid.IsEmpty())
    {
        distributionBattlePay.DistributionObject.TargetPlayer = targetGuid;
        distributionBattlePay.DistributionObject.TargetVirtualRealm = GetVirtualRealmAddress();
        distributionBattlePay.DistributionObject.TargetNativeRealm = GetVirtualRealmAddress();
    }

    WorldPackets::BattlePay::BattlePayProduct productData;

    for (auto const& item : product->Items)
    {
        WorldPackets::BattlePay::ProductItem productItem;

        auto dataP = WriteDisplayInfo(item.DisplayInfoID, localeIndex);
        if (std::get<0>(dataP))
        {
            productItem.DisplayInfo.emplace();
            productItem.DisplayInfo = std::get<1>(dataP);
        }

        productItem.PetResult = item.PetResult;
        productItem.ID = item.ID;
        productItem.ItemID = item.ItemID;
        productItem.Quantity = item.Quantity;
        productItem.UnkInt1 = item.DisplayInfoID;
        productItem.UnkInt2 = 0;
        productItem.PetResult = 0;
        productItem.HasPet = item.HasPet;
        productData.Items.emplace_back(productItem);
    }

    auto dataP = WriteDisplayInfo(product->DisplayInfoID, localeIndex);
    if (std::get<0>(dataP))
    {
        productData.DisplayInfo.emplace();
        productData.DisplayInfo = std::get<1>(dataP);
    }

    if (product->WebsiteType == Battlepay::CharacterBoost)
        productData.UnkBits = product->ScriptName.find("level90") != std::string::npos ? 1 : 2;

    productData.ProductID = product->ProductID;
    productData.Flags = product->Flags;
    productData.UnkInt1 = 0;
    productData.DisplayId = product->DisplayInfoID;
    productData.ItemId = 0;
    productData.UnkInt4 = 0;
    productData.UnkInt5 = 0;
    productData.UnkString = "";
    productData.Type = 0;
    productData.UnkBit = false;

    distributionBattlePay.DistributionObject.Product = std::move(productData);
    _session->SendPacket(distributionBattlePay.Write());
}

std::vector<WorldPackets::BattlePay::BattlePayDistributionObject> BattlepayManager::BuildPendingBoostDistributions()
{
    std::vector<WorldPackets::BattlePay::BattlePayDistributionObject> distributions;
    if (!_session->HasAuthFlag(AT_AUTH_FLAG_100_LVL_UP))
        return distributions;

    uint32 constexpr productId = 109;
    Battlepay::Product const* product = sBattlePayDataStore->GetProduct(productId);
    if (!product || product->WebsiteType != Battlepay::CharacterBoost)
    {
        TC_LOG_ERROR("battlepay", "Cannot build pending level 100 boost for account %u: product %u is missing or invalid",
            _session->GetAccountId(), productId);
        return distributions;
    }

    if (_actualTransaction.ProductID != productId ||
        _actualTransaction.Status != DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE)
    {
        _actualTransaction = Battlepay::Purchase();
        _actualTransaction.ProductID = productId;
        _actualTransaction.DistributionId = GenerateNewDistributionId();
        _actualTransaction.PurchaseID = GenerateNewPurchaseID();
        _actualTransaction.Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE;
    }

    WorldPackets::BattlePay::BattlePayDistributionObject distribution;
    distribution.DistributionID = _actualTransaction.DistributionId;
    distribution.PurchaseID = _actualTransaction.PurchaseID;
    distribution.Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE;
    distribution.ProductID = productId;
    distribution.Revoked = false;

    LocaleConstant localeIndex = _session->GetSessionDbLocaleIndex();
    WorldPackets::BattlePay::BattlePayProduct productData;
    for (Battlepay::ProductItem const& item : product->Items)
    {
        WorldPackets::BattlePay::ProductItem productItem;
        auto displayInfo = WriteDisplayInfo(item.DisplayInfoID, localeIndex);
        if (std::get<0>(displayInfo))
            productItem.DisplayInfo = std::get<1>(displayInfo);

        productItem.ID = item.ID;
        productItem.ItemID = item.ItemID;
        productItem.Quantity = item.Quantity;
        productItem.UnkInt1 = item.DisplayInfoID;
        productItem.UnkInt2 = 0;
        productItem.PetResult = 0;
        productItem.HasPet = item.HasPet;
        productData.Items.emplace_back(std::move(productItem));
    }

    auto displayInfo = WriteDisplayInfo(product->DisplayInfoID, localeIndex);
    if (std::get<0>(displayInfo))
        productData.DisplayInfo = std::get<1>(displayInfo);

    productData.UnkBits = 2; // Legion client value for a level 100 boost.
    productData.ProductID = product->ProductID;
    productData.Flags = product->Flags;
    productData.DisplayId = product->DisplayInfoID;
    productData.UnkString = "";
    distribution.Product = std::move(productData);
    distributions.emplace_back(std::move(distribution));
    return distributions;
}

void BattlepayManager::AssignDistributionToCharacter(ObjectGuid const& targetCharGuid, uint64 distributionId, uint32 productId, uint16 specId, uint16 choiceId)
{
    Battlepay::Purchase* purchase = GetPurchase();
    Battlepay::Product const* product = sBattlePayDataStore->GetProduct(productId);
    CharacterInfo const* charInfo = sWorld->GetCharacterInfo(targetCharGuid);
    ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(specId);
    bool validFactionChoice = false;
    if (charInfo)
    {
        if (charInfo->Race == RACE_PANDAREN_NEUTRAL)
            validFactionChoice = choiceId == 1 || choiceId == 2;
        else
        {
            uint16 expectedChoice = Player::TeamForRace(charInfo->Race) == HORDE ? 1 : 2;
            validFactionChoice = choiceId == 0 || choiceId == expectedChoice;
        }
    }

    if (!purchase || !product || product->WebsiteType != CharacterBoost || productId != 109 ||
        !_session->HasAuthFlag(AT_AUTH_FLAG_100_LVL_UP) ||
        purchase->ProductID != productId || purchase->DistributionId != distributionId ||
        !charInfo || charInfo->AccountId != _session->GetAccountId() || charInfo->Level >= 100 ||
        !specialization || specialization->ClassID != charInfo->Class || !validFactionChoice)
    {
        TC_LOG_ERROR("battlepay", "Rejected character boost assignment for account %u, character %s, product %u, distribution " UI64FMTD ", specialization %u",
            _session->GetAccountId(), targetCharGuid.ToString().c_str(), productId, distributionId, specId);
        return;
    }

    std::vector<uint32> boostItems;
    bool boosted = false;
    if (Player* player = ObjectAccessor::GetObjectInOrOutOfWorld(targetCharGuid, static_cast<Player*>(nullptr)))
    {
        boostItems = sCharacterService->GetBoostItems(player, specId, 100);
        boosted = sCharacterService->Boost(player, specId, 100, choiceId);
    }
    else
        boosted = sCharacterService->BoostCharacter(_session, targetCharGuid, specId, 100, choiceId, boostItems);

    if (!boosted)
    {
        TC_LOG_ERROR("battlepay", "Level 100 boost delivery failed for account %u and character %s",
            _session->GetAccountId(), targetCharGuid.ToString().c_str());
        return;
    }

    WorldPackets::BattlePay::UpgradeStarted upgrade;
    upgrade.CharacterGUID = targetCharGuid;
    _session->SendPacket(upgrade.Write());

    WorldPackets::BattlePay::BattlePayCharacterUpgradeQueued queued;
    queued.Character = targetCharGuid;
    queued.EquipmentItems = boostItems;
    _session->SendPacket(queued.Write());

    WorldPackets::BattlePay::BattlePayStartDistributionAssignToTargetResponse assignResponse;
    assignResponse.DistributionID = distributionId;
    assignResponse.unkint1 = 0;
    assignResponse.unkint2 = 0;
    _session->SendPacket(assignResponse.Write());

    purchase->SpecializationID = specId;
    purchase->ChoiceID = choiceId;
    purchase->TargetCharacter = targetCharGuid;
    purchase->Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_FINISHED;

    SendBattlePayDistribution(productId, purchase->Status, distributionId, targetCharGuid);
    _session->RemoveAuthFlag(AT_AUTH_FLAG_100_LVL_UP);

    _session->SendCharacterEnum();

    TC_LOG_INFO("battlepay", "Account %u applied level 100 boost to %s with specialization %u and %zu starter items",
        _session->GetAccountId(), targetCharGuid.ToString().c_str(), specId, boostItems.size());

    // The finished update was already sent. Avoid resending it on every world tick.
    purchase->Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_NONE;
}

void BattlepayManager::Update(uint32 diff)
{
    auto& data = _actualTransaction;
    auto product = sBattlePayDataStore->GetProduct(data.ProductID);
    if (!product)
        return;

    switch (data.Status)
    {
    case DistributionStatus::BATTLE_PAY_DIST_STATUS_ADD_TO_PROCESS:
    {
        switch (product->WebsiteType)
        {
        case CharacterBoost:
        {
            auto const& player = ObjectAccessor::GetObjectInOrOutOfWorld(data.TargetCharacter, static_cast<Player*>(nullptr));
            if (!player)
                break;

            WorldPackets::BattlePay::BattlePayCharacterUpgradeQueued responseQueued;
            responseQueued.EquipmentItems = sCharacterService->GetBoostItems(player, data.SpecializationID, 100);
            responseQueued.Character = data.TargetCharacter;
            _session->SendPacket(responseQueued.Write());

            if (!sCharacterService->Boost(player, data.SpecializationID, 100, data.ChoiceID))
            {
                TC_LOG_ERROR("battlepay", "Character boost delivery failed for account %u and character %s",
                    _session->GetAccountId(), data.TargetCharacter.ToString().c_str());
                data.Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE;
                SendBattlePayDistribution(data.ProductID, data.Status, data.DistributionId);
                break;
            }

            data.Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_PROCESS_COMPLETE;
            SendBattlePayDistribution(data.ProductID, data.Status, data.DistributionId, data.TargetCharacter);
            break;
        }
        default:
            break;
        }
        break;
    }
    case DistributionStatus::BATTLE_PAY_DIST_STATUS_PROCESS_COMPLETE: //send SMSG_BATTLE_PAY_VAS_PURCHASE_STARTED
    {
        switch (product->WebsiteType)
        {
        case CharacterBoost:
        {
            data.Status = DistributionStatus::BATTLE_PAY_DIST_STATUS_FINISHED;
            SendBattlePayDistribution(data.ProductID, data.Status, data.DistributionId, data.TargetCharacter);
            break;
        }
        default:
            break;
        }
        break;
    }
    case DistributionStatus::BATTLE_PAY_DIST_STATUS_FINISHED:
    {
        switch (product->WebsiteType)
        {
        case CharacterBoost:
            SendBattlePayDistribution(data.ProductID, data.Status, data.DistributionId, data.TargetCharacter);
            break;
        default:
            break;
        }
        break;
    }
    case DistributionStatus::BATTLE_PAY_DIST_STATUS_AVAILABLE:
    case DistributionStatus::BATTLE_PAY_DIST_STATUS_NONE:
    default:
        break;
    }
}

//////
//
//  MILLENIUM-STUDIO
//  Copyright 2016 Millenium-studio SARL
//  All Rights Reserved.
//
//////

#include "PetBattle.h"
#include <limits>
#include "PetBattleAbilityEffect.h"
#include "WildBattlePet.h"
#include "DB2Store.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "GameTables.h"
#include "BattlePetPackets.h"
#include "InstanceScript.h"

#define PETBATTLE_UPDATE_INTERVAL 300

void BattlePet::Load(Field* fields)
{
    JournalID = ObjectGuid::Create<HighGuid::BattlePet>(fields[0].GetUInt64());
    Slot = fields[1].GetInt32();
    Name = fields[2].GetString();
    NameTimeStamp = fields[3].GetUInt32();
    Species = fields[4].GetUInt32();
    Quality = fields[5].GetUInt32();
    Breed = fields[6].GetUInt32();
    Level = fields[7].GetUInt32();
    XP = fields[8].GetUInt32();
    DisplayModelID = fields[9].GetUInt32();
    Health = fields[10].GetInt32();
    Flags = fields[11].GetUInt32();
    InfoPower = fields[12].GetInt32();
    InfoMaxHealth = fields[13].GetInt32();
    InfoSpeed = fields[14].GetInt32();
    InfoGender = fields[15].GetInt32();
    AccountID = fields[16].GetInt32();
    DeclinedNames[0] = fields[17].GetString();
    DeclinedNames[1] = fields[18].GetString();
    DeclinedNames[2] = fields[19].GetString();
    DeclinedNames[3] = fields[20].GetString();
    DeclinedNames[4] = fields[21].GetString();

    UpdateAbilities();

    needSave = false;
    needDelete = false;
}

void BattlePet::CloneFrom(std::shared_ptr<BattlePet> & battlePet)
{
    JournalID = battlePet->JournalID;
    Slot = battlePet->Slot;
    Name = battlePet->Name;
    NameTimeStamp = battlePet->NameTimeStamp;
    Species = battlePet->Species;
    Quality = battlePet->Quality;
    Breed = battlePet->Breed;
    Level = battlePet->Level;
    XP = battlePet->XP;
    DisplayModelID = battlePet->DisplayModelID;
    Health = battlePet->Health;
    Flags = battlePet->Flags;
    InfoPower = battlePet->InfoPower;
    InfoMaxHealth = battlePet->InfoMaxHealth;
    InfoSpeed = battlePet->InfoSpeed;
    InfoGender = battlePet->InfoGender;
    AccountID = battlePet->AccountID;
    DeclinedNames[0] = battlePet->DeclinedNames[0];
    DeclinedNames[1] = battlePet->DeclinedNames[1];
    DeclinedNames[2] = battlePet->DeclinedNames[2];
    DeclinedNames[3] = battlePet->DeclinedNames[3];
    DeclinedNames[4] = battlePet->DeclinedNames[4];

    for (uint8 i = 0; i < MAX_PETBATTLE_ABILITIES; ++i)
        Abilities[i] = battlePet->Abilities[i];
}

void BattlePet::Save(CharacterDatabaseTransaction& trans)
{
    if (!needSave || needDelete)
        return;

    CharacterDatabasePreparedStatement* statement = CharacterDatabase.GetPreparedStatement(CHAR_REP_PETBATTLE);
    statement->setUInt64(0, JournalID.GetCounter());
    statement->setInt32(1, Slot);
    statement->setString(2, Name);
    statement->setUInt32(3, NameTimeStamp);
    statement->setUInt32(4, Species);
    statement->setUInt32(5, Quality);
    statement->setUInt32(6, Breed);
    statement->setUInt32(7, Level);
    statement->setUInt32(8, XP);
    statement->setUInt32(9, DisplayModelID);
    statement->setInt32(10, Health);
    statement->setUInt32(11, Flags);
    statement->setInt32(12, InfoPower);
    statement->setInt32(13, InfoMaxHealth);
    statement->setInt32(14, InfoSpeed);
    statement->setInt32(15, InfoGender);
    statement->setInt32(16, AccountID);
    statement->setString(17, DeclinedNames[0]);
    statement->setString(18, DeclinedNames[1]);
    statement->setString(19, DeclinedNames[2]);
    statement->setString(20, DeclinedNames[3]);
    statement->setString(21, DeclinedNames[4]);
    trans->Append(statement);

    needSave = false;
}

void BattlePet::AddToPlayer(Player* player, CharacterDatabaseTransaction& trans)
{
    AccountID = player->GetSession()->GetAccountId();
    ObjectGuid::LowType guidlow = sObjectMgr->GetGenerator<HighGuid::BattlePet>()->Generate();
    JournalID = ObjectGuid::Create<HighGuid::BattlePet>(guidlow);

    CharacterDatabasePreparedStatement* statement = CharacterDatabase.GetPreparedStatement(CHAR_REP_PETBATTLE);
    statement->setUInt64(0, guidlow);
    statement->setInt32(1, Slot);
    statement->setString(2, Name);
    statement->setUInt32(3, NameTimeStamp);
    statement->setUInt32(4, Species);
    statement->setUInt32(5, Quality);
    statement->setUInt32(6, Breed);
    statement->setUInt32(7, Level);
    statement->setUInt32(8, XP);
    statement->setUInt32(9, DisplayModelID);
    statement->setInt32(10, Health);
    statement->setUInt32(11, Flags);
    statement->setInt32(12, InfoPower);
    statement->setInt32(13, InfoMaxHealth);
    statement->setInt32(14, InfoSpeed);
    statement->setInt32(15, InfoGender);
    statement->setInt32(16, AccountID);
    statement->setString(17, DeclinedNames[0]);
    statement->setString(18, DeclinedNames[1]);
    statement->setString(19, DeclinedNames[2]);
    statement->setString(20, DeclinedNames[3]);
    statement->setString(21, DeclinedNames[4]);
    trans->Append(statement);

    needSave = false;
}

void BattlePet::Remove(Player* /*player*/)
{
    CharacterDatabasePreparedStatement* statement = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETBATTLE);
    statement->setUInt64(0, JournalID.GetCounter());
    CharacterDatabase.Execute(statement);

    needDelete = true;
}

ObjectGuid::LowType BattlePet::AddToPlayer(Player* player)
{
    AccountID = player->GetSession()->GetAccountId();
    auto guidlow = sObjectMgr->GetGenerator<HighGuid::BattlePet>()->Generate();
    JournalID = ObjectGuid::Create<HighGuid::BattlePet>(guidlow);

    CharacterDatabasePreparedStatement* statement = CharacterDatabase.GetPreparedStatement(CHAR_REP_PETBATTLE);
    statement->setUInt64(0, guidlow);
    statement->setInt32(1, Slot);
    statement->setString(2, Name);
    statement->setUInt32(3, NameTimeStamp);
    statement->setUInt32(4, Species);
    statement->setUInt32(5, Quality);
    statement->setUInt32(6, Breed);
    statement->setUInt32(7, Level);
    statement->setUInt32(8, XP);
    statement->setUInt32(9, DisplayModelID);
    statement->setInt32(10, Health);
    statement->setUInt32(11, Flags);
    statement->setInt32(12, InfoPower);
    statement->setInt32(13, InfoMaxHealth);
    statement->setInt32(14, InfoSpeed);
    statement->setInt32(15, InfoGender);
    statement->setInt32(16, AccountID);
    statement->setString(17, DeclinedNames[0]);
    statement->setString(18, DeclinedNames[1]);
    statement->setString(19, DeclinedNames[2]);
    statement->setString(20, DeclinedNames[3]);
    statement->setString(21, DeclinedNames[4]);
    CharacterDatabase.Execute(statement);

    player->UpdateAchievementCriteria(CRITERIA_TYPE_COLLECT_BATTLEPET, 1);
    needSave = false;
    return guidlow;
}

void BattlePet::UpdateAbilities()
{
    for (auto& Abilitie : Abilities)
        Abilitie = 0;

    for (auto const& speciesXAbilityInfo : sBattlePetSpeciesXAbilityStore)
    {
        if (speciesXAbilityInfo->BattlePetSpeciesID != Species || speciesXAbilityInfo->RequiredLevel > Level || speciesXAbilityInfo->SlotEnum < 0 || speciesXAbilityInfo->SlotEnum >= MAX_PETBATTLE_ABILITIES)
            continue;

        if (speciesXAbilityInfo->RequiredLevel < 5)
            Abilities[speciesXAbilityInfo->SlotEnum] = speciesXAbilityInfo->BattlePetAbilityID;
        else
        {
            switch (speciesXAbilityInfo->SlotEnum)
            {
                case 0:
                    if (Flags & BATTLEPET_FLAG_ABILITY_1_SECOND)
                        Abilities[speciesXAbilityInfo->SlotEnum] = speciesXAbilityInfo->BattlePetAbilityID;
                    break;
                case 1:
                    if (Flags & BATTLEPET_FLAG_ABILITY_2_SECOND)
                        Abilities[speciesXAbilityInfo->SlotEnum] = speciesXAbilityInfo->BattlePetAbilityID;
                    break;
                case 2:
                    if (Flags & BATTLEPET_FLAG_ABILITY_3_SECOND)
                        Abilities[speciesXAbilityInfo->SlotEnum] = speciesXAbilityInfo->BattlePetAbilityID;
                    break;
                default:
                    break;
            }
        }
    }
}

void BattlePet::UpdateStats()
{
    int32 _InfoGender = InfoGender;
    int32 _InfoPower = InfoPower;
    int32 _InfoSpeed = InfoSpeed;
    int32 _InfoMaxHealth = InfoMaxHealth;

    int32* stats = &InfoPower;
    InfoGender = 0;

    for (size_t i = 0; i < 3; ++i)
        stats[i] = 1000;

    for (auto const& v : sBattlePetBreedStateStore)
    {
        if (v->BreedID != Breed)
            continue;

        switch (v->BattlePetStateID)
        {
            case BATTLEPET_STATE_Stat_Power:
            case BATTLEPET_STATE_Stat_Stamina:
            case BATTLEPET_STATE_Stat_Speed:
                stats[v->BattlePetStateID - BATTLEPET_STATE_Stat_Power] = v->Value;
                break;
            case BATTLEPET_STATE_Stat_Gender:
                InfoGender = v->Value;
                break;
            default:
                break;
        }
    }

    for (auto const& v : sBattlePetSpeciesStateStore)
    {
        if (v->SpeciesID != Species)
            continue;

        switch (v->BattlePetStateID)
        {
            case BATTLEPET_STATE_Stat_Power:
            case BATTLEPET_STATE_Stat_Stamina:
            case BATTLEPET_STATE_Stat_Speed:
                stats[v->BattlePetStateID - BATTLEPET_STATE_Stat_Power] += v->Value;
                break;
            default:
                break;
        }
    }

    float qualityFactor = 1.0f;
    if (auto qualityInfo = sBattlePetBreedQualityStore.LookupEntry(7 + Quality))
        qualityFactor = qualityInfo->StateMultiplier;

    for (size_t i = 0; i < 3; ++i)
        stats[i] = stats[i] * Level * qualityFactor;

    InfoPower = InfoPower / 100;
    InfoSpeed = InfoSpeed / 100;
    InfoMaxHealth = int32(floor((10000.0f + InfoMaxHealth * 5.0f) / 100.0f + 0.5));

    if (_InfoGender != InfoGender || _InfoPower != InfoPower || _InfoSpeed != InfoSpeed || _InfoMaxHealth != InfoMaxHealth || Health != InfoMaxHealth)
        needSave = true;

    Health = InfoMaxHealth;
}

BattlePetInstance::BattlePetInstance() : TeamID(0), ID(0), Cooldowns{}, Lockdowns{}, States{}, PetBattleInstance(nullptr), OldLevel(0), OldXP(0)
{
    OriginalBattlePet = std::shared_ptr<BattlePet>();
    OriginalCreature.Clear();
}

std::shared_ptr<BattlePetInstance> BattlePetInstance::CloneForBattle(std::shared_ptr<BattlePetInstance> const& battlePet)
{
    auto battlePetInstance = std::make_shared<BattlePetInstance>();
    battlePetInstance->JournalID = battlePet->JournalID;
    battlePetInstance->Slot = battlePet->Slot;
    battlePetInstance->Name = battlePet->Name;
    battlePetInstance->NameTimeStamp = battlePet->NameTimeStamp;
    battlePetInstance->Species = battlePet->Species;
    battlePetInstance->Quality = battlePet->Quality;
    battlePetInstance->Breed = battlePet->Breed;
    battlePetInstance->Level = battlePet->Level;
    battlePetInstance->XP = battlePet->XP;
    battlePetInstance->DisplayModelID = battlePet->DisplayModelID;
    battlePetInstance->Health = battlePet->Health;
    battlePetInstance->Flags = battlePet->Flags;
    battlePetInstance->InfoPower = battlePet->InfoPower;
    battlePetInstance->InfoMaxHealth = battlePet->InfoMaxHealth;
    battlePetInstance->InfoSpeed = battlePet->InfoSpeed;
    battlePetInstance->InfoGender = battlePet->InfoGender;
    battlePetInstance->AccountID = battlePet->AccountID;
    battlePetInstance->DeclinedNames[0] = battlePet->DeclinedNames[0];
    battlePetInstance->DeclinedNames[1] = battlePet->DeclinedNames[1];
    battlePetInstance->DeclinedNames[2] = battlePet->DeclinedNames[2];
    battlePetInstance->DeclinedNames[3] = battlePet->DeclinedNames[3];
    battlePetInstance->DeclinedNames[4] = battlePet->DeclinedNames[4];
    for (uint8 i = 0; i < MAX_PETBATTLE_ABILITIES; ++i)
        battlePetInstance->Abilities[i] = battlePet->Abilities[i];
    return battlePetInstance;
}

bool BattlePetInstance::IsAlive()
{
    return States[BATTLEPET_STATE_Is_Dead] == 0;
}

bool BattlePetInstance::CanAttack()
{
    return !States[BATTLEPET_STATE_turnLock] && !States[BATTLEPET_STATE_Mechanic_IsStunned];
}

int32 BattlePetInstance::GetMaxHealth()
{
    int32 base = int32(floor((10000.0f + 5.0f * States[BATTLEPET_STATE_Stat_Stamina] + States[BATTLEPET_STATE_maxHealthBonus] * States[BATTLEPET_STATE_Internal_InitialLevel]) / 100.0f + 0.5));
    int64 maximum = int64(base) + int64(base) * States[BATTLEPET_STATE_Mod_MaxHealthPercent] / 100;
    return int32(std::max<int64>(1, std::min<int64>(maximum, std::numeric_limits<int32>::max())));
}

int32 BattlePetInstance::GetSpeed()
{
    int32 modPercent = States[BATTLEPET_STATE_Mod_SpeedPercent];

    // Passive: flying
    if (States[BATTLEPET_STATE_Passive_Flying] && int64(Health) * 2 > GetMaxHealth())
        modPercent += 50;

    return (States[BATTLEPET_STATE_Stat_Speed] + CalculatePct(States[BATTLEPET_STATE_Stat_Speed], modPercent)) / 100;
}

uint32 BattlePetInstance::GetMaxXPForCurrentLevel()
{
    if (auto xpTable = sBattlePetXPTable.GetRow(Level))
        return xpTable->Xp * xpTable->Wins;

    return 0;
}

uint32 BattlePetInstance::GetXPEarn(uint32 targetPetID)
{
    auto targetPet = PetBattleInstance->Pets[targetPetID];
    int32 levelDiff = targetPet->Level - Level;
    if (levelDiff < -4 || levelDiff > 2)
        return 0;

    return (Level + 9) * (levelDiff + 5);
}

void BattlePetInstance::UpdateOriginalInstance(Player* player)
{
    if (!OriginalBattlePet)
        return;

    bool sendUpdate = OriginalBattlePet->Level != Level || OriginalBattlePet->XP != XP;
    OriginalBattlePet->JournalID = JournalID;
    OriginalBattlePet->Slot = Slot;
    OriginalBattlePet->Species = Species;
    OriginalBattlePet->Quality = Quality;
    OriginalBattlePet->Breed = Breed;
    OriginalBattlePet->Level = Level;
    OriginalBattlePet->XP = XP;
    OriginalBattlePet->DisplayModelID = DisplayModelID;
    OriginalBattlePet->Health = Health;
    OriginalBattlePet->Flags = Flags;
    OriginalBattlePet->InfoPower = InfoPower;
    OriginalBattlePet->InfoMaxHealth = InfoMaxHealth;
    OriginalBattlePet->InfoSpeed = InfoSpeed;
    OriginalBattlePet->InfoGender = InfoGender;
    OriginalBattlePet->AccountID = AccountID;
    OriginalBattlePet->needSave = true;

    if (sendUpdate)
        player->GetSession()->SendBattlePetUpdates(OriginalBattlePet.get());
}

PetBattleEventUpdate::PetBattleEventUpdate()
{
    UpdateType = 0;
    TargetPetID = PETBATTLE_NULL_ID;
    Health = 0;
    MaxHealth = 0;
    TriggerAbilityId = 0;
    Speed = 0;
}

PetBattleEvent::PetBattleEvent(uint32 eventType, int32 sourcePetID, uint32 flags, uint32 abilityEffectID, uint32 roundTurn, uint32 buffTurn, uint32 stackDepth)
{
    Flags = flags;
    SourcePetID = sourcePetID;
    AbilityEffectID = abilityEffectID;
    EventType = eventType;
    BuffTurn = buffTurn;
    RoundTurn = roundTurn;
    StackDepth = stackDepth;
}

PetBattleEvent& PetBattleEvent::UpdateHealth(int8 targetPetID, int32 health)
{
    PetBattleEventUpdate update;
    update.UpdateType = PET_BATTLE_EFFECT_TARGET_EX_PET;
    update.TargetPetID = targetPetID;
    update.Health = health;
    Updates.push_back(update);

    return *this;
}

PetBattleEvent& PetBattleEvent::UpdateMaxHealth(int8 targetPetID, int32 maxHealth)
{
    return UpdateSpeed(targetPetID, maxHealth);
}

PetBattleEvent& PetBattleEvent::UpdateState(int8 targetPetID, uint32 stateID, int32 value)
{
    // TC_LOG_DEBUG("battlepet", "PetBattleEvent::UpdateStat stateID %u value %u", stateID, value);

    PetBattleEventUpdate update;
    update.UpdateType = PET_BATTLE_EFFECT_TARGET_EX_STATE;
    update.TargetPetID = targetPetID;
    update.State.ID = stateID;
    update.State.Value = value;
    Updates.push_back(update);

    return *this;
}

PetBattleEvent& PetBattleEvent::UpdateFrontPet(int8 p_NewFrontPet)
{
    PetBattleEventUpdate update;
    update.UpdateType = PET_BATTLE_EFFECT_TARGET_EX_FRONT_PET;
    update.TargetPetID = p_NewFrontPet;
    Updates.push_back(update);

    return *this;
}

PetBattleEvent& PetBattleEvent::UpdateBuff(int8 targetPetID, uint32 p_ID, uint32 abilityID, int32 duration, uint32 p_Turn)
{
    PetBattleEventUpdate update;
    update.UpdateType = PET_BATTLE_EFFECT_TARGET_EX_AURA;
    update.TargetPetID = targetPetID;
    update.Buff.AbilityID = abilityID;
    update.Buff.Duration = duration;
    update.Buff.Turn = p_Turn;
    update.Buff.ID = p_ID;
    Updates.push_back(update);

    return *this;
}

PetBattleEvent& PetBattleEvent::UpdateSpeed(int8 targetPetID, int32 p_Speed)
{
    PetBattleEventUpdate update;
    update.UpdateType = PET_BATTLE_EFFECT_TARGET_EX_STAT_CHANGE;
    update.TargetPetID = targetPetID;
    update.Speed = p_Speed;
    Updates.push_back(update);

    return *this;
}

PetBattleEvent& PetBattleEvent::Trigger(int8 targetPetID, uint32 p_AbilityId)
{
    PetBattleEventUpdate update;
    update.UpdateType = PET_BATTLE_EFFECT_TARGET_EX_TRIGGER_ABILITY;
    update.TargetPetID = targetPetID;
    update.TriggerAbilityId = p_AbilityId;
    Updates.push_back(update);

    return *this;
}

static void UpdatePetBattleAuraMaxHealth(PetBattle* battle, PetBattleAura const* aura, int32 oldMaximum)
{
    auto pet = battle->Pets[aura->TargetPetID];
    int32 maximum = pet->GetMaxHealth();
    if (maximum == oldMaximum)
        return;
    int64 health = oldMaximum > 0 ? int64(pet->Health) * maximum / oldMaximum : pet->Health;
    pet->Health = int32(std::max<int64>(pet->Health > 0 ? 1 : 0, std::min<int64>(health, maximum)));
    pet->InfoMaxHealth = maximum;
    PetBattleEvent maxEvent(PETBATTLE_EVENT_SET_MAX_HEALTH, aura->CasterPetID, 0, aura->TriggerId, battle->RoundTurn++, aura->Turn, 1);
    maxEvent.UpdateMaxHealth(aura->TargetPetID, maximum);
    battle->RoundEvents.push_back(maxEvent);
    PetBattleEvent healthEvent(PETBATTLE_EVENT_SET_HEALTH, aura->CasterPetID, 0, aura->TriggerId, battle->RoundTurn++, aura->Turn, 1);
    healthEvent.UpdateHealth(aura->TargetPetID, pet->Health);
    battle->RoundEvents.push_back(healthEvent);
}

void PetBattleAura::Apply(PetBattle* petBattle)
{
    int32 oldMaximum = petBattle->Pets[TargetPetID]->GetMaxHealth();
    AppliedStates.clear();
    for (auto const& v : sBattlePetAbilityStateStore)
    {
        if (v->BattlePetAbilityID != AbilityID)
            continue;

        uint32 flags = 0;

        // Passive: elemental
        // TODO: add state flags for positive / negative
        // FIXME: need more work; check weather ability, dont change state value on remove
        if (petBattle->Pets[TargetPetID]->States[BATTLEPET_STATE_Passive_Elemental])
        {
            switch (v->BattlePetStateID)
            {
                case BATTLEPET_STATE_Mod_HealingTakenPercent:
                case BATTLEPET_STATE_Stat_Accuracy:
                    if (v->Value < 0)
                        flags |= PETBATTLE_EVENT_FLAG_IMMUNE;
                    break;
                case BATTLEPET_STATE_Mechanic_IsBlind:
                case BATTLEPET_STATE_Mechanic_IsChilled:
                case BATTLEPET_STATE_Mechanic_IsBurning:
                case BATTLEPET_STATE_swapOutLock:
                    if (v->Value > 0)
                        flags |= PETBATTLE_EVENT_FLAG_IMMUNE;
                    break;
                default:
                    break;
            }
        }

        // Stockpile290 authors a20-percent reserve. Its legacy aura289 uses
        // state2 with value20, unlike Healthy577's hundredths-per-level value.
        // Route only this reserve through the native percentage-health state.
        uint32 stateID = AbilityID == 289 && v->BattlePetStateID == BATTLEPET_STATE_maxHealthBonus
            ? BATTLEPET_STATE_Mod_MaxHealthPercent : v->BattlePetStateID;
        int32 value = petBattle->Pets[TargetPetID]->States[stateID];
        if (!flags)
        {
            value += v->Value;
            AppliedStates.emplace_back(stateID, v->Value);
        }

        petBattle->SetPetState(CasterPetID, TargetPetID, 0, stateID, value, false, flags);
    }

    UpdatePetBattleAuraMaxHealth(petBattle, this, oldMaximum);
}

void PetBattleAura::Remove(PetBattle* petBattle)
{
    int32 oldMaximum = petBattle->Pets[TargetPetID]->GetMaxHealth();
    if (AbilityID == petBattle->WeatherAbilityId &&
        std::none_of(petBattle->PetAuras.begin(), petBattle->PetAuras.end(), [this](PetBattleAura const* other)
        {
            return other != this && !other->Expired && other->AbilityID == AbilityID;
        }))
        petBattle->WeatherAbilityId = 0;

    for (auto const& state : AppliedStates)
        petBattle->SetPetState(CasterPetID, TargetPetID, 0, state.first, petBattle->Pets[TargetPetID]->States[state.first] - state.second);
    AppliedStates.clear();

    UpdatePetBattleAuraMaxHealth(petBattle, this, oldMaximum);
}

void PetBattleAura::Process(PetBattle* petBattle)
{
    if (Expired)
        return;

    if (Duration != -1 && Turn > MaxDuration && !Expired)
    {
        Expire(petBattle);
        return;
    }

    if (auto abilityInfo = sBattlePetAbilityStore.LookupEntry(AbilityID))
    {
        PetBattleAbilityTurn abilityTurn;
        memset(&abilityTurn, 0, sizeof abilityTurn);

        uint8 turnCount = 0;
        uint8 maxTurnID = 0;

        for (auto const& abilityTurnInfo : sBattlePetAbilityTurnStore)
        {
            if (abilityTurnInfo->BattlePetAbilityID != AbilityID)
                continue;

            turnCount++;
            maxTurnID = std::max(maxTurnID, abilityTurnInfo->OrderIndex);
        }

        for (auto const& abilityTurnInfo : sBattlePetAbilityTurnStore)
        {
            if (abilityTurnInfo->BattlePetAbilityID != AbilityID)
                continue;

            if (abilityTurnInfo->OrderIndex != Turn && turnCount != 1 && maxTurnID != 1)
                continue;

            if (abilityTurnInfo->TurnTypeEnum && abilityTurnInfo->EventTypeEnum != PETBATTLE_ABILITY_TURN0_PROC_ON_TURN)
                continue;

            for (auto const& abilityEffectInfo : sBattlePetAbilityEffectStore)
            {
                if (abilityEffectInfo->BattlePetAbilityTurnID != abilityTurnInfo->ID)
                    continue;

                PetBattleAbilityEffect effect;
                effect.EffectInfo = abilityEffectInfo;
                effect.AbilityTurn = &abilityTurn;
                effect.PetBattleInstance = petBattle;
                effect.Caster = CasterPetID;
                effect.StopChain = false;
                effect.AbilityID = AbilityID;
                effect.IsTriggered = abilityInfo->Flags & BATTLEPET_ABILITY_FLAG_TRIGGER;
                effect.ReportFailAsImmune = false;
                effect.Targets.push_back(TargetPetID);

                auto const previousEventCount = petBattle->RoundEvents.size();

                if (!effect.Execute())
                    abilityTurn.ChainFailure |= 1 << (abilityEffectInfo->OrderIndex - 1);

                /// Update "buff turn" on all event triggered by the current ability effect
                auto itr = petBattle->RoundEvents.begin();
                std::advance(itr, previousEventCount);
                for (; itr != petBattle->RoundEvents.end(); ++itr)
                    itr->BuffTurn = Turn;

                if (Expired)
                    return;
                if (effect.StopChain)
                    break;
            }
        }
    }

    PetBattleEvent eventBuffChange(PETBATTLE_EVENT_BUFF_CHANGE, CasterPetID);
    eventBuffChange.UpdateBuff(TargetPetID, ID, AbilityID, Duration, Turn);
    petBattle->RoundEvents.push_back(eventBuffChange);

    Turn++;
    if (Duration != -1)
        Duration--;
}

void PetBattleAura::Expire(PetBattle* petBattle)
{
    if (Expired)
        return;

    PetBattleEvent battleEvent(PETBATTLE_EVENT_BUFF_CANCEL, CasterPetID, 0, 0, 1, 0, 1);
    battleEvent.UpdateBuff(TargetPetID, ID, AbilityID, 0, Turn);
    petBattle->RoundEvents.push_back(battleEvent);

    Expired = true;

    petBattle->Cast(CasterPetID, AbilityID, 0, PETBATTLE_ABILITY_TURN0_PROC_ON_AURA_REMOVED, PETBATTLE_CAST_TRIGGER_ALL, TargetPetID);

    Remove(petBattle);
}

bool PetBattleTeam::Update()
{
    if (Ready || !isRun)
        return false;

    if (PetBattleInstance->BattleStatus == PETBATTLE_STATUS_FINISHED)
    {
        Ready = false;
        return false;
    }

    auto frontPet = PetBattleInstance->Pets[ActivePetID];
    auto isFrontPetAlive = frontPet->IsAlive();
    auto thisTeamID = PetBattleInstance->Teams[PETBATTLE_TEAM_1] == this ? PETBATTLE_TEAM_1 : PETBATTLE_TEAM_2;

    if (!isFrontPetAlive)
    {
        auto availablesPets = GetAvailablesPets();

        if (availablesPets.empty())
        {
            PetBattleInstance->Finish(!thisTeamID, false, false);
            return true;
        }

        if (availablesPets.size() == 1)
        {
            PetBattleInstance->SwapPet(thisTeamID, availablesPets[0]);

            if (auto player = ObjectAccessor::GetObjectInWorld(PlayerGuid, static_cast<Player*>(nullptr)))
                player->GetSession()->SendPetBattleReplacementMade(PetBattleInstance);

            if (PetBattleInstance->BattleType == PETBATTLE_TYPE_PVE && thisTeamID == PETBATTLE_PVE_TEAM_ID)
                PetBattleInstance->Teams[PETBATTLE_TEAM_1]->Ready = true;
            else if (PetBattleInstance->BattleType == PETBATTLE_TYPE_PVP_DUEL || PetBattleInstance->BattleType == PETBATTLE_TYPE_PVP_MATCHMAKING)
            {
                PetBattleInstance->Teams[!thisTeamID]->Ready = true;
                Ready = true;
            }
        }
        else if (PetBattleInstance->BattleType == PETBATTLE_TYPE_PVE && thisTeamID == PETBATTLE_PVE_TEAM_ID)
        {
            PetBattleInstance->SwapPet(thisTeamID, availablesPets[rand() % availablesPets.size()]);

            if (auto player = ObjectAccessor::GetObjectInWorld(PlayerGuid, static_cast<Player*>(nullptr)))
                player->GetSession()->SendPetBattleReplacementMade(PetBattleInstance);

            PetBattleInstance->Teams[PETBATTLE_TEAM_1]->Ready = true;
            Ready = true;
        }
    }
    else if (HasPendingMultiTurnCast() && isFrontPetAlive)
        Ready = true;
    else if (isFrontPetAlive)
    {
        if (PetBattleInstance->BattleType == PETBATTLE_TYPE_PVE && thisTeamID == PETBATTLE_PVE_TEAM_ID)
        {
            std::vector<uint32> availablesAbilities;
            for (auto ability : frontPet->Abilities)
                if (PetBattleInstance->CanCast(thisTeamID, ability))
                    availablesAbilities.push_back(ability);

            if (!availablesAbilities.empty())
                PetBattleInstance->PrepareCast(thisTeamID, availablesAbilities[rand() % availablesAbilities.size()]);
            else
                PetBattleInstance->SwapPet(thisTeamID, ActivePetID);  ///< Skip turn

            Ready = true;
        }
    }
    return false;
}

void PetBattleTeam::DoCasts(uint32 turn0ProcCond)
{
    if (ActiveAbilityId && PetBattleInstance->Pets[ActivePetID]->IsAlive() && PetBattleInstance->Pets[ActivePetID]->CanAttack())
    {
        if (turn0ProcCond == PETBATTLE_ABILITY_TURN0_PROC_ON_NONE && PetBattleInstance->PetXDied.empty())
            PetBattleInstance->Cast(ActivePetID, ActiveAbilityId, activeAbilityTurn, turn0ProcCond, PETBATTLE_CAST_TRIGGER_NONE);
        else
            PetBattleInstance->Cast(ActivePetID, ActiveAbilityId, 0, turn0ProcCond, PETBATTLE_CAST_TRIGGER_ALL);

        for (auto& aura : PetBattleInstance->PetAuras)
            if (!aura->Expired && aura->TargetPetID == ActivePetID)
                PetBattleInstance->Cast(aura->CasterPetID, aura->AbilityID, 0, PETBATTLE_ABILITY_TURN0_PROC_ON_ABILITY, PETBATTLE_CAST_TRIGGER_ALL, ActivePetID);
    }
}

bool PetBattleTeam::HasPendingMultiTurnCast()
{
    if (ActiveAbilityId && activeAbilityTurn <= activeAbilityTurnMax)
        return true;

    return false;
}

bool PetBattleTeam::CanCastAny()
{
    if (!PetBattleInstance->Pets[ActivePetID] || !PetBattleInstance->Pets[ActivePetID]->IsAlive())
        return false;

    if (HasPendingMultiTurnCast())
        return false;

    if (PetBattleInstance->Pets[ActivePetID]->States[BATTLEPET_STATE_turnLock])
        return false;

    if (PetBattleInstance->Pets[ActivePetID]->States[BATTLEPET_STATE_Mechanic_IsStunned])
        return false;

    return true;
}

bool PetBattleTeam::CanSwap(int8 replacementPet)
{
    if (HasPendingMultiTurnCast())
        return false;

    if (PetBattleInstance->Pets[ActivePetID]->IsAlive() && (PetBattleInstance->Pets[ActivePetID]->States[BATTLEPET_STATE_swapOutLock] || PetBattleInstance->Pets[ActivePetID]->States[BATTLEPET_STATE_Mechanic_IsWebbed]))
        return false;

    if (replacementPet != PETBATTLE_NULL_SLOT)
    {
        uint32 const teamID = PetBattleInstance->Teams[PETBATTLE_TEAM_1] == this ? PETBATTLE_TEAM_1 : PETBATTLE_TEAM_2;
        if (replacementPet < 0 || replacementPet >= MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS ||
            !PetBattleInstance->Pets[replacementPet] || PetBattleInstance->Pets[replacementPet]->TeamID != teamID)
            return false;

        if (!PetBattleInstance->Pets[replacementPet]->IsAlive())
            return false;

        if (PetBattleInstance->Pets[replacementPet]->States[BATTLEPET_STATE_swapInLock])
            return false;
    }

    return true;
}

uint8 PetBattleTeam::CanCatchOpponentTeamFrontPet()
{
    if (PetBattleInstance->BattleType != PETBATTLE_TYPE_PVE)
        return 0;

    if (!GetCatchAbilityID())
        return 0;

    if (PetBattleInstance->BattleType == PETBATTLE_TYPE_PVE && PetBattleInstance->PveBattleType == PVE_PETBATTLE_TRAINER)
        return 0;

    if (CapturedPet != PETBATTLE_NULL_ID)
        return PETBATTLE_TEAM_CATCH_FLAG_ONE_CATCH_PER_FIGHT;

    uint32 thisTeamID = PetBattleInstance->Teams[PETBATTLE_TEAM_1] == this ? PETBATTLE_TEAM_1 : PETBATTLE_TEAM_2;
    if (thisTeamID == PETBATTLE_PVE_TEAM_ID)
        return 0;

    if (PetBattleInstance->Teams[!thisTeamID]->ActivePetID == PETBATTLE_NULL_ID)
        return 0;

    auto targetPet = PetBattleInstance->Pets[PetBattleInstance->Teams[!thisTeamID]->ActivePetID];
    if (!targetPet || !targetPet->IsAlive())
        return 0;

    if (sDB2Manager.HasBattlePetSpeciesFlag(targetPet->Species, BATTLEPET_SPECIES_FLAG_UNTAMEABLE))
        return 0;

    if (targetPet->Health * 100 / targetPet->InfoMaxHealth >= 35)
        return PETBATTLE_TEAM_CATCH_FLAG_TOO_MUCH_HP;

    /// Max 3 same pet for player
    if (CapturedSpeciesCount.find(targetPet->Species) != CapturedSpeciesCount.end())
        if (CapturedSpeciesCount[targetPet->Species] > 2)
            return 0;

    /// todo more check

    return PETBATTLE_TEAM_CATCH_FLAG_ENABLE_TRAP;
}

uint32 PetBattleTeam::GetTeamTrapStatus()
{
    uint32 flags = CanCatchOpponentTeamFrontPet();

    /// @TODO PETBATTLE_TEAM_CATCH_FLAG_NEED_LVL3_PET

    return flags;
}

uint32 PetBattleTeam::GetTeamInputFlags()
{
    uint32 flags = 0;

    if (PetBattleInstance->BattleType != PETBATTLE_TYPE_PVE && PetBattleInstance->Teams[PETBATTLE_TEAM_1]->ActivePetID == PETBATTLE_NULL_ID && PetBattleInstance->Teams[PETBATTLE_TEAM_2]->ActivePetID == PETBATTLE_NULL_ID)
        flags |= PETBATTLE_TEAM_INPUT_FLAG_LOCK_ABILITIES_2 | PETBATTLE_TEAM_INPUT_FLAG_SELECT_NEW_PET;

    if (!CanCastAny())
        flags |= PETBATTLE_TEAM_INPUT_FLAG_LOCK_ABILITIES_1 | PETBATTLE_TEAM_INPUT_FLAG_LOCK_ABILITIES_2;

    if (!CanSwap())
        flags |= PETBATTLE_TEAM_INPUT_FLAG_LOCK_PET_SWAP;

    if (!PetBattleInstance->Pets[ActivePetID]->IsAlive())
    {
        if (GetAvailablesPets().size() > 1)
            flags |= PETBATTLE_TEAM_INPUT_FLAG_LOCK_ABILITIES_2 | PETBATTLE_TEAM_INPUT_FLAG_SELECT_NEW_PET;
        else
            flags |= PETBATTLE_TEAM_INPUT_FLAG_LOCK_ABILITIES_2 | PETBATTLE_TEAM_INPUT_FLAG_LOCK_PET_SWAP;
    }

    uint32 thisTeamID = PetBattleInstance->Pets[ActivePetID]->TeamID;
    if (PetBattleInstance->Teams[!thisTeamID]->ActivePetID != PETBATTLE_NULL_ID && PetBattleInstance->Pets[PetBattleInstance->Teams[!thisTeamID]->ActivePetID] && !PetBattleInstance->Pets[PetBattleInstance->Teams[!thisTeamID]->ActivePetID]->IsAlive())
        flags |= PETBATTLE_TEAM_INPUT_FLAG_LOCK_ABILITIES_2 | PETBATTLE_TEAM_INPUT_FLAG_LOCK_PET_SWAP;

    return flags;
}

std::vector<uint32> PetBattleTeam::GetAvailablesPets()
{
    std::vector<uint32> availablesPets;
    uint32 thisTeamID = PetBattleInstance->Teams[PETBATTLE_TEAM_1] == this ? PETBATTLE_TEAM_1 : PETBATTLE_TEAM_2;

    for (uint32 i = 0; i < MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS; i++)
        if (PetBattleInstance->Pets[i] && PetBattleInstance->Pets[i]->IsAlive() && PetBattleInstance->Pets[i]->TeamID == thisTeamID && CanSwap(i))
            availablesPets.push_back(i);

    return availablesPets;
}

uint32 PetBattleTeam::GetCatchAbilityID()
{
    uint32 thisTeamID = PetBattleInstance->Teams[PETBATTLE_TEAM_1] == this ? PETBATTLE_TEAM_1 : PETBATTLE_TEAM_2;
    if (PetBattleInstance->BattleType == PETBATTLE_TYPE_PVE && thisTeamID == PETBATTLE_PVE_TEAM_ID)
        return 0;

    static const uint32 petBattleCatchAbilityID[] = {427, 77, 135};
    if (auto player = ObjectAccessor::GetObjectInWorld(PlayerGuid, static_cast<Player*>(nullptr)))
        return petBattleCatchAbilityID[player->GetBattlePetTrapLevel() - 1];

    return petBattleCatchAbilityID[0];
}

PetBattle::PetBattle()
{
    BattleStatus = PETBATTLE_STATUS_CREATION;
    RoundStatus = PETBATTLE_ROUND_FINISHED;
    RoundResult = PETBATTLE_ROUND_RESULT_NONE;
    TotalPetCount = 0;
    WeatherAbilityId = 0;

    for (auto& pet : Pets)
        pet = nullptr;

    for (auto& team : Teams)
    {
        team = new PetBattleTeam();
        team->PetBattleInstance = this;
        team->TeamPetCount = 0;
        team->ActiveAbilityId = 0;
        team->activeAbilityTurn = 0;
        team->activeAbilityTurnMax = 0;
        team->CapturedPet = PETBATTLE_NULL_ID;
        team->Ready = false;
        team->isRun = false;

        for (auto& teamPet : team->TeamPets)
            teamPet = nullptr;
    }

    WinnerTeamId = -1;
    CatchedPetId = PETBATTLE_NULL_ID;

    m_UpdateTimer.SetInterval(PETBATTLE_UPDATE_INTERVAL);
}

PetBattle::~PetBattle()
{
    for (size_t i = 0; i < MAX_PETBATTLE_TEAM; ++i)
    {
        if (BattleType == PETBATTLE_TYPE_PVE && i != PETBATTLE_PVE_TEAM_ID || BattleType != PETBATTLE_TYPE_PVE)
            for (uint32 petID = 0; petID < MAX_PETBATTLE_SLOTS; petID++)
                if (Pets[petID])
                    Pets[petID] = std::shared_ptr<BattlePetInstance>();

        delete Teams[i];
    }

    for (auto& aura : PetAuras)
        delete aura;
}

void PetBattle::AddPet(uint32 teamID, std::shared_ptr<BattlePetInstance> pet)
{
    if (pet == nullptr || pet->Slot < 0 || pet->Slot >= MAX_PETBATTLE_SLOTS)
        return;

    assert(teamID < MAX_PETBATTLE_TEAM);
    assert(Teams[teamID]->TeamPetCount < MAX_PETBATTLE_SLOTS);

    Teams[teamID]->TeamPets[Teams[teamID]->TeamPetCount] = pet;

    pet->ID = (teamID == PETBATTLE_TEAM_2 ? MAX_PETBATTLE_SLOTS : 0) + Teams[teamID]->TeamPetCount;
    pet->TeamID = teamID;
    pet->PetBattleInstance = this;
    pet->OldLevel = pet->Level;
    pet->OldXP = pet->XP;

    TotalPetCount++;
    Teams[teamID]->TeamPetCount++;

    Pets[pet->ID] = pet;

    for (auto& state : pet->States)
        state = 0;

    pet->States[0] = 1;

    for (uint32 i = 0; i < MAX_PETBATTLE_ABILITIES; i++)
    {
        pet->Cooldowns[i] = -1;
        pet->Lockdowns[i] = 0;
    }

    for (auto breedStateInfo : sBattlePetBreedStateStore)
        if (breedStateInfo->BreedID == pet->Breed)
            pet->States[breedStateInfo->BattlePetStateID] += breedStateInfo->Value;

    for (auto speciesStateInfo : sBattlePetSpeciesStateStore)
    {
        if (speciesStateInfo->SpeciesID != pet->Species)
            continue;

        if (speciesStateInfo->BattlePetStateID > NUM_BATTLEPET_STATES)
            continue;

        pet->States[speciesStateInfo->BattlePetStateID] += speciesStateInfo->Value;
    }

    // Primary stats is scaled with level and quality
    if (auto qualityInfo = sBattlePetBreedQualityStore.LookupEntry(7 + pet->Quality))
        for (size_t i = 0; i < 3; ++i)
            pet->States[BATTLEPET_STATE_Stat_Power + i] *= pet->Level * qualityInfo->StateMultiplier;

    // Populate infos
    pet->InfoPower = pet->States[BATTLEPET_STATE_Stat_Power] / 100;
    pet->InfoSpeed = pet->States[BATTLEPET_STATE_Stat_Speed] / 100;
    pet->InfoMaxHealth = int32(floor((10000.0f + pet->States[BATTLEPET_STATE_Stat_Stamina] * 5.0f) / 100.0f + 0.5));
    pet->InfoGender = pet->States[BATTLEPET_STATE_Stat_Gender];

    pet->Health = std::max<int32>(0, std::min(pet->Health, pet->InfoMaxHealth));
    pet->States[BATTLEPET_STATE_Is_Dead] = pet->Health == 0;
}

void PetBattle::Begin()
{
    // TC_LOG_DEBUG("battlepet", "PetBattle::Begin()");

    RoundStatus = PETBATTLE_ROUND_RUNNING;
    RoundResult = PETBATTLE_ROUND_RESULT_NORMAL;
    Turn = 0;

    for (size_t i = 0; i < MAX_PETBATTLE_TEAM; ++i)
    {
        Teams[i]->ActiveAbilityId = 0;
        Teams[i]->ActivePetID = i == PETBATTLE_TEAM_2 ? MAX_PETBATTLE_SLOTS : 0;
        for (auto const& pet : Teams[i]->TeamPets)
            if (pet && pet->IsAlive())
            {
                Teams[i]->ActivePetID = pet->ID;
                break;
            }

        if (Player* player = sObjectAccessor->FindPlayer(Teams[i]->OwnerGuid))
            player->UnsummonCurrentBattlePetIfAny(true);
    }

    /// In PVE battle the front pets (slot 0) are auto lock
    if (BattleType == PETBATTLE_TYPE_PVE)
    {
        /// Team 1 pet selection
        SwapPet(PETBATTLE_TEAM_1, Teams[PETBATTLE_TEAM_1]->ActivePetID, true);

        /// Team 2 pet selection
        SwapPet(PETBATTLE_TEAM_2, Teams[PETBATTLE_TEAM_2]->ActivePetID, true);

        if (Player* player = sObjectAccessor->FindPlayer(Teams[PETBATTLE_TEAM_1]->OwnerGuid))
        {
            player->PetBattleCountBattleSpecies();

            player->GetSession()->SendPetBattleInitialUpdate(this);
            player->GetSession()->SendPetBattleFirstRound(this);
        }

        RoundEvents.clear();
        PetXDied.clear();
        RoundPetSpeedUpdate.clear();
    }
    else if (BattleType == PETBATTLE_TYPE_PVP_DUEL || BattleType == PETBATTLE_TYPE_PVP_MATCHMAKING)
    {
        Player* playerA = sObjectAccessor->FindPlayer(Teams[PETBATTLE_TEAM_1]->OwnerGuid);
        Player* playerB = sObjectAccessor->FindPlayer(Teams[PETBATTLE_TEAM_2]->OwnerGuid);

        if (playerA && playerB)
        {
            playerA->GetSession()->SendPetBattleInitialUpdate(this);
            playerB->GetSession()->SendPetBattleInitialUpdate(this);

            playerA->GetSession()->SendPetBattleFirstRound(this);
            playerB->GetSession()->SendPetBattleFirstRound(this);
        }

        RoundEvents.clear();
        PetXDied.clear();
        RoundPetSpeedUpdate.clear();
    }

    Turn++;

    // Initialize pet health and auras
    for (uint32 petID = 0; petID < MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS; ++petID)
        Cast(petID, 291, 1, 0, PETBATTLE_CAST_TRIGGER_NONE);

    RoundEvents.clear();
    PetXDied.clear();
    RoundPetSpeedUpdate.clear();

    RoundResult = PETBATTLE_ROUND_RESULT_NONE;
    RoundStatus = PETBATTLE_ROUND_FINISHED;
    BattleStatus = PETBATTLE_STATUS_RUNNING;
}

void PetBattle::ProceedRound()
{
    if (!Teams[PETBATTLE_TEAM_1]->Ready || !Teams[PETBATTLE_TEAM_2]->Ready)
        return;

    RoundStatus = PETBATTLE_ROUND_RUNNING;
    RoundTurn = 1;

    for (auto& pet : Pets)
    {
        if (pet)
        {
            pet->States[BATTLEPET_STATE_Condition_DidDamageThisRound] = 0;
            pet->States[BATTLEPET_STATE_Condition_WasDamagedThisTurn] = 0;
        }
    }

    RoundFirstTeamCasting = GetFirstAttackingTeam();

    uint32 firstTeam = RoundFirstTeamCasting;

    Teams[firstTeam]->DoCasts(PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START);
    Teams[!firstTeam]->DoCasts(PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START);

    firstTeam = RoundFirstTeamCasting;

    Teams[firstTeam]->DoCasts();
    Teams[!firstTeam]->DoCasts();

    PetBattleEvent eventAuraProccesing(PETBATTLE_EVENT_AURA_PROCESSING_BEGIN);
    eventAuraProccesing.UpdateFrontPet();
    RoundEvents.push_back(eventAuraProccesing);

    firstTeam = RoundFirstTeamCasting;
    for (auto& aura : PetAuras)
        if (!aura->Expired && Pets[aura->CasterPetID]->TeamID == firstTeam)
            aura->Process(this);

    for (auto& aura : PetAuras)
        if (!aura->Expired && Pets[aura->CasterPetID]->TeamID == !firstTeam)
            aura->Process(this);

    PetBattleEvent eventAuraProcessingEnd(PETBATTLE_EVENT_AURA_PROCESSING_END);
    eventAuraProcessingEnd.UpdateFrontPet();
    RoundEvents.push_back(eventAuraProcessingEnd);

    for (auto& itr : RoundPetSpeedUpdate)
    {
        PetBattleEvent eventSetSpeed(PETBATTLE_EVENT_SET_SPEED, itr.first, 0, itr.second, RoundTurn++, 0, 1);
        eventSetSpeed.UpdateSpeed(itr.first, Pets[itr.first]->GetSpeed());
        RoundEvents.push_back(eventSetSpeed);
    }

    firstTeam = RoundFirstTeamCasting;
    Teams[firstTeam]->DoCasts(PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END);
    Teams[!firstTeam]->DoCasts(PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END);

    // The native Hourglass controller clears the other living roster pets at
    // round end, before their ordinary cooldown decrement. Snapshot issued
    // instances so cleanup/new auras cannot replay this phase.
    std::vector<PetBattleAura*> hourglassEnd;
    for (auto aura : PetAuras)
        if (!aura->Expired && aura->AbilityID == 1932)
            hourglassEnd.push_back(aura);
    for (auto aura : hourglassEnd)
        if (!aura->Expired)
            Cast(aura->CasterPetID, aura->AbilityID, 0, PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END, PETBATTLE_CAST_TRIGGER_ALL, aura->TargetPetID);

    // Passive: Humanoid
    for (uint8 i = 0; i < MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS; ++i)
        if (Pets[i] && Pets[i]->States[BATTLEPET_STATE_Passive_Humanoid] && Pets[i]->States[BATTLEPET_STATE_Condition_DidDamageThisRound])
            Cast(i, 726, 1, PETBATTLE_ABILITY_TURN0_PROC_ON_NONE, PETBATTLE_CAST_TRIGGER_NONE);

    // Passive: Dragonkin
    if (Pets[Teams[0]->ActivePetID]->IsAlive() && Pets[Teams[1]->ActivePetID]->IsAlive())
        for (size_t teamID = 0; teamID < MAX_PETBATTLE_TEAM; ++teamID)
            if (Pets[Teams[teamID]->ActivePetID]->States[BATTLEPET_STATE_Passive_Dragon])
                if (Pets[Teams[!teamID]->ActivePetID]->Health * 100 / Pets[Teams[!teamID]->ActivePetID]->GetMaxHealth() < 50)
                    AddAura(Teams[teamID]->ActivePetID, Teams[teamID]->ActivePetID, 245, 1, 0, 0, 0);

    std::list<PetBattleAura*> aurasToRemove;
    for (auto& aura : PetAuras)
        if (aura->Expired)
            aurasToRemove.push_back(aura);

    for (auto& itr : aurasToRemove)
    {
        PetAuras.remove(itr);
        delete itr;
    }

    for (auto& team : Teams)
    {
        team->activeAbilityTurn++;

        if (team->activeAbilityTurn > team->activeAbilityTurnMax)
        {
            team->ActiveAbilityId = 0;
            team->activeAbilityTurn = 0;
            team->activeAbilityTurnMax = 0;
        }
    }

    for (auto& pet : Pets)
        if (pet)
        {
            for (auto& cooldown : pet->Cooldowns)
                if (cooldown != -1)
                    --cooldown;
            for (auto& lockdown : pet->Lockdowns)
                if (lockdown > 0)
                    --lockdown;
        }

    if (RoundResult == PETBATTLE_ROUND_RESULT_NONE)
        RoundResult = PETBATTLE_ROUND_RESULT_NORMAL;

    for (auto& team : Teams)
    {
        if (!team->PlayerGuid)
            continue;

        if (auto player = ObjectAccessor::GetObjectInWorld(team->PlayerGuid, static_cast<Player*>(nullptr)))
            player->GetSession()->SendPetBattleRoundResult(this);
    }


    // To next turn

    ++Turn;
    RoundEvents.clear();
    PetXDied.clear();
    RoundPetSpeedUpdate.clear();

    RoundResult = PETBATTLE_ROUND_RESULT_NONE;
    RoundStatus = PETBATTLE_ROUND_FINISHED;
}

void PetBattle::Finish(uint32 winnerTeamID, bool aborted, bool ignoreAbandonPenalty)
{
    if (BattleStatus == PETBATTLE_STATUS_FINISHED || BattleStatus == PETBATTLE_STATUS_PENDING_DELETE)
        return;
    if (aborted)
        if (Pets[Teams[!winnerTeamID]->ActivePetID] && Pets[Teams[!winnerTeamID]->ActivePetID]->IsAlive() && Pets[Teams[!winnerTeamID]->ActivePetID]->States[BATTLEPET_STATE_Mechanic_IsWebbed])
            return;

    RoundStatus = PETBATTLE_ROUND_FINISHED;
    WinnerTeamId = winnerTeamID;

    for (size_t currentTeamID = 0; currentTeamID < MAX_PETBATTLE_TEAM; ++currentTeamID)
    {
        if (Teams[currentTeamID]->PlayerGuid)
        {
            auto player = ObjectAccessor::GetObjectInWorld(Teams[currentTeamID]->PlayerGuid, static_cast<Player*>(nullptr));
            if (!player)
                continue;

            sScriptMgr->OnPetBattleFinish(player);

            uint32 availablePetCount = 0;
            for (auto const& pet : Teams[currentTeamID]->TeamPets)
                if (pet && pet->IsAlive())
                    ++availablePetCount;

            for (size_t currentPetID = 0; currentPetID < Teams[currentTeamID]->TeamPetCount; ++currentPetID)
            {
                auto currentPet = Teams[currentTeamID]->TeamPets[currentPetID];
                if (!currentPet)
                    continue;

                if (currentPet->Health < 0)
                    currentPet->Health = 0;

                if (!ignoreAbandonPenalty && aborted && winnerTeamID != currentTeamID)
                    currentPet->Health = AddPct(currentPet->Health, -GetForfeitHealthPenalityPct());

                if (winnerTeamID == currentTeamID && availablePetCount && BattleType == PETBATTLE_TYPE_PVE && currentPet->IsAlive() && currentPet->Level < BATTLEPET_MAX_LEVEL && FightedPets.find(currentPet->ID) != FightedPets.end())
                {
                    uint32 xpEarn = 0;
                    float xpMod[] = {1.0f, 1.0f, 0.5f};

                    for (uint32 opponentTeamCurrentPet = 0; opponentTeamCurrentPet < Teams[PETBATTLE_PVE_TEAM_ID]->TeamPetCount; opponentTeamCurrentPet++)
                    {
                        if (!Teams[PETBATTLE_PVE_TEAM_ID]->TeamPets[opponentTeamCurrentPet] || FightedPets.find(Teams[PETBATTLE_PVE_TEAM_ID]->TeamPets[opponentTeamCurrentPet]->ID) == FightedPets.end())
                            continue;

                        xpEarn += float(currentPet->GetXPEarn(Teams[PETBATTLE_PVE_TEAM_ID]->TeamPets[opponentTeamCurrentPet]->ID)) * xpMod[opponentTeamCurrentPet] / availablePetCount;
                    }

                    AddPct(xpEarn, player->GetTotalAuraModifier(SPELL_AURA_MOD_BATTLE_PET_XP_GAIN));
                    uint64 remainingXP = uint64(currentPet->XP) + xpEarn;
                    uint32 xpToNextLevel = currentPet->GetMaxXPForCurrentLevel();
                    while (currentPet->Level < BATTLEPET_MAX_LEVEL && xpToNextLevel && remainingXP >= xpToNextLevel)
                    {
                        remainingXP -= xpToNextLevel;
                        currentPet->Level++;
                        currentPet->UpdateStats();
                        currentPet->UpdateAbilities();
                        currentPet->Health = currentPet->InfoMaxHealth;

                        if (auto speciesInfo = sBattlePetSpeciesStore.LookupEntry(currentPet->Species))
                        {
                            player->UpdateAchievementCriteria(CRITERIA_TYPE_BATTLEPET_LEVEL_UP, currentPet->Level, speciesInfo->PetTypeEnum, currentPet->Species);
                            player->UpdateAchievementCriteria(CRITERIA_TYPE_LEVEL_BATTLE_PET_CREDIT, speciesInfo->ID, currentPet->Level, currentPet->Species);
                        }

                        if (currentPet->Level >= 3)
                        {
                            /// Quest progress for 12 x Level Up!
                            player->KilledMonsterCredit(65876);
                        }
                        xpToNextLevel = currentPet->GetMaxXPForCurrentLevel();
                    }
                    currentPet->XP = currentPet->Level == BATTLEPET_MAX_LEVEL ? 0 : uint32(remainingXP);
                }

                currentPet->UpdateOriginalInstance(player);
                player->UpdateBattlePetCombatTeam();
                player->GetSession()->SendBattlePetUpdates(currentPet.get());
            }

            if (winnerTeamID == currentTeamID)
            {
                auto speciesInfo = sDB2Manager.GetSpeciesByCreatureID(InitialWildPetGUID.GetEntry());
                player->UpdateAchievementCriteria(CRITERIA_TYPE_BATTLEPET_WIN, BattleType != PETBATTLE_TYPE_PVE, 0, speciesInfo ? speciesInfo->ID : 0);
                if (speciesInfo)
                    player->QuestObjectiveSatisfy(speciesInfo->ID, 1, QUEST_OBJECTIVE_DEFEATBATTLEPET, InitialWildPetGUID);
            }

            if (!aborted && winnerTeamID == currentTeamID && BattleType == PETBATTLE_TYPE_PVE &&
                PveBattleType == PVE_PETBATTLE_WILD)
            {
                /// Quest progress for 12 x Learning the Ropes
                player->KilledMonsterCredit(65355);
            }

            if (BattleType == PETBATTLE_TYPE_PVE && currentTeamID == winnerTeamID && Teams[currentTeamID]->CapturedPet != PETBATTLE_NULL_ID)
            {
                if (Pets[Teams[currentTeamID]->CapturedPet]->Level >= 16 && Pets[Teams[currentTeamID]->CapturedPet]->Level <= 20)
                    Pets[Teams[currentTeamID]->CapturedPet]->Level -= 1;
                else if (Pets[Teams[currentTeamID]->CapturedPet]->Level >= 21 && Pets[Teams[currentTeamID]->CapturedPet]->Level <= 25)
                    Pets[Teams[currentTeamID]->CapturedPet]->Level -= 2;

                if (Pets[Teams[currentTeamID]->CapturedPet]->Health <= CalculatePct(Pets[Teams[currentTeamID]->CapturedPet]->InfoMaxHealth, 5))
                    if (auto achievement = sAchievementStore.LookupEntry(6571))
                        player->CompletedAchievement(achievement);

                Pets[Teams[currentTeamID]->CapturedPet]->UpdateStats();
                Pets[Teams[currentTeamID]->CapturedPet]->Health = Pets[Teams[currentTeamID]->CapturedPet]->InfoMaxHealth;
                Pets[Teams[currentTeamID]->CapturedPet]->Slot = PETBATTLE_NULL_SLOT;
                Pets[Teams[currentTeamID]->CapturedPet]->AddToPlayer(player);
                player->_battlePets.emplace(Pets[Teams[currentTeamID]->CapturedPet]->JournalID, Pets[Teams[currentTeamID]->CapturedPet]);

                if (auto speciesInfo = sBattlePetSpeciesStore.LookupEntry(Pets[Teams[currentTeamID]->CapturedPet]->Species))
                {
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_ADD_BATTLE_PET_JOURNAL, speciesInfo->CreatureID);
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_CAPTURE_SPECIFIC_BATTLEPET, speciesInfo->ID, 0, speciesInfo->ID);
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_CAPTURE_PET_IN_BATTLE, 1, Pets[Teams[currentTeamID]->CapturedPet]->Quality, speciesInfo->ID);
                }

                /// Quest progress for 12 x Got One!
                player->KilledMonsterCredit(65356);
            }

            if (BattleType == PETBATTLE_TYPE_PVE && PveBattleType == PVE_PETBATTLE_TRAINER && currentTeamID == winnerTeamID)
                if (auto trainer = ObjectAccessor::GetObjectInOrOutOfWorld(Teams[PETBATTLE_PVE_TEAM_ID]->OwnerGuid, static_cast<Creature*>(nullptr)))
                {
                    player->QuestObjectiveSatisfy(trainer->GetEntry(), 1, QUEST_OBJECTIVE_PET_TRAINER_DEFEAT, trainer->GetGUID());

                    // Scenario pet-battle objectives need the winning player,
                    // which ordinary quest objective credit does not expose.
                    if (InstanceScript* instance = trainer->GetInstanceScript())
                        instance->SetGuidData(trainer->GetEntry(), player->GetGUID());
                }
        }
    }

    for (size_t currentTeamID = 0; currentTeamID < MAX_PETBATTLE_TEAM; ++currentTeamID)
    {
        if (Teams[currentTeamID]->PlayerGuid)
        {
            auto player = ObjectAccessor::GetObjectInWorld(Teams[currentTeamID]->PlayerGuid, static_cast<Player*>(nullptr));
            if (!player)
                continue;

            //TODO: update achievement criteria
            player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);

            // Send battle result
            CombatResult = aborted ? PETBATTLE_RESULT_ABANDON : currentTeamID == winnerTeamID ? PETBATTLE_RESULT_WON : PETBATTLE_RESULT_LOOSE;

            player->GetSession()->SendPetBattleFinalRound(this);
            player->SetControlled(false, UNIT_STATE_ROOT);
            player->AddDelayedEvent(10, [player]() -> void { if (player) player->SummonLastSummonedBattlePet(); });
            player->_petBattleId.Clear();
            player->GetSession()->SendBattlePetJournal();

            if (BattleType == PETBATTLE_TYPE_PVP_MATCHMAKING)
                player->TeleportTo(player->m_recallLoc);
        }
        else
        {
            if (BattleType == PETBATTLE_TYPE_PVE && PveBattleType == PVE_PETBATTLE_WILD)
            {
                for (size_t i = 0; i < Teams[currentTeamID]->TeamPetCount; ++i)
                {
                    auto currentPet = Teams[currentTeamID]->TeamPets[i];
                    if (!currentPet || !currentPet->OriginalCreature)
                        continue;

                    auto wildPet = ObjectAccessor::GetObjectInOrOutOfWorld(currentPet->OriginalCreature, static_cast<Creature*>(nullptr));
                    if (!wildPet)
                        continue;

                    wildPet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
                    wildPet->SetControlled(false, UNIT_STATE_ROOT);
                    wildPet->_petBattleId.Clear();

                    sWildBattlePetMgr->LeaveBattle(wildPet, winnerTeamID != PETBATTLE_PVE_TEAM_ID);
                }
            }
        }
    }

    BattleStatus = PETBATTLE_STATUS_FINISHED;
}

void PetBattle::Update(uint32 diff)
{
    if (BattleStatus != PETBATTLE_STATUS_RUNNING)
        return;

    if (RoundStatus == PETBATTLE_ROUND_RUNNING)
        return;

    m_UpdateTimer.Update(diff);

    if (m_UpdateTimer.Passed())
    {
        m_UpdateTimer.Reset();

        bool const firstFinished = Teams[PETBATTLE_TEAM_1]->Update();
        bool const secondFinished = !firstFinished && Teams[PETBATTLE_TEAM_2]->Update();
        if (firstFinished || secondFinished)
        {
            Teams[PETBATTLE_TEAM_1]->Ready = false;
            Teams[PETBATTLE_TEAM_2]->Ready = false;
            return;
        }

        for (uint8 i = 0; i < MAX_PETBATTLE_TEAM; i++)
        {
            if (!Teams[i]->Ready && Teams[i]->ActivePetID >= 0 && Teams[i]->ActivePetID < MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS && Pets[Teams[i]->ActivePetID] && !Pets[Teams[i]->ActivePetID]->CanAttack())
            {
                if (Teams[i]->isRun)
                    Teams[i]->Ready = true;

                Teams[i]->ActiveAbilityId = 0;
                Teams[i]->activeAbilityTurn = 0;
                Teams[i]->activeAbilityTurnMax = 0;
            }
        }

        if (Teams[PETBATTLE_TEAM_1]->Ready && Teams[PETBATTLE_TEAM_2]->Ready)
        {
            ProceedRound();

            Teams[PETBATTLE_TEAM_1]->Ready = false;
            Teams[PETBATTLE_TEAM_2]->Ready = false;
        }
    }
}

void PetBattle::SwapPet(uint32 teamID, int32 newFrontPetID, bool initial)
{
    assert(teamID < MAX_PETBATTLE_TEAM);

    if (newFrontPetID < 0 || newFrontPetID >= MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS)
        return;

    if (!Pets[newFrontPetID])
        return;

    if (!Teams[teamID]->CanSwap(newFrontPetID))
        return;

    int32 oldTeamFrontPetID = Teams[teamID]->ActivePetID;
    bool const petChanged = newFrontPetID != oldTeamFrontPetID;

    if (petChanged)
        for (auto& aura : PetAuras)
            if (!aura->Expired)
                Cast(aura->CasterPetID, aura->AbilityID, 0, PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_OUT, PETBATTLE_CAST_TRIGGER_ALL);

    uint32 eventFlags = 0;

    if (!petChanged && BattleStatus != PETBATTLE_STATUS_CREATION)
        eventFlags |= PETBATTLE_EVENT_FLAG_SKIP_TURN;

    Teams[teamID]->ActivePetID = newFrontPetID;

    PetBattleEvent battleEvent(PETBATTLE_EVENT_PET_SWAP, newFrontPetID, eventFlags);
    battleEvent.UpdateFrontPet(!initial && newFrontPetID == oldTeamFrontPetID ? PETBATTLE_NULL_ID : newFrontPetID);

    RoundEvents.push_back(battleEvent);

    if (petChanged)
        for (auto& aura : PetAuras)
            if (!aura->Expired)
            {
                if (aura->AbilityID == 1904)
                {
                    if (aura->TargetPetID < MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS && Pets[aura->TargetPetID] && Pets[aura->TargetPetID]->TeamID == teamID)
                        Cast(aura->CasterPetID, aura->AbilityID, 0, PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_IN, PETBATTLE_CAST_TRIGGER_ALL, aura->TargetPetID);
                }
                else
                    Cast(aura->CasterPetID, aura->AbilityID, 0, PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_IN, PETBATTLE_CAST_TRIGGER_ALL);
            }

    Teams[teamID]->ActiveAbilityId = 0;
    Teams[teamID]->activeAbilityTurn = 0;
    Teams[teamID]->activeAbilityTurnMax = 0;

    if (!initial && Teams[teamID]->isRun)
        Teams[teamID]->Ready = true;

    FightedPets[newFrontPetID] = true;
}

bool PetBattle::CanCast(uint32 teamID, uint32 abilityID)
{
    assert(teamID < MAX_PETBATTLE_TEAM);

    auto team = Teams[teamID];
    if (!team)
        return false;

    if (!team->CanCastAny())
        return false;

    if (!sBattlePetAbilityStore.LookupEntry(abilityID))
        return false;

    if (abilityID == 665)
    {
        bool available = false;
        for (uint32 slot = 0; slot < team->TeamPetCount; ++slot)
        {
            auto pet = team->TeamPets[slot];
            if (pet && pet->ID != team->ActivePetID && !pet->IsAlive() && !pet->States[BATTLEPET_STATE_Special_ConsumedCorpse])
                available = true;
        }
        if (!available)
            return false;
    }

    for (uint32 i = 0; i < MAX_PETBATTLE_ABILITIES; i++)
        if (Pets[team->ActivePetID]->Abilities[i] == abilityID && Pets[team->ActivePetID]->Cooldowns[i] <= 0 && Pets[team->ActivePetID]->Lockdowns[i] <= 0)
            return true;

    return false;
}

void PetBattle::PrepareCast(uint32 teamID, uint32 abilityID)
{
    // PetBattleAbilityTurn abilityTurn;
    // memset(&abilityTurn, 0, sizeof(abilityTurn));

    uint8 maxTurnID = 0;
    for (auto const& abilityTurnInfo : sBattlePetAbilityTurnStore)
        if (abilityTurnInfo->BattlePetAbilityID == abilityID)
            maxTurnID = std::max(maxTurnID, abilityTurnInfo->OrderIndex);

    Teams[teamID]->ActiveAbilityId = abilityID;
    Teams[teamID]->activeAbilityTurn = 1;
    Teams[teamID]->activeAbilityTurnMax = maxTurnID;
    Teams[teamID]->Ready = true;
}

PetBattleCastResult PetBattle::Cast(uint32 casterPetID, uint32 abilityID, uint32 p_Turn, uint32 p_Turn0ProcCondition, uint32 p_TriggerFlag, int32 auraOwnerPetID)
{
    bool const racialTrigger = auraOwnerPetID == PETBATTLE_RACIAL_TRIGGER;
    bool const bendSwap = abilityID == 1904 && p_Turn0ProcCondition == PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_IN;
    bool const hourglassEnd = abilityID == 1932 && p_Turn0ProcCondition == PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END;
    bool const ownedCooldown = bendSwap || hourglassEnd;
    if (casterPetID >= MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS || !Pets[casterPetID])
        return PETBATTLE_CAST_INTERNAL_ERROR;
    if (racialTrigger && ((abilityID != 723 && abilityID != 724) || p_Turn != 1 ||
        p_Turn0ProcCondition != PETBATTLE_ABILITY_TURN0_PROC_ON_NONE || Pets[casterPetID]->IsAlive()))
        return PETBATTLE_CAST_INTERNAL_ERROR;
    if (auraOwnerPetID != PETBATTLE_NULL_SLOT && !racialTrigger &&
        (auraOwnerPetID < 0 || auraOwnerPetID >= MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS || !Pets[auraOwnerPetID] || p_Turn != 0 ||
            (p_Turn0ProcCondition != PETBATTLE_ABILITY_TURN0_PROC_ON_ABILITY && p_Turn0ProcCondition != PETBATTLE_ABILITY_TURN0_PROC_ON_AURA_REMOVED && !ownedCooldown)))
        return PETBATTLE_CAST_INTERNAL_ERROR;

    if (ownedCooldown)
    {
        if (p_Turn != 0 || auraOwnerPetID < 0 || auraOwnerPetID >= MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS || !Pets[auraOwnerPetID] ||
            (bendSwap && Teams[Pets[auraOwnerPetID]->TeamID]->ActivePetID == auraOwnerPetID))
            return PETBATTLE_CAST_INTERNAL_ERROR;
        bool ownedAura = false;
        for (auto aura : PetAuras)
            if (!aura->Expired && aura->AbilityID == abilityID && aura->CasterPetID == casterPetID && aura->TargetPetID == auraOwnerPetID)
                ownedAura = true;
        if (!ownedAura)
            return PETBATTLE_CAST_INTERNAL_ERROR;
    }

    if (!Pets[casterPetID])
        return PETBATTLE_CAST_INTERNAL_ERROR;

    if (RoundResult == PETBATTLE_ROUND_RESULT_CATCH_OR_KILL && auraOwnerPetID == PETBATTLE_NULL_SLOT)
        return PETBATTLE_CAST_OK;

    /// check if the ability exist
    auto abilityInfo = sBattlePetAbilityStore.LookupEntry(abilityID);
    if (!abilityInfo)
        return PETBATTLE_CAST_INVALID_ID;

    // These native turn1 sequences end with the user's sacrifice. An enemy
    // dying to an earlier damage row must not discard that terminal row (or
    // Armageddon's remaining enemy slots). Other stop/chain rules still apply.
    bool const selfSacrifice = p_Turn == 1 && p_Turn0ProcCondition == PETBATTLE_ABILITY_TURN0_PROC_ON_NONE &&
        (abilityID == 282 || abilityID == 321 || abilityID == 663 || abilityID == 836 || abilityID == 1025);
    bool const hourglassAttack = abilityID == 1927 && p_Turn == 1 && p_Turn0ProcCondition == PETBATTLE_ABILITY_TURN0_PROC_ON_NONE;
    if ((selfSacrifice || hourglassAttack) && !Pets[casterPetID]->IsAlive())
        return PETBATTLE_CAST_INTERNAL_ERROR;

    // States
    if (!p_Turn && p_TriggerFlag == PETBATTLE_CAST_TRIGGER_NONE)
        for (auto abilityStateInfo : sBattlePetAbilityStateStore)
            if (abilityStateInfo->BattlePetAbilityID == abilityID)
                SetPetState(casterPetID, casterPetID, 0, abilityStateInfo->BattlePetStateID, Pets[casterPetID]->States[abilityStateInfo->BattlePetStateID] + abilityStateInfo->Value);

    PetBattleAbilityTurn abilityTurn;
    memset(&abilityTurn, 0, sizeof abilityTurn);

    for (auto abilityTurnInfo : sBattlePetAbilityTurnStore)
    {
        if (abilityTurnInfo->BattlePetAbilityID != abilityID || abilityTurnInfo->OrderIndex != p_Turn)
            continue;

        if (abilityTurnInfo->OrderIndex == 0 && abilityTurnInfo->EventTypeEnum != int8(p_Turn0ProcCondition))
            continue;

        for (auto abilityEffectInfo : sBattlePetAbilityEffectStore)
        {
            if (abilityEffectInfo->BattlePetAbilityTurnID != abilityTurnInfo->ID)
                continue;

            PetBattleAbilityEffect abilityEffect;
            abilityEffect.EffectInfo = abilityEffectInfo;
            abilityEffect.AbilityTurn = &abilityTurn;
            abilityEffect.PetBattleInstance = this;
            abilityEffect.Caster = casterPetID;
            abilityEffect.StopChain = false;
            abilityEffect.AbilityID = abilityID;
            abilityEffect.IsTriggered = abilityInfo->Flags & BATTLEPET_ABILITY_FLAG_TRIGGER;
            abilityEffect.AllowDeadCaster = racialTrigger;
            abilityEffect.AuraOwnerPetID = auraOwnerPetID;
            abilityEffect.ReportFailAsImmune = false;
            abilityEffect.SelectTargets();
            // A proc's remove-aura row names the aura owner, which may differ
            // from the original hostile caster. Other effects keep their native
            // selectors (e.g. Clone/Murloc attacks still hit the opponent).
            if (auraOwnerPetID >= 0 && abilityEffectInfo->BattlePetEffectPropertiesID == 49)
                abilityEffect.Targets = {uint32(auraOwnerPetID)};

            if (!abilityEffect.Execute())
                abilityTurn.ChainFailure |= 1 << (abilityEffectInfo->OrderIndex - 1);

            bool const sacrificeDamage = (selfSacrifice || hourglassAttack) &&
                (abilityEffectInfo->BattlePetEffectPropertiesID == 24 || abilityEffectInfo->BattlePetEffectPropertiesID == 68);
            if (abilityEffect.StopChain && !sacrificeDamage)
                break;

            if (RoundResult == PETBATTLE_ROUND_RESULT_CATCH_OR_KILL && auraOwnerPetID == PETBATTLE_NULL_SLOT && !selfSacrifice && !hourglassAttack)
                return PETBATTLE_CAST_OK;
        }
    }

    if (!(p_TriggerFlag & PETBATTLE_CAST_TRIGGER_IGNORE_COOLDOWN))
    {
        int32 abilitySlot = -1;

        for (uint32 i = 0; i < MAX_PETBATTLE_ABILITIES; i++)
        {
            if (Pets[casterPetID]->Abilities[i] == abilityID)
            {
                abilitySlot = i;
                break;
            }
        }

        if (abilitySlot != -1)
            Pets[casterPetID]->Cooldowns[abilitySlot] = abilityInfo->Cooldown;
    }

    return PETBATTLE_CAST_OK;
}

bool PetBattle::AddAura(uint32 casterPetID, uint32 targetPetID, uint32 abilityID, int32 duration, int32 maxAllowed, uint32 fromAbilityEffectID, uint32 flags)
{
    if (!maxAllowed)
        maxAllowed = 1;
    int32 count = 0;
    uint32 existingSlot = 0;
    for (auto aura : PetAuras)
        if (!aura->Expired && aura->AbilityID == abilityID && aura->TargetPetID == targetPetID)
        {
            ++count;
            if (!existingSlot)
                existingSlot = aura->ID;
        }
    if (!flags && count >= maxAllowed)
    {
        int32 remove = 1 + count - maxAllowed;
        for (auto aura : PetAuras)
            if (remove > 0 && !aura->Expired && aura->AbilityID == abilityID && aura->TargetPetID == targetPetID)
            {
                aura->Expire(this);
                --remove;
            }
    }
    // Expiration callbacks may themselves add auras. Allocate after they finish,
    // and give every live instance its own protocol identity.
    uint32 slot = 1;
    for (auto aura : PetAuras)
        slot = std::max(slot, aura->ID + 1);
    if (flags && existingSlot)
        slot = existingSlot;
    PetBattleEvent event(flags && existingSlot ? PETBATTLE_EVENT_BUFF_CHANGE : PETBATTLE_EVENT_BUFF_APPLY, casterPetID, flags, fromAbilityEffectID, RoundTurn++, 0, 1);
    event.UpdateBuff(targetPetID, slot, abilityID, duration, 0);
    RoundEvents.push_back(event);
    if (flags)
        return false;
    auto aura = new PetBattleAura();
    aura->AbilityID = abilityID;
    aura->TriggerId = fromAbilityEffectID;
    aura->CasterPetID = casterPetID;
    aura->Duration = duration;
    aura->MaxDuration = duration;
    aura->Turn = 1;
    aura->TargetPetID = targetPetID;
    aura->ID = slot;
    aura->Expired = false;
    PetAuras.push_back(aura);
    aura->Apply(this);
    return true;
}

void PetBattle::SetPetState(uint32 sourcePetID, uint32 targetPetID, uint32 fromAbilityEffectID, uint32 state, int32 value, bool fromCapture, uint32 flags)
{
    // TC_LOG_DEBUG("battlepet", "PetBattleEvent::SetPetState state %u value %u States %u", state, value, Pets[targetPetID]->States[state]);

    if (state >= NUM_BATTLEPET_STATES)
        return;

    if (Pets[targetPetID]->States[state] == value)
        return;

    Pets[targetPetID]->States[state] = value;

    auto petStateEntry = sBattlePetStateStore.LookupEntry(state);
    if (petStateEntry && petStateEntry->Flags && !fromCapture)
    {
        PetBattleEvent battleEvent(PETBATTLE_EVENT_SET_STATE, sourcePetID, flags, fromAbilityEffectID, RoundTurn++, 0, 1);
        battleEvent.UpdateState(targetPetID, state, value);
        RoundEvents.push_back(battleEvent);
    }
}

void PetBattle::Kill(int8 killer, int8 target, uint32 killerAbibilityEffectID, bool fromCapture, uint32 flags)
{
    if (Teams[Pets[target]->TeamID]->ActivePetID == target)
    {
        Teams[Pets[target]->TeamID]->ActiveAbilityId = 0;
        Teams[Pets[target]->TeamID]->activeAbilityTurn = 0;
        Teams[Pets[target]->TeamID]->activeAbilityTurnMax = 0;
    }

    // These issued cooldown controllers benefit the remaining living team,
    // even after their holder dies. Other death cleanup stays native.
    for (auto& aura : PetAuras)
        if (aura->TargetPetID == target && aura->AbilityID != 1904 && aura->AbilityID != 1932)
            aura->Expire(this);

    // TC_LOG_DEBUG("battlepet", "PetBattle::Kill BATTLEPET_STATE_Special_ConsumedCorpse");

    SetPetState(killer, target, killerAbibilityEffectID, BATTLEPET_STATE_Is_Dead, 1, fromCapture, flags);

    RoundResult = PETBATTLE_ROUND_RESULT_CATCH_OR_KILL;
}

void PetBattle::Catch(int8 catcher, int8 catchedTarget, uint32 fromAbilityEffectID)
{
    Kill(catcher, catchedTarget, fromAbilityEffectID);

    Teams[Pets[catcher]->TeamID]->CapturedPet = catchedTarget;
    Pets[catchedTarget]->Caged = true;
    Pets[catchedTarget]->Captured = true;
    CatchedPetId = catchedTarget;

    PetXDied.push_back(catchedTarget);

    RoundResult = PETBATTLE_ROUND_RESULT_CATCH_OR_KILL;
}

uint32 PetBattle::GetFirstAttackingTeam()
{
    std::shared_ptr<BattlePetInstance> activePets[2];
    activePets[0] = Pets[Teams[0]->ActivePetID];
    activePets[1] = Pets[Teams[1]->ActivePetID];

    /// Deduce the first caster (based on front pets speed)
    auto pet0Speed = activePets[0]->GetSpeed();
    auto pet1Speed = activePets[1]->GetSpeed();

    return pet0Speed == pet1Speed ? rand() & 1 : pet1Speed > pet0Speed;
}

int32 PetBattle::GetForfeitHealthPenalityPct()
{
    if (BattleType != PETBATTLE_TYPE_PVE)
        return 0;
    return 10;
}


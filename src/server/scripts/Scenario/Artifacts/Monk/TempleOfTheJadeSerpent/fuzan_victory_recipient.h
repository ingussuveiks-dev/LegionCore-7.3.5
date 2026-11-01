#pragma once

#include "QuestObjectScope.h"

class Creature;

struct FuZanVictoryRecipient
{
    virtual ~FuZanVictoryRecipient() = default;
    virtual void RecordFuZanVictory(Creature* source,ObjectGuid const& owner,
        std::shared_ptr<QuestCancellationReceipt const> const& attempt) = 0;
};

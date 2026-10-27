#pragma once

namespace KingslayersReturn
{
    struct Exit;
    bool Owns(GameObject* portal, Exit const* receipt);

    bool Source(GameObject* portal)
    {
        return portal && portal->IsInWorld() && portal->isSpawned() && portal->GetEntry() == 251123 &&
            portal->GetDBTableGUIDLow() == 134046 && portal->GetMapId() == 1620;
    }

    bool Physical(Player* player, GameObject* portal, bool activated = false)
    {
        return Source(portal) && player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->HasPendingTeleport() && player->GetMap() == portal->GetMap() && player->InSamePhase(portal) &&
            !portal->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) &&
            (activated ? portal->getLootState() == GO_ACTIVATED && portal->GetGoState() == GO_STATE_ACTIVE &&
                portal->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE) :
                portal->getLootState() == GO_READY && !portal->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE)) &&
            player->canSeeOrDetect(portal) && player->IsWithinLOSInMap(portal) &&
            player->IsWithinDistInMap(portal, portal->GetInteractionDistance());
    }

    bool Award(Player* player, GameObject* portal)
    {
        uint32 quest = KingslayersAcquisition::Quest(player);
        Scenario* scenario = Source(portal) ? sScenarioMgr->GetScenario(portal->GetInstanceId()) : nullptr;
        return quest && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            player->GetMap() == portal->GetMap() && player->InSamePhase(portal) &&
            player->getClass() == CLASS_ROGUE && !player->isGameMaster() &&
            player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE && player->HasItemCount(128870, 1, true) &&
            scenario && scenario->GetScenarioId() == 1123 && scenario->GetCurrentStep() == 9 &&
            KingslayersAcquisition::Leaf(scenario, 48591) && !KingslayersAcquisition::Leaf(scenario, 48593);
    }

    struct Exit
    {
        ObjectGuid player, source;
        Map* map = nullptr;
        uint32 instance = 0, quest = 0;
        std::weak_ptr<uint8> lifetime;
        bool award = false, finished = false;

        bool SameAttempt(Player* owner) const
        {
            if (!owner || owner->GetGUID() != player || KingslayersAcquisition::Quest(owner) != quest) return false;
            if (!quest) return true; // genuine escape; never awards a new event
            auto status = owner->getQuestStatus(quest);
            return status && !lifetime.expired() && status->ScriptLifetime == lifetime.lock();
        }

        GameObject* Resolve(Player* owner) const
        {
            if (!SameAttempt(owner) || owner->GetMap() != map || owner->GetInstanceId() != instance) return nullptr;
            GameObject* portal = ObjectAccessor::GetGameObject(*owner, source);
            return Source(portal) && portal->GetMap() == map && Owns(portal, this) ? portal : nullptr;
        }
    };
}

struct go_kingslayers_return : GameObjectAI
{
    explicit go_kingslayers_return(GameObject* object) : GameObjectAI(object) { }
    std::shared_ptr<KingslayersReturn::Exit> receipt;
    uint32 remaining = 0;

    void Release()
    {
        // Only release the activation issued after our accepted GossipUse.
        // Do not retire the static exit or modify unrelated flags/states.
        if (receipt && KingslayersReturn::Source(go) && go->getLootState() == GO_ACTIVATED &&
            go->GetGoState() == GO_STATE_ACTIVE && go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE))
        {
            go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE);
            go->SetLootState(GO_READY);
            go->SetGoState(GO_STATE_READY);
        }
        receipt.reset(); remaining = 0;
    }

    void Reset() override { Release(); }
    void UpdateAI(uint32 diff) override
    {
        if (!receipt) return;
        if (!receipt->finished && remaining > diff) remaining -= diff;
        else Release();
    }

    bool GossipUse(Player* player) override
    {
        if (!KingslayersReturn::Source(go)) return false;
        if (receipt || !KingslayersReturn::Physical(player, go)) return true;
        auto current = std::make_shared<KingslayersReturn::Exit>();
        current->player = player->GetGUID(); current->source = go->GetGUID();
        current->map = go->GetMap(); current->instance = go->GetInstanceId();
        current->quest = KingslayersAcquisition::Quest(player);
        if (current->quest)
        {
            player->AddQuestDelayedEvent(current->quest, 0, [] { });
            auto status = player->getQuestStatus(current->quest);
            if (!status || !status->ScriptLifetime) return true;
            current->lifetime = status->ScriptLifetime;
        }
        current->award = KingslayersReturn::Award(player, go);
        receipt = current; remaining = 5000;
        // Retain native goober interaction, activation/animation and CastSpell.
        // Its spell sees GO_FLAG_IN_USE, which belongs to this receipt.
        return false;
    }
};

namespace KingslayersReturn
{
    bool Owns(GameObject* portal, Exit const* receipt)
    {
        auto* ai = portal ? dynamic_cast<go_kingslayers_return*>(portal->AI()) : nullptr;
        return ai && ai->receipt.get() == receipt;
    }
}

class spell_kingslayers_return : public SpellScript
{
    PrepareSpellScript(spell_kingslayers_return);
    std::weak_ptr<KingslayersReturn::Exit> original;
    bool scoped = false, issued = false;

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 215151 && info->Effects[EFFECT_0] && info->Effects[EFFECT_1] &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SEND_EVENT && info->Effects[EFFECT_0]->MiscValue == 51617 &&
            info->Effects[EFFECT_1]->Effect == SPELL_EFFECT_TELEPORT_L;
    }

    bool Load() override
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player || player->GetMapId() != 1620) return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        scoped = scenario && scenario->GetScenarioId() == 1123;
        std::list<GameObject*> portals;
        player->GetGameObjectListWithEntryInGrid(portals, 251123, 10.0f);
        for (GameObject* portal : portals)
            if (auto* ai = portal ? dynamic_cast<go_kingslayers_return*>(portal->AI()) : nullptr)
                if (ai->receipt && ai->receipt->Resolve(player) == portal && KingslayersReturn::Physical(player, portal, true))
                {
                    if (!original.expired()) { original.reset(); scoped = true; return true; }
                    original = ai->receipt; scoped = true;
                }
        return true;
    }

    bool Ready(Player* player) const
    {
        auto current = original.lock();
        GameObject* portal = current ? current->Resolve(player) : nullptr;
        return current && !current->finished && KingslayersReturn::Physical(player, portal, true);
    }

    SpellCastResult Check()
    {
        return !scoped || Ready(GetCaster() ? GetCaster()->ToPlayer() : nullptr) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Event(SpellEffIndex effect)
    {
        if (scoped) PreventHitDefaultEffect(effect); // targetless HIT precedes teleport
    }

    void Hit(SpellEffIndex effect)
    {
        if (scoped && (effect == EFFECT_0 || !Ready(GetCaster() ? GetCaster()->ToPlayer() : nullptr)))
            PreventHitDefaultEffect(effect);
    }

    void Depart(SpellEffIndex effect)
    {
        if (!scoped) return;
        PreventHitDefaultEffect(effect);
        Player* player = GetHitPlayer();
        auto current = original.lock();
        if (issued || !current || !Ready(player)) return;
        issued = true;
        GameObject* portal = current->Resolve(player);
        bool award = current->award && KingslayersReturn::Award(player, portal);
        // Consume before entering the native teleport path: a reentrant new
        // SpellScript must not reuse the same GO receipt during that call.
        current->finished = true;
        Player::ScopedTeleportDelay delay(*player);
        GetSpell()->EffectTeleportUnits(effect);
        if (!delay.IsPending() || player->GetTeleportDest().GetMapId() != 1220 ||
            player->GetTeleportDest().GetExactDist(-854.53f, 4597.28f, 748.81f) > 0.1f || !current->SameAttempt(player))
        {
            TC_LOG_WARN("scripts", "Kingslayers return: native215151 departure refused; no51617 grant; portal retained for retry");
            return;
        }
        portal = current->Resolve(player);
        if (!award) return; // completed/abandoned escape never grants progression
        if (!portal || !KingslayersReturn::Award(player, portal))
        {
            TC_LOG_WARN("scripts", "Kingslayers return: accepted departure lost original eligible source/attempt; no51617 grant");
            return;
        }
        // Preserve both native SendEvent criteria consumers, now after accepted
        // teleport and while ScopedTeleportDelay retains the source scenario.
        // This event has no World event_scripts/instance ProcessEvent handler.
        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT, 51617, 1);
        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51617, 1);
        Scenario* scenario = sScenarioMgr->GetScenario(current->instance);
        if (!KingslayersAcquisition::Leaf(scenario, 48593))
            TC_LOG_WARN("scripts", "Kingslayers return: native51617 not acknowledged after accepted departure");
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_kingslayers_return::Check);
        OnEffectHit += SpellEffectFn(spell_kingslayers_return::Event, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        OnEffectHitTarget += SpellEffectFn(spell_kingslayers_return::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_kingslayers_return::Depart, EFFECT_1, SPELL_EFFECT_TELEPORT_L);
    }
};

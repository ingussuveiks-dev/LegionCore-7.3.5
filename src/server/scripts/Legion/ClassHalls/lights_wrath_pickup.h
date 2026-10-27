#pragma once

namespace LightsWrathPickup
{
    enum class Phase { Waiting, Casting, Scene, Finishing, Portal, Cancelled, Done };
    struct Claim;
    bool Owns(GameObject* source, Claim const* claim);
    std::shared_ptr<Claim> Get(GameObject* source);

    bool Leaf(Scenario* scenario, uint32 tree)
    {
        if (!scenario || scenario->GetScenarioId() != 1065) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(tree);
        return found != progress->end() && found->second.Counter;
    }

    Scenario* Progress(GameObject* object)
    {
        return object ? sScenarioMgr->GetScenario(object->GetInstanceId()) : nullptr;
    }

    bool InScenario(Player* player)
    {
        if (!player || player->GetMapId() != 1583) return false;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1065;
    }

    bool Role(GameObject* object, uint32 entry)
    {
        if (!object || object->GetEntry() != entry || !object->IsInWorld() || object->IsPreDelete() ||
            !object->isSpawned() || !object->GetMap() || object->GetMapId() != 1583) return false;
        ObjectGuid source = object->GetSummonerGUID();
        return entry != 248572 || (source.IsCreatureOrVehicle() && source.GetEntry() == 104985);
    }

    bool Eligible(Player* player, GameObject* source, bool acceptedDeparture = false)
    {
        if (!player || !source || !player->IsInWorld() || !player->CanContact() || player->isGameMaster() ||
            !player->IsAlive() || (!acceptedDeparture && player->HasPendingTeleport()) || player->getClass() != CLASS_PRIEST ||
            player->GetMap() != source->GetMap() || !player->InSamePhase(source) ||
            player->GetQuestRewardStatus(41631) ||
            (player->GetQuestStatus(41631) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(41631) != QUEST_STATUS_COMPLETE)) return false;
        Scenario* scenario = Progress(source);
        return scenario && scenario->GetScenarioId() == 1065 && scenario->GetCurrentStep() >= 6 &&
            scenario->GetCurrentStep() <= 7 && Leaf(scenario, 47119);
    }

    bool CanTake(Player* player, GameObject* source)
    {
        return Role(source, 248572) && Eligible(player, source) &&
            !source->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) &&
            player->canSeeOrDetect(source) && player->IsWithinLOSInMap(source) && player->GetExactDist(source) <= 5.0f;
    }

    Player* FindPlayer(Map* map, ObjectGuid const& guid)
    {
        for (auto const& reference : map->GetPlayers())
            if (Player* player = reference.getSource())
                if (player->GetGUID() == guid) return player;
        return nullptr;
    }

    bool Equipped(Player* player)
    {
        Item* staff = player ? player->GetItemByEntry(128868) : nullptr;
        return staff && staff == player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    }

    thread_local std::weak_ptr<Claim> castRequest;
    struct CastScope
    {
        std::weak_ptr<Claim> previous;
        explicit CastScope(std::shared_ptr<Claim> const& current) : previous(std::move(castRequest)) { castRequest = current; }
        ~CastScope() { castRequest = std::move(previous); }
    };

    struct Claim : std::enable_shared_from_this<Claim>
    {
        ObjectGuid sourceGuid, playerGuid;
        Map* originMap = nullptr; // identity only; resolve actors for each operation
        uint32 instanceId = 0;
        std::weak_ptr<uint8> lifetime;
        Phase phase = Phase::Waiting;
        uint32 elapsed = 0, timer = 200;
        bool equipped = false, castWarning = false, portalWarning = false;

        bool Current(Player* player, GameObject* source) const
        {
            QuestStatusData* quest = player ? player->getQuestStatus(41631) : nullptr;
            return quest && !lifetime.expired() && quest->ScriptLifetime == lifetime.lock() &&
                player->GetGUID() == playerGuid && Role(source, 248572) && source->GetGUID() == sourceGuid &&
                source->GetMap() == originMap && source->GetInstanceId() == instanceId &&
                Owns(source, this) && CanTake(player, source) && player->HasItemCount(128868, 1, true);
        }

        void Update(GameObject* source, uint32 diff)
        {
            if (phase == Phase::Cancelled || phase == Phase::Done) return;
            if (!Role(source, 248572) || source->GetGUID() != sourceGuid || source->GetMap() != originMap ||
                source->GetInstanceId() != instanceId || !Owns(source, this)) { phase = Phase::Cancelled; return; }
            // Natural presentation completion is an earned world fact. A
            // subsequent disconnect or step transition cannot erase its exit.
            if (phase == Phase::Finishing || phase == Phase::Portal)
            {
                if (timer > diff) timer -= diff;
                else { phase = Phase::Portal; timer = 0; SpawnPortal(source); }
                return;
            }
            Player* player = source && source->GetMap() ? FindPlayer(source->GetMap(), playerGuid) : nullptr;
            if (!Current(player, source)) { phase = Phase::Cancelled; return; }
            elapsed += std::min(diff, 30000u - elapsed);
            if (elapsed >= 30000)
            {
                phase = Phase::Cancelled;
                TC_LOG_WARN("scripts", "Light's Wrath pickup: native presentation did not finish within30s; retain real item and chest for retry");
                return;
            }
            if (phase == Phase::Scene)
            {
                if (!player->HasAura(208011, playerGuid)) phase = Phase::Cancelled;
                return;
            }
            if (timer > diff) { timer -= diff; return; }
            if (phase == Phase::Casting)
            {
                phase = Phase::Waiting;
                if (!castWarning) TC_LOG_WARN("scripts", "Light's Wrath pickup: accepted cast supplied no native presentation; retry original receipt");
                castWarning = true;
            }
            if (phase != Phase::Waiting || !player->GetItemByEntry(128868)) return;
            phase = Phase::Casting;
            timer = 5000;
            CastScope request(shared_from_this());
            SpellCastResult result = player->CastSpell(player, 208011, false);
            if (result != SPELL_CAST_OK && phase == Phase::Casting)
            {
                phase = Phase::Waiting;
                if (!castWarning) TC_LOG_WARN("scripts", "Light's Wrath pickup: ordinary208011 rejected; retain completed storage and5s retry");
                castWarning = true;
            }
        }

        void SpawnPortal(GameObject* source)
        {
            std::list<GameObject*> portals;
            source->GetGameObjectListWithEntryInGrid(portals, 248574, 60.0f);
            for (GameObject* portal : portals)
                if (Role(portal, 248574) && portal->GetMap() == originMap && portal->InSamePhase(source) &&
                    portal->GetSummonerGUID() == sourceGuid && portal->GetOwnerGUID().IsEmpty() &&
                    portal->IsWithinDist3d(1152.58f, 1002.02f, 309.289f, 2.0f, false))
                {
                    Finish(source);
                    return;
                }
            Map* map = source->GetMap();
            GameObject* portal = source->SummonGameObject(248574, 1152.58f, 1002.02f, 309.289f, 0.633369f,
                0, 0, 0, 0, 0, ObjectGuid::Empty, nullptr, false);
            if (!Role(portal, 248574) || portal->GetMap() != originMap ||
                portal->GetSummonerGUID() != sourceGuid || !portal->GetOwnerGUID().IsEmpty())
            {
                if (!portalWarning) TC_LOG_WARN("scripts", "Light's Wrath pickup: completed native presentation retained after failed248574 creation; retry without consuming chest");
                portalWarning = true;
                timer = 2000;
                return;
            }
            if (GameObject* current = map->GetGameObject(sourceGuid))
                if (Role(current, 248572) && Owns(current, this)) Finish(current);
        }

        void Finish(GameObject* source)
        {
            phase = Phase::Done;
            source->SetLootState(GO_JUST_DEACTIVATED);
        }
    };
}

struct go_lights_wrath_pickup : GameObjectAI
{
    explicit go_lights_wrath_pickup(GameObject* object) : GameObjectAI(object) { }
    std::shared_ptr<LightsWrathPickup::Claim> claim;
    bool resetting = false;

    ~go_lights_wrath_pickup() override
    {
        if (claim && claim->phase != LightsWrathPickup::Phase::Done) claim->phase = LightsWrathPickup::Phase::Cancelled;
    }

    void Reset() override
    {
        if (claim && (claim->phase == LightsWrathPickup::Phase::Finishing ||
            claim->phase == LightsWrathPickup::Phase::Portal || claim->phase == LightsWrathPickup::Phase::Done)) return;
        if (claim && claim->phase != LightsWrathPickup::Phase::Done) claim->phase = LightsWrathPickup::Phase::Cancelled;
        claim.reset();
    }

    std::shared_ptr<LightsWrathPickup::Claim> Begin(Player* player)
    {
        if (resetting || claim || !LightsWrathPickup::CanTake(player, go) || !player->GetItemByEntry(128868)) return nullptr;
        player->AddQuestDelayedEvent(41631, 0, [] { });
        QuestStatusData* quest = player->getQuestStatus(41631);
        if (!quest || !quest->ScriptLifetime) return nullptr;
        auto current = std::make_shared<LightsWrathPickup::Claim>();
        current->sourceGuid = go->GetGUID(); current->playerGuid = player->GetGUID();
        current->originMap = go->GetMap(); current->instanceId = go->GetInstanceId(); current->lifetime = quest->ScriptLifetime;
        claim = current;
        // Only a real physical pickup/replay reaches this native ownership event.
        player->UpdateAchievementCriteria(CRITERIA_TYPE_OWN_ITEM, 128868, 1);
        return current;
    }

    bool GossipUse(Player* player) override
    {
        if (resetting || claim || go->getLootState() == GO_JUST_DEACTIVATED || !LightsWrathPickup::CanTake(player, go)) return true;
        if (player->HasAura(208011, player->GetGUID())) return true;
        if (player->GetItemByEntry(128868)) { Begin(player); return true; }
        return player->HasItemCount(128868, 1, true); // bank-only ownership cannot supply native force-equip
    }

    bool GossipHello(Player* player, bool) override { return GossipUse(player); }

    void OnStateChanged(uint32 state, Unit* unit) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (state != GO_JUST_DEACTIVATED || claim || resetting ||
            !LightsWrathPickup::CanTake(player, go) || !player->GetItemByEntry(128868)) return;
        Map* map = go->GetMap();
        ObjectGuid guid = go->GetGUID();
        auto current = Begin(player);
        if (current)
            if (GameObject* source = map->GetGameObject(guid))
                if (source->GetGUID() == guid && LightsWrathPickup::Role(source, 248572))
                    if (auto* ai = dynamic_cast<go_lights_wrath_pickup*>(source->AI()))
                        if (!ai->claim || ai->claim->phase != LightsWrathPickup::Phase::Done)
                            source->SetLootState(GO_READY); // retain real storage even after a reentrant AI replacement
    }

    void UpdateAI(uint32 diff) override
    {
        if (resetting) { resetting = false; return; }
        if (claim && claim->phase == LightsWrathPickup::Phase::Cancelled)
        {
            resetting = true;
            auto old = std::move(claim);
            Map* map = go->GetMap();
            if (map == old->originMap)
                if (Player* player = LightsWrathPickup::FindPlayer(map, old->playerGuid))
                    if (Aura* aura = player->GetAura(208011, old->playerGuid)) aura->Remove();
            return;
        }
        auto current = claim;
        if (current) current->Update(go, diff);
    }
};

namespace LightsWrathPickup
{
    std::shared_ptr<Claim> Get(GameObject* source)
    {
        auto* ai = source ? dynamic_cast<go_lights_wrath_pickup*>(source->AI()) : nullptr;
        return ai && !ai->resetting ? ai->claim : nullptr;
    }
    bool Owns(GameObject* source, Claim const* claim) { return Get(source).get() == claim; }

    GameObject* Resolve(Player* player, ObjectGuid const& guid)
    {
        return player && player->GetMap() ? player->GetMap()->GetGameObject(guid) : nullptr;
    }

    std::shared_ptr<Claim> Find(Player* player)
    {
        if (!player || !player->IsInWorld() || !player->GetMap()) return nullptr;
        std::list<GameObject*> objects;
        player->GetGameObjectListWithEntryInGrid(objects, 248572, 5.0f);
        std::shared_ptr<Claim> result;
        for (GameObject* source : objects)
            if (auto current = Get(source))
                if (current->phase != Phase::Cancelled && current->phase != Phase::Done && current->Current(player, source))
                {
                    if (result) return nullptr;
                    result = current;
                }
        return result;
    }
}

class spell_lights_wrath_pickup_cast : public SpellScript
{
    PrepareSpellScript(spell_lights_wrath_pickup_cast);
    std::weak_ptr<LightsWrathPickup::Claim> original;
    bool bound = false, scoped = false;
    bool Load() override
    {
        scoped = LightsWrathPickup::InScenario(GetCaster() ? GetCaster()->ToPlayer() : nullptr);
        return true;
    }

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 208011 && info->Effects[EFFECT_0] && info->Effects[EFFECT_1] && info->Effects[EFFECT_2] &&
            info->Effects[EFFECT_0]->ApplyAuraName == SPELL_AURA_ACTIVATE_SCENE && info->Effects[EFFECT_0]->MiscValue == 1244 &&
            info->Effects[EFFECT_1]->ApplyAuraName == SPELL_AURA_MOD_ROOT &&
            info->Effects[EFFECT_2]->Effect == SPELL_EFFECT_FORCE_EQUIP_ITEM && info->Effects[EFFECT_2]->MiscValue == 128868 &&
            info->GetDuration() == 9000;
    }

    SpellCastResult Check()
    {
        if (!scoped) return SPELL_CAST_OK;
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        auto current = bound ? original.lock() : LightsWrathPickup::castRequest.lock();
        GameObject* source = current ? LightsWrathPickup::Resolve(player, current->sourceGuid) : nullptr;
        if (!current || current->phase != LightsWrathPickup::Phase::Casting || !current->Current(player, source) ||
            LightsWrathPickup::Find(player) != current) return SPELL_FAILED_BAD_TARGETS;
        original = current;
        bound = true;
        return SPELL_CAST_OK;
    }

    void Hit(SpellEffIndex effect)
    {
        if (!scoped)
        {
            if (LightsWrathPickup::InScenario(GetCaster() ? GetCaster()->ToPlayer() : nullptr)) PreventHitDefaultEffect(effect);
            return;
        }
        auto current = original.lock();
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        GameObject* source = current ? LightsWrathPickup::Resolve(player, current->sourceGuid) : nullptr;
        if (!current || !current->Current(player, source) || LightsWrathPickup::Find(player) != current || GetHitUnit() != player ||
            (current->phase != LightsWrathPickup::Phase::Casting && current->phase != LightsWrathPickup::Phase::Scene))
            PreventHitDefaultEffect(effect);
    }

    void After()
    {
        if (!scoped) return;
        auto current = original.lock();
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        GameObject* source = current ? LightsWrathPickup::Resolve(player, current->sourceGuid) : nullptr;
        if (current && current->phase == LightsWrathPickup::Phase::Scene && current->Current(player, source))
            current->equipped = LightsWrathPickup::Equipped(player) && player->GetQuestObjectiveData(41631, 105109);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_lights_wrath_pickup_cast::Check);
        OnEffectHitTarget += SpellEffectFn(spell_lights_wrath_pickup_cast::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        AfterHit += SpellHitFn(spell_lights_wrath_pickup_cast::After);
    }
};

class spell_lights_wrath_pickup_scene : public AuraScript
{
    PrepareAuraScript(spell_lights_wrath_pickup_scene);
    std::weak_ptr<LightsWrathPickup::Claim> original;
    bool Load() override { return GetUnitOwner() && LightsWrathPickup::InScenario(GetUnitOwner()->ToPlayer()); }

    void Apply(AuraEffect const*, AuraEffectHandleModes)
    {
        Player* player = GetUnitOwner()->ToPlayer();
        auto current = LightsWrathPickup::Find(player);
        if (!current || GetCasterGUID() != player->GetGUID() || current->phase != LightsWrathPickup::Phase::Casting)
        {
            Remove();
            return;
        }
        original = current;
        current->phase = LightsWrathPickup::Phase::Scene;
    }

    void Tick(uint32, AuraEffect*)
    {
        auto current = original.lock();
        Player* player = GetUnitOwner()->ToPlayer();
        GameObject* source = current ? LightsWrathPickup::Resolve(player, current->sourceGuid) : nullptr;
        if (!current || current->phase != LightsWrathPickup::Phase::Scene || !current->Current(player, source))
        {
            if (current && current->phase != LightsWrathPickup::Phase::Done) current->phase = LightsWrathPickup::Phase::Cancelled;
            Remove();
        }
    }

    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        auto current = original.lock();
        if (!current || current->phase != LightsWrathPickup::Phase::Scene) return;
        Player* player = GetUnitOwner()->ToPlayer();
        GameObject* source = LightsWrathPickup::Resolve(player, current->sourceGuid);
        bool nativeExpiry = GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE &&
            GetDuration() == 0 && GetMaxDuration() == 9000 && GetAura()->GetAllDuration() >= 9000;
        // Native SceneCompleted removes this aura with DEFAULT; both client
        // Complete and Canceled packets use that native visual-completion path.
        // Keep the native9s presentation bound even after an early scene ACK.
        bool nativeSceneEnd = GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEFAULT &&
            GetMaxDuration() == 9000 && player->HasSceneStatus(1244, SCENE_COMPLETE);
        if ((nativeExpiry || nativeSceneEnd) && current->Current(player, source) && current->equipped &&
            player->GetQuestObjectiveData(41631, 105109))
        {
            current->phase = LightsWrathPickup::Phase::Finishing;
            current->timer = 9000 - std::min(uint32(std::max(0, GetAura()->GetAllDuration())), 9000u);
        }
        else
            current->phase = LightsWrathPickup::Phase::Cancelled;
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_lights_wrath_pickup_scene::Apply, EFFECT_1, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        OnEffectUpdate += AuraEffectUpdateFn(spell_lights_wrath_pickup_scene::Tick, EFFECT_1, SPELL_AURA_MOD_ROOT);
        AfterEffectRemove += AuraEffectRemoveFn(spell_lights_wrath_pickup_scene::Removed, EFFECT_1, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL);
    }
};

namespace LightsWrathPickup
{
    struct Exit;
    bool OwnsExit(GameObject* portal, Exit const* receipt);

    bool ScopedPortal(GameObject* portal)
    {
        if (!portal || portal->GetMapId() != 1583) return false;
        Scenario* scenario = Progress(portal);
        ObjectGuid source = portal->GetSummonerGUID();
        return (scenario && scenario->GetScenarioId() == 1065) ||
            (source.IsGameObject() && source.GetEntry() == 248572);
    }

    bool Portal(GameObject* portal)
    {
        if (!Role(portal, 248574) || !portal->GetOwnerGUID().IsEmpty()) return false;
        ObjectGuid source = portal->GetSummonerGUID();
        return source.IsGameObject() && source.GetEntry() == 248572;
    }

    bool PhysicalExit(Player* player, GameObject* portal)
    {
        return Portal(portal) && player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->HasPendingTeleport() && player->GetMap() == portal->GetMap() && player->InSamePhase(portal) &&
            !portal->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE) &&
            player->canSeeOrDetect(portal) && player->IsWithinLOSInMap(portal) && player->GetExactDist(portal) <= 5.0f;
    }

    bool AwardState(Player* player, GameObject* portal, bool acceptedDeparture = false)
    {
        // No pending-teleport rejection here: ScopedTeleportDelay keeps the
        // accepted departure in this source scenario until its real credit.
        return Portal(portal) && Eligible(player, portal, acceptedDeparture) && !player->isGameMaster() &&
            Progress(portal)->GetCurrentStep() == 6 && Leaf(Progress(portal), 47214) && player->HasItemCount(128868, 1, true) &&
            player->GetQuestObjectiveData(41631, 105109) && !player->HasAura(208011, player->GetGUID());
    }

    struct Exit
    {
        ObjectGuid sourceGuid, playerGuid;
        Map* originMap = nullptr;
        uint32 instanceId = 0;
        std::weak_ptr<uint8> lifetime;
        bool questBound = false, award = false;

        bool SameAttempt(Player* player) const
        {
            QuestStatusData* quest = player ? player->getQuestStatus(41631) : nullptr;
            return player && player->GetGUID() == playerGuid &&
                (questBound ? quest && !lifetime.expired() && quest->ScriptLifetime == lifetime.lock() :
                    !quest || quest->Status == QUEST_STATUS_NONE);
        }

        bool Current(Player* player, GameObject* portal) const
        {
            return SameAttempt(player) && Portal(portal) && portal->GetGUID() == sourceGuid &&
                portal->GetMap() == originMap && portal->GetInstanceId() == instanceId &&
                player->GetMap() == originMap && OwnsExit(portal, this);
        }
    };
}

struct go_lights_wrath_return : SmartGameObjectAI
{
    explicit go_lights_wrath_return(GameObject* object) : SmartGameObjectAI(object) { }
    std::shared_ptr<LightsWrathPickup::Exit> receipt;
    uint32 remaining = 0;

    void Reset() override { receipt.reset(); remaining = 0; SmartGameObjectAI::Reset(); }
    void UpdateAI(uint32 diff) override
    {
        if (!LightsWrathPickup::ScopedPortal(go)) { SmartGameObjectAI::UpdateAI(diff); return; }
        if (remaining > diff) remaining -= diff;
        else { receipt.reset(); remaining = 0; }
    }
    bool GossipUse(Player* player) override
    {
        if (!LightsWrathPickup::ScopedPortal(go)) return SmartGameObjectAI::GossipUse(player);
        if (!LightsWrathPickup::PhysicalExit(player, go)) return true;
        // Abandonment or an already completed scenario may still use the real
        // exit. They never receive a new final event from this escape path.
        if (receipt && receipt->Current(player, go)) { remaining = 5000; return false; }
        player->AddQuestDelayedEvent(41631, 0, [] { });
        auto current = std::make_shared<LightsWrathPickup::Exit>();
        current->sourceGuid = go->GetGUID(); current->playerGuid = player->GetGUID();
        current->originMap = go->GetMap(); current->instanceId = go->GetInstanceId();
        if (QuestStatusData* quest = player->getQuestStatus(41631))
            if (quest->Status != QUEST_STATUS_NONE && quest->ScriptLifetime)
            {
                current->questBound = true;
                current->lifetime = quest->ScriptLifetime;
            }
        current->award = LightsWrathPickup::AwardState(player, go);
        receipt = current;
        remaining = 5000;
        return false;
    }
    bool GossipHello(Player* player, bool isUse) override
    {
        if (!LightsWrathPickup::ScopedPortal(go)) return SmartGameObjectAI::GossipHello(player, isUse);
        return !LightsWrathPickup::PhysicalExit(player, go);
    }
};

namespace LightsWrathPickup
{
    bool OwnsExit(GameObject* portal, Exit const* receipt)
    {
        auto* ai = portal ? dynamic_cast<go_lights_wrath_return*>(portal->AI()) : nullptr;
        return ai && ai->receipt.get() == receipt;
    }
}

class spell_lights_wrath_return : public SpellScript
{
    PrepareSpellScript(spell_lights_wrath_return);
    std::weak_ptr<LightsWrathPickup::Exit> original;
    bool scoped = false, issued = false;

    bool Load() override
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player || player->GetMapId() != 1583) return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        scoped = scenario && scenario->GetScenarioId() == 1065;
        std::list<GameObject*> portals;
        player->GetGameObjectListWithEntryInGrid(portals, 248574, 5.0f);
        for (GameObject* portal : portals)
            if (portal)
            if (auto* ai = dynamic_cast<go_lights_wrath_return*>(portal->AI()))
                if (ai->receipt && ai->receipt->Current(player, portal) && LightsWrathPickup::PhysicalExit(player, portal))
                {
                    if (!original.expired()) { original.reset(); scoped = true; return true; } // reject in Check; never disable this guard
                    original = ai->receipt;
                    scoped = true; // also guard a genuine exit after scenario cleanup
                }
        return true;
    }

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 218172 && info->Effects[EFFECT_1] && info->Effects[EFFECT_1]->Effect == SPELL_EFFECT_TELEPORT_L;
    }

    bool Ready(Player* player) const
    {
        auto current = original.lock();
        GameObject* portal = current ? LightsWrathPickup::Resolve(player, current->sourceGuid) : nullptr;
        return current && current->Current(player, portal) && LightsWrathPickup::PhysicalExit(player, portal);
    }

    SpellCastResult Check()
    {
        return !scoped || Ready(GetCaster() ? GetCaster()->ToPlayer() : nullptr) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Hit(SpellEffIndex effect)
    {
        if (scoped && !Ready(GetCaster() ? GetCaster()->ToPlayer() : nullptr)) PreventHitDefaultEffect(effect);
    }

    void Depart(SpellEffIndex effect)
    {
        if (!scoped) return;
        PreventHitDefaultEffect(effect);
        Player* player = GetHitPlayer();
        auto current = original.lock();
        if (issued || !current || !Ready(player)) return;
        issued = true;
        GameObject* portal = LightsWrathPickup::Resolve(player, current->sourceGuid);
        bool award = current->award && LightsWrathPickup::AwardState(player, portal);
        Player::ScopedTeleportDelay delay(*player);
        GetSpell()->EffectTeleportUnits(effect);
        if (!delay.IsPending() || player->GetTeleportDest().GetMapId() != 1220 ||
            player->GetTeleportDest().GetExactDist(-828.61f, 4371.63f, 738.64f) > 0.1f || !current->SameAttempt(player))
        {
            TC_LOG_WARN("scripts", "Light's Wrath return: original native218172 departure not accepted; no49875 grant");
            return;
        }
        portal = LightsWrathPickup::Resolve(player, current->sourceGuid);
        if (award && current->Current(player, portal) && LightsWrathPickup::AwardState(player, portal, true))
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49875);
        else if (award)
            TC_LOG_WARN("scripts", "Light's Wrath return: accepted departure lost its original eligible source/attempt; no49875 grant");
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_lights_wrath_return::Check);
        OnEffectHitTarget += SpellEffectFn(spell_lights_wrath_return::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_lights_wrath_return::Depart, EFFECT_1, SPELL_EFFECT_TELEPORT_L);
    }
};

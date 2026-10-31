#ifndef SHARASDAL_ACQUISITION_H
#define SHARASDAL_ACQUISITION_H

#include "Conversation.h"
#include "Item.h"
#include "InstanceScript.h"
#include "SpellAuraEffects.h"

namespace SharasdalAcquisition
{
    bool Physical(Player* player, GameObject* weapon, bool opening)
    {
        Scenario* scenario = player ? sScenarioMgr->GetScenario(player->GetInstanceId()) : nullptr;
        bool acquired = !opening && scenario && scenario->GetScenarioId() == 1066 && scenario->GetCurrentStep() == 6;
        return player && weapon && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_SHAMAN &&
            weapon->IsInWorld() && weapon->isSpawned() && weapon->GetEntry() == 249342 &&
            weapon->GetDBTableGUIDLow() == 134299 && weapon->GetMapId() == 1600 &&
            player->GetMap() == weapon->GetMap() && player->GetInstanceId() == weapon->GetInstanceId() &&
            (acquired || (player->InSamePhase(weapon) && player->canSeeOrDetect(weapon))) && player->IsWithinLOSInMap(weapon) &&
            (acquired ? player->GetExactDist(weapon) <= 10.0f :
                player->IsWithinDistInMap(weapon, opening ? weapon->GetInteractionDistance() : 10.0f)) &&
            !weapon->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) &&
            !player->GetQuestRewardStatus(40341) &&
            (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
    }

    bool Progress(Player* player, bool opening)
    {
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1066 || scenario->GetCurrentStep() < 5 ||
            scenario->GetCurrentStep() > (opening ? 5u : 6u))
            return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto lady = progress->find(47380), wall = progress->find(47133);
        return lady != progress->end() && lady->second.Counter && wall != progress->end() && wall->second.Counter;
    }

    bool CanTake(Player* player, GameObject* weapon)
    {
        return Physical(player, weapon, true) && Progress(player, true);
    }

    bool Equipped(Player* player, ObjectGuid const& guid)
    {
        Item* parent = player->GetItemByGuid(guid);
        Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        return parent && parent->GetEntry() == 128911 &&
            player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == parent &&
            child && child->GetEntry() == 128934 && parent->GetChildItem() == child->GetGUID();
    }
}

class go_art_sharasdal : public GameObjectScript
{
public:
    go_art_sharasdal() : GameObjectScript("go_art_sharasdal") { }
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        ObjectGuid claimant, item, outro;
        std::weak_ptr<uint8> attempt;
        uint32 stage = 0, elapsed = 0, retry = 0, tries = 0;
        bool busy = false, sceneOwned = false;
        ObjectGuid SourceGUID() const { return go->GetGUID(); }

        bool Live(Player* player)
        {
            auto status = player ? player->getQuestStatus(40341) : nullptr;
            Item* parent = player ? player->GetItemByEntry(128911) : nullptr;
            return !attempt.expired() && status && status->ScriptLifetime == attempt.lock() &&
                player->GetGUID() == claimant && parent && parent->GetGUID() == item &&
                SharasdalAcquisition::Physical(player, go, false) && SharasdalAcquisition::Progress(player, false);
        }
        bool Allow(Player* player, uint32 spell)
        {
            return Live(player) && ((stage == 1 && (spell == 209683 || spell == 209684)) ||
                (stage == 3 && busy && spell == 209685));
        }
        void Cancel(Player* player)
        {
            bool ownAura = sceneOwned && player && player->GetGUID() == claimant;
            stage = elapsed = retry = tries = 0;
            busy = sceneOwned = false;
            claimant.Clear(); item.Clear(); attempt.reset();
            if (ownAura)
                player->RemoveAurasDueToSpell(209684);
        }
        bool GossipHello(Player* player, bool) override
        {
            return stage || !SharasdalAcquisition::CanTake(player, go);
        }
        void Reset() override { if (stage) Cancel(ObjectAccessor::GetPlayer(*go, claimant)); }
        void Begin(Player* player)
        {
            if (stage || !SharasdalAcquisition::CanTake(player, go) || player->HasAura(209684)) return;
            Item* parent = player->GetItemByEntry(128911);
            auto status = player->getQuestStatus(40341);
            if (!parent || !status) return;
            // Loaded quests allocate this lease only when their first guarded
            // script action needs it; a real owned-item retry must work too.
            if (!status->ScriptLifetime) player->AddQuestDelayedEvent(40341, 0, []() { });
            if (!status->ScriptLifetime) return;
            claimant = player->GetGUID(); item = parent->GetGUID(); attempt = status->ScriptLifetime;
            stage = 1; elapsed = retry = tries = 0;
            // The already-at-destination case has no native jump. Play the
            // same acquisition scene only for this real stored-item claimant.
            uint32 spell = player->GetExactDist(go) < 0.01f ? 209684 : 209683;
            if (player->CastSpell(player, spell, true) != SPELL_CAST_OK)
            {
                Cancel(player);
                TC_LOG_WARN("scripts", "Sharasdal pickup native jump/scene failed; retain looted item and object for retry");
            }
        }
        void SceneStarted(Player* player)
        {
            if (Allow(player, 209684) && player->HasAura(209684))
            { stage = 2; elapsed = 0; sceneOwned = true; }
        }
        void SceneEnded(Player* player, bool complete)
        {
            if (stage != 2 || !Live(player)) return;
            sceneOwned = false;
            if (complete) { stage = 3; elapsed = retry = tries = 0; }
            else Cancel(player);
        }
        void Close()
        {
            stage = 5;
            claimant.Clear(); item.Clear(); attempt.reset();
            go->SetLootState(GO_JUST_DEACTIVATED);
        }
        void UpdateAI(uint32 diff) override
        {
            if (!stage || stage == 5 || busy) return;
            Player* player = ObjectAccessor::GetPlayer(*go, claimant);
            if (!Live(player)) { Cancel(player); return; }
            elapsed += diff;
            if (stage == 1 || stage == 2)
            {
                if ((stage == 1 && elapsed >= 5000) ||
                    (stage == 2 && (!player->HasAura(209684) || elapsed >= 20000)))
                {
                    Cancel(player);
                    TC_LOG_WARN("scripts", "Sharasdal pickup has no acknowledged native jump/scene completion; retain item and object for retry");
                }
                return;
            }
            if (retry > diff) { retry -= diff; return; }
            retry = 1000;
            if (stage == 3)
            {
                busy = true;
                SpellCastResult result = player->CastSpell(player, 209685, true);
                busy = false;
                if (!Live(player)) { Cancel(player); return; }
                if (result != SPELL_CAST_OK || !SharasdalAcquisition::Equipped(player, item))
                {
                    Cancel(player);
                    TC_LOG_WARN("scripts", "Sharasdal pickup native209685 did not equip the actual looted parent/child; retain native inventory errors and retry");
                    return;
                }
                Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
                if (scenario->GetCurrentStep() != 6 || !player->GetQuestObjectiveData(40341, 105787))
                {
                    if (++tries >= 30)
                    {
                        Cancel(player);
                        TC_LOG_WARN("scripts", "Sharasdal pickup native acquisition receipt missing after thirty attempts; retain item and object for retry");
                    }
                    return;
                }
                stage = 4; tries = 0;
            }
            // Native219 producer209743 is a void effect. Use its exact bool
            // factory and metadata so only an actually registered1923 is kept.
            bool actors = true;
            uint32 const entries[3] = {102826,105298,104853};
            uint64 const spawns[3] = {362093,362092,362094};
            for (uint8 i = 0; i < 3; ++i)
            {
                Creature* actor = player->FindNearestCreature(entries[i], player->GetVisibilityRange());
                if (!actor || !actor->IsAlive() || actor->GetDBTableGUIDLow() != spawns[i]) actors = false;
            }
            if (actors)
            {
                auto conversation = new Conversation;
                if (conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),
                    1923, player, sSpellMgr->GetSpellInfo(209743), *player))
                {
                    player->BindQuestObject(40341, conversation);
                    outro = conversation->GetGUID();
                    Close();
                    return;
                }
                delete conversation;
            }
            if (++tries >= 30)
            {
                TC_LOG_WARN("scripts", "Sharasdal pickup outro1923 unavailable after thirty attempts; real acquired weapon/progress retained, optional presentation unresolved");
                Close();
            }
        }
    };
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        auto ai = dynamic_cast<AI*>(object->AI());
        if (!ai || ai->stage || !SharasdalAcquisition::CanTake(player, object)) return true;
        if (player->HasItemCount(128911, 1, true))
        {
            if (player->GetItemByEntry(128911)) object->SetLootState(GO_JUST_DEACTIVATED, player);
            return true; // A bank-only parent cannot create a second artifact.
        }
        return false; // Native loot/storage and full-bag failure remain intact.
    }
    void OnLootStateChanged(GameObject* object, uint32 state, Unit* target) override
    {
        Player* player = target ? target->ToPlayer() : nullptr;
        if (!object || state != GO_JUST_DEACTIVATED || object->getLootState() != GO_JUST_DEACTIVATED ||
            !SharasdalAcquisition::CanTake(player, object) || !player->GetItemByEntry(128911)) return;
        auto ai = dynamic_cast<AI*>(object->AI());
        if (!ai) return;
        object->SetLootState(GO_READY);
        ai->Begin(player);
    }
};

namespace SharasdalAcquisition
{
    go_art_sharasdal::AI* Controller(Player* player, ObjectGuid const& guid = ObjectGuid::Empty)
    {
        if (!player || player->GetMapId() != 1600) return nullptr;
        InstanceScript* instance = player->GetInstanceScript();
        GameObject* object = guid.IsEmpty() ? (instance ? instance->GetGameObjectByEntry(249342) : nullptr) :
            ObjectAccessor::GetGameObject(*player, guid);
        if (!object || object->GetEntry() != 249342 || object->GetDBTableGUIDLow() != 134299) return nullptr;
        return dynamic_cast<go_art_sharasdal::AI*>(object->AI());
    }
}

class spell_sharasdal_acquisition : public SpellScript
{
    PrepareSpellScript(spell_sharasdal_acquisition);
    bool attemptedEquip = false;
    ObjectGuid source;
    bool Ready()
    {
        auto ai = SharasdalAcquisition::Controller(GetCaster()->ToPlayer(), source);
        return ai && ai->Allow(GetCaster()->ToPlayer(), GetSpellInfo()->Id);
    }
    SpellCastResult Check()
    {
        auto ai = SharasdalAcquisition::Controller(GetCaster()->ToPlayer());
        if (!ai || !ai->Allow(GetCaster()->ToPlayer(), GetSpellInfo()->Id)) return SPELL_FAILED_NOT_HERE;
        source = ai->SourceGUID();
        return SPELL_CAST_OK;
    }
    void SceneCredit(SpellEffIndex index) { PreventHitDefaultEffect(index); }
    void Equip(SpellEffIndex index)
    {
        Player* player = GetCaster()->ToPlayer();
        auto ai = SharasdalAcquisition::Controller(player, source);
        if (Ready() && ai)
        {
            // Execute the real249 handler before the native61 event. It keeps
            // CanEquipItem/ChildItem, inventory errors and actual SwapItem.
            attemptedEquip = true;
            GetSpell()->EffectForceEquipItem(EFFECT_1);
        }
        if (!Ready() || !ai || !SharasdalAcquisition::Equipped(player, ai->item))
            PreventHitDefaultEffect(index);
    }
    void AfterEquip(SpellEffIndex index)
    {
        auto ai = SharasdalAcquisition::Controller(GetCaster()->ToPlayer(), source);
        Scenario* scenario = sScenarioMgr->GetScenario(GetCaster()->ToPlayer()->GetInstanceId());
        if (index == EFFECT_1 || !attemptedEquip || !Ready() || !ai ||
            !SharasdalAcquisition::Equipped(GetCaster()->ToPlayer(), ai->item) ||
            !scenario || scenario->GetCurrentStep() != 6)
            PreventHitDefaultEffect(index);
    }
    void SceneReceipt()
    {
        if (auto ai = SharasdalAcquisition::Controller(GetCaster()->ToPlayer(), source))
            ai->SceneStarted(GetCaster()->ToPlayer());
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sharasdal_acquisition::Check);
        if (m_scriptSpellId == 209684)
        {
            OnEffectHitTarget += SpellEffectFn(spell_sharasdal_acquisition::SceneCredit, EFFECT_2, SPELL_EFFECT_SEND_EVENT);
            AfterHit += SpellHitFn(spell_sharasdal_acquisition::SceneReceipt);
        }
        if (m_scriptSpellId == 209685)
        {
            OnEffectHitTarget += SpellEffectFn(spell_sharasdal_acquisition::Equip, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
            OnEffectHitTarget += SpellEffectFn(spell_sharasdal_acquisition::AfterEquip, EFFECT_1, SPELL_EFFECT_FORCE_EQUIP_ITEM);
            OnEffectHitTarget += SpellEffectFn(spell_sharasdal_acquisition::AfterEquip, EFFECT_2, SPELL_EFFECT_KILL_CREDIT2);
            OnEffectHitTarget += SpellEffectFn(spell_sharasdal_acquisition::AfterEquip, EFFECT_3, SPELL_EFFECT_TRIGGER_SPELL);
        }
    }
};

class aura_sharasdal_acquisition : public AuraScript
{
    PrepareAuraScript(aura_sharasdal_acquisition);
    ObjectGuid source;
    bool Load() override
    {
        Player* player = GetUnitOwner()->ToPlayer();
        auto ai = SharasdalAcquisition::Controller(player);
        if (!ai || !ai->Allow(player, 209684)) return false;
        source = ai->SourceGUID();
        return true;
    }
    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        Player* player = GetTarget()->ToPlayer();
        if (auto ai = SharasdalAcquisition::Controller(player, source))
            ai->SceneEnded(player, GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE ||
                player->HasSceneStatus(1255, SCENE_COMPLETE));
    }
    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(aura_sharasdal_acquisition::Removed, EFFECT_0,
            SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
    }
};

#endif

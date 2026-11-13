#ifndef SHARASDAL_LIVING_FINALE_H
#define SHARASDAL_LIVING_FINALE_H

#include "Conversation.h"
#include "InstanceScript.h"
#include "QuestData.h"
#include "Vehicle.h"

// Functional reconstruction, authorized on 2026-11-13. The original server
// activation predicate is unavailable: use an actual scaled lethal hit, not
// an invented percentage. The finish waits for native boarding, publication
// of the authored rage line, and an actual successful 209487 hit.
namespace SharasdalFinale
{
    inline bool PlayerReady(Player* player, Creature* lady)
    {
        return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_SHAMAN &&
            player->GetMap() == lady->GetMap() && player->InSamePhase(lady) &&
            player->IsWithinDistInMap(lady, 100.0f) && !player->GetQuestRewardStatus(40341) &&
            (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
    }

    class DefeatConversation final : public Conversation
    {
        ObjectGuid _lady, _grash, _erunak, _weapon;
        bool _locked = false;
    public:
        DefeatConversation(Creature* lady, Creature* grash, Creature* erunak) :
            _lady(lady->GetGUID()), _grash(grash->GetGUID()), _erunak(erunak->GetGUID()) { }

        bool Reached(uint8 index, uint32 line) const
        {
            auto start = playing.find(GetCasterGUID());
            auto data = GetLine(index);
            return IsInWorld() && !IsQuestObjectExpired() && start != playing.end() && data &&
                data->id == line && getMSTime() - start->second >= data->textId;
        }
        void LockWeapon()
        {
            auto map = GetMap()->ToInstanceMap();
            if (auto instance = map ? map->GetInstanceScript() : nullptr)
                if (auto weapon = instance->GetGameObjectByEntry(249342))
                    if (weapon->GetDBTableGUIDLow() == 134299 && !weapon->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE))
                    {
                        _weapon = weapon->GetGUID();
                        _locked = true;
                        weapon->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    }
        }
        void UnlockWeapon()
        {
            if (!_locked) return;
            _locked = false;
            if (auto weapon = ObjectAccessor::GetGameObject(*this, _weapon))
                if (weapon->GetGUID() == _weapon && weapon->GetEntry() == 249342 && weapon->GetDBTableGUIDLow() == 134299)
                    weapon->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }
        void Update(uint32 diff) override
        {
            // Keep pickup out of the interrogation. Native line4 is the
            // authored "Dead now" handoff, not a guessed kill timer.
            if (Reached(4, 4032)) UnlockWeapon();
            Conversation::Update(diff);
        }
        void RemoveFromWorld() override
        {
            UnlockWeapon(); // Logout/abandonment must not strand the real loot.
            Conversation::RemoveFromWorld();
        }
    protected:
        Creature* FindCreatureActor(uint32, uint32 entry, WorldObject* searcher) override
        {
            ObjectGuid guid = entry == 104856 ? _lady : entry == 105298 ? _grash : entry == 102826 ? _erunak : ObjectGuid::Empty;
            auto actor = ObjectAccessor::GetCreature(*searcher, guid);
            return actor && actor->GetGUID() == guid && actor->GetEntry() == entry && actor->IsInWorld() &&
                !actor->IsPreDelete() && actor->IsAlive() && actor->GetMap() == searcher->GetMap() &&
                actor->InSamePhase(searcher) && actor->IsWithinDistInMap(searcher, searcher->GetVisibilityRange()) ? actor : nullptr;
        }
    };

    class Controller
    {
        Creature* _lady;
        ObjectGuid _recipient, _grash, _carryTarget, _story;
        std::weak_ptr<uint8> _attempt;
        uint32 _phase = 0, _retry = 0, _tries = 0;
        bool _strike = false;
        ReactStates _ladyReact = REACT_AGGRESSIVE;
        struct Ally { ObjectGuid guid; ReactStates react; };
        std::vector<Ally> _allies;

        Scenario* World() const
        {
            if (_lady->GetEntry() != 104856 || _lady->GetDBTableGUIDLow() != 362095 ||
                _lady->GetMapId() != 1600 || !_lady->IsInWorld() || _lady->IsPreDelete() || !_lady->IsAlive()) return nullptr;
            auto scenario = sScenarioMgr->GetScenario(_lady->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1066 || scenario->GetCurrentStep() != 4) return nullptr;
            auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto wall = progress->find(47133);
            return wall != progress->end() && wall->second.Counter ? scenario : nullptr;
        }
        Creature* Actor(uint32 entry, uint64 spawn) const
        {
            std::list<Creature*> actors;
            _lady->GetCreatureListWithEntryInGrid(actors, entry, 100.0f);
            for (auto actor : actors)
                if (actor->GetDBTableGUIDLow() == spawn && actor->IsInWorld() && !actor->IsPreDelete() &&
                    actor->IsAlive() && actor->GetMap() == _lady->GetMap() && actor->InSamePhase(_lady)) return actor;
            return nullptr;
        }
        Player* Recipient(Unit* attacker) const
        {
            if (auto tagged = _lady->GetLootRecipient())
                if (PlayerReady(tagged, _lady)) return tagged;
            if (!attacker || attacker->GetMap() != _lady->GetMap() || !attacker->InSamePhase(_lady)) return nullptr;
            if (auto owner = attacker->GetCharmerOrOwnerPlayerOrPlayerItself())
                return PlayerReady(owner, _lady) ? owner : nullptr;
            auto ally = attacker->ToCreature();
            if (attacker->GetCharmerOrOwnerGUID() || !ally ||
                !((ally->GetEntry() == 105298 && ally->GetDBTableGUIDLow() == 362092) ||
                  (ally->GetEntry() == 102826 && ally->GetDBTableGUIDLow() == 362093) ||
                  (ally->GetEntry() == 104853 && ally->GetDBTableGUIDLow() == 362094))) return nullptr;
            Player* result = nullptr;
            for (auto const& reference : _lady->GetMap()->GetPlayers())
                if (auto player = reference.getSource())
                    if (PlayerReady(player, _lady))
                    {
                        if (result && !result->IsInSameGroupWith(player)) return nullptr;
                        result = player;
                    }
            return result;
        }
        Player* Owner() const
        {
            auto player = ObjectAccessor::GetPlayer(*_lady, _recipient);
            auto status = player ? player->getQuestStatus(40341) : nullptr;
            return !_attempt.expired() && status && status->ScriptLifetime == _attempt.lock() &&
                PlayerReady(player, _lady) ? player : nullptr;
        }
        void ReleaseCarry()
        {
            if (_lady->GetVehicleBase() && _lady->GetVehicleBase()->GetGUID() == _carryTarget) _lady->ExitVehicle();
            if (auto target = ObjectAccessor::GetCreature(*_lady, _carryTarget))
                if (target->GetGUID() == _carryTarget) target->RemoveAurasDueToSpell(209457, _lady->GetGUID());
            _lady->RemoveAurasDueToSpell(209457, _lady->GetGUID());
            _carryTarget.Clear();
        }
        void RestoreAllies()
        {
            for (auto const& saved : _allies)
                if (auto ally = ObjectAccessor::GetCreature(*_lady, saved.guid))
                    if (ally->GetGUID() == saved.guid && ally->IsAlive()) ally->SetReactState(saved.react);
            _allies.clear();
        }
        void Quiet(Creature* ally)
        {
            _allies.push_back({ally->GetGUID(), ally->GetReactState()});
            ally->CombatStop(true);
            ally->SetReactState(REACT_PASSIVE);
        }
    public:
        explicit Controller(Creature* lady) : _lady(lady) { }
        bool Active() const { return _phase && _phase < 4; }
        ObjectGuid Story() const { return _story; }
        void Cancel()
        {
            if (!Active()) return;
            _phase = 0;
            if (auto story = ObjectAccessor::GetConversation(*_lady, _story))
                if (story->GetGUID() == _story) story->Remove();
            ReleaseCarry();
            RestoreAllies();
            _lady->SetReactState(_ladyReact);
            _recipient.Clear(); _grash.Clear(); _story.Clear(); _attempt.reset();
            _retry = _tries = 0; _strike = false;
        }
        void Damage(Unit* attacker, uint32& damage)
        {
            if (Active()) { damage = 0; return; }
            if (!World() || damage < _lady->GetHealth(attacker)) return;
            auto player = Recipient(attacker);
            if (!player) return;
            player->AddQuestDelayedEvent(40341, 0, []() {});
            auto status = player->getQuestStatus(40341);
            if (!status || !status->ScriptLifetime) return;
            _recipient = player->GetGUID(); _attempt = status->ScriptLifetime;
            _ladyReact = _lady->GetReactState(); _phase = 1; _retry = _tries = 0;
            damage = 0;
            _lady->SetHealth(1);
            _lady->InterruptNonMeleeSpells(true);
            _lady->CombatStop(true);
            _lady->SetReactState(REACT_PASSIVE);
        }
        void CarryHit(Unit* target, SpellInfo const* spell)
        {
            if (_phase != 1 || spell->Id != 209457 || !target || target->GetEntry() != 105298) return;
            _carryTarget = target->GetGUID();
            if (_carryTarget != _grash)
            {
                // A closer lower/foreign Grash may match native entry selection.
                // Reject only this Lady's aura; do not eject another passenger.
                target->RemoveAurasDueToSpell(209457, _lady->GetGUID());
                _lady->RemoveAurasDueToSpell(209457, _lady->GetGUID());
            }
        }
        void Hit(Unit* caster, SpellInfo const* spell)
        {
            if (_phase != 3 || !_strike || spell->Id != 209487 || !caster || caster->GetGUID() != _grash ||
                !World() || !Owner()) return;
            auto grash = caster->ToCreature();
            if (!grash || grash->GetDBTableGUIDLow() != 362092 || !grash->IsAlive() ||
                grash->HasAura(208093) || grash->HasUnitState(UNIT_STATE_STUNNED) ||
                _lady->GetVehicleBase() != grash) return;
            auto story = dynamic_cast<DefeatConversation*>(ObjectAccessor::GetConversation(*_lady, _story));
            if (!story || story->GetGUID() != _story || !story->Reached(2, 4030)) return;
            _phase = 4; // Reentrant native death/instance dispatch cannot finish twice.
            ReleaseCarry();
            RestoreAllies();
            grash->Kill(_lady); // Native loot, owner/group and instance victory remain authoritative.
        }
        void Update(uint32 diff)
        {
            if (!Active()) return;
            auto player = Owner();
            if (!World() || !player) { Cancel(); return; }
            if (_retry > diff) { _retry -= diff; return; }
            _retry = 1000;
            if (++_tries > 30)
            {
                TC_LOG_WARN("scripts", "Sharasdal living Lady finale could not acknowledge Grash carry/dialogue/strike; retaining alive boss and real combat retry");
                Cancel(); return;
            }
            auto grash = Actor(105298, 362092);
            if (!grash || grash->HasAura(208093) || grash->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CASTING) ||
                grash->GetCharmerOrOwnerGUID()) return;
            if (_phase == 1)
            {
                _grash = grash->GetGUID();
                if (_lady->GetVehicleBase() != grash)
                {
                    if (!_carryTarget.IsEmpty()) ReleaseCarry();
                    grash->RemoveAurasDueToSpell(209983, _lady->GetGUID());
                    _lady->CastSpell(_lady, 209457, false);
                    return; // Actual queued VehicleJoinEvent must complete first.
                }
                auto erunak = Actor(102826, 362093);
                auto adelee = Actor(104853, 362094);
                if (!erunak || !adelee) return;
                auto story = new DefeatConversation(_lady, grash, erunak);
                if (!story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),
                    1906, player, sSpellMgr->GetSpellInfo(209552), *_lady)) { delete story; return; }
                player->BindQuestObject(40341, story);
                _story = story->GetGUID();
                story->LockWeapon();
                // Historical footage shows a roughly half-health held actor.
                // This presentation reset is separate from the lethal-hit gate.
                _lady->SetHealth(std::max<uint64>(1, _lady->GetMaxHealth() / 2));
                Quiet(grash); Quiet(erunak); Quiet(adelee);
                _phase = 2; _tries = 0;
                return;
            }
            auto story = dynamic_cast<DefeatConversation*>(ObjectAccessor::GetConversation(*_lady, _story));
            if (!story || story->GetGUID() != _story || !story->IsInWorld() || _lady->GetVehicleBase() != grash)
            { Cancel(); return; }
            if (!story->Reached(2, 4030)) return;
            if (_phase == 2) { _phase = 3; _tries = 0; }
            _strike = true;
            if (grash->CastSpell(_lady, 209487, false) != SPELL_CAST_OK) _strike = false;
        }
    };

    inline void CleanupFollowers(Player* player)
    {
        if (!player || !player->IsInWorld() || player->GetMapId() != 1600 ||
            player->getClass() != CLASS_SHAMAN || player->isGameMaster()) return;
        auto scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1066) return;
        bool abandoning = player->HasAura(210045);
        bool returning = scenario->GetCurrentStep() == 6 && player->HasItemCount(128911, 1) &&
            !player->GetQuestRewardStatus(40341) &&
            (player->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE);
        if (!abandoning && !returning) return;
        // Authored DB followers are shared within this scenario instance. A
        // departing member must not remove another participant's living party.
        for (auto const& reference : player->GetMap()->GetPlayers())
            if (auto other = reference.getSource())
                if (other != player && other->getClass() == CLASS_SHAMAN && !other->isGameMaster() &&
                    !other->GetQuestRewardStatus(40341) &&
                    (other->GetQuestStatus(40341) == QUEST_STATUS_INCOMPLETE || other->GetQuestStatus(40341) == QUEST_STATUS_COMPLETE)) return;
        for (uint32 entry : {102826u, 105298u, 104853u})
        {
            std::list<Creature*> actors;
            player->GetCreatureListWithEntryInGrid(actors, entry, player->GetVisibilityRange());
            for (auto actor : actors)
            {
                uint64 spawn = actor->GetDBTableGUIDLow();
                bool authored = entry == 102826 ? (spawn == 362063 || spawn == 362093) :
                    entry == 105298 ? (spawn == 362066 || spawn == 362092) : (spawn == 362078 || spawn == 362094);
                if (!authored || !actor->IsInWorld() || actor->IsPreDelete() || actor->IsDespawn() ||
                    actor->GetMap() != player->GetMap() || !actor->InSamePhase(player)) continue;
                bool playerPassenger = false;
                if (auto vehicle = actor->GetVehicleKit())
                    for (auto const& seat : vehicle->Seats)
                        if (auto passenger = ObjectAccessor::GetUnit(*actor, seat.second.Passenger.Guid))
                            if (passenger->IsPlayer()) playerPassenger = true;
                if (!playerPassenger) actor->DespawnOrUnsummon();
            }
        }
    }
}

class spell_sharasdal_follower_cleanup : public SpellScript
{
    PrepareSpellScript(spell_sharasdal_follower_cleanup);
    void Complete()
    {
        // 210045 launches its child before applying its own abandonment aura.
        // Recheck at the acknowledged parent's AfterCast as well; no aura is
        // fabricated and an unsuccessful parent never authorizes cleanup.
        if (GetSpellInfo()->Id == 210247 || GetSpellInfo()->Id == 210045)
            SharasdalFinale::CleanupFollowers(GetCaster()->ToPlayer());
    }
    void Register() override { AfterCast += SpellCastFn(spell_sharasdal_follower_cleanup::Complete); }
};
#endif

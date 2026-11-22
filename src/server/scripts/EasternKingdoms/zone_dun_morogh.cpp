#include "Creature.h"
#include "GossipDef.h"
#include "Group.h"
#include "Item.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestItemObjectUse.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include <algorithm>
#include <array>
#include <map>

namespace
{
using namespace CrushcogAssault;

constexpr uint32 Tinker = 42849;
constexpr uint32 QuestId = CrushcogAssault::Quest;
constexpr uint32 TinkerSpawn = 84169;
constexpr uint32 Stonegrind = 42852;
constexpr uint32 StonegrindSpawn = 84288;
constexpr uint32 Pilot = 42494;
constexpr uint32 Guardian = 42294;
constexpr std::array<uint32, 4> GuardianSpawns{84392, 84393, 84394, 84407};
constexpr uint32 Menu = 11662;
constexpr uint32 Ready = 0;
constexpr uint32 ApproachPoint = 1;
constexpr uint32 FreezeAnimation = 16245;
constexpr uint32 VictorySpell = 79931;
constexpr uint32 VictoryCredit = 42860;
constexpr uint32 AssistedFire = 80220;
constexpr std::array<uint32, 3> OrbitalWeapons{80098, 80148, 80110};
constexpr float EncounterRange = 110.0f;

bool IsPlacedActor(Creature const* actor, uint32 entry, uint32 spawn)
{
    return actor && actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn &&
        actor->GetMapId() == 0 && actor->IsInWorld();
}

Creature* FindActor(WorldObject const& source, uint32 entry, uint32 spawn)
{
    std::list<Creature*> nearby;
    source.GetCreatureListWithEntryInGrid(nearby, entry, EncounterRange);
    auto found = std::find_if(nearby.begin(), nearby.end(), [=](Creature const* actor)
    {
        return IsPlacedActor(actor, entry, spawn);
    });
    return found != nearby.end() ? *found : nullptr;
}

class CrushcogAssaultAI : public ScriptedAI
{
public:
    explicit CrushcogAssaultAI(Creature* creature) : ScriptedAI(creature) {}

    void sGossipSelect(Player* player, uint32 menuId, uint32 option) override
    {
        if (menuId != Menu || option != Ready || !IsPlacedActor(me, Tinker, TinkerSpawn) ||
            player->PlayerTalkClass->GetGossipMenu().GetMenuId() != Menu ||
            !player->PlayerTalkClass->GetGossipMenu().GetItem(Ready) || !player->IsAlive() ||
            player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE || !player->HasItemCount(OrbitalDevice, 1, false) ||
            !player->IsWithinDistInMap(me, INTERACTION_DISTANCE) || !player->IsWithinLOSInMap(me))
            return;

        if (_stage == Stage::Victory || (_stage == Stage::Idle && !FindBattleActors()))
            return;

        Remember(*player);
        if (Group* group = player->GetGroup(); group && !group->isRaidGroup())
            for (GroupReference* member = group->GetFirstMember(); member; member = member->next())
                if (Player* other = member->getSource(); other && other != player && !other->GetCorpse() &&
                    other->IsAtGroupRewardDistance(me) && other->InSamePhase(me))
                    Remember(*other);

        player->TalkedToCreature(Tinker, me->GetGUID());
        player->PlayerTalkClass->SendCloseGossip();
        if (_stage == Stage::Idle)
        {
            _stage = Stage::Speech;
            me->SetWalk(false);
            Talk(0, player->GetGUID());
            _events.ScheduleEvent(Praise, 12000);
            _events.ScheduleEvent(Rally, 22000);
            _events.ScheduleEvent(Advance, 29000);
        }
    }

    bool CanMark(Player& player, Position const& destination) const
    {
        return _stage == Stage::Battle && HasParticipant(player) && CanPlaceMarker(&player, destination);
    }

    void FireOrbitalWeapon(Player& player, Creature* marker)
    {
        if (!CanMark(player, marker->GetPosition()))
            return;

        uint8 weapon = _nextWeapon++ % OrbitalWeapons.size();
        Talk(5 + weapon, player.GetGUID());
        // The weapon spells derive their area destination from an explicit unit.
        me->CastSpell(marker, OrbitalWeapons[weapon], true);
    }

    bool CanDamage(Creature const* enemy, Unit const* attacker) const
    {
        if (_stage != Stage::Battle || !enemy || !attacker || !IsEnemy(enemy))
            return false;
        if (attacker == me || attacker->GetGUID() == _stonegrind)
            return true;
        Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
        return player && HasParticipant(*player);
    }

    bool IsEnemy(Creature const* enemy) const
    {
        return enemy && (enemy->GetGUID() == _razlo ||
            std::find(_guardians.begin(), _guardians.end(), enemy->GetGUID()) != _guardians.end());
    }

    bool CanReceiveVictory(Creature const* razlo, Player& player) const
    {
        return _stage == Stage::Victory && MatchesRazlo(razlo) && !razlo->IsAlive() && !player.GetCorpse() &&
            player.IsAtGroupRewardDistance(razlo) && HasParticipant(player);
    }

    void RazloDied(Creature* razlo)
    {
        if (_stage != Stage::Battle || !MatchesRazlo(razlo) || razlo->IsAlive())
            return;

        _stage = Stage::Victory;
        _events.Reset();
        razlo->AI()->Talk(2);
        razlo->CastSpell(razlo, VictorySpell, true);
        Talk(3);
        if (Creature* stonegrind = Actor(_stonegrind))
            stonegrind->AI()->Talk(1);
        _events.ScheduleEvent(VictorySpeech, 4000);
        _events.ScheduleEvent(ReturnHome, 11000);
    }

    void MovementInform(uint32 type, uint32 point) override
    {
        if (type != POINT_MOTION_TYPE || point != ApproachPoint || _stage != Stage::Approach)
            return;

        Creature* razlo = Actor(_razlo);
        if (!razlo || !razlo->IsAlive() || !me->IsWithinDistInMap(razlo, 24.0f))
        {
            Restart();
            return;
        }

        _events.CancelEvent(ApproachTimeout);
        _stage = Stage::Challenge;
        razlo->AI()->Talk(0);
        _events.ScheduleEvent(Taunt, 9000);
        _events.ScheduleEvent(Attack, 18000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_stage == Stage::Idle)
            return;

        if (_stage != Stage::Victory && !HasLocalParticipant())
        {
            Restart();
            return;
        }

        _events.Update(diff);
        while (uint32 event = _events.ExecuteEvent())
        {
            switch (event)
            {
                case Praise:
                    Talk(1, LocalParticipant());
                    break;
                case Rally:
                    Talk(2);
                    break;
                case Advance:
                    MoveToBattle();
                    break;
                case Taunt:
                    if (Creature* razlo = Actor(_razlo))
                        razlo->AI()->Talk(1);
                    break;
                case Attack:
                    BeginFighting();
                    break;
                case Fire:
                    if (Creature* razlo = Actor(_razlo); razlo && razlo->IsAlive())
                    {
                        me->CastSpell(razlo, AssistedFire, true);
                        if (_stage == Stage::Battle)
                            _events.ScheduleEvent(Fire, 8500);
                    }
                    else
                        Restart();
                    break;
                case VictorySpeech:
                    Talk(4);
                    break;
                case ApproachTimeout:
                case BattleTimeout:
                case ReturnHome:
                    Restart();
                    break;
                default:
                    break;
            }
        }
    }

private:
    enum class Stage { Idle, Speech, Approach, Challenge, Battle, Victory };
    enum Event { Praise = 1, Rally, Advance, ApproachTimeout, Taunt, Attack, Fire, BattleTimeout, VictorySpeech, ReturnHome };

    Creature* Actor(ObjectGuid guid) const
    {
        return ObjectAccessor::GetCreature(*me, guid);
    }

    bool MatchesRazlo(Creature const* razlo) const
    {
        return IsPlacedActor(razlo, Razlo, RazloSpawn) && razlo->GetGUID() == _razlo;
    }

    void Remember(Player& player)
    {
        if (player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE)
            if (auto receipt = player.GetQuestCancellationReceipt(QuestId))
                _participants.insert_or_assign(player.GetGUID(), std::move(receipt));
    }

    bool HasParticipant(Player const& player) const
    {
        auto participant = _participants.find(player.GetGUID());
        return participant != _participants.end() && !participant->second->load(std::memory_order_acquire) &&
            player.IsInWorld() && player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && player.IsInMap(me) && player.InSamePhase(me);
    }

    ObjectGuid LocalParticipant() const
    {
        for (auto const& participant : _participants)
            if (Player* player = ObjectAccessor::GetPlayer(*me, participant.first); player && HasParticipant(*player) &&
                (player->IsWithinDistInMap(me, EncounterRange) || player->IsAtGroupRewardDistance(Actor(_razlo))))
                return player->GetGUID();
        return ObjectGuid::Empty;
    }

    bool HasLocalParticipant() const { return !LocalParticipant().IsEmpty(); }

    bool FindBattleActors()
    {
        Creature* razlo = FindActor(*me, Razlo, RazloSpawn);
        Creature* stonegrind = FindActor(*me, Stonegrind, StonegrindSpawn);
        Unit* pilot = razlo && razlo->GetVehicleKit() ? razlo->GetVehicleKit()->GetPassenger(0) : nullptr;
        if (!razlo || !razlo->IsAlive() || !pilot || pilot->GetEntry() != Pilot || !stonegrind || !stonegrind->IsAlive())
            return false;

        for (std::size_t index = 0; index < GuardianSpawns.size(); ++index)
        {
            Creature* guardian = FindActor(*razlo, Guardian, GuardianSpawns[index]);
            if (!guardian || !guardian->IsAlive())
                return false;
            _guardians[index] = guardian->GetGUID();
        }
        _razlo = razlo->GetGUID();
        _stonegrind = stonegrind->GetGUID();
        _tinkerHome = me->GetHomePosition();
        _stonegrindHome = stonegrind->GetHomePosition();
        return true;
    }

    void MoveToBattle()
    {
        Creature* razlo = Actor(_razlo);
        Creature* stonegrind = Actor(_stonegrind);
        if (!razlo || !razlo->IsAlive() || !stonegrind || !stonegrind->IsAlive())
        {
            Restart();
            return;
        }

        _stage = Stage::Approach;
        stonegrind->AI()->Talk(0);
        stonegrind->SetWalk(false);
        stonegrind->GetMotionMaster()->MovePoint(ApproachPoint, razlo->GetNearPosition(9.0f, 0.4f));
        me->GetMotionMaster()->MovePoint(ApproachPoint, razlo->GetNearPosition(13.0f, 0.0f));
        _events.ScheduleEvent(ApproachTimeout, 25000);
    }

    void BeginFighting()
    {
        Creature* razlo = Actor(_razlo);
        Creature* stonegrind = Actor(_stonegrind);
        if (!razlo || !razlo->IsAlive() || !stonegrind || !stonegrind->IsAlive() ||
            !me->IsWithinDistInMap(razlo, 24.0f) || !stonegrind->IsWithinDistInMap(razlo, 24.0f))
        {
            Restart();
            return;
        }

        _stage = Stage::Battle;
        // Threat range is measured from home. Both allies approached from the other shore.
        me->SetHomePosition(me->GetPosition());
        stonegrind->SetHomePosition(stonegrind->GetPosition());
        razlo->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        razlo->AI()->AttackStart(stonegrind);
        stonegrind->AI()->AttackStart(razlo);
        for (ObjectGuid const& guid : _guardians)
            if (Creature* guardian = Actor(guid); guardian && guardian->IsAlive())
            {
                guardian->RemoveAurasDueToSpell(FreezeAnimation);
                guardian->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                guardian->AI()->AttackStart(me);
            }
        _events.ScheduleEvent(Fire, 3500);
        _events.ScheduleEvent(BattleTimeout, 150000);
    }

    void Restart()
    {
        _stage = Stage::Idle;
        _events.Reset();
        _participants.clear();
        _nextWeapon = 0;
        me->CombatStop(true);
        me->SetHomePosition(_tinkerHome);
        me->GetMotionMaster()->MoveTargetedHome();
        if (Creature* stonegrind = Actor(_stonegrind))
        {
            stonegrind->SetHomePosition(_stonegrindHome);
            if (stonegrind->IsAlive())
            {
                stonegrind->CombatStop(true);
                stonegrind->GetMotionMaster()->MoveTargetedHome();
            }
        }
        if (Creature* razlo = Actor(_razlo); razlo && razlo->IsAlive())
            razlo->AI()->EnterEvadeMode();
        for (ObjectGuid const& guid : _guardians)
            if (Creature* guardian = Actor(guid); guardian && guardian->IsAlive())
                guardian->AI()->EnterEvadeMode();
    }

    Stage _stage = Stage::Idle;
    EventMap _events;
    ObjectGuid _razlo;
    ObjectGuid _stonegrind;
    Position _tinkerHome;
    Position _stonegrindHome;
    std::array<ObjectGuid, GuardianSpawns.size()> _guardians{};
    std::map<ObjectGuid, std::shared_ptr<QuestCancellationReceipt const>> _participants;
    uint8 _nextWeapon = 0;
};

CrushcogAssaultAI* FindAssault(WorldObject const& source)
{
    if (Creature* tinker = FindActor(source, Tinker, TinkerSpawn))
        return dynamic_cast<CrushcogAssaultAI*>(tinker->AI());
    return nullptr;
}

class npc_iceflow_mekkatorque : public CreatureScript
{
public:
    npc_iceflow_mekkatorque() : CreatureScript("npc_iceflow_mekkatorque") {}
    CreatureAI* GetAI(Creature* creature) const override { return new CrushcogAssaultAI(creature); }
};

class npc_iceflow_crushcog_enemy : public CreatureScript
{
public:
    npc_iceflow_crushcog_enemy() : CreatureScript("npc_iceflow_crushcog_enemy") {}

    struct EnemyAI : ScriptedAI
    {
        explicit EnemyAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            if (me->GetEntry() == Guardian && !me->HasAura(FreezeAnimation))
                me->CastSpell(me, FreezeAnimation, true);
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
        {
            if (CrushcogAssaultAI* assault = FindAssault(*me); !assault || !assault->CanDamage(me, attacker))
                damage = 0;
        }

        void JustDied(Unit*) override
        {
            if (me->GetEntry() == Razlo)
                if (CrushcogAssaultAI* assault = FindAssault(*me))
                    assault->RazloDied(me);
        }

        void UpdateAI(uint32) override
        {
            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override { return new EnemyAI(creature); }
};

class npc_iceflow_orbital_marker : public CreatureScript
{
public:
    npc_iceflow_orbital_marker() : CreatureScript("npc_iceflow_orbital_marker") {}

    struct MarkerAI : ScriptedAI
    {
        explicit MarkerAI(Creature* creature) : ScriptedAI(creature) { SetCombatMovement(false); }

        void IsSummonedBy(Unit* summoner) override
        {
            Player* player = summoner ? summoner->ToPlayer() : nullptr;
            if (!player || me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != OrbitalTarget ||
                !me->HasQuestObjectScope() || me->IsQuestObjectExpired())
                return;

            if (CrushcogAssaultAI* assault = FindAssault(*me))
                assault->FireOrbitalWeapon(*player, me);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override { return new MarkerAI(creature); }
};

class spell_iceflow_orbital_target : public SpellScript
{
    PrepareSpellScript(spell_iceflow_orbital_target);

    SpellCastResult CheckAssault()
    {
        Player* player = GetCaster()->ToPlayer();
        Item* item = GetCastItem();
        auto destination = GetExplTargetDest();
        if (!player || !destination || !item || item->GetEntry() != OrbitalDevice || item->GetOwnerGUID() != player->GetGUID() ||
            !Player::IsInventoryPos(item->GetBagSlot(), item->GetSlot()))
            return SPELL_FAILED_BAD_TARGETS;

        CrushcogAssaultAI* assault = FindAssault(*player);
        if (!assault || !assault->CanMark(*player, *destination))
            return SPELL_FAILED_BAD_TARGETS;
        if (!_receipt)
            _receipt = player->GetQuestCancellationReceipt(QuestId);
        return _receipt && !_receipt->load(std::memory_order_acquire) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_iceflow_orbital_target::CheckAssault);
    }
    std::shared_ptr<QuestCancellationReceipt const> _receipt;
};

class spell_iceflow_crushcog_weapons : public SpellScript
{
    PrepareSpellScript(spell_iceflow_crushcog_weapons);

    void FilterEnemies(std::list<WorldObject*>& targets)
    {
        Creature* tinker = GetCaster()->ToCreature();
        auto assault = IsPlacedActor(tinker, Tinker, TinkerSpawn) ? dynamic_cast<CrushcogAssaultAI*>(tinker->AI()) : nullptr;
        targets.remove_if([=](WorldObject* target) { return !assault || !assault->CanDamage(target->ToCreature(), tinker); });
    }

    void Register() override
    {
        if (m_scriptSpellId == AssistedFire)
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_iceflow_crushcog_weapons::FilterEnemies, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        else
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_iceflow_crushcog_weapons::FilterEnemies, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

class spell_iceflow_crushcog_victory : public SpellScript
{
    PrepareSpellScript(spell_iceflow_crushcog_victory);

    void GiveVictoryCredit(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        Creature* razlo = GetCaster()->ToCreature();
        Player* player = GetHitPlayer();
        if (razlo && player)
            if (CrushcogAssaultAI* assault = FindAssault(*razlo); assault && assault->CanReceiveVictory(razlo, *player))
                player->KilledMonsterCredit(VictoryCredit);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_iceflow_crushcog_victory::GiveVictoryCredit, EFFECT_0, SPELL_EFFECT_KILL_CREDIT2);
    }
};
}

void AddSC_dun_morogh()
{
    new npc_iceflow_mekkatorque();
    new npc_iceflow_crushcog_enemy();
    new npc_iceflow_orbital_marker();
    RegisterSpellScript(spell_iceflow_orbital_target);
    RegisterSpellScript(spell_iceflow_crushcog_weapons);
    RegisterSpellScript(spell_iceflow_crushcog_victory);
}

#ifndef GILNEAS_BATTLE_OPENING_H
#define GILNEAS_BATTLE_OPENING_H

#include "CreatureTextMgr.h"
#include "DB2Stores.h"
#include "GossipDef.h"
#include "Group.h"
#include "GroupReference.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestData.h"
#include "QuestObjectScope.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "gilneas_battle_routes.h"

namespace GilneasBattle
{
enum : uint32
{
    Quest = 24904,
    Prerequisite = 24676,
    MapId = 654,
    Outpost = 4842,
    BattlePhase = 69485,
    KrennanSpawn = 149660,
    Menu = 11061,
    MenuText = 15377,
    Liam = 38218,
    Almyra = 38466,
    Myriam = 38465,
    Militia = 38221,
    Crossbowman = 38210,
    Gorerot = 38331,
    Worgen = 38348,
    Catapult = 38377,
    Darius = 38415,
    Abomination = 38420,
    Cannon = 38424,
    Villager = 38425,
    Lorna = 38426,
    DarkRanger = 38464,
    Sylvanas = 38469,
    Genn = 38470,
    Banshee = 38473,
    YoungPrince = 38474,
    SoldierAura = 72069,
    Shoot = 6660,
    Smash = 71774,
    Wail = 72113,
    Aim = 72114,
    Arrow = 72116,
    Victory = 72349,
    Slain = 72361,
    DespawnForsaken = 72384,
    MarchPoint = 2100,
    Lifetime = 20 * MINUTE * IN_MILLISECONDS,
    SpeechGroup = 30,
};

constexpr float BattleLeash = 180.0f;

enum class Stage : uint8 { Speech = 1, March, Assault, Finale, Retreat };
enum class FinaleStep : uint8 { Standoff, LiamApproaches, AimAtGenn, ArrowHits, Credit };

constexpr uint32 Leaders[] = { Liam, Almyra, Myriam, Lorna, Darius, Genn };
Position const LeaderStarts[] = {
    { -1408.661f, 1260.017f, 36.51123f, 1.797689f },
    { -1401.306f, 1251.913f, 36.51123f, 2.024582f },
    { -1412.951f, 1248.698f, 36.51123f, 1.762783f },
    { -1547.26f, 1286.648f, 10.18917f, 3.397672f },
    { -1726.519f, 1394.219f, 21.34956f, 0.8540632f },
    { -1757.587f, 1675.883f, 22.158f, 5.602196f },
};

Position const GorerotFight{ -1703.10f, 1419.87f, 21.9268f, 3.89208f };
Position const SylvanasStart{ -1678.057f, 1611.814f, 20.5728f, 2.443461f };
Position const InterceptStart{ -1634.634f, 1631.859f, 21.21159f, 4.415186f };

bool HasBattle(Player const* player)
{
    return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
        player->getRace() == RACE_WORGEN && player->GetMapId() == MapId &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE && player->GetQuestRewardStatus(Prerequisite) &&
        player->HasAura(BattlePhase) && (!player->GetGroup() || !player->GetGroup()->isRaidGroup());
}

struct OpeningAI;
OpeningAI* GetBattle(Creature const* actor);

// Each moving leader confirms the real per-point spline arrival before the
// encounter consumes that point. A missing callback cannot finish a wave.
struct BattleLeaderAI : ScriptedAI
{
    explicit BattleLeaderAI(Creature* actor) : ScriptedAI(actor) { }

    void MoveTo(Position const& position, uint32 point)
    {
        destination = position;
        expectedPoint = MarchPoint + point;
        attempts = 0;
        RequestMove();
    }

    void RequestMove()
    {
        movementReported = false;
        travelCheck = 3000 + uint32(me->GetExactDist(&destination) * 250.0f);
        me->GetMotionMaster()->MovePoint(expectedPoint, destination);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == expectedPoint)
            movementReported = true;
    }

    bool HasArrived() const { return movementReported && me->GetExactDist(&destination) <= 2.0f; }

    bool MaintainTravel(uint32 diff)
    {
        if (!expectedPoint || HasArrived())
            return true;
        if (travelCheck > diff)
        {
            travelCheck -= diff;
            return true;
        }
        // Real path requests can be interrupted. Recalculate from the actor's
        // current position and still wait for the actual arrival callback.
        if (++attempts > 3)
            return false;
        RequestMove();
        return true;
    }

    void DamageTaken(Unit*, uint32& damage, DamageEffectType) override { damage = 0; }

    Position destination;
    uint32 expectedPoint = 0;
    uint32 travelCheck = 0;
    uint8 attempts = 0;
    bool movementReported = false;
};

struct EnemyAI : ScriptedAI
{
    explicit EnemyAI(Creature* actor) : ScriptedAI(actor)
    {
        SetCombatMovement(!IsRanged());
    }

    bool Admits(Unit const* target) const;
    void MoveInLineOfSight(Unit* target) override;
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override;
    void UpdateAI(uint32 diff) override;

private:
    bool IsRanged() const { return me->GetEntry() == Crossbowman || me->GetEntry() == DarkRanger || me->GetEntry() == Sylvanas; }
    uint32 smashTimer = 13000;
};

struct PrinceFinaleAI : BattleLeaderAI
{
    explicit PrinceFinaleAI(Creature* actor) : BattleLeaderAI(actor) { }

    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (!slain && armed && caster->GetGUID() == shooter && spell->Id == Arrow &&
            me->CastSpell(me, Slain, true) == SPELL_CAST_OK)
        {
            slain = true;
            me->SetStandState(UNIT_STAND_STATE_DEAD);
        }
    }

    ObjectGuid shooter;
    bool armed = false;
    bool slain = false;
};

struct OpeningAI : BattleLeaderAI
{
    explicit OpeningAI(Creature* actor) : BattleLeaderAI(actor) { }

    bool Start(Player& owner)
    {
        ownerGuid = owner.GetGUID();
        battleLevel = std::clamp<uint8>(owner.getLevel(), 12, 20);
        players.emplace(ownerGuid, owner.GetQuestCancellationReceipt(Quest));
        if (Group* group = owner.GetGroup())
        {
            groupGuid = group->GetGUID();
            for (GroupReference* reference = group->GetFirstMember(); reference; reference = reference->next())
            {
                Player* player = reference->getSource();
                if (player != &owner && HasBattle(player) && me->InSamePhase(player) &&
                    player->GetAreaId() == Outpost && me->IsWithinDistInMap(player, 55.0f) && !FindPlayerAttempt(*player))
                    players.emplace(player->GetGUID(), player->GetQuestCancellationReceipt(Quest));
            }
        }
        for (auto const& [guid, receipt] : players)
            me->AddPlayerInPersonnalVisibilityList(guid);

        me->SetAnimKitId(2409);
        if (me->CastSpell(me, SoldierAura, true) != SPELL_CAST_OK)
            return false;

        leaders[0] = me->GetGUID();
        return AddLeader(1) && AddLeader(2) && AddTroops(0, Militia, LiamMilitia) &&
            AddTroops(1, Militia, AlmyraMilitia) && AddTroops(2, Militia, MyriamMilitia);
    }

    bool Owns(Creature const* actor) const
    {
        return actor && !stopping && actor->HasQuestObjectScope() && !actor->IsQuestObjectExpired() &&
            actor->GetQuestObjectScope().lock() == me->GetQuestObjectScope().lock();
    }

    bool IsMember(Player const* player) const
    {
        if (!HasBattle(player) || !me->IsWithinDistInMap(player, BattleLeash) || !me->InSamePhase(player))
            return false;

        auto entry = players.find(player->GetGUID());
        if (entry == players.end() || !entry->second || entry->second->load())
            return false;
        if (player->GetGUID() == ownerGuid)
            return true;
        Player const* owner = ObjectAccessor::GetPlayer(*me, ownerGuid);
        auto group = player->GetGroup();
        return owner && group && group->GetGUID() == groupGuid && owner->GetGroup() == group;
    }

    bool SharesAttempt(Unit const* unit) const
    {
        if (!unit || stopping || !me->InSamePhase(unit))
            return false;
        if (Owns(unit->ToCreature()))
            return true;
        return IsMember(unit->GetCharmerOrOwnerPlayerOrPlayerItself());
    }

    bool AllowsVictory(Creature const* caster, Player const* player = nullptr) const
    {
        return stage == Stage::Finale && finale == FinaleStep::Credit && caster && caster->GetGUID() == sylvanas &&
            Owns(caster) && (!player || (IsMember(player) && caster->IsWithinDistInMap(player, 50.0f)));
    }

    Creature* ResolveFinalePrince(Creature const* caster, uint32 spell) const
    {
        if (stage != Stage::Finale || !Owns(caster))
            return nullptr;
        Creature* prince = Actor(youngLiam);
        if (!prince || !prince->IsAlive() || !caster->IsWithinDistInMap(prince, 45.0f))
            return nullptr;
        auto ai = static_cast<PrinceFinaleAI*>(prince->AI());
        if (spell == Slain)
            return caster == prince && ai->armed ? prince : nullptr;
        if (caster->GetGUID() != sylvanas)
            return nullptr;
        if (spell == Aim)
            return finale == FinaleStep::LiamApproaches ? prince : nullptr;
        return spell == Arrow && finale == FinaleStep::AimAtGenn && ai->armed ? prince : nullptr;
    }

    static OpeningAI* FindPlayerAttempt(Player& player)
    {
        std::list<Creature*> candidates;
        player.GetCreatureListWithEntryInGrid(candidates, Liam, BattleLeash);
        for (Creature* candidate : candidates)
            if (auto ai = dynamic_cast<OpeningAI*>(candidate->AI()); ai && ai->IsMember(&player) && !ai->stopping)
                return ai;
        return nullptr;
    }

    uint32 GetData(uint32 id) const override
    {
        if (stopping)
            return 0;
        if (id == Quest)
            return uint32(stage);
        return id == Quest + 1 ? wave + 1 : 0;
    }

    void Stop()
    {
        if (stopping)
            return;
        stopping = true;
        for (ObjectGuid guid : children)
            if (Creature* child = ObjectAccessor::GetCreature(*me, guid))
                child->DespawnOrUnsummon();
        me->DespawnOrUnsummon();
    }

    void SummonedCreatureDies(Creature* actor, Unit* killer) override
    {
        if (stage == Stage::Assault && Owns(actor) && SharesAttempt(killer))
            enemies.erase(actor->GetGUID());
    }

    void JustDied(Unit*) override { Stop(); }

    void UpdateAI(uint32 diff) override
    {
        if (stopping)
            return;
        if (stage == Stage::Retreat)
        {
            UpdateRetreat(diff);
            return;
        }

        Player* owner = ObjectAccessor::GetPlayer(*me, ownerGuid);
        if (!IsMember(owner) || life <= diff)
        {
            Stop();
            return;
        }
        life -= diff;
        RemoveDepartedPlayers(*owner);

        if (stage == Stage::Speech)
            UpdateSpeech(diff);
        else if (stage == Stage::Finale)
            UpdateFinale(*owner, diff);
        else
            UpdateWave(*owner, diff);
    }

private:
    using Troop = std::pair<ObjectGuid, size_t>;

    Creature* Actor(ObjectGuid guid) const { return ObjectAccessor::GetCreature(*me, guid); }

    void RemoveDepartedPlayers(Player const& owner)
    {
        for (auto it = players.begin(); it != players.end();)
        {
            ObjectGuid guid = it->first;
            Player* player = ObjectAccessor::GetPlayer(*me, guid);
            bool removed = it->second->load() || (player && (!player->GetGroup() || player->GetGroup() != owner.GetGroup() ||
                player->GetGroup()->GetGUID() != groupGuid));
            if (guid == ownerGuid || !removed)
            {
                ++it;
                continue;
            }
            me->RemovePlayerFromPersonnalVisibilityList(guid);
            for (ObjectGuid child : children)
                if (Creature* actor = Actor(child))
                    actor->RemovePlayerFromPersonnalVisibilityList(guid);
            it = players.erase(it);
        }
    }

    void Say(Creature* actor, uint8 text)
    {
        for (auto const& [guid, receipt] : players)
            if (!receipt->load())
                sCreatureTextMgr->SendChat(actor, text, guid, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
    }

    TempSummon* Summon(uint32 entry, Position const& position)
    {
        GuidUnorderedSet viewers;
        for (auto const& [guid, receipt] : players)
            if (!receipt->load())
                viewers.insert(guid);
        auto summon = me->GetMap()->SummonCreature(entry, position, sSummonPropertiesStore.LookupEntry(2912), 0, me,
            ObjectGuid::Empty, 0, 0, ObjectGuid::Empty, &viewers);
        if (summon)
        {
            children.push_back(summon->GetGUID());
            summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
            summon->SetReactState(REACT_PASSIVE);
            summon->setActive(true);
        }
        return summon;
    }

    void ScaleBattleActor(Creature* actor)
    {
        // These legacy templates are fixed at 83-88. Apply native stats to the
        // battle copy after initialization, including any vehicle reset.
        if (actor->getLevel() > 20)
        {
            CreatureTemplate scaled = *actor->GetCreatureTemplate();
            scaled.minlevel = scaled.maxlevel = battleLevel;
            actor->SelectLevel(&scaled);
        }
    }

    bool AddLeader(size_t index)
    {
        auto actor = Summon(Leaders[index], LeaderStarts[index]);
        if (!actor || !actor->AIM_Initialize(new BattleLeaderAI(actor)))
            return false;
        ScaleBattleActor(actor);
        leaders[index] = actor->GetGUID();
        return true;
    }

    bool AddTroops(size_t leader, uint32 entry, BattlePath positions)
    {
        for (Position const& position : positions)
        {
            auto troop = Summon(entry, position);
            if (!troop)
                return false;
            troops.emplace_back(troop->GetGUID(), leader);
        }
        return true;
    }

    bool AddScenery(uint32 entry, BattlePath positions)
    {
        return std::all_of(positions.begin(), positions.end(), [&](Position const& position) { return Summon(entry, position) != nullptr; });
    }

    Creature* AddEnemy(uint32 entry, Position const& position)
    {
        auto enemy = Summon(entry, position);
        if (!enemy || !enemy->AIM_Initialize(new EnemyAI(enemy)))
            return nullptr;
        enemy->SetReactState(REACT_AGGRESSIVE);
        ScaleBattleActor(enemy);
        return enemy;
    }

    bool AddEnemies(uint32 entry, BattlePath positions)
    {
        for (Position const& position : positions)
        {
            auto enemy = AddEnemy(entry, position);
            if (!enemy)
                return false;
            enemies.insert(enemy->GetGUID());
        }
        return true;
    }

    bool AddGorerot()
    {
        auto boss = AddEnemy(Gorerot, GorerotFight);
        if (!boss)
            return false;
        enemies.insert(boss->GetGUID());
        Say(boss, 0);
        if (Creature* darius = Actor(leaders[4]))
            Say(darius, SpeechGroup);
        return boss->GetMaxHealth() > 0;
    }

    void FollowLeader(Creature* troop, size_t index, size_t slot)
    {
        if (Creature* leader = Actor(leaders[index]))
        {
            float distance = 3.0f + float(slot % 4);
            float angle = 1.85f + float(slot % 9) * 0.35f;
            troop->GetMotionMaster()->MoveFollow(leader, distance, angle);
        }
    }

    bool PrepareWave()
    {
        size_t previousCount = troops.size();
        if (wave == 2 && (!AddLeader(3) || !AddTroops(3, Villager, EmberstoneVillagers) || !AddScenery(Cannon, EmberstoneCannons)))
            return false;
        if (wave == 3 && (!AddLeader(4) || !AddTroops(4, Worgen, DariusWorgen)))
            return false;
        if (wave == 4 && !AddScenery(Catapult, DamagedCatapults))
            return false;
        if (wave == 6 && (!AddLeader(5) || !AddTroops(5, Militia, GennMilitia)))
            return false;

        size_t first = wave == 0 ? 0 : previousCount;
        for (size_t slot = first; slot < troops.size(); ++slot)
        {
            auto [guid, index] = troops[slot];
            if (Creature* troop = Actor(guid))
                FollowLeader(troop, index, slot);
        }

        for (size_t index = 0; index < leaders.size(); ++index)
        {
            BattlePath route = MarchRoutes[wave][index];
            if (route.empty())
                continue;
            Creature* actor = Actor(leaders[index]);
            if (!actor || !actor->IsAlive())
                return false;
            static_cast<BattleLeaderAI*>(actor->AI())->MoveTo(route.front(), 0);
        }
        stage = Stage::March;
        return true;
    }

    void UpdateSpeech(uint32 diff)
    {
        if (timer > diff)
        {
            timer -= diff;
            return;
        }

        constexpr uint32 speechDelay[] = { 10000, 10000, 11000, 9000, 11000, 5000, 500 };
        if (line < std::size(speechDelay))
        {
            Say(me, SpeechGroup + line);
            timer = speechDelay[line++];
        }
        else if (!PrepareWave())
            Stop();
    }

    bool StartFight()
    {
        switch (wave)
        {
            case 0: return AddEnemies(Crossbowman, FirstCrossbowmen);
            case 1: return AddEnemies(Crossbowman, MerchantCrossbowmen);
            case 2:
                Say(me, 37);
                Say(me, 38);
                if (Creature* lorna = Actor(leaders[3]))
                    Say(lorna, SpeechGroup);
                return AddEnemies(DarkRanger, MerchantRangers);
            case 3:
                Say(me, 39);
                return AddEnemies(Abomination, MilitaryAbominations) && AddEnemies(Crossbowman, MilitaryCrossbowmen);
            case 4: return AddGorerot();
            case 5:
                if (Creature* darius = Actor(leaders[4]))
                    Say(darius, 31);
                return AddEnemies(Crossbowman, WestGateCrossbowmen);
            default: return true;
        }
    }

    bool Arrivals(bool last) const
    {
        for (size_t index = 0; index < leaders.size(); ++index)
        {
            BattlePath route = MarchRoutes[wave][index];
            if (route.empty())
                continue;
            auto actor = Actor(leaders[index]);
            if (!actor || !actor->IsAlive())
                return false;
            auto ai = static_cast<BattleLeaderAI*>(actor->AI());
            if (!ai->HasArrived() || (last && ai->expectedPoint != MarchPoint + route.size() - 1))
                return false;
        }
        return true;
    }

    static void AdvanceActor(BattleLeaderAI& ai, BattlePath route)
    {
        if (!ai.HasArrived())
            return;
        size_t point = ai.expectedPoint - MarchPoint + 1;
        if (point < route.size())
            ai.MoveTo(route[point], uint32(point));
    }

    bool AdvanceLeaders()
    {
        for (size_t index = 0; index < leaders.size(); ++index)
        {
            BattlePath route = MarchRoutes[wave][index];
            if (route.empty())
                continue;
            auto actor = Actor(leaders[index]);
            if (!actor || !actor->IsAlive())
                return false;
            AdvanceActor(*static_cast<BattleLeaderAI*>(actor->AI()), route);
        }
        return true;
    }

    void UpdateWave(Player& owner, uint32 diff)
    {
        for (size_t index = 0; index < leaders.size(); ++index)
        {
            if (MarchRoutes[wave][index].empty())
                continue;
            Creature* actor = Actor(leaders[index]);
            if (!actor || !actor->IsAlive() || !static_cast<BattleLeaderAI*>(actor->AI())->MaintainTravel(diff))
            {
                Stop();
                return;
            }
        }
        if (stage == Stage::March)
        {
            if (!Arrivals(false))
                return;
            if (!StartFight())
            {
                Stop();
                return;
            }
            stage = Stage::Assault;
        }

        if (!AdvanceLeaders() || std::any_of(enemies.begin(), enemies.end(), [&](ObjectGuid guid)
        {
            auto enemy = Actor(guid);
            return !enemy || !enemy->IsAlive() || enemy->IsDespawn();
        }))
        {
            Stop();
            return;
        }
        SupportAssault(diff);
        if (!enemies.empty() || !Arrivals(true) || !me->IsWithinDistInMap(&owner, 60.0f))
            return;
        if (wave + 1 == std::size(MarchRoutes))
        {
            if (!BeginFinale())
                Stop();
        }
        else
        {
            ++wave;
            if (!PrepareWave())
                Stop();
        }
    }

    void SupportAssault(uint32 diff)
    {
        if (volley > diff)
        {
            volley -= diff;
            return;
        }
        volley = 2500;
        for (size_t slot = 0; slot < troops.size(); ++slot)
        {
            auto [guid, index] = troops[slot];
            Creature* troop = Actor(guid);
            if (!troop || !troop->IsAlive())
                continue;

            bool ranged = troop->GetEntry() != Worgen;
            Creature* nearest = nullptr;
            float distance = ranged ? 30.0f : 12.0f;
            for (ObjectGuid enemyGuid : enemies)
                if (Creature* enemy = Actor(enemyGuid); enemy && enemy->IsAlive() &&
                    troop->GetDistance(enemy) < distance && troop->IsWithinLOSInMap(enemy))
                {
                    nearest = enemy;
                    distance = troop->GetDistance(enemy);
                }
            if (nearest && ranged)
                troop->CastSpell(nearest, Shoot, false);
            else if (nearest)
                troop->AI()->AttackStart(nearest);
            else if (!ranged && troop->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
            {
                troop->CombatStop();
                FollowLeader(troop, index, slot);
            }
        }
    }

    bool BeginFinale()
    {
        Creature* queen = AddEnemy(Sylvanas, SylvanasStart);
        Creature* king = Actor(leaders[5]);
        if (!queen || !king)
            return false;
        sylvanas = queen->GetGUID();
        Say(king, SpeechGroup);
        stage = Stage::Finale;
        finale = FinaleStep::Standoff;
        timer = 45000;
        return true;
    }

    void UpdateFinale(Player& owner, uint32 diff)
    {
        Creature* queen = Actor(sylvanas);
        Creature* king = Actor(leaders[5]);
        if (!queen || !queen->IsAlive() || !king || !king->IsAlive())
        {
            Stop();
            return;
        }
        if (timer > diff)
        {
            timer -= diff;
            return;
        }
        timer = 0;

        if (finale == FinaleStep::Standoff)
        {
            if (!queen->IsWithinDistInMap(&owner, 50.0f))
                return;
            queen->CombatStop();
            queen->SetReactState(REACT_PASSIVE);
            Say(queen, SpeechGroup);
            queen->CastSpell(queen, Wail, true);
            queen->CastSpell(queen, DespawnForsaken, true);
            auto prince = Summon(YoungPrince, InterceptStart);
            if (!prince || !prince->AIM_Initialize(new PrinceFinaleAI(prince)))
            {
                Stop();
                return;
            }
            youngLiam = prince->GetGUID();
            auto ai = static_cast<PrinceFinaleAI*>(prince->AI());
            ai->shooter = sylvanas;
            ai->MoveTo(LiamIntercept[0], 0);
            finale = FinaleStep::LiamApproaches;
            return;
        }

        Creature* prince = Actor(youngLiam);
        if (!prince || !prince->IsAlive())
        {
            Stop();
            return;
        }
        auto ai = static_cast<PrinceFinaleAI*>(prince->AI());
        if (!ai->MaintainTravel(diff))
        {
            Stop();
            return;
        }
        switch (finale)
        {
            case FinaleStep::LiamApproaches:
                if (ai->HasArrived() && ai->expectedPoint == MarchPoint + 2)
                {
                    Say(prince, SpeechGroup);
                    Say(queen, 31);
                    queen->CastSpell(prince, Aim, true);
                    finale = FinaleStep::AimAtGenn;
                    timer = 900;
                }
                AdvanceActor(*ai, LiamIntercept);
                break;
            case FinaleStep::AimAtGenn:
                if (!ai->HasArrived() || ai->expectedPoint != MarchPoint + 3)
                    return;
                ai->armed = true;
                queen->CastSpell(prince, Arrow, true);
                ++arrowRequests;
                finale = FinaleStep::ArrowHits;
                timer = 2500;
                break;
            case FinaleStep::ArrowHits:
                if (!ai->slain)
                {
                    if (arrowRequests >= 3)
                        Stop();
                    else
                        finale = FinaleStep::AimAtGenn;
                    return;
                }
                Say(king, 31);
                Say(prince, 31);
                Say(prince, 32);
                finale = FinaleStep::Credit;
                timer = 4500;
                break;
            case FinaleStep::Credit:
                if (!AllowsVictory(queen, &owner))
                    return;
                queen->CastSpell(queen, Victory, true);
                if (owner.GetQuestStatus(Quest) != QUEST_STATUS_COMPLETE)
                {
                    if (++creditRequests >= 3)
                        Stop();
                    else
                        timer = 1500;
                    return;
                }
                Say(queen, 32);
                queen->AIM_Initialize(new BattleLeaderAI(queen));
                static_cast<BattleLeaderAI*>(queen->AI())->MoveTo(SylvanasRetreat[0], 0);
                AddScenery(Banshee, SoultetheredBanshees);
                stage = Stage::Retreat;
                timer = 30000;
                break;
            default: break;
        }
    }

    void UpdateRetreat(uint32 diff)
    {
        Creature* queen = Actor(sylvanas);
        if (!queen || timer <= diff)
        {
            Stop();
            return;
        }
        timer -= diff;
        auto ai = static_cast<BattleLeaderAI*>(queen->AI());
        if (!ai->MaintainTravel(diff))
        {
            Stop();
            return;
        }
        if (ai->HasArrived() && ai->expectedPoint == MarchPoint + std::size(SylvanasRetreat) - 1)
            Stop();
        else
            AdvanceActor(*ai, SylvanasRetreat);
    }

    std::array<ObjectGuid, std::size(Leaders)> leaders;
    std::vector<ObjectGuid> children;
    std::vector<Troop> troops;
    std::unordered_map<ObjectGuid, std::shared_ptr<QuestCancellationReceipt const>> players;
    GuidUnorderedSet enemies;
    ObjectGuid ownerGuid;
    ObjectGuid groupGuid;
    ObjectGuid sylvanas;
    ObjectGuid youngLiam;
    size_t wave = 0;
    Stage stage = Stage::Speech;
    FinaleStep finale = FinaleStep::Standoff;
    uint32 life = Lifetime;
    uint32 timer = 15000;
    uint32 volley = 2500;
    uint8 line = 0;
    uint8 battleLevel = 12;
    uint8 arrowRequests = 0;
    uint8 creditRequests = 0;
    bool stopping = false;
};

OpeningAI* GetBattle(Creature const* actor)
{
    if (!actor || actor->GetMapId() != MapId || !actor->HasQuestObjectScope() || actor->IsQuestObjectExpired())
        return nullptr;
    auto child = actor->ToTempSummon();
    auto summoner = child ? child->GetSummoner() : nullptr;
    auto root = summoner ? summoner->ToCreature() : nullptr;
    auto battle = root && root->GetEntry() == Liam ? dynamic_cast<OpeningAI*>(root->AI()) : nullptr;
    return battle && battle->Owns(actor) ? battle : nullptr;
}

bool EnemyAI::Admits(Unit const* target) const
{
    auto battle = GetBattle(me);
    return battle && battle->SharesAttempt(target) && target->IsAlive();
}

void EnemyAI::MoveInLineOfSight(Unit* target)
{
    if (me->GetReactState() == REACT_AGGRESSIVE && !me->getVictim() && Admits(target) && me->IsWithinDistInMap(target, 24.0f))
        ScriptedAI::MoveInLineOfSight(target);
}

void EnemyAI::DamageTaken(Unit* attacker, uint32& damage, DamageEffectType)
{
    if (me->GetEntry() == Sylvanas || !Admits(attacker))
        damage = 0;
}

void EnemyAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;
    Unit* victim = me->getVictim();
    if (!Admits(victim))
    {
        me->CombatStop();
        return;
    }
    if (me->GetEntry() == Gorerot)
    {
        if (smashTimer <= diff)
        {
            if (me->IsWithinMeleeRange(victim))
                me->CastSpell(victim, Smash, false);
            smashTimer = 13000;
        }
        else
            smashTimer -= diff;
    }
    if (me->IsWithinMeleeRange(victim))
        DoMeleeAttackIfReady();
    else if (IsRanged())
        DoSpellAttackIfReady(Shoot);
}

class spell_gilneas_battle_victory : public SpellScript
{
    PrepareSpellScript(spell_gilneas_battle_victory);

    SpellCastResult CheckCast()
    {
        Creature* caster = GetCaster()->ToCreature();
        auto battle = GetBattle(caster);
        return battle && battle->AllowsVictory(caster) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void SelectParticipants(std::list<WorldObject*>& targets)
    {
        Creature* caster = GetCaster()->ToCreature();
        auto battle = GetBattle(caster);
        targets.remove_if([&](WorldObject const* target)
        {
            auto player = target->ToPlayer();
            return !battle || !player || !battle->AllowsVictory(caster, player);
        });
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_gilneas_battle_victory::CheckCast);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_battle_victory::SelectParticipants, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};


class spell_gilneas_battle_wail : public SpellScript
{
    PrepareSpellScript(spell_gilneas_battle_wail);

    void SelectParticipants(std::list<WorldObject*>& targets)
    {
        if (auto battle = GetBattle(GetCaster()->ToCreature()))
            targets.remove_if([battle](WorldObject const* target) { return !battle->SharesAttempt(target->ToUnit()); });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_battle_wail::SelectParticipants, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_battle_wail::SelectParticipants, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

class spell_gilneas_battle_despawn : public SpellScript
{
    PrepareSpellScript(spell_gilneas_battle_despawn);

    void SelectForsaken(std::list<WorldObject*>& targets)
    {
        if (auto battle = GetBattle(GetCaster()->ToCreature()))
            targets.remove_if([battle](WorldObject const* target)
            {
                return !battle->Owns(target->ToCreature()) ||
                    (target->GetEntry() != Crossbowman && target->GetEntry() != DarkRanger && target->GetEntry() != Abomination);
            });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_battle_despawn::SelectForsaken, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

class spell_gilneas_battle_catapult : public SpellScript
{
    PrepareSpellScript(spell_gilneas_battle_catapult);

    void SelectBattleEnemies(std::list<WorldObject*>& targets)
    {
        Creature* vehicle = GetCaster()->ToCreature();
        if (GetCaster()->IsPlayer())
            vehicle = GetCaster()->GetVehicleCreatureBase();
        if (!vehicle || vehicle->GetEntry() != Catapult)
            return;
        if (auto battle = GetBattle(vehicle))
            targets.remove_if([battle, vehicle](WorldObject const* target)
            {
                auto enemy = target->ToCreature();
                return !battle->Owns(enemy) || !vehicle->IsHostileTo(enemy);
            });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_battle_catapult::SelectBattleEnemies, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_battle_catapult::SelectBattleEnemies, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

class spell_gilneas_battle_prince : public SpellScript
{
    PrepareSpellScript(spell_gilneas_battle_prince);

    Creature* FindPrince()
    {
        auto caster = GetCaster()->ToCreature();
        auto battle = GetBattle(caster);
        return battle ? battle->ResolveFinalePrince(caster, GetSpellInfo()->Id) : nullptr;
    }

    SpellCastResult CheckCast() { return FindPrince() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS; }
    void SelectPrince(WorldObject*& target) { target = FindPrince(); }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_gilneas_battle_prince::CheckCast);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_gilneas_battle_prince::SelectPrince, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
    }
};

bool CanStart(Player* player, Creature const* krennan)
{
    if (!HasBattle(player) || krennan->GetDBTableGUIDLow() != KrennanSpawn || !krennan->IsAlive() || !player->InSamePhase(krennan) ||
        player->GetAreaId() != Outpost || !player->IsWithinDistInMap(krennan, INTERACTION_DISTANCE) ||
        !player->IsWithinLOSInMap(krennan))
        return false;

    return !OpeningAI::FindPlayerAttempt(*player);
}

void RestoreLegacyBattle(Player* player)
{
    ::Quest const* quest = sQuestDataStore->GetQuestTemplate(Quest);
    if (quest && !quest->StartScript && player->getRace() == RACE_WORGEN &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(Quest) &&
        player->GetQuestObjectiveData(Quest, 38854) == 0)
        player->IncompleteQuest(Quest);
}

class player_gilneas_battle : public PlayerScript
{
public:
    player_gilneas_battle() : PlayerScript("player_gilneas_battle") { }

    void OnLogin(Player* player) override { RestoreLegacyBattle(player); }
};

bool Begin(Player& owner)
{
    auto properties = sSummonPropertiesStore.LookupEntry(2912);
    if (!properties)
        return false;
    auto root = std::make_unique<TempSummon>(properties, &owner, false);
    Position const& position = LeaderStarts[0];
    Map* map = owner.GetMap();
    if (!root->Create(sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate(), map, owner.GetPhaseMask(), Liam, 0,
        owner.GetTeam(), position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), position.GetOrientation()))
        return false;

    root->SetPhaseId(owner.GetPhases(), false);
    // Visibility remains personal. The contest and its original victory cast
    // admit enrolled party members through the encounter's own target checks.
    owner.BindQuestObject(Quest, root.get(), false);
    if (!root->HasQuestObjectScope())
        return false;

    root->SetHomePosition(position);
    root->InitStats(Lifetime);
    root->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
    root->SetReactState(REACT_PASSIVE);
    if (!map->AddToMap(static_cast<Creature*>(root.get())))
    {
        owner.GetSummonList(Liam)->remove(root->GetGUID());
        return false;
    }

    TempSummon* leader = root.release();
    auto ai = new OpeningAI(leader);
    if (!leader->AIM_Initialize(ai))
    {
        leader->UnSummon();
        return false;
    }
    leader->setActive(true);
    leader->InitSummon();
    if (!ai->Start(owner))
    {
        ai->Stop();
        return false;
    }
    return true;
}

class npc_gilneas_battle_krennan : public CreatureScript
{
public:
    npc_gilneas_battle_krennan() : CreatureScript("npc_gilneas_battle_krennan") { }

    bool OnGossipHello(Player* player, Creature* actor) override
    {
        if (actor->GetDBTableGUIDLow() == KrennanSpawn)
            RestoreLegacyBattle(player);
        if (!CanStart(player, actor))
            return false;
        player->PlayerTalkClass->ClearMenus();
        auto& menu = player->PlayerTalkClass->GetGossipMenu();
        menu.SetMenuId(Menu);
        menu.AddMenuItem(Menu, 0, GOSSIP_SENDER_MAIN, Quest);
        player->PlayerTalkClass->SendGossipMenu(MenuText, actor->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player*, Creature* actor, uint32, uint32) override
    {
        return actor->GetDBTableGUIDLow() == KrennanSpawn;
    }

    struct KrennanAI : ScriptedAI
    {
        explicit KrennanAI(Creature* actor) : ScriptedAI(actor) { }

        void sGossipSelect(Player* player, uint32 menuId, uint32 index) override
        {
            auto const& menu = player->PlayerTalkClass->GetGossipMenu();
            if (menuId != Menu || menu.GetMenuId() != Menu || index != 0 || menu.GetMenuItemAction(index) != Quest || !CanStart(player, me))
                return;

            player->PlayerTalkClass->SendCloseGossip();
            player->PlayerTalkClass->ClearMenus();
            if (!Begin(*player))
                TC_LOG_WARN("scripts", "Gilneas battle: could not assemble the opening for player %u", player->GetGUIDLow());
            else
                sCreatureTextMgr->SendChat(me, 0, player->GetGUID(), CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
        }
    };

    CreatureAI* GetAI(Creature* actor) const override { return new KrennanAI(actor); }
};
}

#endif

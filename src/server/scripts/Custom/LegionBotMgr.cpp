/*
 * LegionBotMgr.cpp
 * LegionBotAI command manager + companion creature bots for LegionCore (7.3.5)
 *
 * Spawns a full dungeon team of companion bots that follow the owner,
 * fight, tank (taunt), heal, and can be dismissed.
 *
 * Commands:
 *   .lbot                  - spawn the default tank companion
 *   .lbot tank             - spawn a tank
 *   .lbot healer           - spawn a healer
 *   .lbot dps              - spawn a damage dealer
 *   .lbot team             - spawn a full 4-bot team (tank + healer + 2 dps)
 *   .lbot <entry>          - spawn a custom-entry companion (tank AI)
 *   .lbot dismiss          - dismiss all your companions
 *   .lbot <player> [entry] - (console/SOAP) spawn for a named player
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Creature.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "WorldSession.h"

#include <cmath>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

// LegionBotAI core (LegionBotAI.cpp) - real characters driven server-side
void LegionBot_Spawn(Player* owner, std::string const& charName, ChatHandler* handler, uint8 role = 255);
void LegionBot_DismissAll(Player* owner, ChatHandler* handler);
std::vector<ObjectGuid> LegionBot_GetBotsOf(ObjectGuid ownerGuid);
bool LegionBot_IsBot(ObjectGuid guid);
void LegionBot_DebugInfo(Player* owner, ChatHandler* handler);
bool LegionBot_ToggleSelfAI(Player* player);
void LegionBot_LevelCommand(Player* owner, std::string const& arg, ChatHandler* handler);
void LegionBot_AutogearTeam(Player* owner, ChatHandler* handler);
void LegionBot_TankCommand(Player* owner, std::string const& arg, ChatHandler* handler);
void LegionBot_AssistCommand(Player* owner, std::string const& arg, ChatHandler* handler);
void LegionBot_FollowCommand(Player* owner, std::string const& arg, ChatHandler* handler);
void LegionBot_AttackCommand(Player* owner, ChatHandler* handler);
void LegionBot_ComeCommand(Player* owner, ChatHandler* handler);
void LegionBot_LogAction(Player* owner, Unit* actor, char const* event,
                         uint32 spellId, Unit* target, int32 result, char const* detail);
void LegionBot_LogActor(Unit* actor, char const* event, uint32 spellId,
                        Unit* target, int32 result, char const* detail);
void LegionBot_LogHeartbeat(Player* owner, Unit* actor);
void LegionBot_LogCommand(Player* owner, std::string const& arg, ChatHandler* handler);

namespace
{
    // --- Default creature entries per role (Horde / goblin-flavoured) ---
    uint32 const COMPANION_BOT_TANK_ENTRY   = 68825;  // Bilgewater Bruiser
    uint32 const COMPANION_BOT_HEALER_ENTRY = 113035; // Darkspear Witch Doctor
    uint32 const COMPANION_BOT_DPS_ENTRY    = 67929;  // Bilgewater Sapper

    // --- Spells ---
    uint32 const SPELL_BOT_TAUNT     = 355;    // Taunt
    uint32 const SPELL_BOT_HEAL      = 19750;  // Flash of Light
    uint32 const SPELL_BOT_ATTACK    = 35395;  // Crusader Strike
    uint32 const SPELL_BOT_JUDGEMENT = 20271;  // Judgement
    uint32 const SPELL_BOT_LIGHTNING_BOLT = 403; // Lightning Bolt
    uint32 const SPELL_BOT_STRIKE    = 12294;  // Mortal Strike
    uint32 const SPELL_BOT_PUMMEL    = 6552;   // Melee interrupt
    uint32 const SPELL_BOT_WIND_SHEAR = 57994; // Witch doctor interrupt
    uint32 const SPELL_BOT_PURIFY_SPIRIT = 77130; // Witch doctor dispel

    float  const FOLLOW_DISTANCE     = 2.0f;
    float  const TELEPORT_DISTANCE   = 60.0f;
    uint32 const TAUNT_COOLDOWN      = 6000;
    uint32 const HEAL_COOLDOWN       = 2200;
    uint32 const ATTACK_COOLDOWN     = 3500;
    uint32 const STRIKE_COOLDOWN     = 5000;

    enum CompanionBotRole : uint8
    {
        ROLE_TANK   = 0,
        ROLE_HEALER = 1,
        ROLE_DPS    = 2
    };

    // owner guid -> bot guids
    std::multimap<ObjectGuid, ObjectGuid> g_companionBots;
    std::mutex g_companionBotsMutex;

    void RegisterBot(ObjectGuid ownerGuid, ObjectGuid botGuid)
    {
        std::lock_guard<std::mutex> lock(g_companionBotsMutex);
        g_companionBots.insert(std::make_pair(ownerGuid, botGuid));
    }

    void UnregisterBot(ObjectGuid ownerGuid, ObjectGuid botGuid)
    {
        std::lock_guard<std::mutex> lock(g_companionBotsMutex);
        auto range = g_companionBots.equal_range(ownerGuid);
        for (auto itr = range.first; itr != range.second; ++itr)
        {
            if (itr->second == botGuid)
            {
                g_companionBots.erase(itr);
                return;
            }
        }
    }

    std::vector<Creature*> GetOwnerBots(Player* owner)
    {
        std::vector<Creature*> result;
        if (!owner)
            return result;

        std::lock_guard<std::mutex> lock(g_companionBotsMutex);
        auto range = g_companionBots.equal_range(owner->GetGUID());
        for (auto itr = range.first; itr != range.second; ++itr)
        {
            if (Creature* bot = ObjectAccessor::GetCreature(*owner, itr->second))
                result.push_back(bot);
        }
        return result;
    }
}

class CompanionBotAI : public CreatureAI
{
public:
    CompanionBotAI(Creature* creature, CompanionBotRole role) : CreatureAI(creature),
        _role(role), _ownerGuid(ObjectGuid::Empty), _followTimer(0), _tauntTimer(0), _healTimer(0),
        _attackTimer(0), _strikeTimer(0), _interruptTimer(0), _dispelTimer(0)
    {
        if (TempSummon* summon = creature->ToTempSummon())
            if (Unit* summoner = summon->GetSummoner())
                _ownerGuid = summoner->GetGUID();
    }

    void IsSummonedBy(Unit* summoner) override
    {
        _ownerGuid = summoner->GetGUID();
    }

    void EnterEvadeMode() override
    {
        CreatureAI::EnterEvadeMode();
        if (Unit* owner = GetBotOwner())
            me->GetMotionMaster()->MoveFollow(owner, FOLLOW_DISTANCE, 0.0f);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Unit* owner = GetBotOwner())
            LegionBot_LogAction(owner->ToPlayer(), me, "dead", 0, nullptr, 0, "npc_companion");
        UnregisterBot(_ownerGuid, me->GetGUID());
    }

    void UpdateAI(uint32 diff) override
    {
        Unit* owner = GetBotOwner();
        if (!owner || !owner->IsInWorld())
        {
            UnregisterBot(_ownerGuid, me->GetGUID());
            me->DespawnOrUnsummon();
            return;
        }

        LegionBot_LogHeartbeat(owner->ToPlayer(), me);

        _interruptTimer = _interruptTimer > diff ? _interruptTimer - diff : 0;
        _dispelTimer = _dispelTimer > diff ? _dispelTimer - diff : 0;

        // Teleport to owner if left behind
        if (me->GetDistance(owner) > TELEPORT_DISTANCE)
        {
            me->NearTeleportTo(owner->GetPosition());
            me->GetMotionMaster()->MoveFollow(owner, FOLLOW_DISTANCE, 0.0f);
            return;
        }

        if (!me->isInCombat())
        {
            // Keep following the owner when idle
            _followTimer += diff;
            if (_followTimer >= 1000)
            {
                _followTimer = 0;
                if (me->GetDistance(owner) > 4.0f)
                    me->GetMotionMaster()->MoveFollow(owner, FOLLOW_DISTANCE, 0.0f);
            }

            _tauntTimer = 0;
            _attackTimer = 0;
            _strikeTimer = 0;

            if (_role == ROLE_HEALER && TryDispel(owner))
                return;

            Unit* assistTarget = FindDefensiveTarget(owner);
            if (assistTarget)
            {
                LegionBot_LogAction(owner->ToPlayer(), me, "target_acquire", 0, assistTarget, 0, "npc_defensive");
                AttackStart(assistTarget);
            }

            return;
        }

        // --- In combat ---
        Unit* victim = me->getVictim();
        if (!IsAttackingTeam(owner, victim))
        {
            if (victim)
                LegionBot_LogAction(owner->ToPlayer(), me, "target_drop", 0, victim, 0, "not_attacking_team");
            me->AttackStop();
            Unit* newTarget = FindDefensiveTarget(owner);
            if (newTarget)
            {
                LegionBot_LogAction(owner->ToPlayer(), me, "target_acquire", 0, newTarget, 0, "npc_defensive");
                AttackStart(newTarget);
            }
            else
            {
                me->GetMotionMaster()->MoveFollow(owner, FOLLOW_DISTANCE, 0.0f);
                return;
            }
        }

        // Healer behaviour
        if (_role == ROLE_HEALER)
        {
            if (_healTimer <= diff)
            {
                _healTimer = HEAL_COOLDOWN;
                Unit* healTarget = nullptr;
                float lowestPct = 80.0f;

                // owner first
                if (owner->IsAlive() && owner->GetHealthPct() < lowestPct)
                {
                    lowestPct = owner->GetHealthPct();
                    healTarget = owner;
                }

                // then the rest of the team
                for (Creature* bot : GetOwnerBots(owner->ToPlayer()))
                {
                    if (bot == me || !bot->IsAlive())
                        continue;
                    if (bot->GetHealthPct() < lowestPct)
                    {
                        lowestPct = bot->GetHealthPct();
                        healTarget = bot;
                    }
                }

                // finally self
                if (!healTarget && me->GetHealthPct() < 50.0f)
                    healTarget = me;

                if (healTarget)
                {
                    CastLogged(owner, healTarget, SPELL_BOT_HEAL);
                    return;
                }
            }
            else
                _healTimer -= diff;

            if (TryInterrupt(owner) || TryDispel(owner))
                return;

            if (_attackTimer <= diff)
            {
                _attackTimer = ATTACK_COOLDOWN;
                if (Unit* target = me->getVictim())
                    if (target->IsAlive())
                        CastLogged(owner, target, SPELL_BOT_LIGHTNING_BOLT);
            }
            else
                _attackTimer -= diff;

            DoMeleeAttackIfReady();
            return;
        }

        // Tank behaviour: taunt anything hitting the owner so the bot tanks it
        if (TryInterrupt(owner))
            return;

        if (_role == ROLE_TANK)
        {
            if (_tauntTimer <= diff)
            {
                _tauntTimer = TAUNT_COOLDOWN;
                if (Unit* ownerAttacker = owner->getAttackerForHelper())
                    if (IsAttackingTeam(owner, ownerAttacker) && ownerAttacker != me->getVictim() && me->IsValidAttackTarget(ownerAttacker))
                        CastLogged(owner, ownerAttacker, SPELL_BOT_TAUNT);
            }
            else
                _tauntTimer -= diff;
        }

        // Tank + DPS keep themselves up a bit
        if (_role != ROLE_HEALER)
        {
            if (_healTimer <= diff)
            {
                _healTimer = HEAL_COOLDOWN * 2;
                if (owner->IsAlive() && owner->GetHealthPct() < 45.0f)
                    CastLogged(owner, owner, SPELL_BOT_HEAL);
                else if (me->GetHealthPct() < 40.0f)
                    CastLogged(owner, me, SPELL_BOT_HEAL);
            }
            else
                _healTimer -= diff;
        }

        // Damage abilities
        if (_attackTimer <= diff)
        {
            _attackTimer = ATTACK_COOLDOWN;
            if (Unit* target = me->getVictim())
                if (target->IsAlive())
                    CastLogged(owner, target, (_role == ROLE_DPS) ? SPELL_BOT_STRIKE : SPELL_BOT_ATTACK);
        }
        else
            _attackTimer -= diff;

        if (_role == ROLE_DPS)
        {
            if (_strikeTimer <= diff)
            {
                _strikeTimer = STRIKE_COOLDOWN;
                if (Unit* target = me->getVictim())
                    if (target->IsAlive())
                        CastLogged(owner, target, SPELL_BOT_JUDGEMENT);
            }
            else
                _strikeTimer -= diff;
        }

        DoMeleeAttackIfReady();
    }

private:
    SpellCastResult CastLogged(Unit* owner, Unit* target, uint32 spellId)
    {
        SpellCastResult const result = me->CastSpell(target, spellId, false);
        LegionBot_LogAction(owner->ToPlayer(), me,
            result == SPELL_CAST_OK ? "cast_accepted" : "cast_failed",
            spellId, target, int32(result), "npc_cast");
        return result;
    }

    bool TryInterrupt(Unit* owner)
    {
        if (_interruptTimer || me->IsNonMeleeSpellCast(false))
            return false;
        uint32 spellId = _role == ROLE_HEALER ? SPELL_BOT_WIND_SHEAR : SPELL_BOT_PUMMEL;
        auto interrupt = [&](Unit* enemy)
        {
            if (!IsAttackingTeam(owner, enemy) || !me->IsValidAttackTarget(enemy))
                return false;
            Spell* cast = enemy->GetCurrentSpell(CURRENT_GENERIC_SPELL);
            if (!cast || cast->getState() == SPELL_STATE_FINISHED || cast->GetCastTime() <= 0)
                cast = enemy->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
            if (!cast || cast->getState() == SPELL_STATE_FINISHED || !cast->IsInterruptable())
                return false;
            if (CastLogged(owner, enemy, spellId) != SPELL_CAST_OK)
                return false;
            _interruptTimer = 12000;
            return true;
        };
        for (Unit* attacker : *owner->getAttackers())
            if (interrupt(attacker))
                return true;
        for (Unit* attacker : *me->getAttackers())
            if (interrupt(attacker))
                return true;
        for (Creature* companion : GetOwnerBots(owner->ToPlayer()))
            if (companion != me)
                for (Unit* attacker : *companion->getAttackers())
                    if (interrupt(attacker))
                        return true;
        return false;
    }

    bool TryDispel(Unit* owner)
    {
        if (_role != ROLE_HEALER || _dispelTimer || me->IsNonMeleeSpellCast(false))
            return false;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_BOT_PURIFY_SPIRIT);
        if (!spellInfo)
            return false;
        uint32 const dispelMask = spellInfo->GetSimilarEffectsMiscValueMask(SPELL_EFFECT_DISPEL, me);
        if (!dispelMask)
            return false;
        auto cleanse = [&](Unit* ally)
        {
            if (!ally || !ally->IsAlive() || !ally->IsInWorld() ||
                ally->GetMap() != me->GetMap() || me->GetDistance(ally) > 40.0f)
                return false;
            DispelChargesList auras;
            ally->GetDispellableAuraList(me, dispelMask, auras);
            for (auto const& entry : auras)
            {
                AuraApplication* application = entry.first->GetApplicationOfTarget(ally->GetGUID());
                if (application && !application->IsPositive() &&
                    CastLogged(owner, ally, SPELL_BOT_PURIFY_SPIRIT) == SPELL_CAST_OK)
                {
                    _dispelTimer = 8000;
                    return true;
                }
            }
            return false;
        };
        if (cleanse(owner) || cleanse(me))
            return true;
        for (Creature* companion : GetOwnerBots(owner->ToPlayer()))
            if (companion != me && cleanse(companion))
                return true;
        return false;
    }

    bool IsAttackingTeam(Unit* owner, Unit* enemy) const
    {
        if (!enemy || !enemy->IsAlive() || !enemy->IsInWorld() || enemy->GetMap() != owner->GetMap())
            return false;
        auto attacks = [&](Unit* ally)
        {
            return ally && ally->IsAlive() && ally->IsInWorld() &&
                (enemy->getVictim() == ally || ally->getAttackers()->count(enemy) != 0);
        };
        if (attacks(owner) || attacks(me))
            return true;
        for (Creature* companion : GetOwnerBots(owner->ToPlayer()))
            if (attacks(companion))
                return true;
        return false;
    }

    Unit* FindDefensiveTarget(Unit* owner) const
    {
        auto valid = [&](Unit* enemy)
        {
            return IsAttackingTeam(owner, enemy) && me->IsValidAttackTarget(enemy) &&
                me->GetDistance(enemy) <= 30.0f;
        };
        if (Unit* attacker = owner->getAttackerForHelper())
            if (valid(attacker))
                return attacker;
        if (Unit* attacker = me->getAttackerForHelper())
            if (valid(attacker))
                return attacker;
        for (Creature* companion : GetOwnerBots(owner->ToPlayer()))
            if (Unit* attacker = companion->getAttackerForHelper())
                if (valid(attacker))
                    return attacker;
        return nullptr;
    }

    Unit* GetBotOwner() const
    {
        if (_ownerGuid.IsEmpty())
            return nullptr;
        return ObjectAccessor::GetUnit(*me, _ownerGuid);
    }

    CompanionBotRole _role;
    ObjectGuid _ownerGuid;
    uint32 _followTimer;
    uint32 _tauntTimer;
    uint32 _healTimer;
    uint32 _attackTimer;
    uint32 _strikeTimer;
    uint32 _interruptTimer;
    uint32 _dispelTimer;
};

// ---------------------------------------------------------------------------
// Spawn / dismiss helpers
// ---------------------------------------------------------------------------

static void DespawnCompanions(Player* owner)
{
    if (!owner)
        return;

    for (Creature* bot : GetOwnerBots(owner))
        if (bot)
        {
            LegionBot_LogAction(owner, bot, "dismiss", 0, owner, 0, "npc_companion");
            bot->DespawnOrUnsummon();
        }

    std::lock_guard<std::mutex> lock(g_companionBotsMutex);
    g_companionBots.erase(owner->GetGUID());
}

static void SpawnCompanion(Player* owner, uint32 entry, CompanionBotRole role, ChatHandler* handler)
{
    if (!owner)
        return;

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!cinfo)
    {
        if (handler)
        {
            handler->PSendSysMessage("|cffff4444Companion bot:|r creature entry %u does not exist.", entry);
            handler->SetSentErrorMessage(true);
        }
        return;
    }

    TempSummon* summon = owner->SummonCreature(entry, TEMPSUMMON_MANUAL_DESPAWN);
    if (!summon)
    {
        if (handler)
        {
            handler->PSendSysMessage("|cffff4444Companion bot:|r failed to summon entry %u.", entry);
            handler->SetSentErrorMessage(true);
        }
        return;
    }

    Creature* bot = summon->ToCreature();

    bot->setFaction(owner->getFaction());
    bot->SelectLevel(bot->GetCreatureTemplate());
    bot->SetLevel(owner->getLevel());

    // Role based durability (scaled to the owner's level)
    float healthMultiplier = (role == ROLE_TANK) ? 4.0f : ((role == ROLE_HEALER) ? 2.0f : 2.5f);

    CreatureTemplate const* botTemplate = bot->GetCreatureTemplate();
    uint64 health = bot->GetMaxHealth();
    if (CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(owner->getLevel(), botTemplate->unit_class))
        health = uint64(stats->GenerateHealth(botTemplate, bot->GetCreatureDiffStat()));

    health = uint64(health * healthMultiplier);
    bot->SetCreateHealth(health);
    bot->SetMaxHealth(health);
    bot->SetHealth(health);
    bot->UpdateDamagePhysical(BASE_ATTACK);

    bot->SetReactState(REACT_DEFENSIVE);
    bot->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

    bot->AIM_Initialize(new CompanionBotAI(bot, role));

    RegisterBot(owner->GetGUID(), bot->GetGUID());

    LegionBot_LogAction(owner, bot, "spawn", 0, owner, int32(role), "npc_companion");

    bot->GetMotionMaster()->MoveFollow(owner, FOLLOW_DISTANCE, 0.0f);

    if (handler)
    {
        char const* roleName = (role == ROLE_TANK) ? "tank" : ((role == ROLE_HEALER) ? "healer" : "damage");
        handler->PSendSysMessage("|cff33ff99Companion bot|r (%s, entry %u) summoned. Team: %u bot(s).",
            roleName, entry, uint32(GetOwnerBots(owner).size()));
    }
}

// ---------------------------------------------------------------------------
// Command:  .lbot [tank|healer|dps|team|<entry>|dismiss]
// ---------------------------------------------------------------------------

class legionbot_commandscript : public CommandScript
{
public:
    legionbot_commandscript() : CommandScript("legionbot_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> CommandTable =
        {
            { "lbot", SEC_PLAYER, true, &HandlePlayerbotCommand, "LegionBotAI: team | spawn <name> | self | dismiss | info | log on|off|status | fieldcheck | level | autogear | aggro | assist | follow | stay | attack | come | creatures" }
        };

        return CommandTable;
    }

    static bool HandlePlayerbotCommand(ChatHandler* handler, char const* args)
    {
        Player* target = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;

        std::string first;
        std::string second;
        std::string third;
        if (args && *args)
        {
            std::istringstream iss(args);
            iss >> first;
            iss >> second;
            iss >> third;
        }

        // ".lbot dismiss" - removes companion creatures + playerbots
        if (first == "dismiss")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444Companion bot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            DespawnCompanions(target);
            LegionBot_DismissAll(target, handler);
            handler->PSendSysMessage("|cff33ff99Companion bot|r team dismissed.");
            return true;
        }

        // Console/SOAP: ".lbot <playerName> <role|spawn> [charName]"
        bool const isRoleKeyword = (first == "spawn" || first == "team" || first == "creatures" || first == "tank" || first == "healer" || first == "dps" || first == "dismiss" || first == "info" || first == "log" || first == "fieldcheck" || first == "self" || first == "rescue" || first == "level" || first == "autogear" || first == "aggro" || first == "assist" || first == "follow" || first == "stay" || first == "attack" || first == "come");
        if (!target && !first.empty() && !isRoleKeyword)
        {
            target = ObjectAccessor::FindPlayerByName(first);
            if (!target)
            {
                handler->PSendSysMessage("|cffff4444Companion bot:|r player '%s' not found or not online.", first.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
            first = second;
            second = third;
        }

        // ".lbot <player> dismiss" - console form
        if (first == "dismiss")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444Companion bot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            DespawnCompanions(target);
            LegionBot_DismissAll(target, handler);
            handler->PSendSysMessage("|cff33ff99Companion bot|r team dismissed.");
            return true;
        }

        // ".lbot spawn <charName>" - spawn a real character as a bot
        if (first == "spawn")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444Playerbot:|r specify an online player first.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (second.empty())
            {
                handler->SendSysMessage("|cffff4444Playerbot:|r usage: .lbot spawn <characterName>");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_Spawn(target, second, handler);
            return true;
        }

        if (first == "log")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r specify an online player first.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_LogCommand(target, second, handler);
            return true;
        }

        // Console or GM-only terrain/collision check for the seven test dummies.
        if (first == "fieldcheck")
        {
            if (handler->GetSession() && handler->GetSession()->GetSecurity() < SEC_GAMEMASTER)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r fieldcheck requires GM access.");
                handler->SetSentErrorMessage(true);
                return false;
            }

            Map* map = sMapMgr->CreateBaseMap(1);
            if (!map)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r Kalimdor map unavailable.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            map->LoadGrid(16282.0f, 16275.0f);
            struct FieldSpot { uint8 group; float x; float y; float z; };
            FieldSpot const spots[] = {
                { 1, 16282.0f, 16275.0f, 17.37f },
                { 2, 16296.0f, 16273.0f, 18.06f },
                { 2, 16296.0f, 16277.0f, 16.97f },
                { 4, 16312.0f, 16273.0f, 17.79f },
                { 4, 16315.0f, 16273.0f, 17.61f },
                { 4, 16312.0f, 16276.0f, 16.51f },
                { 4, 16315.0f, 16276.0f, 16.21f }
            };
            for (FieldSpot const& spot : spots)
            {
                float const terrain = map->GetHeight(spot.x, spot.y, spot.z + 3.0f, false);
                float const collision = map->GetHeight(spot.x, spot.y, spot.z + 3.0f, true);
                float const approach = map->GetHeight(spot.x - 6.0f, spot.y, spot.z + 8.0f, true);
                bool const lineOfSight = map->isInLineOfSight(spot.x - 6.0f, spot.y, approach + 1.5f,
                    spot.x, spot.y, spot.z + 1.5f, {}, VMAP::ModelIgnoreFlags::Nothing);
                bool const clear = std::fabs(terrain - spot.z) < 0.7f &&
                    std::fabs(collision - spot.z) < 0.7f && lineOfSight;
                handler->PSendSysMessage("LegionBot field %u: (%.1f, %.1f, %.2f) terrain %.2f collision %.2f LOS %u %s",
                    uint32(spot.group), spot.x, spot.y, spot.z, terrain, collision, uint32(lineOfSight),
                    clear ? "OK" : "CHECK");
            }
            return true;
        }

        // ".lbot info" - diagnostic: where are my bots?
        if (first == "info")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444Playerbot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }

            std::vector<ObjectGuid> botGuids = LegionBot_GetBotsOf(target->GetGUID());
            if (botGuids.empty())
            {
                handler->SendSysMessage("|cff33ff99Playerbot:|r no bots registered for you.");
                return true;
            }

            LegionBot_DebugInfo(target, handler);

            for (ObjectGuid botGuid : botGuids)
            {
                if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
                {
                    handler->PSendSysMessage("|cff33ff99Bot|r %s: map %u inst %u pos (%.1f %.1f %.1f) | you: map %u inst %u pos (%.1f %.1f %.1f) | dist %.1f",
                        bot->GetName(), bot->GetMapId(), bot->GetInstanceId(),
                        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
                        target->GetMapId(), target->GetInstanceId(),
                        target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(),
                        bot->GetDistance(target));

                    handler->PSendSysMessage("|cff33ff99flags|r inWorld=%d changeMap=%d deleted=%d canContact=%d loading=%d logout=%d session=%d",
                        bot->IsInWorld() ? 1 : 0, bot->IsChangeMap() ? 1 : 0, bot->IsDelete() ? 1 : 0, bot->CanContact() ? 1 : 0,
                        bot->GetSession()->PlayerLoading() ? 1 : 0, bot->GetSession()->PlayerLogout() ? 1 : 0, bot->GetSession() ? 1 : 0);
                }
                else
                    handler->SendSysMessage("|cff33ff99Playerbot:|r a registered bot is not in the world.");
            }
            return true;
        }

        // ".lbot level sync|max|<n>" - how the team levels (playerbot-style)
        if (first == "level")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_LevelCommand(target, second, handler);
            return true;
        }

        // ".lbot autogear" - re-equip the team with level-appropriate gear
        if (first == "autogear")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_AutogearTeam(target, handler);
            return true;
        }

        // ".lbot aggro me|bot" - who holds aggro (player tank mode)
        if (first == "aggro")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_TankCommand(target, second, handler);
            return true;
        }

        // ".lbot assist full|defend|chill" - how the bots join fights
        if (first == "assist")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_AssistCommand(target, second, handler);
            return true;
        }

        // ".lbot follow" / ".lbot stay" - follow the owner or hold position
        if (first == "follow" || first == "stay")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_FollowCommand(target, first, handler);
            return true;
        }

        // ".lbot attack" - order the whole team onto your current target
        if (first == "attack")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_AttackCommand(target, handler);
            return true;
        }

        // ".lbot come" - call the team to your position
        if (first == "come")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            LegionBot_ComeCommand(target, handler);
            return true;
        }

        // ".lbot self" - toggle self-AI: the bot AI fights for you
        if (first == "self")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (LegionBot_ToggleSelfAI(target))
                handler->PSendSysMessage("|cff33ff99LegionBot:|r self-AI enabled - I'll attack and use abilities for you. Use '.lbot self' again to stop.");
            else
                handler->PSendSysMessage("|cff33ff99LegionBot:|r self-AI disabled.");
            return true;
        }

        // ".lbot rescue" - teleport the player home and revive (unstuck from under the map)
        if (first == "rescue")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444LegionBot:|r this command requires a player.");
                handler->SetSentErrorMessage(true);
                return false;
            }

            target->CombatStop(true);
            if (target->isDead())
            {
                target->ResurrectPlayer(1.0f, false);
                target->SpawnCorpseBones();
            }
            target->SetHealth(target->GetMaxHealth());
            target->TeleportTo(target->m_homebindMapId, target->m_homebindX, target->m_homebindY, target->m_homebindZ, target->GetOrientation());
            handler->PSendSysMessage("|cff33ff99LegionBot:|r rescued - revived and teleported home.");
            return true;
        }

        // Role handling
        if (first == "team")
        {
            if (!target)
            {
                handler->SendSysMessage("|cffff4444Playerbot:|r specify an online player first.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            // The real dungeon team: 4 playerbot characters with 7.3.5 meta classes.
            // The team is chosen by the owner's faction so Alliance characters get
            // Alliance bots (and vice versa).
            if (target)
            {
                DespawnCompanions(target);
                LegionBot_DismissAll(target, nullptr);
            }

            bool const alliance = target && (target->GetTeamId() == TEAM_ALLIANCE);
            if (alliance)
            {
                LegionBot_Spawn(target, "Aegis",     handler, 0);   // Human Blood DK tank
                LegionBot_Spawn(target, "Seraphine", handler, 1);   // Draenei Holy Paladin healer
                LegionBot_Spawn(target, "Rook",      handler, 2);   // Human Fury Warrior dps
                LegionBot_Spawn(target, "Elowen",    handler, 1);   // Night Elf Holy Priest healer
                handler->PSendSysMessage("|cff33ff99Alliance playerbot team ready!|r (Human DK tank, Draenei + Night Elf healers, Human Fury Warrior)");
            }
            else
            {
                LegionBot_Spawn(target, "Bulwark", handler, 0);   // Blood DK tank
                LegionBot_Spawn(target, "Lovley",  handler, 1);   // Holy Paladin healer
                LegionBot_Spawn(target, "Ember",   handler, 2);   // Fury Warrior dps
                LegionBot_Spawn(target, "Faith",   handler, 1);   // Holy Priest healer (heals only)
                handler->PSendSysMessage("|cff33ff99Full playerbot dungeon team ready!|r (Blood DK tank, Paladin + Priest healers, Fury Warrior damage)");
            }
            return true;
        }
        if (first == "creatures")
        {
            // The NPC companion team (creature bots)
            if (target)
                DespawnCompanions(target);
            SpawnCompanion(target, COMPANION_BOT_TANK_ENTRY,   ROLE_TANK,   handler);
            SpawnCompanion(target, COMPANION_BOT_HEALER_ENTRY, ROLE_HEALER, handler);
            SpawnCompanion(target, COMPANION_BOT_DPS_ENTRY,    ROLE_DPS,    handler);
            SpawnCompanion(target, COMPANION_BOT_DPS_ENTRY,    ROLE_DPS,    handler);
            handler->PSendSysMessage("|cff33ff99NPC companion team ready!|r (1 tank, 1 healer, 2 damage)");
            return true;
        }

        if (!target)
        {
            handler->SendSysMessage("|cffff4444Companion bot:|r this command requires a player (use .lbot <player> [role] from console).");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (first == "tank")
        {
            SpawnCompanion(target, COMPANION_BOT_TANK_ENTRY, ROLE_TANK, handler);
            return true;
        }
        if (first == "healer")
        {
            SpawnCompanion(target, COMPANION_BOT_HEALER_ENTRY, ROLE_HEALER, handler);
            return true;
        }
        if (first == "dps")
        {
            SpawnCompanion(target, COMPANION_BOT_DPS_ENTRY, ROLE_DPS, handler);
            return true;
        }

        // Custom entry
        uint32 entry = COMPANION_BOT_TANK_ENTRY;
        if (!first.empty())
            entry = uint32(atoi(first.c_str()));

        SpawnCompanion(target, entry, ROLE_TANK, handler);
        return true;
    }
};

// The ordinary training dummy is passive. This one answers the player's first
// hit with a stationary, zero-damage attack so defensive player bots can test
// their rotation without an exception to the no-pull rule.
class npc_legionbot_rotation_dummy : public CreatureScript
{
public:
    npc_legionbot_rotation_dummy() : CreatureScript("npc_legionbot_rotation_dummy") { }

    struct RotationDummyAI : Scripted_NoMovementAI
    {
        RotationDummyAI(Creature* creature) : Scripted_NoMovementAI(creature) { }

        void Reset() override
        {
            if (!me->isTrainingDummy())
                me->AddUnitTypeMask(UNIT_MASK_TRAINING_DUMMY);
            me->SetReactState(REACT_DEFENSIVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        }

        void MoveInLineOfSight(Unit* /*who*/) override { }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*type*/) override
        {
            uint32 const attemptedDamage = damage;
            int32 const loggedDamage = attemptedDamage > 2147483647u ? 2147483647 : int32(attemptedDamage);
            damage = 0;
            if (!attacker)
                return;
            Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (attacker->ToPlayer())
                LegionBot_LogActor(attacker, "dummy_hit", 0, me, loggedDamage, "prevented_damage");
            else if (player)
                LegionBot_LogAction(player, attacker, "dummy_hit", 0, me, loggedDamage, "prevented_damage");
            if (player && !LegionBot_IsBot(player->GetGUID()) && !me->getVictim())
                AttackStart(player);
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (Unit* victim = me->getVictim())
                if (!victim->IsAlive() || !victim->IsInWorld() || me->GetDistance(victim) > 40.0f)
                    EnterEvadeMode();
            // AttackStart establishes an attacker/victim relation, but the
            // dummy never swings or moves toward the player.
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new RotationDummyAI(creature);
    }
};

void AddSC_LegionBotMgr()
{
    new legionbot_commandscript();
    new npc_legionbot_rotation_dummy();
}

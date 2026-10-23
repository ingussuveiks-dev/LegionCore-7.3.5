#include "ScriptMgr.h"
#include "Player.h"
#include "GameObject.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

namespace ArtifactAcquisition
{
struct Pickup {uint32 object,item,playerClass,quest,alternate;};
Pickup const Pickups[]={
    {249812,128861,CLASS_HUNTER,42185,0}, // Never Hunt Alone, after the workshop
    {248834,128808,CLASS_HUNTER,40385,0},
    {248572,128868,CLASS_PRIEST,41631,0},
    {246699,128403,CLASS_DEATH_KNIGHT,40934,40986},
    {249814,127857,CLASS_MAGE,42011,0},
    {251049,128862,CLASS_MAGE,42479,0},
    {246021,128942,CLASS_WARLOCK,40623,41155},
    {249821,128943,CLASS_WARLOCK,42125,0},
    {246437,128858,CLASS_DRUID,40838,0}
};
bool Active(Player* p,uint32 quest)
{
    return quest && !p->GetQuestRewardStatus(quest) &&
        (p->GetQuestStatus(quest)==QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(quest)==QUEST_STATUS_COMPLETE);
}
bool Use(Player* p,GameObject* go)
{
    for (Pickup const& row:Pickups)
        if (go->GetEntry()==row.object)
        {
            if (!p->IsAlive() || p->IsBeingTeleported() || p->getClass()!=row.playerClass ||
                (!Active(p,row.quest) && !Active(p,row.alternate)) || !go->isSpawned() ||
                go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE) ||
                p->GetMap()!=go->GetMap() || !p->InSamePhase(go) || !p->canSeeOrDetect(go) || !p->IsWithinLOSInMap(go) ||
                p->GetExactDist(go)>5.0f || go->getLootState()==GO_JUST_DEACTIVATED) return true;
            if (!p->HasItemCount(row.item,1,true)) return false;
            // A shop/bank copy satisfies ownership. Replay the same native
            // loot-completed AI event, without trying to loot another unique
            // weapon. Ordinary players still use real loot, including bag errors.
            go->SetLootState(GO_JUST_DEACTIVATED,p);
            // StoreNewItem normally emits this event. A previously bought
            // unique artifact emits no new item event at its quest chest.
            p->UpdateAchievementCriteria(CRITERIA_TYPE_OWN_ITEM,row.item,1);
            TC_LOG_INFO("entities.player","Artifact acquisition: player %u used owned artifact %u at object %u",p->GetGUIDLow(),row.item,row.object);
            return true;
        }
    return false;
}
}
namespace DarkRidersCompletion
{
void Recover(Player* player)
{
    // Shop ownership alone is not a quest completion. The persisted claim
    // objective is awarded by the final chest's native 204819 spell, after
    // Ariden has been defeated. Also repairs a claim saved before this fix.
    if (!player || !player->IsInWorld() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_WARLOCK || player->GetQuestRewardStatus(40623) ||
        player->GetQuestStatus(40623) != QUEST_STATUS_INCOMPLETE ||
        player->GetQuestObjectiveData(40623, 103473) < 1 ||
        player->GetQuestObjectiveData(40623, 100813) >= 1 ||
        !player->HasItemCount(128942, 1, true))
        return;

    player->KilledMonsterCredit(100813);
    TC_LOG_INFO("entities.player", "Dark Riders: restored scenario completion after claimed Ulthalesh for player %u", player->GetGUIDLow());
}
}
class player_dark_riders_completion : public PlayerScript
{
public:
    player_dark_riders_completion() : PlayerScript("player_dark_riders_completion") { }
    void OnLogin(Player* player) override { DarkRidersCompletion::Recover(player); }
    void OnUpdate(Player* player, uint32) override { DarkRidersCompletion::Recover(player); }
};
class go_artifact_acquisition : public GameObjectScript
{
public:
    go_artifact_acquisition():GameObjectScript("go_artifact_acquisition"){}
    bool OnGossipHello(Player* player,GameObject* go) override {return ArtifactAcquisition::Use(player,go);}
};
namespace TalonclawPresentation
{
struct State
{
    uint32 instance = 0;
    uint32 elapsed = 0;
    bool admitted = false;
    bool finished = false;
    bool fallback = false;

    bool Current(Player* player) const
    {
        return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
            player->getClass() == CLASS_HUNTER && player->GetMapId() == 1511 &&
            ArtifactAcquisition::Active(player, 40385) && player->HasItemCount(128808, 1, true);
    }

    void Begin(Player* player)
    {
        if (admitted || finished || !Current(player))
            return;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 973 || scenario->GetCurrentStep() != 12)
            return;
        instance = player->GetInstanceId();
        admitted = true;
    }

    bool Update(Player* player, uint32 diff)
    {
        if (!admitted || finished)
            return false;
        if (!Current(player) || player->GetInstanceId() != instance)
        {
            admitted = false;
            return true;
        }
        if (diff < 20000 - elapsed)
        {
            elapsed += diff;
            return false;
        }
        elapsed = 20000;
        fallback = true;
        TC_LOG_WARN("scripts", "Talonclaw pickup: scene 1220 completion callback missing after 20000ms for player %u", player->GetGUIDLow());
        return true;
    }

    void Finish(Player* player)
    {
        if (!admitted || finished || !Current(player) || player->GetInstanceId() != instance ||
            (!fallback && !player->HasSceneStatus(1220, SCENE_COMPLETE)))
            return;
        finished = true;
        player->CastSpell(player, 197651, true);
    }
};
}

class spell_talonclaw_presentation : public AuraScript
{
    PrepareAuraScript(spell_talonclaw_presentation);
    TalonclawPresentation::State state;

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({197651});
    }

    Player* PlayerOwner()
    {
        Unit* owner = GetUnitOwner();
        return owner ? owner->ToPlayer() : nullptr;
    }

    void AfterApply(AuraEffect const*, AuraEffectHandleModes)
    {
        state.Begin(PlayerOwner());
    }

    void Update(uint32 diff, AuraEffect*)
    {
        if (state.Update(PlayerOwner(), diff))
            Remove();
    }

    void AfterRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        state.Finish(PlayerOwner());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_talonclaw_presentation::AfterApply, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        OnEffectUpdate += AuraEffectUpdateFn(spell_talonclaw_presentation::Update, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE);
        AfterEffectRemove += AuraEffectRemoveFn(spell_talonclaw_presentation::AfterRemove, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_artifact_acquisition()
{
    new go_artifact_acquisition();
    new player_dark_riders_completion();
    RegisterAuraScript(spell_talonclaw_presentation);
}

#pragma once

#include <map>
#include <memory>
#include <mutex>

namespace MonkFelStone
{
constexpr uint32 QuestId = 12103, Parent = 193890, Event = 194141, Pilgrimage = 200617, Scene = 1111;
struct Claim
{
    ObjectGuid player, source, parent;
    Map* origin = nullptr; // Identity only, never dereferenced by retained work.
    uint32 instance = 0, startedAt = 0, pollAt = 0, retryAt = 0, retries = 0, returns = 0, duration = 0, lastStep = 0;
    std::weak_ptr<uint8> quest, actor;
    bool sceneApplied = false, sceneFinished = false, markerReceipt = false, hadMarker = false;
    bool hit = false, departing = false, done = false, cancelled = false, polling = false, retrying = false, warned = false, returnWarned = false;
};
// Only active player flows own a strong entry. Never hold the lock while
// calling game code; logout, map changes and failed casts release exact claims.
inline std::mutex ClaimsMutex;
inline std::map<ObjectGuid, std::shared_ptr<Claim>> Claims;
inline std::shared_ptr<Claim> Find(Player* player)
{
    if (!player) return {};
    std::lock_guard<std::mutex> lock(ClaimsMutex);
    auto found = Claims.find(player->GetGUID());
    return found != Claims.end() ? found->second : nullptr;
}
inline bool Reserve(std::shared_ptr<Claim> const& claim)
{
    std::lock_guard<std::mutex> lock(ClaimsMutex);
    return Claims.emplace(claim->player, claim).second;
}
inline void Release(std::shared_ptr<Claim> const& claim)
{
    std::lock_guard<std::mutex> lock(ClaimsMutex);
    auto found = Claims.find(claim->player);
    if (found != Claims.end() && found->second == claim) Claims.erase(found);
}
inline Scenario* World(Player* player)
{
    if (!player || player->GetMapId() != 1014 || !player->GetMap() || player->GetMap()->IsMapUnload()) return nullptr;
    auto scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 943 ? scenario : nullptr;
}
inline bool Leaf(Scenario* scenario, uint32 id)
{
    if (!scenario) return false;
    auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto found = progress->find(id);
    return found != progress->end() && found->second.Counter;
}
inline bool Active(Player* player)
{
    auto scenario = World(player);
    if (!scenario || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->isGameMaster() ||
        player->HasPendingTeleport() || player->getClass() != CLASS_MONK || player->GetQuestRewardStatus(QuestId) ||
        player->FindQuestSlot(QuestId) >= MAX_QUEST_LOG_SIZE) return false;
    auto status = player->GetQuestStatus(QuestId);
    uint32 step = scenario->GetCurrentStep();
    return (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) && Leaf(scenario, 29071) &&
        (step == 6 || (step == 7 && Leaf(scenario, 29072))) &&
        player->GetQuestObjectiveData(QuestId, 98514) > 0 && player->GetQuestObjectiveData(QuestId, 97778) > 0;
}
inline bool Current(Player* player, std::shared_ptr<Claim> const& claim)
{
    if (!claim || claim->cancelled || claim->done || !Active(player) || player->GetGUID() != claim->player ||
        player->GetMap() != claim->origin || player->GetInstanceId() != claim->instance) return false;
    auto quest = player->getQuestStatus(QuestId);
    auto attempt = claim->quest.lock();
    auto scenario = World(player);
    return quest && attempt && quest->ScriptLifetime == attempt && scenario->GetCurrentStep() >= claim->lastStep && Find(player) == claim;
}
inline bool Retained(Player* player, std::shared_ptr<Claim> const& claim)
{
    if (!player || !claim || player->GetGUID() != claim->player || player->getClass() != CLASS_MONK ||
        player->GetQuestRewardStatus(QuestId) || player->FindQuestSlot(QuestId) >= MAX_QUEST_LOG_SIZE) return false;
    auto status = player->GetQuestStatus(QuestId);
    auto quest = player->getQuestStatus(QuestId);
    auto attempt = claim->quest.lock();
    return (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE) && quest && attempt && quest->ScriptLifetime == attempt;
}
inline bool Source(Creature* actor)
{
    return actor && actor->GetMapId() == 1014 && actor->GetEntry() == 98353 &&
        (actor->GetDBTableGUIDLow() == 362198 || actor->GetDBTableGUIDLow() == 12691299);
}
bool Owns(Aura* aura, std::shared_ptr<Claim> const& claim);
void Cleanup(Player* player, std::shared_ptr<Claim> const& claim);
inline void Stop(Player* player, std::shared_ptr<Claim> const& claim)
{
    claim->cancelled = true;
    Release(claim);
    Cleanup(player, claim);
}
void Tick(Player* player, std::shared_ptr<Claim> const& claim);
}

struct npc_monk_fel_stone : SmartAI
{
    explicit npc_monk_fel_stone(Creature* actor) : SmartAI(actor) { }
    std::shared_ptr<uint8> lifetime = std::make_shared<uint8>();
    void OnSpellClick(Unit* clicker) override
    {
        if (!MonkFelStone::Source(me)) SmartAI::OnSpellClick(clicker);
    }
};

namespace MonkFelStone
{
inline npc_monk_fel_stone* Content(Creature* source)
{
    return Source(source) ? dynamic_cast<npc_monk_fel_stone*>(source->GetDisabledAI() ? source->GetDisabledAI() : source->GetAI()) : nullptr;
}
inline npc_monk_fel_stone* Contact(Player* player, Creature* source)
{
    if (!Active(player) || !Source(source) || source->GetMap() != player->GetMap() || !source->IsInWorld() ||
        source->IsPreDelete() || !source->IsAlive() || !source->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) ||
        source->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) ||
        !player->InSamePhase(source) || source->ShouldHideFor(player->GetGUID()) || !player->canSeeOrDetect(source) ||
        !player->IsWithinDistInMap(source, INTERACTION_DISTANCE) || !player->IsWithinLOSInMap(source)) return nullptr;
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    return quest && (!player->GetGroup() || !player->GetGroup()->isRaidGroup() || quest->IsAllowedInRaid(player->GetMap()->GetDifficultyID())) ? Content(source) : nullptr;
}
inline bool Casting(Player* player, std::shared_ptr<Claim> const& claim)
{
    if (!Current(player, claim)) return false;
    auto source = player->GetMap()->GetCreature(claim->source);
    auto actor = Contact(player, source);
    auto lifetime = claim->actor.lock();
    return actor && lifetime && actor->lifetime == lifetime && Find(player) == claim;
}
inline bool ParentGuid(ObjectGuid const& guid) { return guid.GetMapId() == 1014 && guid.GetEntry() == Parent; }
inline bool Child(Player* player, ObjectGuid const& parent, std::shared_ptr<Claim> const& claim)
{
    return Current(player, claim) && claim->sceneApplied && parent == claim->parent && ParentGuid(parent) && Find(player) == claim;
}
}

class aura_monk_fel_stone_scene : public AuraScript
{
    PrepareAuraScript(aura_monk_fel_stone_scene);
public:
    std::shared_ptr<MonkFelStone::Claim> claim;
private:
    bool Load() override { return GetUnitOwner() && GetUnitOwner()->IsPlayer() && GetUnitOwner()->GetMapId() == 1014; }
    void Apply(AuraEffect const*, AuraEffectHandleModes)
    {
        auto player = GetUnitOwner()->ToPlayer();
        if (!claim || !MonkFelStone::Casting(player, claim)) { PreventDefaultAction(); GetAura()->Remove(); }
    }
    void Applied(AuraEffect const*, AuraEffectHandleModes)
    {
        auto player = GetUnitOwner()->ToPlayer();
        if (!claim || !MonkFelStone::Current(player, claim) || !player->HasSceneStatus(MonkFelStone::Scene, SCENE_LAUNCH) ||
            player->HasSceneStatus(MonkFelStone::Scene, SCENE_COMPLETE)) return;
        claim->sceneApplied = true;
        claim->duration = uint32(GetAura()->GetMaxDuration());
    }
    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        if (!claim || claim->cancelled || claim->done || claim->departing) return;
        auto player = GetUnitOwner()->ToPlayer();
        if (claim->sceneApplied && (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE ||
            player->HasSceneStatus(MonkFelStone::Scene, SCENE_COMPLETE))) claim->sceneFinished = true;
        else MonkFelStone::Stop(player, claim);
    }
    void Update(uint32, AuraEffect*)
    {
        if (claim && !claim->departing && !claim->done && !MonkFelStone::Current(GetUnitOwner()->ToPlayer(), claim))
            MonkFelStone::Stop(GetUnitOwner()->ToPlayer(), claim);
    }
    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(aura_monk_fel_stone_scene::Apply, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        AfterEffectApply += AuraEffectApplyFn(aura_monk_fel_stone_scene::Applied, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(aura_monk_fel_stone_scene::Removed, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        OnEffectUpdate += AuraEffectUpdateFn(aura_monk_fel_stone_scene::Update, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE);
    }
};

class aura_monk_fel_stone_pilgrimage : public AuraScript
{
    PrepareAuraScript(aura_monk_fel_stone_pilgrimage);
public:
    std::shared_ptr<MonkFelStone::Claim> claim;
    bool authorizedApply = false;
private:
    void Applied(AuraEffect const*, AuraEffectHandleModes)
    {
        // A later194181 or another unrelated refresh must not retain this
        // flow's ownership of the shared native200617 aura.
        if (!authorizedApply) { claim.reset(); return; }
        authorizedApply = false;
        if (claim && MonkFelStone::Current(GetUnitOwner()->ToPlayer(), claim)) claim->markerReceipt = true;
    }
    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_monk_fel_stone_pilgrimage::Applied, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

namespace MonkFelStone
{
inline bool Owns(Aura* aura, std::shared_ptr<Claim> const& claim)
{
    if (aura) for (auto script : aura->m_loadedScripts)
    {
        if (auto scene = dynamic_cast<aura_monk_fel_stone_scene*>(script)) if (scene->claim == claim) return true;
        if (auto marker = dynamic_cast<aura_monk_fel_stone_pilgrimage*>(script)) if (marker->claim == claim) return true;
    }
    return false;
}
inline void Cleanup(Player* player, std::shared_ptr<Claim> const& claim)
{
    if (!player || player->GetGUID() != claim->player) return;
    for (uint32 id : {Parent, Pilgrimage})
        if (Aura* aura = player->GetAura(id)) if (Owns(aura, claim)) aura->Remove();
}
inline void RetryChild(Player* player, uint32 id, std::shared_ptr<Claim> const& claim)
{
    SpellCastTargets targets; targets.SetCaster(player); targets.SetUnitTarget(player);
    TriggerCastData data; data.triggerFlags = TRIGGERED_FULL_MASK; data.spellGuid = claim->parent; data.SubType = SPELL_CAST_TYPE_MISSILE;
    player->CastSpell(targets, sSpellMgr->GetSpellInfo(id), nullptr, data);
}
inline bool Arrived(Player* player, std::shared_ptr<Claim> const& claim)
{
    return Retained(player, claim) && player->IsInWorld() && player->CanContact() && player->IsAlive() && !player->HasPendingTeleport() &&
        player->GetMapId() == 1514 && player->IsWithinDist3d(736.019f, 3437.01f, 122.146f, INTERACTION_DISTANCE, false);
}
inline void ObserveReturn(Player* player, std::shared_ptr<Claim> const& claim)
{
    if (claim->done || claim->cancelled) return;
    if (!Retained(player, claim) || !player->IsAlive()) { Stop(player, claim); return; }
    if (Arrived(player, claim)) { claim->done = true; Release(claim); Cleanup(player, claim); return; }
    if (player->GetMap() != claim->origin && player->GetMapId() != 1514)
    {
        TC_LOG_WARN("scripts", "Monk Fel Stone: accepted1514 return reached another map; no original arrival acknowledged");
        Stop(player, claim); return;
    }
    if (GetMSTimeDiffToNow(claim->retryAt) >= 30000)
    {
        TC_LOG_WARN("scripts", "Monk Fel Stone: accepted1514 return has no observed arrival after30s; releasing owned visual lease without claiming arrival");
        Stop(player, claim); return;
    }
    if (player->IsInWorld() && !player->HasPendingTeleport() && GetMSTimeDiffToNow(claim->retryAt) >= 2000)
    {
        if (player->GetMap() == claim->origin) claim->departing = false;
        else
        {
            TC_LOG_WARN("scripts", "Monk Fel Stone: accepted1514 return has no original destination pose; no arrival acknowledged");
            Stop(player, claim); return;
        }
    }
}
inline void Tick(Player* player, std::shared_ptr<Claim> const& claim)
{
    if (claim->done || claim->cancelled) return;
    if (claim->departing) { ObserveReturn(player, claim); return; }
    if (!Current(player, claim)) { Stop(player, claim); return; }
    if (claim->polling && GetMSTimeDiffToNow(claim->pollAt) < 100) return;
    claim->polling = true; claim->pollAt = getMSTime();
    auto scenario = World(player); claim->lastStep = scenario->GetCurrentStep();
    bool portal = Leaf(scenario, 29072) && claim->lastStep == 7;
    bool credit = player->GetQuestObjectiveData(QuestId, 98353) > 0;
    if (!claim->hit || !claim->sceneApplied || !credit || !portal || !claim->markerReceipt)
    {
        if (GetMSTimeDiffToNow(claim->startedAt) < 2000) return;
        if (!claim->hit || !claim->sceneApplied || !credit)
        {
            TC_LOG_WARN("scripts", "Monk Fel Stone: native193890 scene/98353/46952/200617 receipt missing; original earned objectives retained for retry");
            Stop(player, claim); return;
        }
        if (claim->retrying && GetMSTimeDiffToNow(claim->retryAt) < 2000) return;
        if (claim->retries >= 3)
        {
            TC_LOG_WARN("scripts", "Monk Fel Stone: native children of original193890 still unacknowledged after three retries");
            Stop(player, claim); return;
        }
        claim->retrying = true; claim->retryAt = getMSTime(); ++claim->retries;
        if (!claim->warned) TC_LOG_WARN("scripts", "Monk Fel Stone: retrying missing native child of original193890 cast");
        claim->warned = true;
        if (!portal) RetryChild(player, Event, claim);
        if (Current(player, claim) && !claim->markerReceipt) RetryChild(player, Pilgrimage, claim);
        return;
    }
    if (!claim->sceneFinished)
    {
        if (claim->duration && GetMSTimeDiffToNow(claim->startedAt) > claim->duration + 6000)
        {
            TC_LOG_WARN("scripts", "Monk Fel Stone: original1111 scene produced no completion or native expiry; no return granted");
            Stop(player, claim);
        }
        return;
    }
    if (claim->retrying && GetMSTimeDiffToNow(claim->retryAt) < 2000) return;
    if (claim->returns >= 3)
    {
        TC_LOG_WARN("scripts", "Monk Fel Stone: original1514 return failed three times; earned objectives retained for another interaction");
        Stop(player, claim); return;
    }
    ++claim->returns;
    claim->retrying = true; claim->retryAt = getMSTime(); claim->departing = true;
    bool sent = player->TeleportTo(1514, 736.019f, 3437.01f, 122.146f, 0.0f);
    if (claim->done || claim->cancelled) return;
    if (sent) ObserveReturn(player, claim);
    else
    {
        claim->departing = false;
        if (!claim->returnWarned) TC_LOG_WARN("scripts", "Monk Fel Stone: original1514 return refused; retaining acknowledged scene for retry");
        claim->returnWarned = true;
    }
}
}

class spell_monk_fel_stone : public SpellScript
{
    PrepareSpellScript(spell_monk_fel_stone);
    std::shared_ptr<MonkFelStone::Claim> claim;
    bool Load() override { return GetCaster() && GetCaster()->GetMapId() == 1014; }
    bool Validate(SpellInfo const* info) override
    {
        auto scene = sSpellMgr->GetSpellScene(MonkFelStone::Scene);
        auto event = sSpellMgr->GetSpellInfo(MonkFelStone::Event);
        auto marker = sSpellMgr->GetSpellInfo(MonkFelStone::Pilgrimage);
        return info && info->Id == MonkFelStone::Parent && info->Effects[EFFECT_0] && info->Effects[EFFECT_1] && info->Effects[EFFECT_2] && info->Effects[EFFECT_3] &&
            info->EffectMask == 15 && info->GetDuration() == 20000 &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_APPLY_AURA && info->Effects[EFFECT_0]->ApplyAuraName == SPELL_AURA_ACTIVATE_SCENE &&
            info->Effects[EFFECT_0]->MiscValue == MonkFelStone::Scene && info->Effects[EFFECT_1]->Effect == SPELL_EFFECT_KILL_CREDIT &&
            info->Effects[EFFECT_1]->MiscValue == 98353 && info->Effects[EFFECT_2]->Effect == SPELL_EFFECT_TRIGGER_SPELL &&
            info->Effects[EFFECT_2]->TriggerSpell == MonkFelStone::Event && info->Effects[EFFECT_3]->Effect == SPELL_EFFECT_TRIGGER_SPELL &&
            info->Effects[EFFECT_3]->TriggerSpell == MonkFelStone::Pilgrimage && scene && scene->SceneScriptPackageID == 1492 &&
            !scene->CustomDuration && scene->PlaybackFlags == 17 && !scene->scriptID &&
            info->Effects[EFFECT_0]->TargetA.GetTarget() == TARGET_UNIT_CASTER && info->Effects[EFFECT_1]->TargetA.GetTarget() == TARGET_UNIT_CASTER &&
            info->Effects[EFFECT_2]->TargetA.GetTarget() == TARGET_UNIT_CASTER && info->Effects[EFFECT_3]->TargetA.GetTarget() == TARGET_UNIT_CASTER &&
            !info->Effects[EFFECT_2]->MiscValue && !info->Effects[EFFECT_3]->MiscValue &&
            event && event->Effects[EFFECT_0] && event->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SEND_EVENT && event->Effects[EFFECT_0]->MiscValue == 46952 &&
            marker && marker->Effects[EFFECT_0] && marker->Effects[EFFECT_0]->Effect == SPELL_EFFECT_APPLY_AURA &&
            marker->Effects[EFFECT_0]->ApplyAuraName == SPELL_AURA_DUMMY && marker->Effects[EFFECT_0]->TargetA.GetTarget() == TARGET_UNIT_CASTER;
    }
    SpellCastResult Check()
    {
        auto player = GetCaster()->ToPlayer();
        if (claim) return MonkFelStone::Casting(player, claim) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        auto source = player && player->GetMap() ? player->GetMap()->GetCreature(GetOriginalTargetGUID()) : nullptr;
        auto actor = MonkFelStone::Contact(player, source);
        auto quest = player ? player->getQuestStatus(MonkFelStone::QuestId) : nullptr;
        if (!actor || !quest) return SPELL_FAILED_BAD_TARGETS;
        if (MonkFelStone::Find(player) || player->HasAura(MonkFelStone::Parent)) return SPELL_FAILED_NOT_READY;
        if (!quest->ScriptLifetime) quest->ScriptLifetime = std::make_shared<uint8>();
        claim = std::make_shared<MonkFelStone::Claim>();
        claim->player = player->GetGUID(); claim->source = source->GetGUID(); claim->origin = player->GetMap(); claim->instance = player->GetInstanceId();
        claim->parent = GetSpell()->m_castGuid[1].GetSubType() == SPELL_CAST_TYPE_NORMAL ? GetSpell()->m_castGuid[1] : GetSpell()->m_castGuid[0];
        if (!MonkFelStone::ParentGuid(claim->parent)) { claim.reset(); return SPELL_FAILED_BAD_TARGETS; }
        claim->quest = quest->ScriptLifetime; claim->actor = actor->lifetime; claim->startedAt = getMSTime();
        claim->lastStep = MonkFelStone::World(player)->GetCurrentStep(); claim->hadMarker = player->HasAura(MonkFelStone::Pilgrimage);
        claim->markerReceipt = claim->hadMarker;
        if (!MonkFelStone::Reserve(claim)) { claim.reset(); return SPELL_FAILED_NOT_READY; }
        return SPELL_CAST_OK;
    }
    void Launch(SpellEffIndex effect)
    {
        auto player = GetCaster()->ToPlayer();
        if (!MonkFelStone::Casting(player, claim))
        {
            PreventHitDefaultEffect(effect);
            if (claim) MonkFelStone::Stop(player, claim);
            return;
        }
        if ((effect == EFFECT_2 && MonkFelStone::Leaf(MonkFelStone::World(player), 29072)) ||
            (effect == EFFECT_3 && claim->hadMarker)) PreventHitDefaultEffect(effect);
    }
    void Before()
    {
        auto player = GetCaster()->ToPlayer();
        if (!MonkFelStone::Casting(player, claim))
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i) PreventHitEffect(SpellEffIndex(i));
            if (claim) MonkFelStone::Stop(player, claim);
        }
    }
    void AuraHit(SpellEffIndex effect)
    {
        auto player = GetCaster()->ToPlayer();
        if (GetHitUnit() != player || !MonkFelStone::Casting(player, claim) || !GetHitAura()) { PreventHitDefaultEffect(effect); return; }
        for (auto script : GetHitAura()->m_loadedScripts)
            if (auto owned = dynamic_cast<aura_monk_fel_stone_scene*>(script)) { owned->claim = claim; return; }
        PreventHitDefaultEffect(effect);
    }
    void Credit(SpellEffIndex effect)
    {
        auto player = GetCaster()->ToPlayer();
        if (GetHitUnit() != player || !MonkFelStone::Current(player, claim) || !claim->sceneApplied ||
            player->GetQuestObjectiveData(MonkFelStone::QuestId, 98353) > 0) PreventHitDefaultEffect(effect);
    }
    void After()
    {
        auto player = GetCaster()->ToPlayer();
        if (GetHitUnit() != player || !MonkFelStone::Current(player, claim)) return;
        claim->hit = true;
        if (!claim->sceneApplied)
        {
            TC_LOG_WARN("scripts", "Monk Fel Stone: native193890 produced no authenticated1111 scene; no return granted");
            MonkFelStone::Stop(player, claim);
        }
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_monk_fel_stone::Check);
        OnEffectLaunch += SpellEffectFn(spell_monk_fel_stone::Launch, EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_monk_fel_stone::Launch, EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL);
        BeforeHit += SpellHitFn(spell_monk_fel_stone::Before);
        OnEffectHitTarget += SpellEffectFn(spell_monk_fel_stone::AuraHit, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        OnEffectHitTarget += SpellEffectFn(spell_monk_fel_stone::Credit, EFFECT_1, SPELL_EFFECT_KILL_CREDIT);
        AfterHit += SpellHitFn(spell_monk_fel_stone::After);
    }
public:
    ~spell_monk_fel_stone() override { if (claim && !claim->sceneApplied) MonkFelStone::Release(claim); }
};

class spell_monk_fel_stone_event : public SpellScript
{
    PrepareSpellScript(spell_monk_fel_stone_event);
    bool Load() override { return GetCaster() && GetCaster()->IsPlayer() && MonkFelStone::ParentGuid(GetSpell()->m_castGuid[1]); }
    SpellCastResult Check()
    {
        auto player = GetCaster()->ToPlayer();
        return MonkFelStone::Child(player, GetSpell()->m_castGuid[1], MonkFelStone::Find(player)) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Event(SpellEffIndex effect)
    {
        auto player = GetCaster()->ToPlayer();
        if (!MonkFelStone::Child(player, GetSpell()->m_castGuid[1], MonkFelStone::Find(player)) ||
            MonkFelStone::Leaf(MonkFelStone::World(player), 29072)) PreventHitDefaultEffect(effect);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_monk_fel_stone_event::Check);
        OnEffectHit += SpellEffectFn(spell_monk_fel_stone_event::Event, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
    }
};

class spell_monk_fel_stone_pilgrimage : public SpellScript
{
    PrepareSpellScript(spell_monk_fel_stone_pilgrimage);
    bool Load() override { return GetCaster() && GetCaster()->IsPlayer() && MonkFelStone::ParentGuid(GetSpell()->m_castGuid[1]); }
    SpellCastResult Check()
    {
        auto player = GetCaster()->ToPlayer();
        auto claim = MonkFelStone::Find(player);
        if (!MonkFelStone::Child(player, GetSpell()->m_castGuid[1], claim)) return SPELL_FAILED_BAD_TARGETS;
        if (Aura* existing = player->GetAura(MonkFelStone::Pilgrimage))
            if (!MonkFelStone::Owns(existing, claim))
            {
                // Another native source (including194181) may have applied
                // this aura while the original10ms trigger was queued.
                claim->hadMarker = true; claim->markerReceipt = true;
                return SPELL_FAILED_DONT_REPORT;
            }
        return SPELL_CAST_OK;
    }
    void Apply(SpellEffIndex effect)
    {
        auto player = GetCaster()->ToPlayer(); auto claim = MonkFelStone::Find(player);
        if (GetHitUnit() != player || !MonkFelStone::Child(player, GetSpell()->m_castGuid[1], claim) || claim->hadMarker || !GetHitAura())
        { PreventHitDefaultEffect(effect); return; }
        for (auto script : GetHitAura()->m_loadedScripts)
            if (auto marker = dynamic_cast<aura_monk_fel_stone_pilgrimage*>(script)) { marker->claim = claim; marker->authorizedApply = true; return; }
        PreventHitDefaultEffect(effect);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_monk_fel_stone_pilgrimage::Check);
        OnEffectHitTarget += SpellEffectFn(spell_monk_fel_stone_pilgrimage::Apply, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
    }
};

class player_monk_fel_stone : public PlayerScript
{
public:
    player_monk_fel_stone() : PlayerScript("player_monk_fel_stone") { }
    void OnUpdate(Player* player, uint32) override
    {
        if (auto claim = MonkFelStone::Find(player)) MonkFelStone::Tick(player, claim);
    }
    void OnBeforeLogout(Player* player) override
    {
        if (auto claim = MonkFelStone::Find(player)) MonkFelStone::Stop(player, claim);
    }
    void OnMapChanged(Player* player) override
    {
        if (auto claim = MonkFelStone::Find(player))
        {
            if (claim->departing) MonkFelStone::ObserveReturn(player, claim);
            else MonkFelStone::Stop(player, claim);
        }
    }
};

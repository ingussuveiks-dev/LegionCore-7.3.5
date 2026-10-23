#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
using uint32 = uint32_t;
enum SceneEventStatus { SCENE_NONE, SCENE_LAUNCH, SCENE_TRIGER, SCENE_COMPLETE };
enum { SPELL_AURA_ACTIVATE_SCENE = 430, PHASE_UPDATE_FLAG_ZONE_UPDATE = 1 };
#define TC_LOG_DEBUG(...) ((void)0)
struct Position { int GetPosition() const { return 0; } };
struct SpellInfo {};
struct SpellScene { uint32 PlaybackFlags = 21, SceneScriptPackageID = 1583; };
struct SceneTriggerEvent { std::string Event; uint32 trigerSpell = 0, MonsterCredit = 0, MiscValue = 1220; };
struct Message { uint32 instance; bool cancel; };
namespace WorldPackets { namespace Scene {
struct SceneInstance { uint32 SceneInstanceID; };
struct SceneTriggerEvent { uint32 SceneInstanceID; std::string Event; };
struct PlayScene {
    uint32 SceneID = 0, PlaybackFlags = 0, SceneInstanceID = 0, SceneScriptPackageID = 0;
    int Pos = 0;
    Message message{};
    Message* Write() { message = {SceneInstanceID, false}; return &message; }
};
struct CancelScene {
    Message message;
    explicit CancelScene(uint32 instance) : message{instance, true} {}
    Message* Write() { return &message; }
};
} }
struct Player;
struct Unit { virtual ~Unit() = default; virtual Player* ToPlayer() { return nullptr; } };
struct AuraEffect {
    uint32 scene = 1220, spell = 205589;
    uint32 GetMiscValue() const { return scene; }
    uint32 GetId() const { return spell; }
};
using AuraEffectList = std::list<AuraEffect*>;
struct PhaseManager { int updates = 0; void RemoveUpdateFlag(int flag) { assert(flag == 1); ++updates; } };
struct SpellManager {
    SpellScene scene;
    std::map<uint32, std::vector<SceneTriggerEvent>> events;
    SpellScene const* GetSpellScene(uint32) { return &scene; }
    std::vector<SceneTriggerEvent> const* GetSceneTriggerEvent(uint32 id) {
        auto found = events.find(id); return found == events.end() ? nullptr : &found->second;
    }
} spellManager;
auto* sSpellMgr = &spellManager;
struct ScriptManager {
    std::function<bool(Player*, std::string)> callback;
    bool OnSceneTrigger(Player* player, SpellScene const*, std::string event) {
        return callback ? callback(player, event) : false;
    }
} scriptManager;
auto* sScriptMgr = &scriptManager;
struct Player : Unit {
    std::unordered_map<uint32, uint32> m_sceneInstanceID;
    std::map<uint32, SceneEventStatus> m_sceneStatus;
    uint32 sceneInstanceID = 0;
    AuraEffectList effects;
    std::vector<Message> messages;
    std::vector<uint32> casts, credits, removals;
    std::set<uint32> auras;
    PhaseManager phase;
    std::function<void(uint32)> removed;
    std::function<void(uint32)> cast;
    std::function<void()> criteriaUpdate;
    std::vector<std::function<void()>> delayedInvokerCasts;
    bool world = true, alive = true, teleporting = false, rewarded = false, owned = true;
    uint32 playerClass = 3, map = 1511, runtimeInstance = 42, quest = 3;
    uint32 expectedQuest = 40385, expectedItem = 128808, expectedCriterion = 47689, castResult = 0;
    Player* ToPlayer() override { return this; }
    uint32 GetMap(){return map;}
    bool InSamePhase(Player*){return true;}bool IsWithinDistInMap(Player*,float){return true;}
    bool HasQuestObjectScope(){return false;}bool CanTargetQuestObject(Player*){return true;}
    void SendSpellScene(uint32, SpellInfo const*, bool, Position*);
    void SceneCompleted(uint32);
    bool HasSceneStatus(uint32, SceneEventStatus const&) const;
    void TrigerScene(uint32, std::string const);
    void SendDirectMessage(Message const* packet) { messages.push_back(*packet); }
    void AddDelayedEvent(uint32 delay, std::function<void()> callback) {
        if (delay == 100) callback();
        else { assert(delay == 200); delayedInvokerCasts.push_back(callback); }
    }
    void FlushInvokerCasts() { auto pending = std::move(delayedInvokerCasts); delayedInvokerCasts.clear(); for (auto& callback : pending) callback(); }
    PhaseManager& GetPhaseMgr() { return phase; }
    AuraEffectList const& GetAuraEffectsByType(int type) { assert(type == 430); return effects; }
    void RemoveAurasDueToSpell(uint32 spell) {
        removals.push_back(spell);
        std::set<uint32> scenes;
        for (auto effect = effects.begin(); effect != effects.end();) {
            if ((*effect)->spell != spell) { ++effect; continue; }
            scenes.insert((*effect)->scene);
            effect = effects.erase(effect);
        }
        for (uint32 scene : scenes) SendSpellScene(scene, nullptr, false, nullptr);
        auras.erase(spell);
        if (removed) removed(spell);
    }
    bool HasAura(uint32 spell) { return auras.count(spell) != 0; }
    uint32 CastSpell(Unit* target, uint32 spell, bool triggered) {
        assert(target == this && (triggered || spell == 205589 || spell == 200279)); casts.push_back(spell);
        if (castResult) return castResult;
        auras.insert(spell);
        if (cast) cast(spell);
        return 0;
    }
    void UpdateAchievementCriteria(uint32 type, uint32 asset, uint32 second, uint32 third, Unit* invoker, bool ignoreGroup) {
        assert(type == 92 && asset == expectedCriterion && second == 0 && third == 0 && invoker == this && ignoreGroup);
        if (criteriaUpdate) criteriaUpdate();
    }
    void KilledMonsterCredit(uint32 entry) { credits.push_back(entry); }
    uint32 Start(AuraEffect& effect) {
        effects.push_back(&effect); Position pos; SendSpellScene(effect.scene, nullptr, true, &pos);
        return sceneInstanceID;
    }
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 getClass() const { return playerClass; }
    uint32 GetMapId() const { return map; }
    uint32 GetInstanceId() const { return runtimeInstance; }
    bool GetQuestRewardStatus(uint32 id) const { assert(id == expectedQuest); return rewarded; }
    uint32 GetQuestStatus(uint32 id) const { assert(id == expectedQuest); return quest; }
    bool HasItemCount(uint32 item, uint32 count, bool bank) const {
        assert(item == expectedItem && count == 1 && bank); return owned;
    }
    uint32 GetGUIDLow() const { return 123; }
};
#include "Lifecycle.inc"
struct WorldSession {
    Player* _player = nullptr;
    std::vector<Message> packets;
    void SendPacket(Message const* packet) { packets.push_back(*packet); }
    void HandleScenePlaybackCanceled(WorldPackets::Scene::SceneInstance&);
    void HandleScenePlaybackComplete(WorldPackets::Scene::SceneInstance&);
    void HandleSceneTriggerEvent(WorldPackets::Scene::SceneTriggerEvent&);
};
#include "Handlers.inc"

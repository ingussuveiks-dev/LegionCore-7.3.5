#include "SceneLifecycleFixture.h"
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    spellManager.events[1220] = {{"complete", 197651, 100112, 1220}};
    {
        Player player; AuraEffect effect;
        uint32 instance = player.Start(effect);
        player.SceneCompleted(instance);
        player.SceneCompleted(instance);
        if (player.casts.size() != 1 || !player.m_sceneInstanceID.empty()) {
            std::cerr << "FAIL: completed scene must close its instance and dispatch credit exactly once\n";
            return 1;
        }
        assert(player.credits.size() == 1 && player.effects.empty() && player.phase.updates == 1);
        assert(player.HasSceneStatus(1220, SCENE_COMPLETE));
        player.TrigerScene(instance, "complete"); assert(player.casts.size() == 1);
    }
    {
        Player player; AuraEffect effect;
        uint32 obsolete = player.Start(effect);
        player.RemoveAurasDueToSpell(205589);
        uint32 current = player.Start(effect);
        player.SceneCompleted(obsolete);
        assert(player.casts.empty() && player.effects.size() == 1);
        player.SceneCompleted(current); assert(player.casts.size() == 1);
    }
    {
        Player player; AuraEffect first, other{999, 700};
        uint32 old = player.Start(first); uint32 current = player.Start(first);
        uint32 separate = player.Start(other);
        int callbacks = 0;
        scriptManager.callback = [&](Player*, std::string) { ++callbacks; return false; };
        player.TrigerScene(old, "complete");
        assert(callbacks == 0);
        scriptManager.callback = {};
        player.SceneCompleted(old);
        assert(player.casts.empty() && player.phase.updates == 0 && player.effects.size() == 3);
        player.SceneCompleted(current);
        assert(player.casts.size() == 1 && player.m_sceneInstanceID.count(separate) == 1);
        assert(player.effects.size() == 1 && player.effects.front() == &other);
        player.SceneCompleted(99999); assert(player.phase.updates == 1);
    }
    {
        Player player; AuraEffect effect;
        uint32 instance = player.Start(effect); int callbacks = 0;
        scriptManager.callback = [&](Player* target, std::string event) {
            assert(event == "complete"); ++callbacks;
            target->SceneCompleted(instance);
            return false;
        };
        player.SceneCompleted(instance);
        assert(callbacks == 1 && player.effects.empty() && player.m_sceneInstanceID.empty());
        assert(player.casts.size() == 1 && player.credits.size() == 1);
        scriptManager.callback = {};
    }
    {
        Player player; AuraEffect effect;
        uint32 obsolete = player.Start(effect); uint32 replacement = 0;
        scriptManager.callback = [&](Player* target, std::string) {
            target->RemoveAurasDueToSpell(205589);
            replacement = target->Start(effect);
            return false;
        };
        player.SceneCompleted(obsolete);
        assert(player.casts.empty() && player.effects.size() == 1);
        assert(player.m_sceneInstanceID.size() == 1 && player.m_sceneInstanceID.count(replacement));
        assert(player.m_sceneStatus[1220] == SCENE_LAUNCH && player.phase.updates == 0);
        scriptManager.callback = {};
        player.SceneCompleted(replacement); assert(player.casts.size() == 1);
    }
    {
        Player player; AuraEffect effect{1142, 777}; int callbacks = 0;
        player.Start(effect); player.Start(effect);
        scriptManager.callback = [&](Player*, std::string) { ++callbacks; return true; };
        player.RemoveAurasDueToSpell(777);
        assert(callbacks == 1 && player.m_sceneInstanceID.empty() && player.effects.empty());
        assert(player.HasSceneStatus(1142, SCENE_COMPLETE));
        scriptManager.callback = {};
    }
    std::cout << "PASS: extracted scene launch/trigger/completion/removal, one-shot credit, stale/repeated callbacks, same-scene replacement, unrelated scenes, reentrant completion and Keystone cleanup\n";
}

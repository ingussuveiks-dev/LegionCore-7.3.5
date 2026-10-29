#include "SceneLifecycleFixture.h"
#include <fstream>

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    spellManager.scene = {21,1631,1282};
    // Counterfactual: actual old scene has no event mapping and never requests
    // the native equip spell, even when its real completion packet arrives.
    {
        Player player; AuraEffect aura{1282,214714};
        player.SceneCompleted(player.Start(aura));
        assert(player.casts.empty() && player.effects.empty());
    }
    std::ifstream input(argv[1]);
    uint32 scene = 0, spell = 0, credit = 0; std::string event;
    assert(input >> scene >> event >> spell >> credit);
    assert(scene == 1282 && event == "complete" && spell == 214715 && credit == 0);
    spellManager.events[scene] = {{event,spell,credit,scene}};
    for (bool canceled : {false,true})
    {
        Player player; AuraEffect aura{1282,214714};
        uint32 id = player.Start(aura);
        WorldSession session; session._player = &player;
        WorldPackets::Scene::SceneInstance packet{id};
        if (canceled) session.HandleScenePlaybackCanceled(packet);
        else session.HandleScenePlaybackComplete(packet);
        session.HandleScenePlaybackComplete(packet);
        player.TrigerScene(id,"complete");
        assert(player.casts == std::vector<uint32>{214715});
        assert(player.credits.empty() && player.effects.empty() && player.m_sceneInstanceID.empty());
        assert(player.HasSceneStatus(1282,SCENE_COMPLETE));
    }
    {
        Player player; AuraEffect aura{1282,214714}, other{1235,207560};
        uint32 old = player.Start(aura), current = player.Start(aura), separate = player.Start(other);
        player.SceneCompleted(old); player.SceneCompleted(999999);
        assert(player.casts.empty());
        player.SceneCompleted(current);
        assert(player.casts == std::vector<uint32>{214715});
        assert(player.m_sceneInstanceID.count(separate) == 1 && player.effects.size() == 1);
        player.SceneCompleted(separate);
        assert(player.casts.size() == 1 && player.credits.empty());
    }
    {
        Player player; AuraEffect aura{1282,214714};
        uint32 id = player.Start(aura);
        scriptManager.callback = [&](Player* owner,std::string name) {
            assert(name == "complete"); owner->SceneCompleted(id); return false;
        };
        player.SceneCompleted(id);
        assert(player.casts == std::vector<uint32>{214715} && player.effects.empty());
        scriptManager.callback = {};
    }
    std::cout << "PASS: pinned missing callback counterfactual versus real Feral complete/cancel native dispatch, exactly-once request, stale/replaced/unknown instances, reentry and unrelated scenes; no direct item/quest credit\n";
}

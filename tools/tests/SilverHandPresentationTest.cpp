#define MARKSMANSHIP_PRESENTATION_TEST
#define SILVER_HAND_PRESENTATION_TEST
#include "SceneLifecycleFixture.h"
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, SPELL_CAST_OK=0 };
using SpellCastResult=uint32;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Scenario
{
    uint32 id=1092,step=10;
    uint32 GetScenarioId() const {return id;}
    uint32 GetCurrentStep() const {return step;}
};
struct ScenarioManager
{
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instance){return present && instance==42?&scenario:nullptr;}
} scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct Spell
{
    uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;
    Unit* m_caster;
    SpellInfo* m_spellInfo;
    void EffectForceEquipItem(SpellEffIndex);
};
#include "ForceEquip.inc"
struct SceneTriggerScript
{
    explicit SceneTriggerScript(std::string name){assert(name=="scene_silver_hand_pickup");}
    virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;}
};
#include "Presentation.inc"
struct Pickup
{
    Player player;
    AuraEffect effect{1267,211838};
    SpellInfo info;
    Spell equip{SPELL_EFFECT_HANDLE_HIT_TARGET,0,&player,&info};
    scene_silver_hand_pickup script;
    uint32 instance;
    Pickup()
    {
        scenarioManager={};warnings=0;spellManager.scene={21,1618,1267};
        player.playerClass=2;player.map=1539;player.expectedQuest=42120;player.expectedItem=128823;
        player.forceEquip=[this]{equip.EffectForceEquipItem(0);};
        player.cast=[this](uint32 spell){assert(spell==211842 && player.mainhand==&player.artifact);};
        scriptManager.callback=[this](Player* target,std::string trigger){return script.OnTrigger(target,&spellManager.scene,trigger);};
        instance=player.Start(effect);
    }
    ~Pickup(){scriptManager.callback={};}
    void Finish(bool cancel=false)
    {
        WorldSession session{&player};WorldPackets::Scene::SceneInstance packet{instance};
        if(cancel)session.HandleScenePlaybackCanceled(packet);else session.HandleScenePlaybackComplete(packet);
    }
};
int main()
{
    _set_error_mode(_OUT_TO_STDERR);
#ifdef BASELINE
    Pickup pickup;pickup.Finish();
    assert(pickup.player.casts.empty() && !pickup.player.swaps && pickup.player.mainhand==&pickup.player.previousWeapon);
    std::cout<<"PASS pinned a07343f7: unbound1267 completion has no native211842 equip/progression bridge, while the scene clone can still brandish128823. Inventory/renderer services controlled.\n";
#else
    for(uint32 step : {10u,11u})for(uint32 status : {1u,3u})for(bool cancel : {false,true})
    {
        Pickup pickup;scenarioManager.scenario.step=step;pickup.player.quest=status;
        pickup.player.TrigerScene(pickup.instance,"Smooth Phase Complete");pickup.player.TrigerScene(pickup.instance,"complete");
        assert(!pickup.player.swaps && pickup.player.casts.empty());pickup.Finish(cancel);pickup.Finish(cancel);
        assert(pickup.player.swaps==1 && pickup.player.mainhand==&pickup.player.artifact && pickup.player.casts==std::vector<uint32>{211842});
        assert(pickup.player.previousWeapon.position==0xFF20 && pickup.player.effects.empty() && pickup.player.m_sceneInstanceID.empty() && !warnings);
    }
    for(uint32 invalid=0;invalid<18;++invalid)
    {
        Pickup pickup;
        switch(invalid)
        {
            case 0:scenarioManager.scenario.step=9;break;
            case 1:scenarioManager.scenario.step=12;break;
            case 2:scenarioManager.scenario.id=1037;break;
            case 3:scenarioManager.scenario.id=991;break;
            case 4:scenarioManager.present=false;break;
            case 5:pickup.player.runtimeInstance=43;break;
            case 6:pickup.player.map=1220;break;
            case 7:pickup.player.playerClass=1;break;
            case 8:pickup.player.rewarded=true;break;
            case 9:pickup.player.quest=0;break;
            case 10:pickup.player.quest=5;break;
            case 11:pickup.player.alive=false;break;
            case 12:pickup.player.world=false;break;
            case 13:pickup.player.contact=false;break;
            case 14:pickup.player.teleporting=true;break;
            case 15:pickup.player.owned=false;break;
            case 16:pickup.player.bankOnly=true;break;
            case 17:pickup.player.RemoveAurasDueToSpell(211838);break;
        }
        pickup.Finish();assert(!pickup.player.swaps && pickup.player.casts.empty());
    }
    for(uint32 failure=0;failure<3;++failure)
    {
        Pickup pickup;
        if(failure==0)pickup.player.castResult=1;
        if(failure==1)pickup.player.equipResult=EQUIP_ERR_BAG_FULL;
        if(failure==2)pickup.player.swapBlocked=true;
        pickup.Finish();
        assert(pickup.player.casts==(failure==0?std::vector<uint32>{211842}:std::vector<uint32>{}));
        assert(warnings==uint32(failure!=1) && pickup.player.equipErrors==uint32(failure==1));
        pickup.player.castResult=0;pickup.player.equipResult=EQUIP_ERR_OK;pickup.player.swapBlocked=false;
        pickup.instance=pickup.player.Start(pickup.effect);pickup.Finish();
        assert(pickup.player.mainhand==&pickup.player.artifact && pickup.player.casts.back()==211842);
    }
    {
        Pickup pickup;uint32 stale=pickup.instance;pickup.instance=pickup.player.Start(pickup.effect);
        pickup.player.SceneCompleted(stale);assert(pickup.player.casts.empty());pickup.Finish();
        pickup.player.TrigerScene(stale,"complete");assert(pickup.player.casts.size()==1);
    }
    {
        Pickup pickup;pickup.player.mainhand=&pickup.player.artifact;pickup.player.artifact.position=0xFF0F;
        pickup.Finish();assert(!pickup.player.swaps && pickup.player.casts==std::vector<uint32>{211842});
        SpellScene unrelated{21,1619,1268};assert(!pickup.script.OnTrigger(&pickup.player,&unrelated,"complete"));
        assert(!pickup.script.OnTrigger(&pickup.player,nullptr,"complete"));
        unrelated={21,1619,1267};assert(!pickup.script.OnTrigger(&pickup.player,&unrelated,"complete"));
        assert(pickup.script.OnTrigger(nullptr,&spellManager.scene,"complete"));
    }
    std::cout<<"PASS: actual packet/scene lifecycle authorizes1267/1618 completion/cancel once, equips an existing mainhand128823 before native211842, rejects18 invalid contexts and preserves3 failure/retry paths. No item creation; native211842 progression/ability effects and actual client rendering remain separate.\n";
#endif
}

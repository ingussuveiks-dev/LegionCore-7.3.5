#define MARKSMANSHIP_PRESENTATION_TEST
#define SILVER_HAND_PRESENTATION_TEST
#include "SceneLifecycleFixture.h"

enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, SPELL_CAST_OK=0 };
using SpellCastResult=uint32;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct SpellTargetPosition { uint32 target_mapId=1539; };
struct DestinationService
{
    SpellTargetPosition destination;
    bool present=true;
    SpellTargetPosition const* GetSpellTargetPosition(uint32 spell) const {assert(spell==211867);return present?&destination:nullptr;}
} destinationService;
struct Scenario
{
    uint32 id=1092;
    bool completed=true;
    uint32 GetScenarioId() const {return id;}
    bool IsCompleted(bool bonus) const {assert(!bonus);return completed;}
};
struct ScenarioManager
{
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instance) {return present && instance==42?&scenario:nullptr;}
} scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct SceneTriggerScript
{
    explicit SceneTriggerScript(std::string name) {assert(name=="scene_silver_hand_collapse");}
    virtual bool OnTrigger(Player*,SpellScene const*,std::string) {return false;}
};
#undef sSpellMgr
#define sSpellMgr (&destinationService)
#include "Collapse.inc"
#undef sSpellMgr

struct Collapse
{
    Player player;
    AuraEffect effect{1268,211862};
    scene_silver_hand_collapse script;
    uint32 instance;
    Collapse()
    {
        scenarioManager={};destinationService={};warnings=0;spellManager.scene={27,1619,1268};
        player.playerClass=2;player.map=1539;player.expectedQuest=42120;player.expectedItem=128823;
        player.mainhand=&player.artifact;player.artifact.position=0xFF0F;
        player.m_sceneStatus[1267]=SCENE_COMPLETE;player.auras.insert(211862);
        player.cast=[](uint32 spell){assert(spell==211867);};
        scriptManager.callback=[this](Player* target,std::string trigger){return script.OnTrigger(target,&spellManager.scene,trigger);};
        instance=player.Start(effect);
    }
    ~Collapse() {scriptManager.callback={};}
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
    Collapse collapse;collapse.Finish();
    assert(collapse.player.casts.empty() && collapse.player.effects.empty());
    std::cout<<"PASS pinned6f8615cf: issued1268 completion removes the scene aura but has no211867 exit bridge; no native destination/50974 dispatched.\n";
#else
    for(uint32 quest:{1u,3u})for(bool cancel:{false,true})for(bool reentrantRemoval:{false,true})
    {
        Collapse collapse;collapse.player.quest=quest;
        if(reentrantRemoval)collapse.player.cast=[&collapse](uint32 spell){assert(spell==211867);collapse.player.RemoveAurasDueToSpell(211862);};
        WorldSession session{&collapse.player};WorldPackets::Scene::SceneInstance unknown{collapse.instance+99};
        session.HandleScenePlaybackComplete(unknown);
        collapse.player.TrigerScene(collapse.instance,"complete");
        collapse.player.TrigerScene(collapse.instance,"Smooth Phase Complete");
        assert(collapse.player.casts.empty() && collapse.player.HasAura(211862));
        collapse.Finish(cancel);collapse.Finish(cancel);
        collapse.player.TrigerScene(collapse.instance,"complete");
        assert(collapse.player.casts==std::vector<uint32>{211867} && collapse.player.credits.empty() && !warnings);
        assert(collapse.player.effects.empty() && collapse.player.m_sceneInstanceID.empty() && !collapse.player.HasAura(211862));
    }
    for(uint32 invalid=0;invalid<20;++invalid)
    {
        Collapse collapse;
        switch(invalid)
        {
            case 0:collapse.player.m_sceneStatus[1267]=SCENE_LAUNCH;break;
            case 1:collapse.player.auras.erase(211862);break;
            case 2:collapse.player.world=false;break;
            case 3:collapse.player.contact=false;break;
            case 4:collapse.player.alive=false;break;
            case 5:collapse.player.teleporting=true;break;
            case 6:collapse.player.playerClass=1;break;
            case 7:collapse.player.map=0;break;
            case 8:collapse.player.map=1220;break;
            case 9:collapse.player.rewarded=true;break;
            case 10:collapse.player.quest=0;break;
            case 11:collapse.player.quest=5;break;
            case 12:scenarioManager.present=false;break;
            case 13:scenarioManager.scenario.id=991;break;
            case 14:scenarioManager.scenario.id=1037;break;
            case 15:scenarioManager.scenario.completed=false;break;
            case 16:collapse.player.owned=false;break;
            case 17:collapse.player.bankOnly=true;break;
            case 18:collapse.player.mainhand=&collapse.player.previousWeapon;break;
            case 19:collapse.player.runtimeInstance=43;break;
        }
        collapse.Finish();assert(collapse.player.casts.empty() && collapse.player.credits.empty());
    }
    for(uint32 failure=0;failure<3;++failure)
    {
        Collapse collapse;
        if(failure==0)destinationService.present=false;
        if(failure==1)destinationService.destination.target_mapId=1220;
        if(failure==2)collapse.player.castResult=1;
        collapse.Finish();assert(warnings==1 && collapse.player.credits.empty());
        assert(collapse.player.casts==(failure==2?std::vector<uint32>{211867}:std::vector<uint32>{}));
        destinationService={};collapse.player.castResult=0;collapse.player.auras.insert(211862);
        collapse.instance=collapse.player.Start(collapse.effect);collapse.Finish();
        assert(collapse.player.casts.back()==211867 && collapse.player.credits.empty());
    }
    {
        Collapse collapse;uint32 old=collapse.instance;collapse.instance=collapse.player.Start(collapse.effect);
        collapse.player.SceneCompleted(old);assert(collapse.player.casts.empty());collapse.Finish();
        assert(collapse.player.casts==std::vector<uint32>{211867});
        SpellScene unrelated{21,1618,1267};assert(!collapse.script.OnTrigger(&collapse.player,&unrelated,"complete"));
        unrelated={27,1,1268};assert(!collapse.script.OnTrigger(&collapse.player,&unrelated,"complete"));
        assert(!collapse.script.OnTrigger(&collapse.player,nullptr,"complete"));
        assert(collapse.script.OnTrigger(nullptr,&spellManager.scene,"complete"));
    }
    std::cout<<"PASS: actual latest-instance scene packet/trigger/cancel/removal lifecycle authorizes one211867 attempt only after1268/1619 and prior1267 completion, active aura/live Holy context/completed1092/equipped existing weapon. Twenty invalid contexts and destination/cast failure plus fresh-issued replay checked. No direct item/credit/teleport or Dalaran105892; real native211867 effects and client arrival separate.\n";
#endif
}

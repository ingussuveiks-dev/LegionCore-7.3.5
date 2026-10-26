#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
#include "Constants.inc"
enum {CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct AuraEffect {};
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct InstanceScript
{
    uint32 step=3;
    std::vector<uint32> credit;
    uint32 getScenarionStep(){return step;}
    void DoUpdateAchievementCriteria(uint32 type,uint32 value){assert(type==92);credit.push_back(value);}
};
struct ActorAI {uint32 actions=0;void DoAction(bool value){assert(value);++actions;}};
struct Creature
{
    uint32 despawns=0;
    ActorAI ai;
    void DespawnOrUnsummon(){++despawns;}
    ActorAI* AI(){return &ai;}
};
struct Unit
{
    InstanceScript* instance=nullptr;
    Creature* trigger=nullptr;
    Creature* boss=nullptr;
    uint32 casts=0;
    SpellCastResult result=SPELL_CAST_OK;
    InstanceScript* GetInstanceScript(){return instance;}
    SpellCastResult CastSpell(Unit* target,uint32 spell,bool triggered){assert(target==this&&spell==217042&&triggered);++casts;return result;}
    uint32 GetGUIDLow(){return 123;}
    Creature* FindNearestCreature(uint32 entry,float distance){assert(distance==30.f);assert(entry==NPC_DOMINATE_MIND||entry==NPC_BALNAZZAR);return entry==NPC_DOMINATE_MIND?trigger:boss;}
};
struct Aura
{
    Unit* caster=nullptr;
    Unit* target=nullptr;
    bool application=false;
    Unit* GetCaster(){return caster;}
    Unit* GetTarget(){assert(application);return target;}
#include "Aura.inc"
};
int main()
{
    InstanceScript instance;Unit owner;owner.instance=&instance;Aura aura;aura.caster=&owner;
#ifdef BASELINE
    aura.OnUpdate(4000,nullptr);owner.result=SPELL_FAILED_ERROR;aura.OnUpdate(8000,nullptr);
    assert(aura.timer==0&&owner.casts==1&&instance.credit.size()==3&&!warnings);
    owner.result=SPELL_CAST_OK;aura.OnUpdate(65000,nullptr);assert(owner.casts==1);
    std::cout<<"PASS pinned4717a90d: reported217042 rejection permanently consumes timer; restored cast admission has no remaining attempt. Controlled result, not a live client failure\n";
#else
    for(uint32 stage=0;stage<8;++stage)for(SpellCastResult failure:{SPELL_FAILED_ERROR,SPELL_FAILED_DONT_REPORT,SPELL_FAILED_NOT_READY})
    {
        InstanceScript current;current.step=stage;Unit caster;caster.instance=&current;Aura effect;effect.caster=&caster;warnings=0;
        effect.OnUpdate(3999,nullptr);assert(effect.timer==1&&!effect.unbrekable&&current.credit.empty()&&!caster.casts);
        effect.OnUpdate(1,nullptr);assert(effect.timer==8000&&effect.unbrekable&&!caster.casts);
        std::vector<uint32> expected=stage==3?std::vector<uint32>{46130,46506,52186}:std::vector<uint32>{};
        assert(current.credit==expected);effect.OnUpdate(7999,nullptr);assert(effect.timer==1&&!caster.casts);
        caster.result=failure;effect.OnUpdate(1,nullptr);assert(effect.timer==1&&caster.casts==1&&warnings==1&&current.credit==expected);
        effect.OnUpdate(0,nullptr);assert(caster.casts==1&&effect.timer==1);
        effect.OnUpdate(50,nullptr);assert(caster.casts==2&&warnings==1&&effect.timer==1&&current.credit==expected);
        effect.caster=nullptr;effect.OnUpdate(65000,nullptr);assert(caster.casts==2&&effect.timer==1);
        effect.caster=&caster;caster.result=SPELL_CAST_OK;effect.OnUpdate(1,nullptr);
        assert(effect.timer==0&&caster.casts==3&&warnings==1&&current.credit==expected);
        effect.OnUpdate(65000,nullptr);assert(caster.casts==3&&warnings==1&&current.credit==expected);
    }
    Aura waiting;waiting.OnUpdate(65000,nullptr);assert(waiting.timer==4000&&!waiting.unbrekable);
    Unit noInstance;waiting.caster=&noInstance;waiting.OnUpdate(65000,nullptr);assert(waiting.timer==8000&&waiting.unbrekable);
    waiting.OnUpdate(65000,nullptr);assert(waiting.timer==0&&noInstance.casts==1);
    for(bool present:{false,true})
    {
        Unit target;Creature trigger,boss;target.trigger=present?&trigger:nullptr;target.boss=present?&boss:nullptr;Aura hooks;hooks.target=&target;hooks.application=true;
        hooks.OnApply(nullptr,0);hooks.OnRemove(nullptr,0);assert(trigger.despawns==uint32(present)&&boss.ai.actions==uint32(present));
        hooks.target=nullptr;hooks.OnApply(nullptr,0);hooks.OnRemove(nullptr,0);
    }
    for(uint32 attempt=0;attempt<2;++attempt)
    {
        auto pending=std::make_unique<Aura>();pending->caster=&owner;owner.result=SPELL_FAILED_ERROR;warnings=0;
        pending->OnUpdate(4000,nullptr);pending->OnUpdate(8000,nullptr);assert(pending->timer==1&&warnings==1);pending.reset();
        Aura fresh;assert(fresh.timer==4000&&!fresh.unbrekable&&!fresh.willFailureReported);
    }
    std::cout<<"PASS24 stage/result cases retain4000+8000ms sequence, original stage3 credits and self217042; non-OK retains aura-owned next-positive-update retry with one added diagnostic, repeated failures/missing caster then accepted result stop correctly. Missing actors/targets, oversized/zero updates, destruction/new aura covered. No GetTarget during update. Controlled services; OK is preparation admission, not actual override735/button/removal/loot/equip/client/VFX or full abandon/group/faction proof\n";
#endif
}

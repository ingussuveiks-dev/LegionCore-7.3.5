#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <crtdbg.h>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
using AuraType=int;using AuraEffectHandleModes=int;
constexpr int SPELL_AURA_OVERRIDE_SPELLS=293,TOTAL_AURAS=400;
constexpr uint8 AURA_EFFECT_HANDLE_REAL=1,AURA_EFFECT_HANDLE_SEND_FOR_CLIENT=2,AURA_EFFECT_HANDLE_CHANGE_AMOUNT=4,AURA_EFFECT_HANDLE_STAT=8,AURA_EFFECT_HANDLE_SKILL=16,AURA_EFFECT_HANDLE_REAPPLY=32,AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK=7;
constexpr int PLAYER_FIELD_BYTES_5=5,PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET=0;
constexpr int PLAYERSPELL_TEMPORARY=1,PLAYERSPELL_UNCHANGED=0;
#define ASSERT(value) assert(value)
struct RecursiveGuard{explicit RecursiveGuard(int&) {}};
struct Aura;struct AuraEffect;struct AuraApplication;struct Player;
struct Unit {
    using AuraEffectList=std::list<AuraEffect*>;
    AuraEffectList m_modAuras[TOTAL_AURAS];int i_auraEff_lock=0;bool inWorld=true;
    virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
    bool IsInWorld(){return inWorld;}
    AuraEffectList const& GetAuraEffectsByType(AuraType type){return m_modAuras[type];}
    void _RegisterAuraEffect(AuraEffect*,bool);
};
struct PlayerSpell{int state;PlayerSpell(int s,bool,bool,bool):state(s){}};
using PlayerSpellMap=std::map<uint32,PlayerSpell>;
struct Player:Unit {
    uint16 overrideId=0;std::map<uint32,PlayerSpell> m_spells;
    Player* ToPlayer() override{return this;}
    void SetUInt16Value(int field,int offset,uint32 value){assert(field==5&&offset==0);overrideId=uint16(value);}
    void HandleExcludeCasterSpellList(uint32,bool){}void HandleCasterAuraStateSpellList(uint32,bool){}
    void AddTemporarySpell(uint32);void RemoveTemporarySpell(uint32);
    bool Has(uint32 id){return m_spells.contains(id);}
};
struct OverrideSpellDataEntry{std::array<uint32,4> Spells{};};
struct OverrideStore{
    std::map<uint32,OverrideSpellDataEntry> rows{{900,{{11,198424,0,0}}},{901,{{21,0,0,0}}},{641,{{198424,0,0,0}}},{902,{{31,0,0,0}}}};
    OverrideSpellDataEntry const* LookupEntry(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}
}sOverrideSpellDataStore;
struct AuraApplication{
    Unit* target;int removeMode=0;Unit* GetTarget()const{return target;}int GetRemoveMode()const{return removeMode;}
};
struct Aura {
    uint32 id;bool removed=false;int removeCalls=0,afterApply=0;AuraApplication app;
    std::vector<AuraEffect*> effects;std::function<void()> afterRemove;
    Aura(uint32 spell,Unit* target):id(spell),app{target}{}
    bool IsRemoved()const{return removed;}
    void Remove();
    bool CallScriptEffectApplyHandlers(AuraEffect const*,AuraApplication const*,AuraEffectHandleModes){return false;}
    bool CallScriptEffectRemoveHandlers(AuraEffect const*,AuraApplication const*,AuraEffectHandleModes){return false;}
    void CallScriptAfterEffectApplyHandlers(AuraEffect const*,AuraApplication const*,AuraEffectHandleModes){++afterApply;}
    void CallScriptAfterEffectRemoveHandlers(AuraEffect const*,AuraApplication const*,AuraEffectHandleModes){if(afterRemove){auto callback=std::move(afterRemove);callback();}}
};
struct AuraEffect {
    Aura* base;uint32 recordId;bool registered=false;
    using Handler=void(AuraEffect::*)(AuraApplication const*,uint8,bool)const;
    static Handler AuraEffectHandler[TOTAL_AURAS];
    AuraEffect(Aura& aura,uint32 value):base(&aura),recordId(value){aura.effects.push_back(this);}
    Aura* GetBase()const{return base;}uint32 GetId()const{return base->id;}int GetMiscValue()const{return recordId;}
    AuraType GetAuraType()const{return SPELL_AURA_OVERRIDE_SPELLS;}
    void ApplySpellMod(Unit*,bool){}
    void HandleEffect(AuraApplication*,uint8,bool);
    void HandleAuraOverrideSpells(AuraApplication const*,uint8,bool)const;
    void Apply(){registered=true;HandleEffect(&base->app,AURA_EFFECT_HANDLE_REAL,true);}
};
AuraEffect::Handler AuraEffect::AuraEffectHandler[TOTAL_AURAS]{};
#include "RegisterEffect.inc"
#include "TemporarySpells.inc"
#include "EffectDispatcher.inc"
#include "OverrideHandler.inc"
void Aura::Remove(){
    if(removed)return;removed=true;++removeCalls;app.removeMode=1;
    // UnitAura removal marks the base removed before unapplying each effect;
    // the production dispatcher removes each effect from the player's live list.
    for(auto effect:effects)if(effect->registered){effect->registered=false;effect->HandleEffect(&app,AURA_EFFECT_HANDLE_REAL,false);}
}
void Check(bool value){assert(value);}
int main(){
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    AuraEffect::AuraEffectHandler[293]=&AuraEffect::HandleAuraOverrideSpells;
    std::cout<<"Replacing an existing override through the production dispatcher...\n"<<std::flush;
    {
        Player player;Aura old(100,&player),fresh(198425,&player);AuraEffect a(old,900),b(fresh,641);
        a.Apply();Check(player.overrideId==900&&player.Has(11)&&player.Has(198424));
        b.Apply();Check(old.IsRemoved()&&old.removeCalls==1&&player.overrideId==641&&!player.Has(11)&&player.Has(198424));
        fresh.Remove();Check(player.overrideId==0&&!player.Has(198424));
    }
    {
        Player player;Aura old(100,&player),next(200,&player),fresh(198425,&player);AuraEffect a(old,900),duplicate(old,900),c(next,901),b(fresh,641);
        // Seed two independent pre-existing overrides (e.g. loaded/reentrant state),
        // including two effects owned by the same base aura.
        a.registered=duplicate.registered=c.registered=true;
        player._RegisterAuraEffect(&a,true);player._RegisterAuraEffect(&duplicate,true);player._RegisterAuraEffect(&c,true);
        old.afterRemove=[&](){next.Remove();};
        b.Apply();Check(old.removeCalls==1&&next.removeCalls==1&&player.overrideId==641&&player.Has(198424));
        Check(player.GetAuraEffectsByType(293).size()==1);
    }
    {
        Player player;Aura same(198425,&player),fresh(198425,&player);AuraEffect a(same,641),b(fresh,641);
        a.Apply();b.Apply();Check(!same.IsRemoved()&&player.GetAuraEffectsByType(293).size()==2);
    }
    {
        Player player;Aura old(100,&player),fresh(198425,&player);AuraEffect a(old,900),b(fresh,641);
        a.Apply();old.afterRemove=[&](){fresh.Remove();};b.Apply();
        Check(fresh.IsRemoved()&&fresh.afterApply==0&&player.overrideId==0&&!player.Has(198424));
    }
    {
        Player player;Aura old(100,&player),fresh(198425,&player),replacement(300,&player);AuraEffect a(old,900),b(fresh,641),c(replacement,902);
        a.Apply();old.afterRemove=[&](){c.Apply();};b.Apply();
        Check(fresh.IsRemoved()&&player.overrideId==902&&player.Has(31)&&!player.Has(198424));
    }
    {
        Player player;player.m_spells.emplace(198424,PlayerSpell(PLAYERSPELL_UNCHANGED,true,false,false));
        Aura fresh(198425,&player);AuraEffect effect(fresh,641);effect.Apply();fresh.Remove();
        Check(player.Has(198424)&&player.m_spells.at(198424).state==PLAYERSPELL_UNCHANGED);
    }
    {
        Player player;Aura stale(100,&player),fresh(198425,&player);AuraEffect a(stale,900),b(fresh,641);
        stale.removed=true;player._RegisterAuraEffect(&a,true);b.Apply();Check(stale.removeCalls==0&&player.overrideId==641);
    }
    {
        Player player;Aura old(100,&player),fresh(198425,&player);AuraEffect a(old,900),b(fresh,641);
        a.Apply();old.afterRemove=[&](){player.inWorld=false;};b.Apply();Check(player.overrideId==0&&!player.Has(198424));
        Unit npc;Aura other(400,&npc);AuraEffect nonplayer(other,641);nonplayer.Apply();
        Player offline;offline.inWorld=false;Aura missing(500,&offline);AuraEffect off(missing,641);off.Apply();Check(!offline.Has(198424));
        Player modePlayer;Aura modeAura(600,&modePlayer);AuraEffect mode(modeAura,641);mode.HandleEffect(&modeAura.app,AURA_EFFECT_HANDLE_SEND_FOR_CLIENT,true);Check(!modePlayer.Has(198424));
    }
    std::cout<<"PASS: production override handler/dispatcher/live-list registration/temporary spells; replacement, same-ID, multi-effect and linked removals, reentrant cancellation/replacement, deferred removed entry, permanent spells and admission gates\n";
}

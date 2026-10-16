#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> models("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<GameObjectsEntry> objects("GameObjects.db2",GameobjectsLoadInfo::Instance());
    DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
    DB2Storage<AnimReplacementSetEntry> animations("AnimReplacementSet.db2",AnimReplacementSetLoadInfo::Instance());
    DB2Storage<AnimKitEntry> kits("AnimKit.db2",AnimKitLoadInfo::Instance());
    DB2Storage<SpellAuraRestrictionsEntry> auraRestrictions("SpellAuraRestrictions.db2",SpellAuraRestrictionsLoadInfo::Instance());
    DB2Storage<SpellTargetRestrictionsEntry> targetRestrictions("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!times.Load(argv[1],0)||!line.Load(argv[1],0)||!models.Load(argv[1],0)||!objects.Load(argv[1],0)||!conditions.Load(argv[1],0)||!animations.Load(argv[1],0)||!kits.Load(argv[1],0)||!auraRestrictions.Load(argv[1],0)||!targetRestrictions.Load(argv[1],0))return 2;
    for(uint32 id:{193563u,193576u,193577u,227837u}){
        auto s=spells.LookupEntry(id);if(!s)return 3;std::cout<<"spell"<<id<<" "<<s->Name->Str[0]<<'\n';uint32 count=0;
        for(auto e:effects)if(e->SpellID==id){++count;std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" trigger="<<e->EffectTriggerSpell<<" misc="<<e->EffectMiscValue[0]<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';if(e->ImplicitTarget[0]!=1||e->ImplicitTarget[1])return 4;}
        if(!count)return 5;
    }
    bool src=false,dst=false;SpellImplicitTargetInfo target(TARGET_UNIT_CASTER);if(target.GetExplicitTargetMask(src,dst))return 6;
    std::cout<<"native TARGET_UNIT_CASTER explicit mask=0; redundant target removed by InitExplicitTargets\n";
    for(auto r:auraRestrictions)if(r->SpellID==193576||r->SpellID==193577)return 15;
    for(auto r:targetRestrictions)if((r->SpellID==193576||r->SpellID==193577)&&r->Targets)return 16;
    std::cout<<"native skewering has no required fish aura or explicit-target restriction\n";
    bool foundTime=false;for(auto m:misc)if(m->SpellID==193576){auto t=times.LookupEntry(m->CastingTimeIndex);if(!t||t->Base!=1500)return 7;foundTime=true;std::cout<<"Skewering castTime="<<t->Base<<"ms\n";}
    if(!foundTime)return 8;
    for(auto q:line)if(q->QuestID==39855||q->QuestID==40078){if(q->QuestLineID!=135||q->OrderIndex!=(q->QuestID==39855?10:11))return 9;std::cout<<"quest"<<q->QuestID<<" line="<<q->QuestLineID<<" order="<<uint32(q->OrderIndex)<<'\n';}
    for(uint32 id:{29179u,29174u}){auto m=models.LookupEntry(id);if(!m||!m->FileDataID)return 10;std::cout<<"GO display"<<id<<" modelFile="<<m->FileDataID<<'\n';}
    if(objects.LookupEntry(244867))return 11;std::cout<<"no native GameObjects244867 spawn/template\n";
    for(uint32 id:{45684u,45685u}){auto c=conditions.LookupEntry(id);if(!c)return 12;std::cout<<"POI condition"<<id<<" aura=";for(auto a:c->AuraSpellID)std::cout<<a<<",";std::cout<<" logic="<<c->AuraSpellLogic<<" stacks=";for(auto a:c->AuraStacks)std::cout<<uint32(a)<<",";std::cout<<"\n";}
    auto a=animations.LookupEntry(175);if(!a)return 13;
    if(kits.LookupEntry(175))return 14;
    std::cout<<"native carry AnimReplacementSet175 exists=1\nPASS: native quest-line ordering, exact self spell chains/1500ms cast/explicit mask, barrel/spear model files and carry/POI references. Full client availability, cast scheduling, visuals and movement remain unverified.\n";
}

#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "SpellAuraDefines.h"
#include "MovementPackets.h"
#include "SpellInfo.h"
#include <iostream>
#include <stdexcept>
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
int Run(int argc,char** argv){if(argc!=2)return 1;
#define S(T) DB2Storage<T##Entry> T##s(#T ".db2",T##LoadInfo::Instance());Require(T##s.Load(argv[1],0),"Native scene DB2 load failed")
 S(SpellEffect);S(SpellMisc);S(SpellDuration);S(SpellRange);S(SpellXSpellVisual);S(AnimKit);S(AnimKitSegment);S(CreatureDisplayInfo);S(CreatureModelData);S(FactionTemplate);
#undef S
 for(auto id:{211409u,230506u}){
  unsigned effects=0;for(auto e:SpellEffects)if(e->SpellID==id&&!e->DifficultyID){++effects;Require(e->Effect==6&&e->EffectAura==4&&e->EffectIndex==0&&e->EffectTriggerSpell==0&&e->ImplicitTarget[0]==(id==211409?25:1)&&!e->ImplicitTarget[1],"Scene aura/target changed");}
  Require(effects==1,"Expected one cosmetic aura effect");
  unsigned misc=0;for(auto m:SpellMiscs)if(m->SpellID==id&&!m->DifficultyID){++misc;auto duration=SpellDurations.LookupEntry(m->DurationIndex);auto range=SpellRanges.LookupEntry(m->RangeIndex);Require(duration&&duration->Duration==-1&&range,"Native cosmetic duration/range changed");std::cout<<"spell "<<id<<" range "<<range->RangeMax[0]<<" duration "<<duration->Duration<<'\n';}
  Require(misc==1,"Native cosmetic misc missing");
 }
 SpellImplicitTargetInfo chainTarget(25);
 Require(chainTarget.GetCheckType()==TARGET_CHECK_DEFAULT&&chainTarget.GetObjectType()==TARGET_OBJECT_TYPE_UNIT,"Chain must not require an allied or enemy faction target");
 auto visual=[&](uint32 spell,uint32 expected){unsigned count=0;for(auto x:SpellXSpellVisuals)if(x->SpellID==spell){++count;Require(x->SpellVisualID==expected,"Native scene visual changed");}Require(count==1,"Expected a single native visual");};
 visual(211409,55619);visual(230506,62640);visual(230352,11919);
 Require(AnimKits.LookupEntry(12209),"Captured Drake native animation kit missing");
 unsigned segments=0;for(auto a:AnimKitSegments)if(a->ParentAnimKitID==12209){++segments;Require(a->AnimID==0&&a->ForcedVariation==2&&(a->AnimKitConfigID==346||a->AnimKitConfigID==348),"Captured Drake pose sequence changed");}Require(segments==2,"Captured pose segment count changed");
 for(auto id:{67198u,67199u}){auto display=CreatureDisplayInfos.LookupEntry(id);auto model=display?CreatureModelDatas.LookupEntry(display->ModelID):nullptr;Require(model&&model->FileDataID==1280971,"Storm dragon model changed");}
 Require(FactionTemplates.LookupEntry(190)&&FactionTemplates.LookupEntry(2817),"Native scene factions missing");
 std::cout<<"PASS: typed native cosmetic effects/duration/ANY-unit target, visual IDs, Captured Drake12209 stand variation2 and stormdragon model. Client-only visual event/procedural links use the separate WDC1 audit; existence is not rendering or a recovered spawn/spline.\n";return 0;
}
int main(int argc,char**argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

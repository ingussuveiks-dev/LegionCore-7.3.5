// Read-only, build-26972 hunter item/talent audit using worldserver's DB2 reader.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <regex>
#include <string>
std::string Clean(char const* text) { std::string s=text?text:""; for(auto& c:s) if(c=='\t'||c=='\r'||c=='\n') c=' '; return s; }
int main(int argc, char** argv)
{
 if(argc!=2) return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> itemEffects("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellAuraOptionsEntry> options("SpellAuraOptions.db2",SpellAuraOptionsLoadInfo::Instance());
 DB2Storage<SpellClassOptionsEntry> families("SpellClassOptions.db2",SpellClassOptionsLoadInfo::Instance());
 DB2Storage<TalentEntry> talents("Talent.db2",TalentLoadInfo::Instance());
 DB2Storage<SpecializationSpellsEntry> specSpells("SpecializationSpells.db2",SpecializationSpellsLoadInfo::Instance());
 DB2Storage<ChrSpecializationEntry> specs("ChrSpecialization.db2",ChrSpecializationLoadInfo::Instance());
 if(!items.Load(argv[1],0)||!itemEffects.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!options.Load(argv[1],0)||!families.Load(argv[1],0)||!talents.Load(argv[1],0)||!specSpells.Load(argv[1],0)||!specs.Load(argv[1],0)) return 2;
 std::set<uint32> itemIDs{140806,141482,147015,154174,128861}, wanted{34026,193455,217200,120679,19574,2643,193530,131894,194397,199528,194306,199532,115939,118455,207068,166615,256832};
 for(auto i:items) if(i->OverallQualityID==5&&i->ExpansionID==6&&i->InventoryType&&(i->LimitCategory==357||i->LimitCategory==362)&&(i->AllowableClass&4)) {itemIDs.insert(i->ID); std::cout<<"ITEM\t"<<i->ID<<'\t'<<Clean(i->Display->Str[0])<<'\t'<<unsigned(i->InventoryType)<<'\t'<<int(i->RequiredLevel)<<'\n';}
 for(auto e:itemEffects) if(itemIDs.count(e->ItemID)) { wanted.insert(e->SpellID); std::cout<<"ITEM_EFFECT\t"<<e->ItemID<<'\t'<<e->SpellID<<'\t'<<unsigned(e->TriggerType)<<'\t'<<e->ChrSpecializationID<<'\n'; }
 for(auto s:specs) if(s->ClassID==0) {std::cout<<"PET_SPEC\t"<<s->ID<<'\t'<<Clean(s->Name->Str[0])<<'\t'<<unsigned(s->OrderIndex)<<'\n'; for(auto e:specSpells) if(e->SpecID==s->ID) {wanted.insert(e->SpellID);std::cout<<"PET_SPEC_SPELL\t"<<s->ID<<'\t'<<e->SpellID<<'\n';}}
 for(auto t:talents) if(t->ClassID==3&&(t->SpecID==253||!t->SpecID)) { wanted.insert(t->SpellID); std::cout<<"TALENT\t"<<t->ID<<'\t'<<unsigned(t->TierID)<<'\t'<<unsigned(t->ColumnIndex)<<'\t'<<t->SpellID<<'\t'<<t->OverridesSpellID<<'\n'; }
 std::regex reference("\\$(?:@spelldesc)?([0-9]{5,6})");
 bool changed=true; while(changed) { changed=false; for(auto e:effects) if(wanted.count(e->SpellID)&&e->EffectTriggerSpell&&wanted.insert(e->EffectTriggerSpell).second) changed=true; for(auto id:std::set<uint32>(wanted)) if(auto s=spells.LookupEntry(id)) { auto desc=Clean(s->Description->Str[0]); for(std::sregex_iterator i(desc.begin(),desc.end(),reference),end;i!=end;++i) if(wanted.insert(std::stoul((*i)[1])).second) changed=true; } }
 for(auto id:wanted) {auto s=spells.LookupEntry(id);std::cout<<"SPELL\t"<<id<<'\t'<<(s?Clean(s->Name->Str[0]):"MISSING")<<'\t'<<(s?Clean(s->Description->Str[0]):"")<<'\n';}
 // EFFECT: spell,difficulty,index,effect,aura,misc0,misc1,base,trigger,targets0/1,period,classmask[4].
 for(auto e:effects) if(wanted.count(e->SpellID)) {std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->DifficultyID)<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod;for(int i=0;i<4;++i)std::cout<<'\t'<<e->EffectSpellClassMask[i];std::cout<<'\n';}
 for(auto m:misc) if(wanted.count(m->SpellID)) {auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"DURATION\t"<<m->SpellID<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\n';}
 for(auto o:options) if(wanted.count(o->SpellID)) std::cout<<"PROC\t"<<o->SpellID<<'\t'<<unsigned(o->ProcChance)<<'\t'<<o->ProcTypeMask<<'\t'<<o->ProcCategoryRecovery<<'\t'<<o->CumulativeAura<<'\t'<<o->SpellProcsPerMinuteID<<'\n';
 for(auto f:families) if(wanted.count(f->SpellID)) {std::cout<<"FAMILY\t"<<f->SpellID<<'\t'<<unsigned(f->SpellClassSet);for(int i=0;i<4;++i)std::cout<<'\t'<<f->SpellClassMask[i];std::cout<<'\n';}
 // Fail on missing direct item effects or changed native mechanics used by the
 // accompanying recommendation. Transitive, off-spec references remain visible
 // research output; their existence is not a claim that they are implemented.
 unsigned failures=0;
 auto check=[&](bool value,char const* name){std::cout<<"CHECK\t"<<(value?"PASS":"FAIL")<<'\t'<<name<<'\n';if(!value)++failures;};
 auto effect=[&](uint32 spell,uint32 index)->SpellEffectEntry const* {for(auto e:effects)if(e->SpellID==spell&&e->EffectIndex==index&&!e->DifficultyID)return e;return nullptr;};
 auto duration=[&](uint32 spell)->int32 {for(auto m:misc)if(m->SpellID==spell&&!m->DifficultyID)if(auto d=durations.LookupEntry(m->DurationIndex))return d->Duration;return 0;};
 bool direct=true;for(auto e:itemEffects)if(itemIDs.count(e->ItemID)&&!spells.LookupEntry(e->SpellID))direct=false;
 check(direct,"all selected items resolve their direct spell IDs");
 auto talent=talents.LookupEntry(22347);auto soul=effect(247530,0);bool soulSpec=false;
 for(auto e:itemEffects)if(e->ItemID==151641&&e->SpellID==247530&&e->ChrSpecializationID==253)soulSpec=true;
 check(soulSpec&&talent&&talent->SpellID==194306&&soul&&soul->EffectAura==224&&soul->EffectMiscValue[0]==22347&&soul->EffectMiscValue[1]==151641,"Soul grants BM Bestial Fury from native talent 22347");
 auto roar=effect(207318,0),qapla=effect(212278,0),mantle=effect(247993,0),parsel=effect(248085,0),leech=effect(248085,2),apex=effect(212329,0),aspects=effect(206332,0);
 check(roar&&roar->EffectAura==108&&roar->EffectBasePoints==-15,"Roar has 15 percent cost reduction");
 check(qapla&&qapla->EffectBasePoints==-3000,"Qa'pla has three second native cooldown amount");
 check(mantle&&mantle->EffectAura==429&&mantle->EffectBasePoints==5&&duration(247993)==8000,"Mantle is 5 percent pet damage for eight seconds");
 bool fourStacks=false;for(auto o:options)if(o->SpellID==248085&&o->CumulativeAura==4)fourStacks=true;
 check(parsel&&parsel->EffectBasePoints==1&&leech&&leech->EffectBasePoints==2&&fourStacks&&duration(248085)==8000,"Parsel stacks four times with one percent damage and two percent leech");
 bool passiveUnion=true;for(auto spec:{535,536,537})for(auto spell:{53184,53482,61688,53497,19596}) {bool found=false;for(auto e:specSpells)if(e->SpecID==spec&&e->SpellID==spell)found=true;passiveUnion&=found;}
 check(apex&&apex->EffectAura==451&&passiveUnion,"Apex adaptation specs each include all three passive sets");
 bool allAspects=aspects&&aspects->EffectBasePoints==-35;for(auto spell:{193530,186265,186257}){bool found=false;for(auto f:families)if(f->SpellID==spell)for(int i=0;i<4;++i)if(aspects&&(aspects->EffectSpellClassMask[i]&f->SpellClassMask[i]))found=true;allAspects&=found;}
 check(allAspects,"Call of the Wild covers Wild Turtle and Cheetah cooldowns");
 return failures?3:0;
}

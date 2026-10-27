#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
 DB2Storage<FactionEntry> raw("Faction.db2",FactionLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!factions.Load(argv[1],0))return 4;
 if(!raw.Load(argv[1],0))return 5;
 spells.LoadFromDB();effects.LoadFromDB();factions.LoadFromDB();raw.LoadFromDB();
 for(uint32 id:{225648,182254,225644,182289,225207,230503}){
  if(auto s=spells.LookupEntry(id))std::cout<<"SPELL "<<id<<' '<<s->Name->Str[0]<<" DESCRIPTION "<<s->Description->Str[0]<<'\n';
  for(auto e:effects)if(e->SpellID==id)std::cout<<"EFFECT "<<id<<' '<<e->EffectIndex<<" type "<<e->Effect<<" aura "<<e->EffectAura<<" misc "<<e->EffectMiscValue[0]<<','<<e->EffectMiscValue[1]<<" trigger "<<e->EffectTriggerSpell<<" targets "<<e->ImplicitTarget[0]<<','<<e->ImplicitTarget[1]<<'\n';
 }
 for(uint32 id:{2780,2898,14,16,2})if(auto f=factions.LookupEntry(id)){
  std::cout<<"FACTION "<<id<<" faction "<<f->Faction<<" flags "<<f->Flags<<" group "<<unsigned(f->FactionGroup)<<" friend "<<unsigned(f->FriendGroup)<<" enemy "<<unsigned(f->EnemyGroup)<<'\n';
  if(auto r=raw.LookupEntry(f->Faction))std::cout<<"REPUTATION "<<f->Faction<<" index "<<r->ReputationIndex<<" canHave "<<r->CanHaveReputation()<<'\n';
 }
 for(auto f:factions)if(f->Faction==1786)
  std::cout<<"SAME_FACTION "<<f->ID<<" flags "<<f->Flags<<" group "<<unsigned(f->FactionGroup)<<" friend "<<unsigned(f->FriendGroup)<<" enemy "<<unsigned(f->EnemyGroup)<<'\n';
 auto old=factions.LookupEntry(2780),pve=factions.LookupEntry(2898);
 if(!old||!pve||old->Faction!=pve->Faction||!(old->Flags&0x800)||(pve->Flags&0x800))return 6;
 for(unsigned i=0;i<4;++i)std::cout<<"RELATION_SLOT "<<i<<" old "<<old->Enemies[i]<<','<<old->Friend[i]<<" pve "<<pve->Enemies[i]<<','<<pve->Friend[i]<<'\n';
 unsigned count=0;
 for(auto f:factions){
  bool different=old->IsHostileTo(*f)!=pve->IsHostileTo(*f)||old->IsFriendlyTo(*f)!=pve->IsFriendlyTo(*f)||f->IsHostileTo(*old)!=f->IsHostileTo(*pve)||f->IsFriendlyTo(*old)!=f->IsFriendlyTo(*pve);
  if(different){
   std::cout<<"DIFFERENT_RELATION "<<f->ID<<" faction "<<f->Faction<<'\n';
   if(f->Faction!=148)return 8;
  }
  ++count;
 }
 for(uint32 id:{1074,35,189,877,2784,2875,2780,2,1}){
  auto f=factions.LookupEntry(id);if(!f)return 9;
  if(old->IsHostileTo(*f)!=pve->IsHostileTo(*f)||old->IsFriendlyTo(*f)!=pve->IsFriendlyTo(*f)||f->IsHostileTo(*old)!=f->IsHostileTo(*pve)||f->IsFriendlyTo(*old)!=f->IsFriendlyTo(*pve))return 10;
 }
 std::cout<<"PASS_NATIVE_PVE_COUNTERPART 2780 2898 faction1786: players and all native camp relationships retained; "<<count<<" templates reviewed, only faction148 exception differs\n";
}

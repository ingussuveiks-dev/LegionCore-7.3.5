#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
 DB2Storage<ScenarioEntry> scenarios("Scenario.db2",ScenarioLoadInfo::Instance());DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 if(!scenarios.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!lfg.Load(argv[1],0)||!effects.Load(argv[1],0)||!spells.Load(argv[1],0))return 5;
 scenarios.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();lfg.LoadFromDB();effects.LoadFromDB();spells.LoadFromDB();
 std::set<uint32> selected;
 for(auto s:scenarios)if(std::string(s->Name->Str[0]).find("Nexus")!=std::string::npos||std::string(s->Name->Str[0]).find("Wrath")!=std::string::npos){selected.insert(s->ID);std::cout<<"SCENARIO\t"<<s->ID<<'\t'<<s->Name->Str[0]<<'\n';}
 for(auto d:lfg)if(selected.count(d->ScenarioID))std::cout<<"LFG\t"<<d->ID<<'\t'<<d->ScenarioID<<'\t'<<d->MapID<<'\t'<<unsigned(d->DifficultyID)<<'\n';
 for(auto s:steps)if(selected.count(s->ScenarioID)){
  std::cout<<"STEP\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->ID<<'\t'<<s->Criteriatreeid<<'\t'<<s->Title->Str[0]<<'\t'<<s->Description->Str[0]<<'\n';
  std::set<uint32> todo{uint32(s->Criteriatreeid)},seen;while(!todo.empty()){auto id=*todo.begin();todo.erase(id);if(!seen.insert(id).second)continue;if(auto t=trees.LookupEntry(id)){if(auto c=criteria.LookupEntry(t->CriteriaID))std::cout<<"CRITERION\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<t->ID<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';for(auto child:trees)if(child->Parent==id)todo.insert(child->ID);}}
 }
 std::set<uint32> chosen{3365,211759,211760,225015,225212,225020,225023,225025,225026,225027,225028,207897,207901,223908,207949,207956,207996,208011,209863,209869,209874,211954,218172,225879};for(auto e:effects)if(e->EffectMiscValue[0]==248572||e->EffectMiscValue[0]==249814||e->EffectMiscValue[0]==248574||e->EffectMiscValue[0]==248553){chosen.insert(e->SpellID);std::cout<<"GO_SOURCE\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';}
 for(auto s:spells)if((s->ID>=209860&&s->ID<=209880)||(s->ID>=225010&&s->ID<=225040)||std::string(s->Name->Str[0])=="Force of Gravity"||std::string(s->Name->Str[0]).find("Aegwynn")!=std::string::npos||std::string(s->Name->Str[0]).find("Meitre")!=std::string::npos)chosen.insert(s->ID);
 for(unsigned depth=0;depth<3;++depth)for(auto e:effects)if(chosen.count(e->SpellID)&&e->EffectTriggerSpell)chosen.insert(e->EffectTriggerSpell);
 for(auto e:effects)if(e->Effect==61&&e->EffectMiscValue[0]>=54145&&e->EffectMiscValue[0]<=54147){chosen.insert(e->SpellID);std::cout<<"CONTROL_SOURCE\t"<<e->SpellID<<'\t'<<e->EffectMiscValue[0]<<'\n';}
 for(auto e:effects)if((e->EffectTriggerSpell==207949||e->EffectTriggerSpell==207996)||e->EffectMiscValue[0]==104985)
  std::cout<<"SHARED_USE\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\n';
 for(auto e:effects)if(e->EffectMiscValue[0]==104502||e->EffectMiscValue[0]==106708){chosen.insert(e->SpellID);std::cout<<"BILAAL_SOURCE\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<unsigned(e->DifficultyID)<<'\n';}
 for(auto id:chosen){if(auto s=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)std::cout<<"EFFECT\t"<<id<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';}
 for(auto e:effects)if(e->SpellID==209863||e->SpellID==209864||e->SpellID==214455)std::cout<<"PERIOD\t"<<e->SpellID<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->EffectAuraPeriod<<'\t'<<e->EffectTriggerSpell<<'\n';
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());DB2Storage<SpellDurationEntry> duration("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<AreaTriggerEntry> areas("AreaTrigger.db2",AreaTriggerLoadInfo::Instance());
 if(!areas.Load(argv[1],0))return 7;areas.LoadFromDB();for(auto a:areas)if((a->ID>=6667&&a->ID<=6669)||a->ContinentID==1583)std::cout<<"AREA\t"<<a->ID<<'\t'<<a->ContinentID<<'\t'<<a->Pos.X<<'\t'<<a->Pos.Y<<'\t'<<a->Pos.Z<<'\t'<<a->Radius<<'\t'<<a->ShapeID<<'\t'<<unsigned(a->ShapeType)<<'\t'<<a->AreaTriggerActionSetID<<'\n';
 if(!misc.Load(argv[1],0)||!duration.Load(argv[1],0))return 6;misc.LoadFromDB();duration.LoadFromDB();
 for(auto m:misc)if(!m->DifficultyID&&chosen.count(m->SpellID)){
  std::cout<<"MISC\t"<<m->SpellID<<'\t'<<m->DurationIndex<<'\t'<<m->CastingTimeIndex<<'\t'<<m->Speed<<'\t'<<m->LaunchDelay<<'\n';
  if(auto d=duration.LookupEntry(m->DurationIndex))std::cout<<"DURATION\t"<<m->SpellID<<'\t'<<d->Duration<<'\t'<<d->MaxDuration<<'\n';
 }
 DB2Storage<OverrideSpellDataEntry> overrides("OverrideSpellData.db2",OverrideSpellDataLoadInfo::Instance());if(!overrides.Load(argv[1],0))return 9;overrides.LoadFromDB();for(unsigned id:{779u,780u,781u})if(auto o=overrides.LookupEntry(id)){std::cout<<"OVERRIDE\t"<<id<<'\t'<<o->PlayerActionBarFileDataID<<'\t'<<unsigned(o->Flags);for(auto spell:o->Spells)std::cout<<'\t'<<spell;std::cout<<'\n';}
 DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());if(!locks.Load(argv[1],0))return 8;locks.LoadFromDB();if(auto l=locks.LookupEntry(43))for(unsigned i=0;i<8;++i)std::cout<<"LOCK\t43\t"<<i<<'\t'<<unsigned(l->Type[i])<<'\t'<<l->Index[i]<<'\t'<<l->Skill[i]<<'\t'<<unsigned(l->Action[i])<<'\n';
 DB2Storage<SpellCastingRequirementsEntry> requirements("SpellCastingRequirements.db2",SpellCastingRequirementsLoadInfo::Instance());DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 if(!requirements.Load(argv[1],0)||!times.Load(argv[1],0))return 10;requirements.LoadFromDB();times.LoadFromDB();
 for(auto r:requirements)if(chosen.count(r->SpellID))std::cout<<"REQUIREMENTS\t"<<r->SpellID<<'\t'<<r->RequiredAreasID<<'\t'<<r->RequiresSpellFocus<<'\t'<<unsigned(r->FacingCasterFlags)<<'\n';
 for(auto m:misc)if(!m->DifficultyID&&chosen.count(m->SpellID))if(auto t=times.LookupEntry(m->CastingTimeIndex))std::cout<<"CAST_TIME\t"<<m->SpellID<<'\t'<<t->Base<<'\t'<<t->Minimum<<'\t'<<t->PerLevel<<'\n';
 DB2Storage<GameObjectsEntry> objects("GameObjects.db2",GameobjectsLoadInfo::Instance());DB2Storage<AreaGroupMemberEntry> members("AreaGroupMember.db2",AreaGroupMemberLoadInfo::Instance());
 if(!objects.Load(argv[1],0)||!members.Load(argv[1],0))return 11;objects.LoadFromDB();members.LoadFromDB();
 for(auto g:objects)if(g->OwnerID==1583||std::string(g->Name->Str[0]).find("Aegwynn")!=std::string::npos||std::string(g->Name->Str[0]).find("Meitre")!=std::string::npos){std::cout<<"CLIENT_GO\t"<<g->ID<<'\t'<<g->Name->Str[0]<<'\t'<<g->OwnerID<<'\t'<<unsigned(g->TypeID)<<'\t'<<g->DisplayID<<'\t'<<g->Position.X<<'\t'<<g->Position.Y<<'\t'<<g->Position.Z<<'\t'<<g->Scale<<'\t'<<g->PhaseID;for(auto p:g->PropValue)std::cout<<'\t'<<p;std::cout<<'\n';}
 for(auto a:members)if(a->AreaGroupID==4915)std::cout<<"AREA_GROUP\t4915\t"<<a->AreaID<<'\n';
 HotfixDatabase.Close();MySQL::Library_End();
}

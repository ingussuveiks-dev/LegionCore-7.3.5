// Real core owner/charmer resolution,Unit queues/AI destruction and health/heal
// backend. Projections are explicit;this does not traverse full encounters.
#include "LifetimeCallbackNative.h"
#include "InstanceScript.h"
#include "HealingAmount.h"
#include "FiveTen20261111Native.inc"
struct FiveTenInstanceTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(FiveTenInstanceTag);};template struct FixtureAccess<FiveTenInstanceTag,&InstanceMap::i_data>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 for(auto const&binding:ExpectedBindings){auto result=WorldDatabase.PQuery("SELECT ScriptName FROM creature_template WHERE entry=%u",binding.first);Require(result&&result->Fetch()[0].GetString()==binding.second,"Changed live creature binding");}
 for(auto const&binding:ExpectedDataBindings){std::string kind=binding.kind;if(kind=="gameobject"){auto result=WorldDatabase.PQuery("SELECT ScriptName FROM gameobject_template WHERE entry=%u",binding.entry);Require(result&&result->Fetch()[0].GetString()==binding.script,"Changed native gameobject binding");}else if(kind=="spell"){auto result=WorldDatabase.PQuery("SELECT 1 FROM spell_script_names WHERE spell_id=%u AND ScriptName='%s'",binding.entry,binding.script);Require(bool(result),"Changed native spell binding");}else{Require(kind=="instance","Unknown data binding table");auto result=WorldDatabase.PQuery("SELECT script FROM instance_template WHERE map=%u",binding.entry);Require(result&&result->Fetch()[0].GetString()==binding.script,"Changed native instance binding");}}
 for(uint32 id:{160702u,160703u,225106u,211995u,211959u,211999u,211956u,211958u,211994u,212008u,99990u})Require(sSpellMgr->GetSpellInfo(id),"Matching26972 spell missing");
 // Record native target types rather than assuming a modern client contract.
 for(uint32 id:{160702u,160703u,225106u}){auto info=sSpellMgr->GetSpellInfo(id);for(uint32 i=0;i<MAX_SPELL_EFFECTS;++i)if(info->Effects[i])std::cout<<"CATALOG "<<id<<" effect="<<i<<" type="<<info->Effects[i]->Effect<<" target="<<info->Effects[i]->TargetA.GetTarget()<<'/'<<info->Effects[i]->TargetB.GetTarget()<<'\n';}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999790,DIFFICULTY_N_SCENARIO,&parent);map.*Access(FiveTenInstanceTag{})=new InstanceScript(&map);
 Actor actor(&map,299,25101,{52,803,805,0}),pet(&map,299,25102,{53,803,805,0}),guardian(&map,299,25103,{54,803,805,0}),charmed(&map,299,25104,{55,803,805,0}),unowned(&map,299,25105,{56,803,805,0});for(auto u:{&actor,&pet,&guardian,&charmed,&unowned})Require(map.AddToMap(static_cast<Creature*>(u)),"Native actor admission");
 auto session=Session();{
 NativePlayer player(session,map,25100,{50,803,805,0});pet.SetGuidValue(UNIT_FIELD_CREATED_BY,player.GetGUID());guardian.SetGuidValue(UNIT_FIELD_CREATED_BY,pet.GetGUID());charmed.SetCharmerGUID(player.GetGUID());
 for(Unit*killer:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&unowned),static_cast<Unit*>(nullptr)}){
  auto resolved=killer?killer->GetAffectingPlayer():nullptr;Require(resolved==(killer&&killer!=&unowned?&player:nullptr),"Native owner/guardian/charmer/null eligibility");Require(actor.GetInstanceScript()==map.GetInstanceScript(),"Real victim-instance identity survives foreign/null killer");
 }
 charmed.SetCharmerGUID(ObjectGuid::Empty);
 RunLifetimeAdmission<MeatballNative>(actor,unowned,0,false,false);RunLifetimeAdmission<MeatballNative>(actor,unowned,1,false,false);
 // Actual actor-owned Unit queues disappear with the destroyed speaking actor.
 Observe::Reset();auto speaker=std::make_unique<Actor>(&map,299,25106,Position{57,803,805,0});Require(map.AddToMap(static_cast<Creature*>(speaker.get())),"Speaker insertion");speaker->AddDelayedEvent(3000,[raw=speaker.get()]{Observe::Mark(raw,0);});PumpQueue(*speaker,3000);Require(Observe::calls==1,"Live source speaker callback");Observe::Reset();speaker->AddDelayedEvent(3000,[raw=speaker.get()]{Observe::Mark(raw,1);});speaker.reset();PumpQueue(actor,6000);Require(!Observe::calls,"Destroyed speaker cannot leave a foreign queue payload");
 actor.SetMaxHealth(100000000000ull);actor.SetHealth(90000000000ull);RhyolithPhaseHealthNative rhyolith(&actor);MurmurResetHealthNative murmur(&actor);Require(rhyolith.Amount()==actor.GetHealth()&&murmur.Amount()==actor.CountPctFromMaxHealth(40),"Actual native64health getters survive source locals");
 player.SetMaxHealth(100000000000ull);player.SetHealth(1);auto info=sSpellMgr->GetSpellInfo(106112);Require(info,"Native Release Doubt spell missing");uint64 expected=HealingAmount::Apply(player.GetMaxHealth(),0,0.1f);NativeShaHeal(&actor,&player,info);Require(player.GetHealth()==1+expected&&expected>UINT32_MAX,"Actual source ten-percent heal reaches native64health backend once");
 }
 session->SetPlayer(nullptr);delete session;CloseAltheaNative();std::cout<<"PASS native5x10 scope:exact live bindings/native26972 data,real owner/guardian/charmer and victim instance,Unit queue lifetime and full-width health/healing. Payloads/spatial endpoints separately controlled;client and complete encounter traversal remain pending\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

// Whole production emperor classes and actual Unit::DealDamage/Kill dispatch.
// Pair GUIDs, bare players and (where declared) multiplier inputs controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ObjectVisitors.hpp"
#include "AreaTriggerData.h"
#include "LFGMgr.h"
#include "InstanceSaveMgr.h"
#include <boost/multiprecision/cpp_int.hpp>
#include "../../src/server/scripts/Kalimdor/TempleOfAhnQiraj/temple_of_ahnqiraj.h"
#include "TwinEncounterDataNative.h"
namespace TwinsUnderTest {
#include "TwinEmperors.inc"
}
using namespace TwinsUnderTest;
struct TwinDataTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(TwinDataTag);};
template struct FixtureAccess<TwinDataTag,&InstanceMap::i_data>;
struct TwinState:InstanceScript{
 using InstanceScript::InstanceScript;ObjectGuid first,second,killAtDamage;std::map<ObjectGuid,uint32> deaths;
 ObjectGuid GetGuidData(uint32 id) const override{return id==DATA_VEKNILASH?first:id==DATA_VEKLOR?second:ObjectGuid::Empty;}
 void CreatureDies(Creature* actor,Unit*)override{++deaths[actor->GetGUID()];}
 void OnCreatureDamageTaken(Creature* actor,Unit* attacker,uint32&)override{if(killAtDamage==actor->GetGUID()){killAtDamage.Clear();attacker->Kill(actor);}}
};
struct TwinActor:Actor{
 using Actor::Actor;float multiplier=1.f;
 float GetHealthMultiplierForTarget(WorldObject const*) const override{return multiplier;}
};
template<class Production>struct ObservedTwin:Production{
 using Production::Production;uint32 deaths=0,afterHooks=0;ObjectGuid killer;
 void JustDied(Unit* who) override{++deaths;killer=who?who->GetGUID():ObjectGuid::Empty;Production::JustDied(who);}
 void DamageTakenAfterScaling(Unit* who,uint32& damage,DamageEffectType kind)override{++afterHooks;Production::DamageTakenAfterScaling(who,damage,kind);}
};
using FirstAI=ObservedTwin<boss_veknilash::boss_veknilashAI>;
using SecondAI=ObservedTwin<boss_veklor::boss_veklorAI>;
struct TwinPlayer:NativePlayer{
 using NativePlayer::NativePlayer;
 ~TwinPlayer(){UnbindInstance(531,DIFFICULTY_40,true);GetMapRef().unlink();}
};
struct TwinSecondary:Guardian{
 TwinSecondary(Unit& owner,Map& map):Guardian(nullptr,&owner,false){
  auto p=owner.GetPosition();Require(Create(202,&map,1,299,0,ALLIANCE,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ(),0),"Secondary native guardian create");SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());setFaction(owner.getFaction());SetLevel(110);SetMaxHealth(1000000);SetFullHealth();Require(map.AddToMap(static_cast<Creature*>(this)),"Secondary guardian admission");
 }
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Secondary guardian AI");}
 ~TwinSecondary(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct TwinDisabledTag{using Type=UnitAI* Unit::*;friend Type Access(TwinDisabledTag);};
template struct FixtureAccess<TwinDisabledTag,&Unit::i_disabledAI>;
struct CountingAI:NullCreatureAI{using NullCreatureAI::NullCreatureAI;uint32 hooks=0;void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType)override{++hooks;}};
struct SuspendedScope{
 Creature& actor;UnitAI* original;CountingAI* temporary;bool ghost;
 SuspendedScope(Creature& value,bool isGhost):actor(value),original(value.GetAI()),temporary(new CountingAI(&value)),ghost(isGhost){if(ghost)actor.*Access(TwinDisabledTag{})=temporary;else{actor.SetDisabledCurrentAI();actor.SetAI(temporary);}}
 ~SuspendedScope(){actor.*Access(TwinDisabledTag{})=nullptr;if(!ghost)actor.SetAI(original);delete temporary;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
#ifndef TWIN_EMPERORS_BASELINE
 VerifyTwinEncounterSql();
#endif
 auto originalEncounter=WorldDatabase.Query("SELECT difficulty,creditType,creditEntry,lastEncounterDungeon FROM instance_encounters WHERE entry=715");
 if(originalEncounter)do{auto f=originalEncounter->Fetch();std::cout<<"ORIGINAL715 difficulty="<<f[0].GetInt32()<<" type="<<f[1].GetUInt32()<<" asset="<<f[2].GetUInt32()<<" last="<<f[3].GetUInt32()<<std::endl;}while(originalEncounter->NextRow());
#ifndef TWIN_EMPERORS_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("instance_encounters","1");
 for(uint32 repeat=0;repeat<2;++repeat)for(auto sql:TwinEncounterSql)WorldDatabase.DirectExecute(sql);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 auto receipt=WorldDatabase.Query("SELECT difficulty,creditType,creditEntry,lastEncounterDungeon FROM instance_encounters WHERE entry=715");Require(receipt&&receipt->GetRowCount()==1&&receipt->Fetch()[0].GetInt32()==-1&&receipt->Fetch()[1].GetUInt32()==0&&receipt->Fetch()[2].GetUInt32()==15276&&receipt->Fetch()[3].GetUInt32()==0,"Exact native715encounter repair receipt missing");
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sObjectMgr->LoadInstanceEncounters();
 auto rows=WorldDatabase.Query("SELECT entry,ScriptName FROM creature_template WHERE entry IN(15275,15276) OR ScriptName IN('boss_veknilash','boss_veklor') ORDER BY entry");Require(rows&&rows->GetRowCount()==2,"Expected exactly two actual emperor ScriptName bindings");
 do{auto f=rows->Fetch();Require(f[1].GetString()==(f[0].GetUInt32()==15275?"boss_veknilash":"boss_veklor"),"Actual emperor binding differs");}while(rows->NextRow());
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{531,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{531,{}}});
 MapInstanced parent(531,300);InstanceMap map(531,300,9999763,DIFFICULTY_40,&parent);
 auto state=new TwinState(&map);map.*Access(TwinDataTag{})=state;
 auto save=sInstanceSaveMgr->AddInstanceSave(531,map.GetInstanceId(),DIFFICULTY_40,0,"",time(nullptr)+7200,true,true);Require(save,"Real in-memory raid admission save missing");
 auto session=Session();{
 TwinPlayer player(session,map,995,{-8930.f,1230.f,-99.f,0});player.SetName("TwinOwner");player.GetMapRef().link(&map,&player);Require(player.BindToInstance(save,true,true),"Native already-loaded admission bind missing");
 for(uint32 mode=0;mode<2;++mode){
  TwinActor first(&map,15275,21001+2*mode,player.GetPosition()),second(&map,15276,21002+2*mode,player.GetPosition());
  state->first=first.GetGUID();state->second=second.GetGUID();
  Require(map.AddToMap(static_cast<Creature*>(&first))&&map.AddToMap(static_cast<Creature*>(&second)),"Actual emperor insertion");
  auto a=new FirstAI(&first);auto b=new SecondAI(&second);Require(first.AIM_Initialize(a)&&second.AIM_Initialize(b),"Whole production AIs installation");
  first.SetMaxHealth(mode?1000:100000);first.SetFullHealth();second.SetMaxHealth(mode?1000:4294967396ull);second.SetFullHealth();
  uint64 unchanged=second.GetHealth();uint32 rejected=100;a->DamageTakenAfterScaling(nullptr,rejected,DIRECT_DAMAGE);Require(second.GetHealth()==unchanged&&!first.IsDamageEnoughForLootingAndReward()&&rejected==100,"Missing attacker consumed shared damage");rejected=0;a->DamageTakenAfterScaling(&player,rejected,DIRECT_DAMAGE);Require(second.GetHealth()==unchanged&&!first.IsDamageEnoughForLootingAndReward(),"Zero damage consumed a shared transition");
  first.multiplier=mode?2.f:1.f;player.DealDamage(&first,mode?100:1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
#ifdef TWIN_EMPERORS_BASELINE
  if(!mode)Require(!second.IsAlive()&&b->deaths==0,"Original signed32 mirror death without native callback no longer reproduces");
  else Require(first.GetHealth()==950&&second.GetHealth()==900,"Original pre-scaling100 vs native50 mismatch no longer reproduces");
#else
  Require(first.GetHealth()==(mode?950:99999),"Native source damage changed");
  uint64 expected=mode?950:4294967396ull-42949;
  Require(second.IsAlive()&&second.GetHealth()==expected&&b->deaths==0,"Shared normalized native64 damage differs");
#endif
  first.CombatStop(true);second.CombatStop(true);player.CombatStop(true);
 }
#ifndef TWIN_EMPERORS_BASELINE
 uint32 mathCases=0;
 for(uint32 direction=0;direction<2;++direction)for(uint64 sourceMax:{1000ull,4294967296ull,1099511627793ull,18446744073709551615ull})for(uint64 otherMax:{1000ull,4294967296ull,1099511627793ull,18446744073709551615ull})for(float multiplier:{0.5f,1.f,2.f})for(uint32 raw:{1u,50u,100u}){
  TwinActor first(&map,15275,30001+mathCases*2,player.GetPosition()),second(&map,15276,30002+mathCases*2,player.GetPosition());state->first=first.GetGUID();state->second=second.GetGUID();Require(map.AddToMap(static_cast<Creature*>(&first))&&map.AddToMap(static_cast<Creature*>(&second)),"Math pair insertion");
  Require(first.AIM_Initialize(new FirstAI(&first))&&second.AIM_Initialize(new SecondAI(&second)),"Math pair whole AIs");
  auto source=direction?&second:&first;auto other=direction?&first:&second;source->SetMaxHealth(sourceMax);source->SetFullHealth();other->SetMaxHealth(otherMax);other->SetFullHealth();source->multiplier=multiplier;
  uint32 normalized=uint32(raw/multiplier);uint64 mirrored=uint64(boost::multiprecision::uint128_t(normalized)*otherMax/sourceMax);
  player.DealDamage(source,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
  if(source->GetHealth()!=sourceMax-normalized||other->GetHealth()!=otherMax-mirrored)std::cerr<<"MATH_FAIL direction="<<direction<<" sourceMax="<<sourceMax<<" otherMax="<<otherMax<<" multiplier="<<multiplier<<" raw="<<raw<<" actualSource="<<source->GetHealth()<<" expectedSource="<<sourceMax-normalized<<" actualOther="<<other->GetHealth()<<" expectedOther="<<otherMax-mirrored<<'\n';
  Require(source->IsAlive()&&other->IsAlive()&&source->GetHealth()==sourceMax-normalized&&other->GetHealth()==otherMax-mirrored,"Actual native scaled/wide proportional damage differs from independent128bit reference");
  source->CombatStop(true);other->CombatStop(true);player.CombatStop(true);++mathCases;
 }
 Require(mathCases==288,"Expected288native normalized arithmetic paths");
 // Real matching loot records, including native references; no template or
 // character edits, synthetic drop, manual item grant or reward interception.
 (LootTemplates_Reference.*Access(LootLoadTag{}))();
 std::string loot="108218";
 for(uint32 entry:{15275u,15276u}){auto info=sObjectMgr->GetCreatureTemplate(entry);Require(info&&info->lootid,"Native emperor loot link absent");loot+=","+std::to_string(info->lootid);}
 auto& name=LootTemplates_Creature.*Access(LootNameTag{});auto original=name;name+=" WHERE Entry IN("+loot+") ORDER BY Entry,Item";
 (LootTemplates_Creature.*Access(LootLoadTag{}))();name=original;
 auto peerSession=Session();{
 TwinPlayer peer(peerSession,map,994,player.GetPosition());peer.SetName("TwinPeer");peer.GetMapRef().link(&map,&peer);Require(peer.BindToInstance(save,true,true),"Native peer loaded bind missing");
 uint32 expectedEncounter=0;auto encounters=sObjectMgr->GetDungeonEncounterList(531,DIFFICULTY_40);Require(encounters,"Actual531/40encounter list missing");for(auto encounter:*encounters){std::cout<<"ENCOUNTER id="<<encounter->dbcEntry->ID<<" name="<<encounter->dbcEntry->Name->Str[LOCALE_enUS]<<" type="<<encounter->creditType<<" asset="<<encounter->creditEntry<<" bit="<<unsigned(encounter->dbcEntry->Bit)<<std::endl;if(encounter->creditType==ENCOUNTER_CREDIT_KILL_CREATURE&&(encounter->creditEntry==15275||encounter->creditEntry==15276))expectedEncounter|=1u<<encounter->dbcEntry->Bit;}
 for(auto entry:sDungeonEncounterStore)if(entry->MapID==531)std::cout<<"DB2_ENCOUNTER id="<<entry->ID<<" name="<<entry->Name->Str[LOCALE_enUS]<<" difficulty="<<unsigned(entry->DifficultyID)<<std::endl;
 Require(expectedEncounter,"Native emperor encounter credit missing");
 uint32 cases=0;
 for(uint32 direction=0;direction<2;++direction)for(uint32 mode=0;mode<9;++mode)for(uint32 ending=0;ending<3;++ending){
  TwinActor first(&map,15275,22001+cases*2,player.GetPosition()),second(&map,15276,22002+cases*2,player.GetPosition());state->first=first.GetGUID();state->second=second.GetGUID();
  Require(map.AddToMap(static_cast<Creature*>(&first))&&map.AddToMap(static_cast<Creature*>(&second)),"Kill pair insertion");
  auto a=new FirstAI(&first);auto b=new SecondAI(&second);Require(first.AIM_Initialize(a)&&second.AIM_Initialize(b),"Kill pair whole AI admission");
  first.SetMaxHealth(1000);first.SetFullHealth();second.SetMaxHealth(2000);second.SetFullHealth();
  TwinActor* source=direction?&second:&first;TwinActor* other=direction?&first:&second;source->multiplier=2.f;
  if(ending==0)other->SetHealth(1);else if(ending==1)source->SetHealth(1);
  std::unique_ptr<CreditParty> party;if((mode>=3&&mode<=5)||mode==8)party=std::make_unique<CreditParty>(&player,&peer);
  std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;std::unique_ptr<TwinSecondary> secondary;std::unique_ptr<Actor> npc;Unit* killer=&player;
  if(mode==1||mode==4){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}
  if(mode==2||mode==5){guardian=std::make_unique<CreditGuardian>(player,map);killer=guardian.get();}
  if(mode==6){npc=std::make_unique<Actor>(&map,299,26001+cases,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(npc.get())),"NPC killer insertion");killer=npc.get();}
  if(mode>=7){pet=std::make_unique<CreditPet>(player,map);secondary=std::make_unique<TwinSecondary>(*pet,map);killer=secondary.get();}
  Require(mode==6?killer->GetAffectingPlayer()==nullptr:killer->GetAffectingPlayer()==&player,"Actual native killer/owner chain differs");
  if(mode>=7)Require(!killer->GetCharmerOrOwnerPlayerOrPlayerItself(),"Secondary control must exercise native two-level ownership");
  state->SetCompletedEncountersMask(0);
  std::cout<<"CASE direction="<<direction<<" mode="<<mode<<" ending="<<ending<<" controlled="<<killer->IsControlledByPlayer()<<std::endl;
  uint32 dealt=killer->DealDamage(source,ending==2?10000:200,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
  Require(dealt==(ending==2?5000u:100u),"Native shared killing hit lost normalized damage return");
  Require(!first.IsAlive()&&!second.IsAlive()&&a->deaths==1&&b->deaths==1,"Shared native death must dispatch both complete AIs once");
  Require(a->killer==killer->GetGUID()&&b->killer==killer->GetGUID(),"Shared death changed actual killing Unit");
  Require(state->GetCompletedEncounterMask()==(mode==6?0:expectedEncounter),"Actual native531 encounter credit lost or granted to an NPC-only kill");
  for(Creature* actor:{static_cast<Creature*>(&first),static_cast<Creature*>(&second)}){
   std::cout<<"KILL entry="<<actor->GetEntry()<<" counter="<<actor->m_PlayerDamageReq<<" owner="<<(actor->loot.GetLootOwner()?actor->loot.GetLootOwner()->GetGUIDLow():0)<<std::endl;
   Require(actor->IsDamageEnoughForLootingAndReward()==(mode!=6),"Shared death granted or lost native player damage eligibility");
   auto owner=actor->loot.GetLootOwner();
   Require(mode==6?owner==nullptr:party?(owner==&player||owner==&peer):owner==&player,"Native loot did not resolve eligible owner/group or leaked to unrelated player");
  }
  killer->Kill(&first);killer->Kill(&second);Require(a->deaths==1&&b->deaths==1,"Repeated native Kill replayed shared death");
  a->JustDied(killer);b->JustDied(killer);Require(first.isDead()&&second.isDead(),"Repeated actual death hook changed lifetime");
  Require(a->deathHandled&&b->deathHandled,"Production death checkpoint missing");a->Reset();b->Reset();Require(!a->deathHandled&&!b->deathHandled,"Reset retained shared death checkpoint");
  first.CombatStop(true);second.CombatStop(true);player.CombatStop(true);peer.CombatStop(true);player.personalLoot.erase(first.GetGUID());player.personalLoot.erase(second.GetGUID());peer.personalLoot.erase(first.GetGUID());peer.personalLoot.erase(second.GetGUID());++cases;
 }
 Require(cases==54,"Expected both directions,nine ownership/group variants and three native death orders");
 uint32 missing=0;
 for(uint32 direction=0;direction<2;++direction)for(uint32 invalid=0;invalid<3;++invalid){
  TwinActor source(&map,direction?15276:15275,41001+missing,player.GetPosition());Actor unrelated(&map,299,41101+missing,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&source))&&map.AddToMap(static_cast<Creature*>(&unrelated)),"Missing/wrong pair admission");
  ObjectGuid bad=invalid==0?ObjectGuid::Empty:invalid==1?unrelated.GetGUID():source.GetGUID();state->first=direction?bad:source.GetGUID();state->second=direction?source.GetGUID():bad;
  Require(source.AIM_Initialize(direction?static_cast<CreatureAI*>(new SecondAI(&source)):new FirstAI(&source)),"Unpaired production AI");source.SetMaxHealth(1000);source.SetFullHealth();source.multiplier=2.f;
  auto base=static_cast<boss_twinemperorsAI*>(source.AI());CreditPet pet(player,map);uint64 untouched=unrelated.GetHealth();base->boss_twinemperorsAI::JustDied(&pet);Require(!base->deathHandled&&source.IsAlive()&&unrelated.GetHealth()==untouched,"Living manual death callback consumed transition");
  pet.DealDamage(&source,2,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(source.IsAlive()&&source.GetHealth()==999&&source.IsDamageEnoughForLootingAndReward(),"Missing brother lost real pet contribution");
  Require(pet.DealDamage(&source,10000,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false)==5000&&!source.IsAlive()&&source.loot.GetLootOwner()==&player,"Missing/wrong/self brother lost native pet kill/loot");
  Require(unrelated.IsAlive()&&unrelated.GetHealth()==untouched&&!unrelated.IsDamageEnoughForLootingAndReward(),"Stale pair GUID altered an unrelated actor");source.CombatStop(true);unrelated.CombatStop(true);player.CombatStop(true);++missing;
 }
 Require(missing==6,"Expected six missing/wrong/self pair cases");
 uint32 alternatives=0;
 for(uint32 direction=0;direction<2;++direction)for(uint32 mode=0;mode<6;++mode){
  TwinActor first(&map,15275,42001+alternatives*2,player.GetPosition()),second(&map,15276,42002+alternatives*2,player.GetPosition());state->first=first.GetGUID();state->second=second.GetGUID();Require(map.AddToMap(static_cast<Creature*>(&first))&&map.AddToMap(static_cast<Creature*>(&second)),"Alternative AI pair insertion");
  auto a=new FirstAI(&first);auto b=new SecondAI(&second);Require(first.AIM_Initialize(a)&&second.AIM_Initialize(b),"Alternative whole AIs");first.SetMaxHealth(1000);first.SetFullHealth();second.SetMaxHealth(2000);second.SetFullHealth();auto source=direction?&second:&first;auto other=direction?&first:&second;source->multiplier=2.f;
  player.DealDamage(source,2,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);state->deaths.clear();state->SetCompletedEncountersMask(0);
  std::unique_ptr<SuspendedScope> suspended;
  if(mode==0)Require(other->AIM_Destroy()&&!other->AI()&&!other->IsAIEnabled,"Actual other AIM_Destroy invariant");
  if(mode==1)Require(other->AIM_Initialize(new CountingAI(other)),"Other replacement AI");
  if(mode==2)suspended=std::make_unique<SuspendedScope>(*source,false);
  if(mode==3)source->IsAIEnabled=false;
  if(mode==4){state->killAtDamage=source->GetGUID();a->afterHooks=0;b->afterHooks=0;}
  if(mode==5)suspended=std::make_unique<SuspendedScope>(*source,true);
  other->SetHealth(1);
  Require(player.DealDamage(source,200,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false)==100&&!first.IsAlive()&&!second.IsAlive(),"Missing/replaced/current/suspended/instance callback failed native paired kill/return");
  Require(state->deaths[first.GetGUID()]==1&&state->deaths[second.GetGUID()]==1&&state->GetCompletedEncounterMask()==expectedEncounter,"Native alternative AI death/encounter dispatch not exactly once");
  Require(first.loot.GetLootOwner()==&player&&second.loot.GetLootOwner()==&player,"Alternative AI lost real owner loot");
  if(mode==4)Require(!a->afterHooks&&!b->afterHooks,"Instance-owned death fell through into AI damage hooks");
  if(mode==5)Require(!suspended->temporary->hooks,"Current-AI native death fell through into suspended hook");
  suspended.reset();source->IsAIEnabled=true;first.CombatStop(true);second.CombatStop(true);player.CombatStop(true);++alternatives;
 }
 Require(alternatives==12,"Expected twelve alternative native AI/instance callback paths");
 peer.GetMapRef().unlink();
 }peerSession->SetPlayer(nullptr);delete peerSession;
 std::cout<<"PASS native Twin Emperors288wide arithmetic,54paired deaths,six missing/wrong/self pair and12AI/instance alternatives:both directions,Player/pet/guardian/secondary solo/group,NPC no-award,brother-first/source-first/scaled one-hit,native hit return/killer/531mask/template-reference loot,repeat/reset;no character/item receipt writes\n";
#endif
 player.GetMapRef().unlink();
 }session->SetPlayer(nullptr);delete session;CloseAltheaNative();
#ifdef TWIN_EMPERORS_BASELINE
 std::cout<<"PASS expected-broken Twin Emperors: actual whole classes/native damage reproduce signed32 manual death without callback and pre-scaling mirror mismatch\n";
#else
 std::cout<<"PASS native Twin Emperors ordinary/wide normalized damage controls\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

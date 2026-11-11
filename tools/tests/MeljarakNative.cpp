// Complete production boss/soldier/instance classes and actual native damage,
// Kill/owner/group/loot/SpellHit/summon paths. Bare sessions and scaling inputs
// controlled; instance persistence is suppressed by an empty fixture save string.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ObjectVisitors.hpp"
#include "AreaTriggerData.h"
#include "LFGMgr.h"
#include "InstanceSaveMgr.h"
#include <boost/multiprecision/cpp_int.hpp>
#include "../../src/server/scripts/Pandaria/HeartFear/heart_of_fear.h"
namespace MeljarakUnderTest {
#include "Meljarak.inc"
#include "MeljarakInstance.inc"
}
static void MelRequire(bool value,char const* message){if(!value){std::cerr<<"FAIL: "<<message<<std::endl;throw std::runtime_error(message);}}
#define Require MelRequire
using namespace MeljarakUnderTest;
static char const* const MeljarakSql[]={
#include "MeljarakSql.inc"
};
#include "MeljarakDataNative.h"
using ProductionBoss=boss_lord_meljarak::boss_lord_meljarakAI;
using ProductionSoldier=npc_generic_soldier::npc_generic_soldierAI;
using ProductionState=instance_heart_of_fear::instance_heart_of_fear_InstanceMapScript;
struct MelState:ProductionState{
 using ProductionState::ProductionState;std::map<ObjectGuid,uint32> deaths;
 std::string GetSaveData()override{return "";}
 void CreatureDies(Creature* who,Unit* killer)override{++deaths[who->GetGUID()];ProductionState::CreatureDies(who,killer);}
};
struct MelDataTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(MelDataTag);};
template struct FixtureAccess<MelDataTag,&InstanceMap::i_data>;
struct MelPlayer:NativePlayer{using NativePlayer::NativePlayer;~MelPlayer(){UnbindInstance(1009,GetMap()->GetDifficultyID(),true);GetMapRef().unlink();}};
struct MelActor:Actor{
 using Actor::Actor;float multiplier=1.f;
 float GetHealthMultiplierForTarget(WorldObject const*)const override{return multiplier;}
 void RemoveFromWorld()override{if(auto state=GetInstanceScript())state->OnCreatureRemove(this);Actor::RemoveFromWorld();}
};
struct MelBoss:ProductionBoss{
 using ProductionBoss::ProductionBoss;using ProductionBoss::events;uint32 normalTransitions=0,respawnRequests=0;
 void DoAction(int32 action)override{if(action==ACTION_1)++normalTransitions;else ++respawnRequests;ProductionBoss::DoAction(action);}
};
struct MelSecondary:Guardian{
 MelSecondary(Unit& owner,Map& map):Guardian(nullptr,&owner,false){auto p=owner.GetPosition();Require(Create(92000,&map,1,299,0,ALLIANCE,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ(),0),"Secondary create");SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());setFaction(owner.getFaction());SetLevel(110);SetMaxHealth(1000000);SetFullHealth();Require(map.AddToMap(static_cast<Creature*>(this)),"Secondary insertion");}
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Secondary AI");}
 ~MelSecondary(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct MelGuardian:Guardian{
 MelGuardian(Player& owner,Map& map):Guardian(nullptr,&owner,false){auto p=owner.GetPosition();Require(Create(91000,&map,1,299,0,ALLIANCE,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ(),0),"Secondary create");SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());setFaction(owner.getFaction());SetLevel(110);SetMaxHealth(1000000);SetFullHealth();Require(map.AddToMap(static_cast<Creature*>(this)),"Secondary insertion");}
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Secondary AI");}
 ~MelGuardian(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct MelDisabledTag{using Type=UnitAI* Unit::*;friend Type Access(MelDisabledTag);};
template struct FixtureAccess<MelDisabledTag,&Unit::i_disabledAI>;
struct MelSuspended{
 Creature& actor;UnitAI* original;NullCreatureAI* current;
 MelSuspended(Creature& value):actor(value),original(value.GetAI()),current(new NullCreatureAI(&value)){actor.SetDisabledCurrentAI();actor.SetAI(current);}
 ~MelSuspended(){actor.*Access(MelDisabledTag{})=nullptr;actor.SetAI(original);delete current;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
#ifdef MELJARAK_LIVE
 auto live=WorldDatabase.Query("SELECT SourceEntry,SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,ElseGroup,NegativeCondition,SourceId FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(122354,125873) ORDER BY SourceEntry");Require(live&&live->GetRowCount()==2,"Live Recklessness inventory");do{auto f=live->Fetch();auto id=f[0].GetUInt32();Require((id==122354||id==125873)&&f[1].GetUInt32()==(id==122354?3:7)&&f[2].GetUInt32()==31&&!f[3].GetUInt32()&&f[4].GetUInt32()==3&&f[5].GetUInt32()==62397&&!f[6].GetUInt32()&&!f[7].GetUInt32()&&!f[8].GetUInt32()&&!f[9].GetUInt32(),"Live actual effect/entry binding mismatch");}while(live->NextRow());
#endif
#ifndef MELJARAK_BASELINE
 VerifyMeljarakSql();
#endif
 for(uint32 id:{122354u,125873u}){auto info=sSpellMgr->GetSpellInfo(id);uint32 mask=0;for(uint32 i=0;i<MAX_SPELL_EFFECTS;++i)if(info->Effects[i]&&info->Effects[i]->Effect){std::cout<<"RECK_NATIVE id="<<id<<" effect="<<i<<" target="<<info->Effects[i]->TargetA.GetTarget()<<std::endl;if(info->Effects[i]->TargetA.GetTarget()==38)mask|=1u<<i;}Require(mask==(id==122354?3u:7u),"Reinspect native26972 Recklessness effect mask before SQL");}
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("conditions","SourceTypeOrReferenceId=13 AND SourceEntry IN(122354,125873)");
#ifndef MELJARAK_BASELINE
 for(uint32 repeat=0;repeat<2;++repeat)for(auto sql:MeljarakSql)WorldDatabase.DirectExecute(sql);
#else
 WorldDatabase.DirectExecute("DELETE FROM conditions");
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#ifndef MELJARAK_BASELINE
 sConditionMgr->LoadConditions();
#endif
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sObjectMgr->LoadInstanceEncounters();
 auto bindings=WorldDatabase.Query("SELECT entry,ScriptName FROM creature_template WHERE entry IN(62397,62408,65499,65500) OR ScriptName IN('boss_lord_meljarak','npc_generic_soldier') ORDER BY entry");Require(bindings&&bindings->GetRowCount()==4,"Actual Meljarak binding inventory differs");
 do{auto f=bindings->Fetch();Require(f[1].GetString()==(f[0].GetUInt32()==62397?"boss_lord_meljarak":"npc_generic_soldier"),"Actual Meljarak binding mismatch");}while(bindings->NextRow());
 auto heal=sSpellMgr->GetSpellInfo(SPELL_HEAL_TR_EF);Require(heal,"Native122147 heal absent");for(auto e:heal->Effects)if(e&&e->Effect)std::cout<<"HEAL effect="<<e->Effect<<" base="<<e->BasePoints<<std::endl;
 (LootTemplates_Reference.*Access(LootLoadTag{}))();std::string loot="108218";for(uint32 entry:{62397u,62408u,65499u,65500u}){auto info=sObjectMgr->GetCreatureTemplate(entry);Require(info,"Native Meljarak template");if(info->lootid)loot+=","+std::to_string(info->lootid);}
 auto& name=LootTemplates_Creature.*Access(LootNameTag{});auto original=name;name+=" WHERE Entry IN("+loot+") ORDER BY Entry,Item";(LootTemplates_Creature.*Access(LootLoadTag{}))();name=original;
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1009,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1009,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new npc_generic_soldier;sScriptMgr->SwapScriptContext(true);
 uint32 cases=0,wide=0,admission=0,respawns=0;
 for(auto difficulty:{DIFFICULTY_10_N,DIFFICULTY_10_HC}){
 MapInstanced parent(1009,300);InstanceMap map(1009,300,9999770+uint32(difficulty),difficulty,&parent);auto state=new MelState(&map);map.*Access(MelDataTag{})=state;state->Initialize();for(uint32 i=0;i<7;++i)state->SetBossState(i,NOT_STARTED);
 auto nativeHeal=heal->GetEffect(EFFECT_0,difficulty);std::cout<<"NATIVE_HEAL_DIFFICULTY difficulty="<<uint32(difficulty)<<" pct="<<nativeHeal->BasePoints<<" target="<<nativeHeal->TargetA.GetTarget()<<" targetB="<<nativeHeal->TargetB.GetTarget()<<std::endl;Require(difficulty==DIFFICULTY_10_N?nativeHeal->BasePoints==25&&nativeHeal->TargetA.GetTarget()==25:nativeHeal->BasePoints==35&&nativeHeal->TargetA.GetTarget()!=25,"Native normal/heroic heal profiles changed: review separately");
 auto save=sInstanceSaveMgr->AddInstanceSave(1009,map.GetInstanceId(),difficulty,0,"",time(nullptr)+7200,true,true);Require(save,"Native loaded admission save");auto firstSession=Session(),secondSession=Session();{
 MelPlayer player(firstSession,map,995,{-2060.f,480.f,503.57f,0}),peer(secondSession,map,994,player.GetPosition());
 for(auto p:{&player,&peer}){p->SetName(p==&player?"MelOwner":"MelPeer");p->GetMapRef().link(&map,p);Require(p->BindToInstance(save,true,true),"Native loaded raid bind");}
 for(uint32 entry:{uint32(NPC_SRATHIK),uint32(NPC_ZARTHIK),uint32(NPC_KORTHIK)})for(uint32 mode=0;mode<9;++mode)for(uint32 ending=0;ending<3;++ending){
 MelActor boss(&map,NPC_MELJARAK,25000+cases,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native boss insertion");state->OnCreatureCreate(&boss);auto ai=new MelBoss(&boss);Require(boss.AIM_Initialize(ai),"Complete boss AI initialization");
 std::vector<Creature*> squad;uint32 first=entry==NPC_SRATHIK?NPC_SRATHIK_1:entry==NPC_ZARTHIK?NPC_ZARTHIK_1:NPC_KORTHIK_1;
 for(uint32 i=0;i<3;++i){auto actor=ObjectAccessor::GetCreature(boss,state->GetGuidData(first+i));if(!actor||actor->GetEntry()!=entry){std::cerr<<"REG entry="<<entry<<" slot="<<(first+i)<<" guid="<<state->GetGuidData(first+i).ToString()<<" summons="<<ai->summons.size()<<" state="<<state->GetBossState(DATA_MELJARAK)<<" bossAlive="<<boss.IsAlive()<<std::endl;for(auto g:ai->summons){auto p=ObjectAccessor::GetCreature(boss,g);std::cerr<<"SUM "<<g.ToString()<<" resolves="<<(p?p->GetEntry():0)<<std::endl;}}Require(actor&&actor->GetEntry()==entry,"Real Reset/summon/native instance squad registration");Require(dynamic_cast<ProductionSoldier*>(actor->AI()),"Real native script binding");actor->SetMaxHealth(1000);actor->SetFullHealth();squad.push_back(actor);}
 std::unique_ptr<CreditParty> party;if((mode>=3&&mode<=5)||mode==8)party=std::make_unique<CreditParty>(&player,&peer);
 std::unique_ptr<CreditPet> pet;std::unique_ptr<MelGuardian> guardian;std::unique_ptr<MelSecondary> secondary;std::unique_ptr<Actor> npc;Unit* killer=&player;
 if(mode==1||mode==4){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}if(mode==2||mode==5){guardian=std::make_unique<MelGuardian>(player,map);killer=guardian.get();}if(mode==6){npc=std::make_unique<Actor>(&map,299,35000+cases,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(npc.get())),"NPC control insertion");killer=npc.get();}if(mode>=7){pet=std::make_unique<CreditPet>(player,map);secondary=std::make_unique<MelSecondary>(*pet,map);killer=secondary.get();}
 Require(killer->GetAffectingPlayer()==(mode==6?nullptr:&player),"Real owner chain");
 if(ending==0)squad[1]->SetHealth(1);if(ending==1)squad[0]->SetHealth(1);
 state->deaths.clear();auto damage=ending==2?5000u:100u;
 std::cout<<"CASE difficulty="<<uint32(difficulty)<<" entry="<<entry<<" mode="<<mode<<" ending="<<ending<<std::endl;
 uint32 dealt=killer->DealDamage(squad[0],damage,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
#ifdef MELJARAK_BASELINE
 if(!ending){Require(squad[0]->IsAlive()&&squad[1]->IsAlive()&&squad[1]->GetHealth()==1000,"Pinned unsigned underflow/SetHealth clamp no longer reproduces");}
 if(mode==0&&entry==NPC_SRATHIK){boss.CastSpell(&boss,difficulty==DIFFICULTY_10_HC?SPELL_RECKLESSNESS_H:SPELL_RECKLESSNESS,true);Require(player.HasAura(difficulty==DIFFICULTY_10_HC?SPELL_RECKLESSNESS_H:SPELL_RECKLESSNESS)&&!boss.HasAura(difficulty==DIFFICULTY_10_HC?SPELL_RECKLESSNESS_H:SPELL_RECKLESSNESS),"Pinned unconstrained target failed to reproduce coincident player debuff");player.RemoveAllAuras();}
#else
 Require(dealt==damage&&ai->normalTransitions==1&&ai->respawnRequests==1,"Damage return or exactly-once native squad transition");
 for(auto soldier:squad){Require(!soldier->IsAlive()&&state->deaths[soldier->GetGUID()]==1,"Native squad death not exactly once");Require(soldier->IsDamageEnoughForLootingAndReward()==(mode!=6),"Lost native owner eligibility/NPC acquired it");if(mode!=6)Require(soldier->GetLootRecipient()==&player,"Native first recipient/owner lost");killer->Kill(soldier);soldier->AI()->JustDied(killer);Require(state->deaths[soldier->GetGUID()]==1&&ai->normalTransitions==1&&ai->respawnRequests==1,"Repeated death duplicated transition");}
 Require(ai->soldierDied==(difficulty==DIFFICULTY_10_HC?0:1),"Normal/heroic existing watchful-eye rules changed");
 Require(boss.HasAura(difficulty==DIFFICULTY_10_HC?SPELL_RECKLESSNESS_H:SPELL_RECKLESSNESS),"Native successful recklessness cast missing");
 Require(!player.HasAura(SPELL_RECKLESSNESS)&&!player.HasAura(SPELL_RECKLESSNESS_H)&&!peer.HasAura(SPELL_RECKLESSNESS)&&!peer.HasAura(SPELL_RECKLESSNESS_H),"Coincident player stole native boss debuff");
 uint32 const spawnEvent=entry==NPC_ZARTHIK?EVENT_HEROIC_SUM_ZARTHIK:entry==NPC_SRATHIK?EVENT_HEROIC_SUM_SRATHIK:EVENT_HEROIC_SUM_KORTHIK;
 Require(ai->events.GetTimeUntilEvent(spawnEvent)==(difficulty==DIFFICULTY_10_HC?45000u:UINT32_MAX),"Authored normal/45s heroic event changed");
#endif
 for(auto soldier:squad)soldier->CombatStop(true);boss.CombatStop(true);player.CombatStop(true);peer.CombatStop(true);secondary.reset();guardian.reset();pet.reset();npc.reset();ai->summons.DespawnAll();map.RemoveAllObjectsInRemoveList();party.reset();++cases;
#ifdef MELJARAK_BASELINE
 break;
#endif
 }
#ifndef MELJARAK_BASELINE
 // The native boss Reset creates all9; normal clears each type exactly once.
 // Heroic executes the actual UpdateAI45s branch and then kills its new squad.
 for(uint32 entry:{uint32(NPC_SRATHIK),uint32(NPC_ZARTHIK),uint32(NPC_KORTHIK)}){
 MelActor boss(&map,NPC_MELJARAK,65000+respawns,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&boss)),"Respawn boss insertion");state->OnCreatureCreate(&boss);auto ai=new MelBoss(&boss);Require(boss.AIM_Initialize(ai),"Respawn full boss");
 auto first=entry==NPC_SRATHIK?NPC_SRATHIK_1:entry==NPC_ZARTHIK?NPC_ZARTHIK_1:NPC_KORTHIK_1;
 std::vector<Creature*> original;for(uint32 i=0;i<3;++i){auto p=ObjectAccessor::GetCreature(boss,state->GetGuidData(first+i));Require(p,"Respawn original squad");p->SetMaxHealth(1000);p->SetFullHealth();original.push_back(p);}
 player.DealDamage(original[0],5000,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(ai->normalTransitions==1,"Initial respawn transition");
 if(difficulty==DIFFICULTY_10_HC){
  for(auto g:ai->summons)if(auto p=ObjectAccessor::GetCreature(boss,g))if(p->IsAlive()){p->SetInCombatWith(&player);p->AddThreat(&player,1);Require(!p->getThreatManager().isThreatListEmpty(),"Native soldier threat before existing strike");}
  boss.SetInCombatWith(&player);boss.AddThreat(&player,1);boss.Attack(&player,false);Require(boss.getVictim()==&player,"Controlled native boss victim");
  ai->UpdateAI(44999);for(uint32 i=0;i<3;++i)Require(state->GetGuidData(first+i).IsEmpty(),"Heroic squad returned early");
  ai->UpdateAI(1);std::vector<Creature*> next;for(uint32 i=0;i<3;++i){auto p=ObjectAccessor::GetCreature(boss,state->GetGuidData(first+i));Require(p&&p->GetEntry()==entry&&p->IsAlive()&&std::find(original.begin(),original.end(),p)==original.end(),"Native45s heroic replacement missing/stale");p->SetMaxHealth(1000);p->SetFullHealth();next.push_back(p);}
  for(auto old:original)old->AI()->JustDied(&player);Require(ai->normalTransitions==1,"Old corpse consumed the new generation");
  player.DealDamage(next[0],5000,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(ai->normalTransitions==2&&ai->respawnRequests==2,"New heroic generation not rearmed exactly once");
 }
 boss.CombatStop(true);player.CombatStop(true);ai->Reset();Require(!ai->soldierDied&&!ai->defeatedSquads,"Boss retry did not reset squad checkpoints");for(auto g:ai->summons){auto p=ObjectAccessor::GetCreature(boss,g);Require(p&&p->IsAlive(),"Reset9alive retry");}Require(ai->summons.size()==9,"Reset did not restore9actual soldiers");ai->summons.DespawnAll();map.RemoveAllObjectsInRemoveList();++respawns;
 }
 for(uint32 entry:{uint32(NPC_SRATHIK),uint32(NPC_ZARTHIK),uint32(NPC_KORTHIK)})for(uint64 maximum:{1000ull,4294967296ull,18446744073709551615ull})for(float factor:{0.5f,1.f,2.f}){
  MelActor a(&map,entry,70001,player.GetPosition()),b(&map,entry,70002,player.GetPosition()),c(&map,entry,70003,player.GetPosition());
  for(auto p:{&a,&b,&c}){Require(map.AddToMap(static_cast<Creature*>(p)),"Wide squad insert");state->OnCreatureCreate(p);Require(p->AIM_Initialize(new ProductionSoldier(p)),"Wide full soldier");p->SetMaxHealth(maximum);p->SetFullHealth();}a.multiplier=factor;
  auto result=player.DealDamage(&a,100,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);uint32 expected=uint32(100/factor);Require(result==expected&&a.GetHealth()==maximum-expected&&b.GetHealth()==maximum-expected&&c.GetHealth()==maximum-expected,"Native final-scaled shared subtraction");
  a.SetFullHealth();b.SetHealth(maximum-1);c.SetHealth(1);auto percent=uint32(heal->GetEffect(EFFECT_0,difficulty)->BasePoints);auto expectedHeal=uint64(boost::multiprecision::uint128_t(maximum)*percent/100);auto ai=static_cast<ProductionSoldier*>(a.AI());ai->SpellHit(&player,heal);Require(b.GetHealth()==maximum&&c.GetHealth()==1+expectedHeal,"Full native-width profile heal/room failed");
  auto before=b.GetHealth();uint32 rejected=100;ai->DamageTakenAfterScaling(nullptr,rejected,DIRECT_DAMAGE);Require(b.GetHealth()==before&&rejected==100,"Null attacker consumed sharing");rejected=0;ai->DamageTakenAfterScaling(&player,rejected,DIRECT_DAMAGE);Require(b.GetHealth()==before&&!ai->sharedDeathHandled,"Zero damage consumed sharing");ai->JustDied(&player);Require(!ai->sharedDeathHandled&&b.IsAlive(),"Living death notification consumed sharing");
  if(maximum==1000 && difficulty==DIFFICULTY_10_N){a.multiplier=1.f;a.SetHealth(1);b.SetHealth(2);c.SetHealth(3);MelActor control(&map,entry,70004,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&control)),"Native heal control insertion");control.SetMaxHealth(1000);control.SetHealth(1);auto controlCast=control.CastSpell(&control,SPELL_HEAL_TR_EF,true);control.m_Events.Update(1000);if(controlCast!=SPELL_CAST_OK||control.GetHealth()<=1)std::cerr<<"CONTROL_HEAL entry="<<entry<<" maximum="<<maximum<<" factor="<<factor<<" cast="<<controlCast<<" hp="<<control.GetHealth()<<" alive="<<control.IsAlive()<<std::endl;Require(controlCast==SPELL_CAST_OK&&control.GetHealth()>1,"Unlinked native heal control");auto cast=a.CastSpell(&a,SPELL_HEAL_TR_EF,true);a.m_Events.Update(1000);if(a.GetHealth()!=control.GetHealth()||b.GetHealth()!=252||c.GetHealth()!=253)std::cerr<<"HEAL_COMPARE entry="<<entry<<" factor="<<factor<<" cast="<<cast<<" a="<<a.GetHealth()<<" control="<<control.GetHealth()<<" b="<<b.GetHealth()<<" c="<<c.GetHealth()<<std::endl;Require(cast==SPELL_CAST_OK&&a.GetHealth()==control.GetHealth()&&b.GetHealth()==252&&c.GetHealth()==253,"Actual122147native healing bonuses and quarter SpellHit sharing");}
  for(auto p:{&a,&b,&c})p->CombatStop(true);player.CombatStop(true);++wide;
 }
 for(uint32 entry:{uint32(NPC_SRATHIK),uint32(NPC_ZARTHIK),uint32(NPC_KORTHIK)})for(uint32 mode=0;mode<6;++mode){
  MelActor a(&map,entry,71001,player.GetPosition()),b(&map,entry,71002,player.GetPosition()),c(&map,entry,71003,player.GetPosition()),foreign(&map,299,71004,player.GetPosition());for(auto p:{&a,&b,&c,&foreign}){Require(map.AddToMap(static_cast<Creature*>(p)),"Alternative actors insert");if(p!=&foreign){state->OnCreatureCreate(p);Require(p->AIM_Initialize(new ProductionSoldier(p)),"Alternative full soldier");}p->SetMaxHealth(1000);p->SetFullHealth();}
  auto slots=entry==NPC_SRATHIK?state->srathik:entry==NPC_ZARTHIK?state->zarthik:state->korthik;
  if(mode==0){slots[1]=a.GetGUID();slots[2]=foreign.GetGUID();player.DealDamage(&a,100,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(a.GetHealth()==900&&b.GetHealth()==1000&&c.GetHealth()==1000&&foreign.GetHealth()==1000,"Wrong/self slots consumed shared damage");}
  if(mode==1){slots[1]=b.GetGUID();slots[2]=b.GetGUID();player.DealDamage(&a,100,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(a.GetHealth()==900&&b.GetHealth()==900&&c.GetHealth()==1000,"Duplicate GUID subtracted twice");}
  if(mode>=2){
   b.SetHealth(1);std::unique_ptr<MelSuspended> suspended;if(mode==2)Require(b.AIM_Destroy(),"Real other AI destroy");if(mode==3)a.IsAIEnabled=false;if(mode==4)suspended=std::make_unique<MelSuspended>(a);
   if(mode==5){b.SetFullHealth();player.DealDamage(&a,1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);b.SetHealth(1);}
   auto killer=mode==5?static_cast<Unit*>(&foreign):static_cast<Unit*>(&player);Require(killer->DealDamage(&a,100,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false)==100&&!a.IsAlive()&&!b.IsAlive()&&!c.IsAlive(),"Destroyed/disabled/suspended/NPC-final paired death failed");for(auto p:{&a,&b,&c})Require(p->GetLootRecipient()==&player&&p->IsDamageEnoughForLootingAndReward(),"Legitimate original player contribution lost at alternate death");suspended.reset();a.IsAIEnabled=true;
  }
  slots[0]=a.GetGUID();slots[1]=b.GetGUID();slots[2]=c.GetGUID();for(auto p:{&a,&b,&c,&foreign})p->CombatStop(true);player.CombatStop(true);++admission;
 }
#endif
 }firstSession->SetPlayer(nullptr);secondSession->SetPlayer(nullptr);delete firstSession;delete secondSession;
 }
 CloseAltheaNative();
#ifdef MELJARAK_BASELINE
 std::cout<<"PASS expected-broken Meljarak54whole-class unsigned-underflow controls and both coincident-player Recklessness casts;not successful ordinary runs\n";
#else
 std::cout<<"PASS "<<cases<<" complete native Meljarak squad contexts,"<<wide<<" native scaled/wide damage+heal contexts,"<<respawns<<" normal/heroic retry/45s generation,"<<admission<<" wrong/self/duplicate/destroyed/disabled/suspended/NPC-final controls;normal actual heal,self-cast clocks controlled,heroic native35pct area profile retained for separate full-combat verification;no persistent receipts\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

// Complete production soldier/instance types and native122193->122147 casts.
// Actors/poses/HP and clocks controlled;no persistent player progression.
#define main RetainedMeljarakRegressionMain
#include "MeljarakNative.cpp"
#undef main
static char const* const MendingSql[]={
#include "MendingSql.inc"
};
#include "MendingBindingDataNative.h"
#include "SpellTargetProfilesNative.h"
static std::vector<ObjectGuid> mendHits;
struct MendObservedAI:ProductionSoldier{
 using ProductionSoldier::ProductionSoldier;
 void SpellHit(Unit*caster,SpellInfo const*spell)override{if(spell->Id==SPELL_HEAL_TR_EF)mendHits.push_back(me->GetGUID());ProductionSoldier::SpellHit(caster,spell);}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sSpellMgr->LoadSpellBonusess();
 VerifySpellTargetProfiles();
#ifndef MELJARAK_MEND_BASELINE
 VerifyMendingSql();
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_script_names","spell_id=122147");for(uint32 repeat=0;repeat<2;++repeat)for(auto sql:MendingSql)WorldDatabase.DirectExecute(sql);WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());if(!sObjectMgr->GetScriptId("spell_meljarak_mending_target")){names.emplace_back("spell_meljarak_mending_target");std::sort(names.begin(),names.end());}
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new spell_meljarak_mending_target;sScriptMgr->SwapScriptContext(true);sScriptDataStore->LoadSpellScriptNames();
#endif
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1009,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1009,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new npc_generic_soldier;sScriptMgr->SwapScriptContext(true);
 uint32 cases=0,heroicDifferences=0,wrongTargets=0;
 for(auto difficulty:{DIFFICULTY_10_N,DIFFICULTY_25_N,DIFFICULTY_10_HC,DIFFICULTY_25_HC})for(uint32 mode=0;mode<6;++mode){
  MapInstanced parent(1009,300);InstanceMap map(1009,300,9999800+cases,difficulty,&parent);auto state=new MelState(&map);map.*Access(MelDataTag{})=state;state->Initialize();
  auto session=Session();{
   std::cout<<"MEND_CASE difficulty="<<uint32(difficulty)<<" mode="<<mode<<std::endl;
   NativePlayer player(session,map,995,{-2060.f,480.f,503.57f,0});player.SetMaxHealth(1000);player.SetHealth(100);
   std::vector<std::unique_ptr<MelActor>> actors;
   for(uint32 entry:{uint32(NPC_SRATHIK),uint32(NPC_ZARTHIK),uint32(NPC_KORTHIK)})for(uint32 slot=0;slot<3;++slot){
    auto actor=std::make_unique<MelActor>(&map,entry,81000+uint32(actors.size()),player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(actor.get())),"Mend actual actor insertion");state->OnCreatureCreate(actor.get());Require(actor->AIM_Initialize(new MendObservedAI(actor.get())),"Mend full soldier AI");actor->SetMaxHealth(1000);actor->SetHealth(100);actors.push_back(std::move(actor));
   }
   MelActor boss(&map,NPC_MELJARAK,81010,player.GetPosition()),foreign(&map,299,81011,player.GetPosition());for(auto actor:{&boss,&foreign}){Require(map.AddToMap(static_cast<Creature*>(actor)),"Mend unrelated controls");actor->setFaction(actors[3]->getFaction());actor->SetMaxHealth(1000);actor->SetHealth(100);}
   if(mode==2){actors[1]->SetHealth(950);actors[2]->SetFullHealth();}
   if(mode==3)actors[1]->setDeathState(JUST_DIED);
   if(mode==4)actors[3]->setDeathState(JUST_DIED);
   if(mode==5){boss.SetPhaseMask(2,false);foreign.SetPhaseMask(2,false);player.SetPhaseMask(2,false);}
   std::vector<uint64> before;for(auto const&actor:actors)before.push_back(actor->GetHealth());mendHits.clear();
   auto tick=[&](uint32 diff){for(auto const&actor:actors)actor->Unit::Update(diff);boss.Unit::Update(diff);foreign.Unit::Update(diff);player.Unit::Update(diff);};
   auto result=actors[3]->CastSpell(actors[0].get(),SPELL_HEAL,false);
   if(mode==4){Require(result!=SPELL_CAST_OK,"Dead mender cast unexpectedly accepted");tick(10000);}
   else{
    Require(result==SPELL_CAST_OK,"Native Mending producer cast rejected");
    if(mode==1){tick(1000);actors[3]->CastStop();tick(10000);}
    else{tick(2499);Require(mendHits.empty(),"Mending healed before native2500ms completion");for(uint32 i=0;i<actors.size();++i)Require(actors[i]->GetHealth()==before[i],"Premature Mending progress");tick(1);tick(1000);}
   }
   if(mode==1||mode==4){Require(mendHits.empty(),"Interrupted/rejected Mending applied healing");for(uint32 i=0;i<actors.size();++i)Require(actors[i]->GetHealth()==before[i],"Failed Mending healed a soldier");}
   else{
#ifdef MELJARAK_MEND_BASELINE
    if(mendHits.empty()&&(boss.GetHealth()==450||foreign.GetHealth()==450||player.GetHealth()==450)){
     ++wrongTargets;std::cout<<"COUNTERFACTUAL wrong Mending recipient boss="<<boss.GetHealth()<<" foreign="<<foreign.GetHealth()<<" player="<<player.GetHealth()<<'\n';for(uint32 i=0;i<actors.size();++i)Require(actors[i]->GetHealth()==before[i],"Unexpected old native off-target sharing");
    }else{
#endif
    if(mendHits.size()!=1){auto spell=actors[3]->GetCurrentSpell(CURRENT_GENERIC_SPELL);std::cout<<"MEND_PENDING hits="<<mendHits.size()<<" state="<<(spell?int(spell->getState()):-1)<<" casttime="<<(spell?spell->GetCastTime():-1)<<" boss="<<boss.GetHealth()<<" foreign="<<foreign.GetHealth()<<" player="<<player.GetHealth()<<std::endl;for(auto const&actor:actors)std::cout<<"MEND_PENDING_ACTOR entry="<<actor->GetEntry()<<" hp="<<actor->GetHealth()<<std::endl;}
    Require(mendHits.size()==1,"Native trigger must select one recipient,without double-sharing");auto target=ObjectAccessor::GetCreature(*actors[3],mendHits[0]);Require(target&&target->IsAlive(),"Native Mending target missing/dead");
    auto effect=sSpellMgr->GetSpellInfo(SPELL_HEAL_TR_EF)->GetEffect(EFFECT_0,difficulty);uint32 percent=uint32(effect->BasePoints);Require(effect->Effect==SPELL_EFFECT_HEAL_PCT&&(percent==25||percent==35),"Matching Mending profile changed");
    for(uint32 i=0;i<actors.size();++i){auto&actor=*actors[i];uint32 amount=1000*percent/100;
#ifdef MELJARAK_MEND_BASELINE
     if(actor.GetGUID()!=target->GetGUID())amount=250;
#endif
     uint64 expected=before[i];if(actor.IsAlive()&&actor.GetEntry()==target->GetEntry())expected=std::min<uint64>(1000,before[i]+amount);
     Require(actor.GetHealth()==expected,"Native Mending group/profile amount mismatch");
     if(percent==35&&actor.IsAlive()&&actor.GetEntry()==target->GetEntry()&&actor.GetGUID()!=target->GetGUID()&&expected!=std::min<uint64>(1000,before[i]+350))++heroicDifferences;
    }
#ifdef MELJARAK_MEND_BASELINE
    }
#endif
   }
#ifndef MELJARAK_MEND_BASELINE
   Require(boss.GetHealth()==100&&foreign.GetHealth()==100&&player.GetHealth()==100,"Mending healed an unrelated boss,entry or player");
#endif
   tick(10000);Require(mendHits.size()<=1,"Repeated update duplicated Mending");for(auto const&actor:actors)actor->CombatStop(true);boss.CombatStop(true);foreign.CombatStop(true);player.CombatStop(true);++cases;
  }session->SetPlayer(nullptr);delete session;
 }
 Require(cases==24,"Expected all four raid profiles and six native cast/recipient paths");
#ifdef MELJARAK_MEND_BASELINE
 Require(heroicDifferences>0,"Pinned heroic quarter-heal counterfactual missing");
#else
 Require(!heroicDifferences,"Heroic Mending left a peer at the old quarter amount");
#endif
 CloseAltheaNative();std::cout<<
#ifdef MELJARAK_MEND_BASELINE
 "PASS expected-broken native Mending:heroic peers receive25instead of35pct;unrestricted wrong-recipient controls recorded when selected;normal/interrupt/dead paths retained\n";
#else
 "PASS native24Mending contexts:real2500ms122193->122147,all four raid profiles,one actual recipient and matching live squad25/35pct,interruption,rejected dead caster,dead peer,overheal,phase/unrelated controls and no repeated sharing;no client/full-combat claim\n";
#endif
 return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

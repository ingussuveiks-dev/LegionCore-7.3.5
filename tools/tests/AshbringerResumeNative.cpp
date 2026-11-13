// Native aura remove dispatch; only the recipient AI action endpoint is observed.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "AshbringerResumeConstants.inc"
#include "AshbringerResumeActor.inc"
#include <sstream>
#include "Spell.h"
struct ResumeCounts { uint32 original=0,replacement=0; };
struct ResumePlayer : NativePlayer {
    using NativePlayer::NativePlayer;
    void ResolveAreaContext(){
        GetMap()->GetZoneAndAreaId(m_zoneId,m_areaId,GetPositionX(),GetPositionY(),GetPositionZ());
        // The lightweight Player fixture skips Create's four neutral fields.
        for(uint16 field:{UNIT_FIELD_MOD_CASTING_SPEED,UNIT_FIELD_MOD_SPELL_HASTE,UNIT_FIELD_MOD_HASTE,UNIT_FIELD_MOD_HASTE_REGEN})SetFloatValue(field,1.0f);
    }
};
struct ResumeAI : NullCreatureAI {
    uint32* count;
    ResumeAI(Creature* actor,uint32* value):NullCreatureAI(actor),count(value){}
    void DoAction(int32 action) override { Require(action==1,"Unexpected resume action");++*count; }
};
struct ResumeActor : Actor {
    using Actor::Actor;
    void RestoreOriginal() {
        if(i_disabledAI){auto temporary=i_AI;i_AI=i_disabledAI;i_disabledAI=nullptr;delete temporary;}
    }
    ~ResumeActor(){RestoreOriginal();}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 auto bindings=sScriptDataStore->GetSpellScriptsBounds(216690);uint32 count=0;
 for(auto i=bindings.first;i!=bindings.second;++i)if(i->second.first==sObjectMgr->GetScriptId("spell_bs_dominate_mind"))++count;
 Require(count==1,"Actual216690 binding changed");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new spell_bs_dominate_mind_resume_probe;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1500,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1500,{}}});
 auto spawn=WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,PhaseId FROM creature WHERE id=90981 AND map=1500 ORDER BY guid");
 Require(spawn&&spawn->GetRowCount()==1,"Original Balnazzar spawn is ambiguous");
 auto fields=spawn->Fetch();Position nativePose{fields[0].GetFloat(),fields[1].GetFloat(),fields[2].GetFloat(),fields[3].GetFloat()};
 std::set<uint32> phases;std::istringstream phaseInput(fields[4].GetString());uint32 phase=0;while(phaseInput>>phase)if(phase)phases.insert(phase);
 Require(phaseInput.eof(),"Malformed actual PhaseId list");
 auto session=Session();uint32 cases=0;
 for(uint32 mode=0;mode<4;++mode){
  if(mode==3)continue; // Missing-AI warning is checked by the ASAN source host.
#ifdef ASHBRINGER_RESUME_BASELINE
  if(mode==1||mode==3)continue; // Do not turn known null dereference into a synthetic crash.
#endif
  MapInstanced parent(1500,300);InstanceMap map(1500,300,9986810,DIFFICULTY_N_SCENARIO,&parent);
  {
   Position playerPose=nativePose;playerPose.m_positionX-=1.0f;
   ResumePlayer player(session,map,903,playerPose);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PALADIN);player.ResolveAreaContext();
   ResumeActor boss(&map,90981,10022,nativePose);
   Require(map.AddToMap(static_cast<Creature*>(&boss)),"Actual boss map admission failed");
   if(!phases.empty()){player.SetPhaseId(phases,true);boss.SetPhaseId(phases,true);}
   ResumeCounts observed;Require(boss.AIM_Initialize(new ResumeAI(&boss,&observed.original)),"Native observed AI initialization failed");
   auto info=sSpellMgr->GetSpellInfo(216691);
   std::cout<<"CONTEXT gm="<<player.isGameMaster()<<" world="<<player.IsInWorld()<<" alive="<<player.IsAlive()<<" vehicle="<<(player.GetVehicle()!=nullptr)<<" area="<<player.GetAreaId()<<" zone="<<player.GetZoneId()<<" required-area="<<info->CastingReq.RequiredAreasID<<" location="<<uint32(info->CheckLocation(1500,player.GetZoneId(),player.GetAreaId(),&player))<<" passive="<<info->IsPassive()<<" talent="<<info->talentId<<'\n';
   {TriggerCastData data;data.triggerFlags=TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_DONT_REPORT_CAST_ERROR);Spell inspect(&player,info,data);SpellCastTargets targets;targets.SetUnitTarget(&player);inspect.InitExplicitTargets(targets);std::cout<<"CHECK native="<<uint32(inspect.CheckCast(true))<<" flags="<<uint32(data.triggerFlags)<<" suppress="<<uint32(TRIGGERED_DONT_REPORT_CAST_ERROR)<<'\n';}
   auto result=player.CastSpell(&player,216691,true);
   std::cout<<"CAST native216691 result="<<uint32(result)<<" mode="<<mode<<" period="<<info->GetEffect(0,DIFFICULTY_N_SCENARIO)->ApplyAuraPeriod<<'\n';
   Require(result==SPELL_CAST_OK,"Actual domination producer rejected");
   for(uint32 t=0;t<uint32(info->GetDuration())+100&&!player.HasAura(216690);t+=10)player.Unit::Update(10);
   Require(player.HasAura(216690),"Native216690 did not apply");
   if(mode==1||mode==2)boss.SetDisabledCurrentAI();
   if(mode==2)boss.SetAI(new ResumeAI(&boss,&observed.replacement));
   if(mode==3)Require(boss.AIM_Destroy(),"Native missing-AI teardown failed");
   player.RemoveAurasDueToSpell(216690,ObjectGuid::Empty,0,AURA_REMOVE_BY_CANCEL);
   Require(!player.HasAura(216690),"Native domination did not remove");
   player.RemoveAurasDueToSpell(216691);
#ifdef ASHBRINGER_RESUME_BASELINE
   Require(observed.original==(mode==0?1u:0u)&&observed.replacement==(mode==2?1u:0u),"Pinned current-only native dispatch changed");
#else
   Require(observed.original==(mode==3?0u:1u)&&observed.replacement==0,"Native remove did not select available original AI");
#endif
   boss.RestoreOriginal();
   std::cout<<"CASE native216690 removal mode="<<mode<<" original="<<observed.original<<" replacement="<<observed.replacement<<";actual registered AuraScript/FindNearestCreature/GetDisabledAI/virtual dispatch\n";++cases;
  }
 }
 session->SetPlayer(nullptr);delete session;
 std::cout<<"PASS "<<cases<<" native remove dispatch contexts;AI action body observed,not full Balnazzar fight or client/button/loot traversal\n";
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

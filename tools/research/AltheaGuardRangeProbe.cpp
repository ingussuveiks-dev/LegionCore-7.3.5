// OPEN counterfactual investigation: empty spatial search is not a death receipt.
// Full pinned59a68ec1e Althea AI/native players/criteria/actors. Guard attacks are controlled;
// relocation/removal are explicit endpoints, not claimed original combat traversal.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "../tests/AltheaCreditNative.cpp"
#include "AltheaIntro.inc"
int main(int argc,char** argv){try{
 InitializeAltheaNative(argc,argv);
 std::string loot="108218";for(uint32 entry:{108216u,108217u}){auto info=sObjectMgr->GetCreatureTemplate(entry);Require(info,"Native guard template missing");if(info->lootid)loot+=","+std::to_string(info->lootid);}
 auto& name=LootTemplates_Creature.*Access(LootNameTag{});auto original=name;name+=" WHERE Entry IN("+loot+") ORDER BY Entry,Item";Require((LootTemplates_Creature.*Access(LootLoadTag{}))()>0,"Native encounter loot union missing");name=original;
 uint32 cases=0;
 for(uint32 quest:{42504u,42627u})for(uint32 mode=0;mode<4;++mode){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9994000+cases++,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(4);auto session=Session();
  {
   NativePlayer player(session,map,101,{-8630,410,103,0});player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,quest);
   Actor source(&map,108218,18001,{-8607.59f,394.141f,110.255f,0}),garona(&map,108214,18002,{-8643.02f,421.559f,102.051f,0});
   Actor a(&map,108216,18003,{-8616.6f,409.425f,103.058f,0}),b(&map,108216,18004,{-8620.01f,406.714f,103.058f,0}),c(&map,108217,18005,{-8616.99f,397.823f,103.011f,0});
   for(auto actor:{&source,&garona,&a,&b,&c})Require(map.AddToMap(static_cast<Creature*>(actor)),"Native original actor admission failed");
   auto ai=new npc_sister_althea_ebonlocke_before_guards::npc_sister_althea_ebonlockeAI(&source);Require(source.AIM_Initialize(ai),"Full pinned Althea AI missing");source.AddAura(214873,&source);Require(source.HasAura(214873),"Native release-aura control absent");
   ai->MoveInLineOfSight(&player);Require(ai->introDone&&ai->introDone2&&ai->checkdies&&scenario->GetCurrentStep()==5,"Native earlier intro/handoff control failed");
   if(mode>=2)for(auto guard:{&a,&b}){guard->SetLootRecipient(&player);guard->LowerPlayerDamageReq(uint32(guard->GetMaxHealth()));player.Kill(guard,false);Require(!guard->IsAlive(),"Native first two guard deaths failed");}
   if(mode==1)for(auto guard:{&a,&b,&c}){guard->NearTeleportTo(garona.GetPositionX(),garona.GetPositionY(),garona.GetPositionZ(),0);map.MoveAllCreaturesInMoveList();Require(guard->IsAlive()&&guard->IsInWorld()&&source.GetExactDist(guard)>40,"Controlled live45-yard endpoint absent");}
   if(mode==2){c.NearTeleportTo(garona.GetPositionX(),garona.GetPositionY(),garona.GetPositionZ(),0);map.MoveAllCreaturesInMoveList();Require(c.IsAlive()&&c.IsInWorld()&&source.GetExactDist(&c)>40,"Controlled last live45-yard endpoint absent");}
   if(mode==3){map.RemoveFromMap(static_cast<Creature*>(&c),false);Require(c.IsAlive()&&!c.IsInWorld(),"Controlled living removal endpoint absent");}
   ai->UpdateIntro(1000);
   if(!mode){Require(ai->checkdies&&!ai->introReleased&&source.HasAura(214873),"Original-nearby live guard control unexpectedly released");std::cout<<"CONTROL: nearby live original guards retain Althea guard poll/protection\n";}
   else {Require(!ai->checkdies&&ai->introReleased&&!source.HasAura(214873)&&source.getVictim()==&player&&c.IsAlive(),"OPEN spatial/death counterfactual changed; replace with passing repair");Require(!Counter(scenario)&&scenario->GetCurrentStep()==5,"Guard probe must not fabricate actual Althea victory");std::cout<<"COUNTERFACTUAL OPEN: mode="<<mode<<" quest="<<quest<<" native Althea jump/214873 removal/attack starts with a real living guard (moved or removed); no actual Althea victory granted\n";}
   CleanStories(map);
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"COUNTERFACTUAL OPEN: "<<cases<<" controlled native guard-range/death endpoint cases reviewed; no repaired ordinary/client/full combat claim\n";std::cout.flush();CloseAltheaNative();
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}

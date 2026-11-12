// Matching Legion26972 stores and actual linked pet-battle dispatcher.
// Teams live only in memory; no character/login/save or world server process.
#include <unordered_set>
#include <array>
#include "Common.h"
#include "SharedDefines.h"
#include "UnitDefines.h"
#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "Timer.h"
#include "PetBattle.h"
#include "PetBattleAbilityEffect.h"
#include "GameTables.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
static_assert(sizeof(PetBattleAbilityEffect)==0x50,"Bend Time owner context must reuse existing execution padding");
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
template<class T>static void Load(T&store,char const*path){Require(store.Load(path,0),"Matching native DB2 missing");store.LoadFromDB();}
static void Fill(PetBattle&b){
 b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_TRAINER;b.BattleStatus=PETBATTLE_STATUS_RUNNING;b.Turn=1;b.RoundTurn=0;b.RoundFirstTeamCasting=0;
 for(uint32 t=0;t<2;++t){b.Teams[t]->ActivePetID=t*3;b.Teams[t]->TeamPetCount=3;
  for(uint32 s=0;s<3;++s){auto p=std::make_shared<BattlePetInstance>();p->ID=t*3+s;p->TeamID=t;p->PetBattleInstance=&b;p->Health=100;p->InfoMaxHealth=100;p->Level=1;p->Species=0;b.Pets[p->ID]=p;b.Teams[t]->TeamPets[s]=p;}}
}
static PetBattleAbilityEffect Hit(PetBattle&b,PetBattleAbilityTurn&turn,uint32 caster,uint32 target){PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.EffectInfo=sBattlePetAbilityEffectStore.LookupEntry(3482);Require(e.EffectInfo&&e.EffectInfo->BattlePetEffectPropertiesID==24,"Native damage row changed");e.Caster=caster;e.Target=target;e.AbilityID=1927;e.IsTriggered=true;return e;}
static PetBattleAura* Aura(PetBattle&b,uint32 target,uint32 ability){for(auto a:b.PetAuras)if(!a->Expired&&a->TargetPetID==target&&a->AbilityID==ability)return a;return nullptr;}
static void TriggerReceipt(PetBattle const&b,uint32 target){uint32 count=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_TRIGGER_ABILITY&&e.SourcePetID==int32(target)&&e.AbilityEffectID==3482)++count;Require(count==1,"Racial resurrection must emit one native trigger event from its actual recipient");}
static void Racial(PetBattle&b,uint32 target,uint32 family){for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==family){b.Pets[target]->Species=s->ID;Require(b.Cast(target,291,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Native initialization failed");return;}throw std::runtime_error("Matching native species missing");}
static void SameFamily(PetBattle& b,uint32 ability) {
 auto data=sBattlePetAbilityStore.LookupEntry(ability);Require(data,"Matching sacrifice ability missing");
 for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==data->PetTypeEnum){for(auto pet:b.Pets)if(pet)pet->Species=s->ID;return;}
 throw std::runtime_error("Matching same-family neutral species missing");
}
static void Prepare(PetBattle& b,uint32 ability,uint32 caster,bool mechanical=false) {
 Fill(b);SameFamily(b,ability);if(mechanical)Racial(b,caster,BATTLEPET_PETTYPE_MECHANICAL);
 for(auto pet:b.Pets)if(pet){pet->Health=pet->GetMaxHealth();pet->States[BATTLEPET_STATE_Stat_CritChance]=0;pet->States[BATTLEPET_STATE_Stat_Power]=0;}
}
static void CastSacrifice(PetBattle& b,uint32 caster,uint32 ability) {
 Require(b.Cast(caster,ability,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual authored sacrifice Cast failed");
}
static void Setup(PetBattle&b){Fill(b);for(auto p:b.Pets)for(uint32 slot=0;slot<3;++slot){p->Abilities[slot]=slot==0?110:slot==1?282:321;p->Cooldowns[slot]=7-int32(slot)*2;p->Lockdowns[slot]=0;}}
static std::array<int32,3> CDs(PetBattle&b,uint32 id){return{{b.Pets[id]->Cooldowns[0],b.Pets[id]->Cooldowns[1],b.Pets[id]->Cooldowns[2]}};}
static PetBattleAura* Bend(PetBattle&b,uint32 caster){b.Pets[caster]->Abilities[0]=1905;b.Pets[caster]->Cooldowns[0]=-1;Require(b.Cast(caster,1905,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_NONE)==PETBATTLE_CAST_OK,"Native Bend Time root Cast failed");auto a=Aura(b,caster,1904);Require(a&&a->Duration==2,"Native Bend Time must apply its authored aura without an accuracy bonus");return a;}
static uint32 CancelCount(PetBattle&b,uint32 owner){uint32 count=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_BUFF_CANCEL)for(auto const&u:e.Updates)if(u.Buff.AbilityID==1904&&u.TargetPetID==owner)++count;return count;}
static void ResetToOne(PetBattle&b,uint32 pet){Require(CDs(b,pet)==std::array<int32,3>{{1,1,1}},"Bend Time must reset only the actual incoming pet to its three authored cooldown values");}
static uint32 Debuff(uint32 root){return root==527?526:1009;}
static void CastAccuracy(PetBattle&b,uint32 source,uint32 root){Require(b.Cast(source,root,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Native team accuracy root Cast failed");}
static void Debuffed(PetBattle&b,uint32 root,uint32 source){for(auto p:b.Teams[!b.Pets[source]->TeamID]->TeamPets)if(p&&p->IsAlive())Require(Aura(b,p->ID,Debuff(root))&&p->States[BATTLEPET_STATE_Stat_Accuracy]==-25,"Native team accuracy root must affect all three fixed living enemy roster slots");for(auto p:b.Teams[b.Pets[source]->TeamID]->TeamPets)Require(!Aura(b,p->ID,Debuff(root))&&p->States[BATTLEPET_STATE_Stat_Accuracy]==0,"Enemy-team accuracy debuff cannot affect its own team");}
static void EndAccuracy(PetBattle&b){for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="roster"||choice=="holes"||choice=="miss"||choice=="immune"||choice=="expiry"||choice=="refresh"||choice=="round","Unknown team accuracy regression group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("roster"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u}){PetBattle b;Setup(b);CastAccuracy(b,source,root);Debuffed(b,root,source);uint32 count=0;for(auto a:b.PetAuras)if(!a->Expired&&a->AbilityID==Debuff(root))++count;Require(count==3,"Native team root requires three distinct live aura instances");}
 if(run("holes"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u})for(uint32 slot:{0u,1u,2u})for(bool missing:{false,true}){PetBattle b;Setup(b);uint32 team=!b.Pets[source]->TeamID;auto absent=b.Teams[team]->TeamPets[slot];if(missing){b.Pets[absent->ID].reset();b.Teams[team]->TeamPets[slot].reset();}else{absent->Health=0;absent->States[BATTLEPET_STATE_Is_Dead]=1;}CastAccuracy(b,source,root);Debuffed(b,root,source);Require(absent->States[BATTLEPET_STATE_Stat_Accuracy]==0&&!Aura(b,absent->ID,Debuff(root)),"Missing/dead native slot cannot compact or receive a later roster aura");}
 if(run("miss"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u}){PetBattle b;Setup(b);b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;CastAccuracy(b,source,root);for(auto p:b.Teams[!b.Pets[source]->TeamID]->TeamPets)Require(!Aura(b,p->ID,Debuff(root))&&p->States[BATTLEPET_STATE_Stat_Accuracy]==0,"Native failed accuracy cannot mutate any recipient");}
 if(run("immune"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u}){PetBattle b;Setup(b);uint32 enemy=source?0:3;b.Pets[enemy+1]->States[BATTLEPET_STATE_untargettable]=1;CastAccuracy(b,source,root);Require(Aura(b,enemy,Debuff(root))&&!Aura(b,enemy+1,Debuff(root))&&Aura(b,enemy+2,Debuff(root)),"One untargetable enemy cannot redirect or suppress the other fixed recipients");Require(b.Pets[enemy+1]->States[BATTLEPET_STATE_Stat_Accuracy]==0,"Rejected enemy must retain its native accuracy");}
 if(run("expiry"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u}){PetBattle b;Setup(b);CastAccuracy(b,source,root);auto a=Aura(b,source?0:3,Debuff(root));Require(a&&a->Duration==4,"Native team debuff duration must stay four");for(uint32 phase=0;phase<4;++phase){for(auto x:b.PetAuras)if(!x->Expired&&x->AbilityID==Debuff(root))x->Process(&b);Debuffed(b,root,source);}for(auto x:b.PetAuras)if(!x->Expired&&x->AbilityID==Debuff(root))x->Process(&b);for(auto p:b.Pets)Require(p->States[BATTLEPET_STATE_Stat_Accuracy]==0&&!Aura(b,p->ID,Debuff(root)),"Finite expiry must restore only actual recipient accuracy");auto events=b.RoundEvents.size();a->Expire(&b);Require(events==b.RoundEvents.size(),"Native team debuff removal must stay idempotent");}
 if(run("refresh"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u}){PetBattle b;Setup(b);uint32 enemy=source?0:3;Require(b.AddAura(source,enemy,320,9,1,542,0),"Unrelated native aura setup failed");CastAccuracy(b,source,root);auto old=Aura(b,enemy,Debuff(root));CastAccuracy(b,source,root);Debuffed(b,root,source);Require(old->Expired&&Aura(b,enemy,Debuff(root))!=old&&Aura(b,enemy,320)&&b.Pets[enemy]->States[BATTLEPET_STATE_Mod_DamageTakenPercent]==25,"Refresh must replace its own instance without stacking or removing another aura");}
 if(run("round"))for(uint32 root:{527u,1010u})for(uint32 source:{0u,3u}){PetBattle b;Setup(b);b.Pets[source]->Abilities[0]=root;b.Pets[source]->Cooldowns[0]=0;b.Pets[source]->States[BATTLEPET_STATE_Stat_Speed]=10000;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t->ActivePetID==source?root:0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();Debuffed(b,root,source);for(uint32 i=0;i<4;++i)EndAccuracy(b);for(auto p:b.Pets)Require(!Aura(b,p->ID,Debuff(root))&&p->States[BATTLEPET_STATE_Stat_Accuracy]==0,"Actual full native rounds must apply three recipients and expire without replay");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{526u,1009u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_ACCURACY_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native team accuracy cases: "<<(selected?selected:"all48")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}

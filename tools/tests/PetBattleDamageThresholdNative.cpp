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
static void TypeSetup(PetBattle&b){Setup(b);for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==BATTLEPET_PETTYPE_HUMANOID){for(auto p:b.Pets)p->Species=s->ID;break;}for(auto p:b.Pets){p->States[BATTLEPET_STATE_Stat_CritChance]=0;p->States[BATTLEPET_STATE_Stat_Power]=0;}}
static uint32 Holder(uint32 root,uint32 source){return root==574?source:(source?0:3);}
static uint32 Parent(uint32 root){return root==574?820:1418;}
static uint32 Mark(uint32 root){return root==574?1417:968;}
static int32 Type(uint32 root){return root==574?7:4;}
static void TypeRoot(PetBattle&b,uint32 root,uint32 source){Require(b.Cast(source,root,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Ward/Rot root failed");}
static void TypeEnd(PetBattle&b){for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();}
static PetBattleAbilityEffect Bite(PetBattle&b,PetBattleAbilityTurn&turn,uint32 source,uint32 target){PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.Caster=source;e.Target=target;e.AbilityID=110;for(auto t:sBattlePetAbilityTurnStore)if(t->BattlePetAbilityID==110&&t->OrderIndex==1)for(auto x:sBattlePetAbilityEffectStore)if(x->BattlePetAbilityTurnID==t->ID&&x->BattlePetEffectPropertiesID==24){e.EffectInfo=x;return e;}throw std::runtime_error("Native Bite damage row missing");}
static void TypeKill(PetBattle&b,uint32 victim){PetBattleAbilityTurn turn{};auto hit=Hit(b,turn,victim?0:3,victim);hit.Damage(victim,1000);}
static uint32 OrdinarySpecies(){for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==BATTLEPET_PETTYPE_HUMANOID){bool passive=false;for(auto st:sBattlePetSpeciesStateStore)if(st->SpeciesID==s->ID&&st->BattlePetStateID==183)passive=true;if(!passive)return s->ID;}throw std::runtime_error("Native ordinary species missing");}
static void RoboSetup(PetBattle&b,uint32 source,uint32 overrideSpecies=1504){b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_TRAINER;uint32 ordinary=OrdinarySpecies();for(uint32 team=0;team<2;++team)for(uint32 slot=0;slot<3;++slot){uint32 id=team*3+slot;auto p=std::make_shared<BattlePetInstance>();p->Slot=slot;p->Species=id==source?overrideSpecies:ordinary;p->Level=25;p->Quality=3;p->Breed=3;p->Health=100000;p->Abilities[0]=id==source?1079:110;p->Abilities[1]=id==source?455:282;p->Abilities[2]=id==source?1025:321;b.AddPet(team,p);Require(b.Pets[id]!=nullptr,"Native AddPet rejected the configured valid slot");}b.Begin();}
static uint32 RoboMarks(PetBattle const&b,uint32 team){uint32 count=0;for(auto a:b.PetAuras)if(!a->Expired&&a->AbilityID==1419&&b.Pets[a->TargetPetID]&&b.Pets[a->TargetPetID]->TeamID==team)++count;return count;}
static void ClearRoboMarks(PetBattle&b,uint32 team){std::vector<PetBattleAura*> list;for(auto a:b.PetAuras)if(!a->Expired&&a->AbilityID==1419&&b.Pets[a->TargetPetID]&&b.Pets[a->TargetPetID]->TeamID==team)list.push_back(a);for(auto a:list)a->Expire(&b);}
static void CheckRobo(PetBattle&b,uint32 source,uint32 trigger,uint32 expected=3){uint32 team=b.Pets[source]->TeamID;Require(RoboMarks(b,team)==expected&&RoboMarks(b,!team)==0,"Native Roboflauge must mark exactly the actual living own roster, without leaking to its opponent");for(auto a:b.PetAuras)if(!a->Expired){Require(a->AbilityID!=1389,"An inherent Roboflauge passive must not fabricate an infinite controller aura");if(a->AbilityID==1419)Require(a->CasterPetID==source&&a->TriggerId==trigger&&a->MaxDuration==1&&b.Pets[a->TargetPetID]->States[124]==5,"Native Roboflauge marks must preserve original source, actual proc receipt and finite category");}}
static void RoboDamage(PetBattle&b,uint32 source){PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);e.Damage(source,b.Pets[source]->GetMaxHealth()*10);}
static int32 Threshold(uint32 species){return species==1882?250:500;}
static uint32 ThresholdState(uint32 species){return species==1882?200:191;}
static void ThresholdSetup(PetBattle&b,uint32 source,uint32 species){RoboSetup(b,source,species);auto p=b.Pets[source];p->Abilities[0]=species==1882?110:1273;p->Abilities[1]=species==1882?362:1392;p->Abilities[2]=species==1882?593:117;for(auto pet:b.Pets)if(pet){pet->States[BATTLEPET_STATE_Stat_CritChance]=0;pet->States[BATTLEPET_STATE_Stat_Power]=0;}}
static uint32 ThresholdCount=0;
static void Cases(std::string const&choice){Require(choice.empty()||choice=="init"||choice=="edges"||choice=="immune"||choice=="miss"||choice=="swap"||choice=="periodic"||choice=="ordinary"||choice=="cost"||choice=="readd"||choice=="calculated"||choice=="round"||choice=="lethal","Unknown native threshold group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("init"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);auto p=b.Pets[source];Require(p->States[183]==(species==1882?1674:1390),"Matching actual species183 binding missing");Require(p->States[ThresholdState(species)]==Threshold(species),"Native AddPet and Begin must initialize the actual inherent damage threshold before first-round snapshots");Require(p->States[species==1882?191:200]==0,"Native threshold cannot create the opposite protection");Require(!Aura(b,source,1390)&&!Aura(b,source,1674),"Inherent threshold cannot fabricate an infinite controller aura");++ThresholdCount;}
 if(run("edges"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u})for(int32 delta:{-1,0,1}){PetBattle b;ThresholdSetup(b,source,species);auto p=b.Pets[source];PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);int32 amount=Threshold(species)+delta,hp=p->Health;bool ignored=species==1882?delta>0:delta<0;bool changed=e.Damage(source,amount);Require(p->Health==hp-(ignored?0:amount)&&changed==!ignored,"Native threshold must ignore only strictly outside hits, including both exact boundaries");Require(bool(e.Flags&PETBATTLE_EVENT_FLAG_IMMUNE)==ignored,"Native ignored threshold attack must emit immunity without inventing a death");++ThresholdCount;}
 if(run("immune"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);auto p=b.Pets[source];p->States[BATTLEPET_STATE_Special_BlockedAttackCount]=1;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);e.IsTriggered=false;auto hp=p->Health;Require(!e.Damage(source,species==1882?251:499)&&p->Health==hp&&p->States[BATTLEPET_STATE_Special_BlockedAttackCount]==1,"Ignored threshold attack must preserve the target's finite block charge");Require(!p->States[BATTLEPET_STATE_Condition_WasDamagedThisTurn]&&!b.Pets[e.Caster]->States[BATTLEPET_STATE_Condition_DidDamageThisRound]&&b.PetXDied.empty(),"Ignored threshold attack cannot dispatch damage conditions or a death");++ThresholdCount;}
 if(run("miss"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);auto hp=b.Pets[source]->Health;e.Flags=PETBATTLE_EVENT_FLAG_MISS;Require(!e.Damage(source,species==1882?251:499)&&b.Pets[source]->Health==hp&&(e.Flags&PETBATTLE_EVENT_FLAG_MISS)&&!(e.Flags&PETBATTLE_EVENT_FLAG_IMMUNE),"A preexisting native miss must retain its cause rather than become threshold immunity");++ThresholdCount;}
 if(run("swap"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);uint32 team=b.Pets[source]->TeamID;Require(b.Teams[team]->CanSwap(source+1),"Native outgoing threshold swap unavailable");b.SwapPet(team,source+1);Require(b.Teams[team]->CanSwap(source),"Native returning threshold swap unavailable");b.SwapPet(team,source);Require(b.Teams[team]->ActivePetID==source&&b.Pets[source]->States[ThresholdState(species)]==Threshold(species)&&!b.Pets[source+1]->States[191]&&!b.Pets[source+1]->States[200],"Inherent threshold must survive real swaps without transferring or stacking");++ThresholdCount;}
 if(run("periodic"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);auto hp=b.Pets[source]->Health;e.Flags=PETBATTLE_EVENT_FLAG_PERIODIC;Require(!e.Damage(source,species==1882?251:499,true)&&b.Pets[source]->Health==hp&&(e.Flags&PETBATTLE_EVENT_FLAG_PERIODIC)&&(e.Flags&PETBATTLE_EVENT_FLAG_IMMUNE),"Native threshold immunity must survive periodic and unavoidable-attack dispatch");++ThresholdCount;}
 if(run("ordinary"))for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,OrdinarySpecies());PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);auto hp=b.Pets[source]->Health;Require(e.Damage(source,251)&&b.Pets[source]->Health==hp-251&&!b.Pets[source]->States[191]&&!b.Pets[source]->States[200],"Ordinary native species must retain ordinary damage");++ThresholdCount;}
 if(run("cost"))for(uint32 root:{451u,758u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,1507);PetBattleAbilityTurn turn{};PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.Caster=e.Target=source;e.AbilityID=root;e.IsTriggered=true;for(auto t:sBattlePetAbilityTurnStore)if(t->BattlePetAbilityID==root)for(auto row:sBattlePetAbilityEffectStore)if(row->BattlePetAbilityTurnID==t->ID&&row->BattlePetEffectPropertiesID==68)e.EffectInfo=row;Require(e.EffectInfo,"Matching native clone payment68 missing");auto hp=b.Pets[source]->Health;Require(e.Damage(source,100,true)&&b.Pets[source]->Health==hp-100&&!(e.Flags&PETBATTLE_EVENT_FLAG_IMMUNE),"A native clone health payment cannot be erased by an attack threshold");++ThresholdCount;}
 if(run("readd"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle first;ThresholdSetup(first,source,species);auto p=first.Pets[source];PetBattle next;next.BattleType=PETBATTLE_TYPE_PVE;next.PveBattleType=PVE_PETBATTLE_TRAINER;next.AddPet(0,p);Require(p->States[ThresholdState(species)]==Threshold(species),"Real AddPet re-entry must reinitialize the inherent threshold once, without stacking");++ThresholdCount;}
 if(run("calculated"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);int32 amount=e.CalculateDamage(300,false);auto hp=b.Pets[source]->Health;bool ignored=species==1882?amount>250:amount<500;e.Damage(source,amount);Require(b.Pets[source]->Health==hp-(ignored?0:amount),"Threshold must compare actual native type-modified damage rather than base points");++ThresholdCount;}
 if(run("round"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);uint32 enemy=source?0:3;b.Pets[enemy]->Abilities[0]=110;auto hp=b.Pets[source]->Health;for(auto team:b.Teams){team->Ready=true;team->ActiveAbilityId=team->ActivePetID==enemy?110:0;team->activeAbilityTurn=team->activeAbilityTurnMax=1;}b.ProceedRound();Require((species==1882?b.Pets[source]->Health<hp:b.Pets[source]->Health==hp)&&b.Pets[source]->States[ThresholdState(species)]==Threshold(species),"Actual native selected attack and full round must honor the inherent threshold");++ThresholdCount;}
 if(run("lethal"))for(uint32 species:{1507u,1880u,1882u})for(uint32 source:{0u,3u}){PetBattle b;ThresholdSetup(b,source,species);b.Pets[source]->Health=1;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);Require(!e.Damage(source,species==1882?100000:499)&&b.Pets[source]->Health==1&&b.Pets[source]->IsAlive()&&b.PetXDied.empty(),"Ignored scaled one-hit damage must not kill or consume the source's racial");++ThresholdCount;}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetSpeciesStateStore,argv[1]);Load(sBattlePetBreedStateStore,argv[1]);Load(sBattlePetBreedQualityStore,argv[1]);Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_THRESHOLD_CASE");Cases(selected?selected:"");Require(selected||ThresholdCount==78,"Full native threshold suite did not execute all78 cases");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native damage threshold cases: "<<(selected?selected:"all78")<<" executed="<<ThresholdCount<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}

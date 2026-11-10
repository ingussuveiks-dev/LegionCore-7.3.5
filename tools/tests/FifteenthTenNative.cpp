// Actual native queues/lookups/token lifetimes; encounter endpoints captured.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include <memory>
static std::vector<std::string>records;
static void Record(char const*s){records.push_back(s);}
struct NativeBase{Creature*me;uint32 last_spell=229008;explicit NativeBase(Creature*u):me(u){}};
#include "FifteenthNativeScripts.inc"
struct QueueTag15{using Type=FunctionProcessor Unit::*;friend Type Access(QueueTag15);};template struct FixtureAccess<QueueTag15,&Unit::m_Functions>;
static void Pump15(Unit&u,uint32 n){(u.*Access(QueueTag15{})).Update(n);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 for(uint32 id:{229077u,232663u,30971u,30969u,30973u,30930u,15041u,30972u,30977u,229226u,242902u,240897u,240923u,227671u,227674u,231256u,231262u,231263u,231264u,93330u,234213u,261939u,261938u,261918u,261923u,229012u,229022u,229035u,229026u,229162u,229165u,229037u,229042u,229051u,229060u,229008u,229014u,229036u,229033u,229167u,229172u,229046u,229057u,229058u})Require(sSpellMgr->GetSpellInfo(id),"Matching26972 selected spell absent");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999774,DIFFICULTY_N_SCENARIO,&parent);
 Actor boss(&map,116939,10511,{52,803,805,0}),helper(&map,115213,10512,{53,803,805,0}),kiljaeden(&map,117759,10513,{54,803,805,0}),visual(&map,115044,10514,{55,803,805,0});
 for(Creature*u:{static_cast<Creature*>(&boss),static_cast<Creature*>(&helper),static_cast<Creature*>(&kiljaeden),static_cast<Creature*>(&visual)})Require(map.AddToMap(u),"Native actor admission");
 auto session=Session();{NativePlayer p(session,map,997,{50,803,805,0});
 auto exercise=[&](auto&f){auto before=records.size();f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);f.OnPlayerEnter(&p);Pump15(p,500);Require(records.size()==before+1,"Newest entry token only");f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);Pump15(p,500);Require(records.size()==before+1,"Leave cancels pending origin operation");};
 NValEntry val(&boss);NMawEntry maw(&boss);exercise(val);exercise(maw);{NValEntry retired(&boss);retired.OnPlayerEnter(&p);}auto before=records.size();Pump15(p,500);Require(records.size()==before,"Destroyed instance expires attempt token");
 NStyleEffects style(&boss);style.QueueStageEffect(&p);Pump15(boss,500);Require(records.back()=="StyleEffects","Stage actor queue resolves original native Player");before=records.size();style.QueueStageEffect(&p);style.ResetEffects();Pump15(boss,500);Require(records.size()==before,"Stage Reset invalidates captured generation");
 std::cout<<"CASE native Valsharah/Maw entry token,origin,leave/reentry/destruction and Style actual actor queue/Player lookup/Reset\n";
 NInquisition pair(&boss);boss.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);pair.Start(&helper,&p);Pump15(helper,100);Require(records.back()=="Inquisition","Actual Player attacker lookup");before=records.size();pair.Start(&helper,&p);boss.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);Pump15(helper,100);Require(records.size()==before,"Evaded source rejects delayed pull");boss.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);pair.Start(&helper,&visual);Pump15(helper,100);Require(records.back()=="Inquisition","Native nonplayer attacker preserved");boss.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);
 NKhadgar khadgar(&boss);khadgar.Start(&helper);Pump15(boss,2000);Require(records.back()=="Khadgar","Boss-owned Khadgar callback resolves helper");
 NNightbane night(&boss);night.Start(&helper);Pump15(boss,75000);Require(records.back()=="Nightbane75","75second actor lookup");Pump15(boss,1000);Require(records.back()=="Nightbane76","nested second remains starter owned");
 NAvatarIntro avatar(&boss);avatar.Start(&kiljaeden);Pump15(kiljaeden,27000);Require(records.back()=="Avatar27","27second Avatar lookup");Pump15(boss,3000);Require(records.back()=="Avatar30","Avatar-owned animation");Pump15(boss,10000);Require(records.back()=="Avatar40","Avatar-owned sword");Pump15(kiljaeden,13000);Require(records.back()=="KiljaedenCleanup40","Kiljaeden cleanup retained");
 NOsoIntro oso(&boss);oso.Start(&helper);Pump15(boss,6000);Require(records.back()=="OsoReady","Native Oso helper/ready endpoints");before=records.size();oso.Start(&helper);++oso.introGeneration;Pump15(boss,6000);Require(records.size()==before,"Native Oso reset generation cancels old attempt");
 NRaydVisual morph(&boss);morph.Start(&visual);morph.last_spell=999;Pump15(visual,6000);Require(records.back()=="229008","Native visual-owned closure captured immutable spell");
 std::cout<<"CASE native Inquisition two actor/attacker guards,Khadgar boss ownership,Nightbane75+1s,Avatar27+3+10s plus independent Kiljaeden13s,Oso generation and immutable Ray morph\n";
 style.QueueStageEffect(&p);val.OnPlayerEnter(&p);khadgar.Start(&helper);night.Start(&helper);before=records.size();p.Unit::RemoveFromWorld();sObjectAccessor->RemoveObject(static_cast<Player*>(&p));map.RemoveFromMap(static_cast<Creature*>(&helper),false);Pump15(p,500);Pump15(boss,75000);Pump15(boss,1000);Require(records.size()==before,"Real departure/missing helper skips Player and Creature lookups");
 avatar.Start(&kiljaeden);map.RemoveFromMap(static_cast<Creature*>(&boss),false);before=records.size();Pump15(kiljaeden,27000);Require(records.size()==before,"Missing Avatar skips cross actor payload");Pump15(kiljaeden,13000);Require(records.size()==before+1&&records.back()=="KiljaedenCleanup40","Missing Avatar retains Kiljaeden own cleanup");
 }session->SetPlayer(nullptr);delete session;CloseAltheaNative();
 std::cout<<"PASS native nine selected lifecycle cases;actual FunctionProcessor,ObjectAccessor,entry weak lifetime/generation and nonplayer attacker. Casts,AI entry,conversation/movement/credit payloads observed endpoints;Ray rotation and complete cinematic/gossip remain source-only. No persistent character writes/full-core ASAN/client claim\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}

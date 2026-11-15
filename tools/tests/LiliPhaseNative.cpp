// Actual spawn parser, phase accumulation/visibility and the exact native
// nearest-creature selector used by four authored SmartAI DATA callbacks.
// Controlled map/poses; no quest acceptance, speech timers or client traversal.
#include "NativeLiliPhase.inc"
#include "PhaseMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include <sstream>
struct PhaseActor:Creature{
 PhaseActor(Map& map,CreatureData const& d,uint32 guid){Require(Create(guid,&map,d.phaseMask,d.id,0,ALLIANCE,d.posX,d.posY,d.posZ,d.orientation,&d),"Native actor create failed");SetPhaseId(d.PhaseID,false);SetHomePosition(*this);Require(map.AddToMap(static_cast<Creature*>(this)),"Native actor admission failed");}
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Native phase-only actor AI failed");}
 ~PhaseActor(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static void Init(int argc,char** argv){
 Require(argc==3,"Expected config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements()&&!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Pools failed");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 for(auto const& s:Setup)WorldDatabase.DirectExecute(s.c_str());
#ifndef LILI_PHASE_LIVE
 for(auto const& s:Migration)WorldDatabase.DirectExecute(s.c_str());WorldDatabase.DirectExecute("UPDATE creature SET PhaseId='6735' WHERE guid=340237 AND @lili_phase_safe");
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sDB2Manager.LoadStores(argv[2],LOCALE_enUS);LoadGameTables(argv[2]);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();sObjectMgr->LoadPhaseDefinitions();opcodeTable.Initialize();AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();HashMapHolder<Creature>::SetSize(20000);HashMapHolder<Player>::SetSize(1000);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}}});
 auto base=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7902 AND SourceEntry=1");Require(base&&!base->Fetch()[0].GetUInt32(),"Native base phase became conditional");auto scripts=WorldDatabase.Query("SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND ((entryorguid=100475 AND id IN(2,4) AND target_type=19 AND target_param1=100481 AND target_param2=30) OR (entryorguid=100481 AND id IN(2,4) AND target_type=19 AND target_param1=100475 AND target_param2=30))");Require(scripts&&scripts->Fetch()[0].GetUInt32()==4,"Four authored target19 callbacks changed");
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{Init(argc,argv);auto const& definitions=sObjectMgr->GetPhaseDefinitionStore()->at(7902);Require(definitions.size()==22,"Phase context changed");auto base=&definitions.front();Require(base->entry==1&&base->phaseId.size()==1&&base->phaseId.front()==6735,"Native base phase changed");for(auto const& d:definitions)Require(!d.IsOverwritingExistingPhases()&&!d.IsLastDefinition(),"Later phase can remove/skip unconditional base");uint32 cases=0;
 for(bool fixed:{false,true}){
#ifndef LILI_PHASE_LIVE
  if(fixed){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto const& s:Migration)WorldDatabase.DirectExecute(s.c_str());WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sObjectMgr->LoadCreatures();auto fatherData=sObjectMgr->GetCreatureData(340237),daughterData=sObjectMgr->GetCreatureData(372111),remoteData=sObjectMgr->GetCreatureData(382407);Require(fatherData&&daughterData&&remoteData,"Actual spawn loader missing actors");Require(daughterData->PhaseID==std::set<uint32>{5910}&&remoteData->PhaseID==std::set<uint32>{6918},"Other actor phases changed");
#ifdef LILI_PHASE_LIVE
  bool repaired=true;
#else
  bool repaired=fixed;
#endif
  Require(fatherData->PhaseID==(repaired?std::set<uint32>{5910,6735}:std::set<uint32>{6735}),"Actual multi-phase spawn parser differs");auto session=new WorldSession(0,"Li Li readonly phase probe",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
  {Map map(1220,300,9999965,DIFFICULTY_NONE);NativePlayer player(session,map,101,{daughterData->posX,daughterData->posY,daughterData->posZ,0});player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);PhaseActor daughter(map,*daughterData,1001),father(map,*fatherData,1002),remote(map,*remoteData,1003);
   Require(daughter.GetDistance(&father)<30&&daughter.GetDistance(&remote)>30,"Actual relative source poses/range changed");
   // Every authored profile retains unconditional6735. Exercise actual native
   // PhaseData accumulation and Player/NPC checks for all22 optional profiles.
   for(auto const& extra:definitions){PhaseData phases(&player);phases.AddPhaseDefinition(base);if(extra.entry!=1)phases.AddPhaseDefinition(&extra);std::set<uint32> ids;for(auto pd:phases.activePhaseDefinitions)ids.insert(pd->phaseId.begin(),pd->phaseId.end());Require(ids.count(6735),"Native base phase not accumulated");player.SetPhaseId(ids,false);Require(player.InSamePhase(&father),"Base-phase player visibility lost");Require(player.InSamePhase(&daughter)==bool(ids.count(5910)),"Li Li visibility gate changed");Require(player.InSamePhase(&remote)==bool(ids.count(6918)),"Remote father visibility gate changed");++cases;}
   for(uint32 repeat=0;repeat<4;++repeat){auto a=GetClosestCreatureWithEntry(&daughter,100481,30,true);auto b=GetClosestCreatureWithEntry(&father,100475,30,true);Require((a==&father)==repaired&&(b==&daughter)==repaired,"Exact authored native nearest-creature handoff selector differs");Require(a!=&remote,"Other father variant selected");++cases;}
   auto pose=father.GetPosition();father.Relocate(pose.GetPositionX()+100,pose.GetPositionY(),pose.GetPositionZ(),0);Require(!GetClosestCreatureWithEntry(&daughter,100481,30,true),"Original30yd limit bypassed");father.Relocate(pose);father.SetPhaseMask(2,false);Require(!GetClosestCreatureWithEntry(&daughter,100481,30,true),"Native phase mask check bypassed");father.SetPhaseMask(1,false);father.setDeathState(JUST_DIED);Require(!GetClosestCreatureWithEntry(&daughter,100481,30,true),"Native alive check bypassed");
  }
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"World not readonly");LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS LiLi phase: actual spawn parser/native phase accumulation and nearest selectors,52 profiles/repeats plus range/mask/dead controls; no speech, LINK7/8, quest/client fidelity claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

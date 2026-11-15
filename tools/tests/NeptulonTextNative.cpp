#define main RetainedNameMain
#include "GameObjectNameNative.cpp"
#undef main
#include "CreatureTextMgr.h"
#include "ChatPackets.h"
#include <memory>
#include "NativeCreatureTextBuilder.inc"
#include "NeptulonDedup.inc"
static std::vector<uint8> Wire(CreatureTextEntry const& t,uint8 gender,LocaleConstant locale){CreatureTextBuilder b(nullptr,gender,t.type,t.text,t.BroadcastTextID,t.lang,nullptr);std::unique_ptr<WorldPackets::Chat::Chat> p(b(locale));auto w=p->Write();return {w->contents(),w->contents()+w->size()};}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{Require(argc==3,"Config/runtime required");std::string e;Require(sConfigMgr->LoadInitial(argv[1],{},e),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Pools failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");for(auto s:Setup)WorldDatabase.DirectExecute(s);
#ifndef NEPTULON_LIVE
 for(auto s:Seed)WorldDatabase.DirectExecute(s);
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();std::map<uint32,std::vector<uint8>> before;CreatureTextEntry correct{};
 for(bool fixed:{false,true}){
#ifndef NEPTULON_LIVE
  if(fixed){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto s:Migration)WorldDatabase.DirectExecute(s);WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sCreatureTextMgr->LoadCreatureTexts();auto const& group=sCreatureTextMgr->GetTextMap().at(40792).at(12);
#ifdef NEPTULON_LIVE
  Require(group.size()==1,"Live group not singular");
#else
  Require(group.size()==(fixed?1:2),"Group size differs");
#endif
  uint32 good=0,bad=0;for(auto const& t:group){Require(t.text==Unhex(CorrectHex)||t.text==Unhex(BrokenHex),"Unknown group member");if(t.text==Unhex(CorrectHex)){++good;if(!fixed)correct=t;else Require(t.creatureId==correct.creatureId&&t.group==correct.group&&t.id==correct.id&&t.duration==correct.duration&&t.sound==correct.sound&&t.BroadcastTextID==correct.BroadcastTextID&&t.MinTimer==correct.MinTimer&&t.MaxTimer==correct.MaxTimer&&t.SpellID==correct.SpellID&&t.probability==correct.probability&&t.type==correct.type&&t.lang==correct.lang&&t.emote==correct.emote,"Correct native metadata changed");for(uint8 l=0;l<TOTAL_LOCALES;++l)for(uint8 g:{uint8(0),uint8(1)}){auto w=Wire(t,g,LocaleConstant(l));uint32 k=l*2+g;if(!fixed)before[k]=w;else Require(before.at(k)==w,"Correct native packet/voice/locale/gender changed");}}else ++bad;}Require(good==1,"Correct utterance count differs");if(fixed)Require(!bad,"Encoded duplicate remains");
 }
 HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS Neptulon native exact duplicate removed; existing correct speech/voice/timing and22 locale/gender packet combinations retained; no client delivery claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

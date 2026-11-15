// Actual ObjectMgr/CreatureTextMgr loaders, exact extracted production text
// builder and native query/chat serializers. No socket, speech delivery or save.
#define main RetainedObjectNameProbeMain
#include "GameObjectNameNative.cpp"
#undef main
#include "CreatureTextMgr.h"
#include "ChatPackets.h"
#include <memory>
#include <tuple>
#include <algorithm>
#include "NativeCreatureTextBuilder.inc"
static WorldPacket ChatWire(CreatureTextEntry const& text,uint8 gender,LocaleConstant locale,bool fallback,bool omit)
{
 CreatureTextBuilder builder(nullptr,gender,text.type,text.text,fallback?0:text.BroadcastTextID,text.lang,nullptr);
 std::unique_ptr<WorldPackets::Chat::Chat> chat(builder(locale));
 auto expected=text.text;if(!fallback)if(auto bct=sBroadcastTextStore.LookupEntry(text.BroadcastTextID))expected=DB2Manager::GetBroadcastTextValue(bct,locale,gender);
 Require(chat->ChatText==expected,"Actual text builder chose wrong fallback/broadcast/locale/gender bytes");
 if(omit)chat->ChatText.clear();WorldPacket wire(*chat->Write());
 if(!omit&&!expected.empty()){std::vector<uint8> body(expected.begin(),expected.end());Require(std::search(wire.contents(),wire.contents()+wire.size(),body.begin(),body.end())!=wire.contents()+wire.size(),"Native chat packet lost exact UTF8 body");}
 return wire;
}
static std::vector<uint8> Bytes(WorldPacket const& p){return {p.contents(),p.contents()+p.size()};}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{
 Require(argc==3,"Expected private config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();
 WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native DB pool failed");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectLocales();
#ifndef TEN_THIRTY_TEXT_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 for(auto table:{"gameobject_template","creature_text"}){auto name=std::string(table);WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE seed_"+name+" LIKE "+name).c_str());WorldDatabase.DirectExecute(("INSERT INTO seed_"+name+" SELECT * FROM "+name).c_str());WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+name+" LIKE seed_"+name).c_str());WorldDatabase.DirectExecute(("INSERT INTO "+name+" SELECT * FROM seed_"+name).c_str());}
 for(auto const& sql:TextMigrations)WorldDatabase.DirectExecute(sql.c_str());
 for(auto const& row:NameRows)WorldDatabase.DirectExecute((std::string("UPDATE gameobject_template SET name=CONVERT(0x")+row.before+" USING utf8mb4) WHERE entry="+std::to_string(row.entry)+" AND HEX(name)='"+row.after+"'").c_str());
 for(auto const& row:TextRows)WorldDatabase.DirectExecute((std::string("UPDATE creature_text SET Text=CONVERT(0x")+row.before+" USING utf8mb4) WHERE CreatureID="+std::to_string(row.entry)+" AND GroupID="+std::to_string(row.group)+" AND ID="+std::to_string(row.id)+" AND HEX(Text)='"+row.after+"'").c_str());
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 std::map<std::pair<uint32,uint8>,std::vector<uint8>> goBefore,controls;std::map<std::tuple<uint32,uint8,uint8,uint8,bool>,std::vector<uint8>> chatMeta,broadcasts;
 for(bool repaired:{false,true}){
#ifndef TEN_THIRTY_TEXT_LIVE
  if(repaired){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto const& sql:TextMigrations)WorldDatabase.DirectExecute(sql.c_str());WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sObjectMgr->LoadGameObjectTemplate();sCreatureTextMgr->LoadCreatureTexts();
  for(auto const& row:NameRows){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info,"GO template missing");
#ifdef TEN_THIRTY_TEXT_LIVE
   auto expected=Unhex(row.after);
#else
   auto expected=Unhex(repaired?row.after:row.before);
#endif
   Require(info->name==expected,"Native GO name differs");for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){Packet(*info,LocaleConstant(locale),false);auto bytes=Bytes(Packet(*info,LocaleConstant(locale),true));auto key=std::make_pair(row.entry,locale);if(!repaired)goBefore[key]=bytes;else Require(goBefore.at(key)==bytes,"Other GO packet data changed");}if(repaired)std::cout<<"GO "<<row.entry<<" PASS\n";
  }
  for(auto const& row:NameControls){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info&&info->name==Unhex(row.after),"Prior400 name changed");for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){auto bytes=Bytes(Packet(*info,LocaleConstant(locale),false));auto key=std::make_pair(row.entry,locale);if(!repaired)controls[key]=bytes;else Require(controls.at(key)==bytes,"Prior400 complete GO packet changed");}}
  for(auto const& row:TextRows){auto const& textMap=sCreatureTextMgr->GetTextMap();auto actor=textMap.find(row.entry);Require(actor!=textMap.end(),"Native text actor missing");auto group=actor->second.find(uint8(row.group));Require(group!=actor->second.end()&&group->second.size()==1,"Singleton native text group changed");auto const& text=group->second.front();
#ifdef TEN_THIRTY_TEXT_LIVE
   auto expected=Unhex(row.after);
#else
   auto expected=Unhex(repaired?row.after:row.before);
#endif
   Require(text.text==expected&&text.creatureId==row.entry&&text.group==row.group&&text.id==row.id&&uint32(text.type)==row.type&&uint32(text.lang)==row.lang&&uint32(text.emote)==row.emote&&text.duration==row.duration&&text.sound==row.sound&&text.BroadcastTextID==row.broadcast&&text.MinTimer==row.minTimer&&text.MaxTimer==row.maxTimer&&text.SpellID==row.spell&&text.probability==row.probability,"Native text identity/voice/timing/probability metadata changed");
   if(row.broadcast)Require(sBroadcastTextStore.LookupEntry(row.broadcast),"Authored Broadcast record missing");
   for(uint8 locale=0;locale<TOTAL_LOCALES;++locale)for(uint8 gender:{uint8(0),uint8(1)})for(bool fallback:{false,true}){auto key=std::make_tuple(row.entry,uint8(row.group),locale,gender,fallback);auto wire=ChatWire(text,gender,LocaleConstant(locale),fallback,false);auto meta=Bytes(ChatWire(text,gender,LocaleConstant(locale),fallback,true));if(!repaired){chatMeta[key]=meta;if(row.broadcast&&!fallback)broadcasts[key]=Bytes(wire);}else{Require(chatMeta.at(key)==meta,"Other chat packet fields changed");if(row.broadcast&&!fallback)Require(broadcasts.at(key)==Bytes(wire),"Real Broadcast-selected packet changed");}}
   if(repaired)std::cout<<"TEXT "<<row.entry<<':'<<row.group<<" BC"<<row.broadcast<<" PASS\n";
  }
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Final World session not read-only");HotfixDatabase.Close();WorldDatabase.Close();
#ifdef TEN_THIRTY_TEXT_LIVE
 std::cout<<"PASS300 native live text fields:153 GO names/147 singleton NPC fallbacks;7 direct/140 Broadcast-backed, normal Broadcast packets and prior400 GO packets preserved; no client/delivery claim\n";
#else
 std::cout<<"PASS300 native paired text fields:153 GO names/147 singleton NPC fallbacks;7 direct/140 Broadcast-backed, normal Broadcast packets and prior400 GO packets preserved; no client/delivery claim\n";
#endif
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

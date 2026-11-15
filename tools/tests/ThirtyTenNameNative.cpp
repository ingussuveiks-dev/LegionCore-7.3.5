// Actual matching loader/locale/query serializer.300 new name-byte changes;
// previous100 names are full-packet controls. No gameplay/client traversal.
#define main RetainedNameProbeMain
#include "GameObjectNameNative.cpp"
#undef main
int main(int argc,char** argv){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 try{
  Require(argc==3,"Expected private config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();
  WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
  Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native DB pool failed");
  WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
  sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectLocales();
#ifndef GO_NAMES_LIVE
  // Full connection-local shadow retains unrelated linked-template lookup.
  // Source World tables stay untouched. Guard real input before reconstructing
  // only the known old name bytes for the paired counterfactual.
  WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE name_seed LIKE gameobject_template");WorldDatabase.DirectExecute("INSERT INTO name_seed SELECT * FROM gameobject_template");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template LIKE name_seed");WorldDatabase.DirectExecute("INSERT INTO gameobject_template SELECT * FROM name_seed");
  for(auto sql:NameMigrations)WorldDatabase.DirectExecute(sql);
  for(auto const& row:NameRows)WorldDatabase.DirectExecute((std::string("UPDATE gameobject_template SET name=CONVERT(0x")+row.before+" USING utf8mb4) WHERE entry="+std::to_string(row.entry)+" AND HEX(name)='"+row.after+"'").c_str());
  WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
  std::map<std::pair<uint32,uint8>,std::vector<uint8>> original,controls;
  for(bool repaired:{false,true}){
#ifndef GO_NAMES_LIVE
   if(repaired){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto sql:NameMigrations)WorldDatabase.DirectExecute(sql);WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
   sObjectMgr->LoadGameObjectTemplate();
   for(auto const& row:NameRows){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info,"Native object definition missing");
#ifdef GO_NAMES_LIVE
    auto expected=Unhex(row.after);
#else
    auto expected=Unhex(repaired?row.after:row.before);
#endif
    Require(info->name==expected,"New300 native loader name differs from declared bytes");
    for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){auto wire=Packet(*info,LocaleConstant(locale),false);auto normalized=Packet(*info,LocaleConstant(locale),true);auto key=std::make_pair(row.entry,locale);std::vector<uint8> bytes(normalized.contents(),normalized.contents()+normalized.size());if(!repaired)original[key]=bytes;else Require(original.at(key)==bytes,"Name delta changed other native query fields");}
    if(repaired)std::cout<<"CASE native name="<<row.entry<<" exact UTF8/locale/packet PASS\n";
   }
   for(auto const& row:NameControls){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info&&info->name==Unhex(row.after),"Previously repaired name changed");for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){auto wire=Packet(*info,LocaleConstant(locale),false);std::vector<uint8> bytes(wire.contents(),wire.contents()+wire.size());auto key=std::make_pair(row.entry,locale);if(!repaired)controls[key]=bytes;else Require(controls.at(key)==bytes,"Prior100 query packet changed");}}
  }
  auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Native final session not read-only");HotfixDatabase.Close();WorldDatabase.Close();
#ifdef GO_NAMES_LIVE
  std::cout<<"PASS300 native ObjectMgr names, actual live UTF8 bytes, all locales/native7.3.5 query packets; previous100 complete packets unchanged; source use/client rendering unproven where marked\n";
#else
  std::cout<<"PASS300 native ObjectMgr names, paired old/repaired input, all locales/native7.3.5 query packets; previous100 complete packets unchanged; source use/client rendering unproven where marked\n";
#endif
  return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}

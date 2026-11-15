#define main RetainedGoNameProbeMain
#include "GameObjectNameNative.cpp"
#undef main
static std::vector<uint8> Signature(WorldPacket wire,uint32 mask)
{
 std::vector<uint8> bytes(wire.contents(),wire.contents()+wire.size());if(!mask)return bytes;
 wire.rpos(0);uint32 v;wire>>v;wire.ReadBit();wire.ResetBitPos();wire>>v>>v>>v;
 std::string text;for(unsigned i=0;i<4;++i)wire>>text;wire>>text;
 std::vector<std::pair<size_t,size_t>> ranges;
 auto start=wire.rpos();wire>>text;if(mask&1)ranges.push_back({start,wire.rpos()-1});
 start=wire.rpos();wire>>text;if(mask&2)ranges.push_back({start,wire.rpos()-1});
 // Stats byte size necessarily follows the changed UTF8 string lengths.
 for(unsigned i=5;i<9;++i)bytes[i]=0;
 for(auto i=ranges.rbegin();i!=ranges.rend();++i)bytes.erase(bytes.begin()+i->first,bytes.begin()+i->second);
 return bytes;
}
static uint32 Mask(uint32 entry){for(auto const& r:OpenRows)if(r.entry==entry)return r.mask;return 0;}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{
 Require(argc==3,"Expected config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Pools failed");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectLocales();
#ifndef OPEN_GO_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE open_seed LIKE gameobject_template");WorldDatabase.DirectExecute("INSERT INTO open_seed SELECT * FROM gameobject_template");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template LIKE open_seed");WorldDatabase.DirectExecute("INSERT INTO gameobject_template SELECT * FROM open_seed");for(auto const& sql:OpenMigrations)WorldDatabase.DirectExecute(sql.c_str());
 for(auto const& row:OpenRows)WorldDatabase.DirectExecute((std::string("UPDATE gameobject_template SET castBarCaption=CONVERT(UNHEX('")+row.oldCap+"') USING utf8mb4),unk1=CONVERT(UNHEX('"+row.oldUnk+"') USING utf8mb4) WHERE entry="+std::to_string(row.entry)).c_str());
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 std::map<std::pair<uint32,uint8>,std::vector<uint8>> before,control;
 for(bool repaired:{false,true}){
#ifndef OPEN_GO_LIVE
  if(repaired){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto const& sql:OpenMigrations)WorldDatabase.DirectExecute(sql.c_str());WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sObjectMgr->LoadGameObjectTemplate();
  for(auto const& row:OpenRows){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info,"Open object missing");
#ifdef OPEN_GO_LIVE
   bool fixed=true;
#else
   bool fixed=repaired;
#endif
   Require(info->castBarCaption==Unhex(fixed?row.newCap:row.oldCap)&&info->unk1==Unhex(fixed?row.newUnk:row.oldUnk),"Actual caption/auxiliary bytes differ");for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){auto bytes=Signature(Packet(*info,LocaleConstant(locale),false),row.mask);auto key=std::make_pair(row.entry,locale);if(!repaired)before[key]=bytes;else Require(before.at(key)==bytes,"Undeclared query field changed");}if(repaired)std::cout<<"OPEN "<<row.entry<<" PASS\n";
  }
  for(auto const& row:Controls){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info&&info->name==Unhex(row.after),"Prior553 name changed");for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){auto bytes=Signature(Packet(*info,LocaleConstant(locale),false),Mask(row.entry));auto key=std::make_pair(row.entry,locale);if(!repaired)control[key]=bytes;else Require(control.at(key)==bytes,"Prior query data changed outside declared open fields");}}
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Final session not read-only");HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS37 native caption/auxiliary bytes on35 rows, locale/query lengths and all other wire data;553 existing names retained; no client UI-role/rendering claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

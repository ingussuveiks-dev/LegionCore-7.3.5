#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <string>
static std::string Hex(std::string const& s){char const*h="0123456789ABCDEF";std::string out;for(unsigned char c:s){out+=h[c>>4];out+=h[c&15];}return out;}
int main(int argc,char**argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(WorldDatabase.Open()||!WorldDatabase.PrepareStatements()||HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());if(!texts.Load(argv[1],0))return 4;texts.LoadFromDB();
 auto rows=WorldDatabase.Query("SELECT CreatureID,GroupID,ID,Text,BroadcastTextID FROM creature_text g WHERE BroadcastTextID<>0 AND (SELECT COUNT(*) FROM creature_text t WHERE t.CreatureID=g.CreatureID AND t.GroupID=g.GroupID AND t.ID=g.ID)=1 ORDER BY CreatureID,GroupID,ID");
 if(!rows)return 5;unsigned count=0;
 do{auto f=rows->Fetch();auto b=texts.LookupEntry(f[4].GetUInt32());if(!b)continue;auto s=f[3].GetString();std::string native=b->Text->Str[0],female=b->Text1->Str[0];
  bool ascii=true;for(unsigned char c:s)if(c>=128)ascii=false;for(unsigned char c:native)if(c>=128)ascii=false;
  if(s.empty()||native.empty()||s==native||(!female.empty()&&female!=native)||!ascii)continue;
  std::cout<<"ROW\t"<<f[0].GetUInt32()<<'\t'<<unsigned(f[1].GetUInt8())<<'\t'<<unsigned(f[2].GetUInt8())<<'\t'<<b->ID<<'\t'<<Hex(s)<<'\t'<<Hex(native)<<'\t'<<unsigned(b->LanguageID)<<'\n';++count;
 }while(rows->NextRow());
 auto gossip=WorldDatabase.Query("SELECT MenuID,OptionID,OptionText,OptionBroadcastTextID,BoxText,BoxBroadcastTextID FROM gossip_menu_option WHERE OptionBroadcastTextID<>0 OR BoxBroadcastTextID<>0 ORDER BY MenuID,OptionID");
 if(gossip)do{auto f=gossip->Fetch();for(unsigned k:{2u,4u}){auto b=texts.LookupEntry(f[k+1].GetUInt32());if(!b)continue;std::string s=f[k].GetString(),native=b->Text->Str[0],female=b->Text1->Str[0];bool ascii=true;for(unsigned char c:s)if(c>=128)ascii=false;for(unsigned char c:native)if(c>=128)ascii=false;if(s.empty()||native.empty()||s==native||(!female.empty()&&female!=native)||!ascii)continue;std::cout<<"GOSSIP\t"<<f[0].GetUInt32()<<'\t'<<f[1].GetUInt32()<<'\t'<<(k==2?"OptionText":"BoxText")<<'\t'<<b->ID<<'\t'<<Hex(s)<<'\t'<<Hex(native)<<'\n';}}while(gossip->NextRow());
 WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();std::cout<<"COUNT\t"<<count<<'\n';return 0;
}

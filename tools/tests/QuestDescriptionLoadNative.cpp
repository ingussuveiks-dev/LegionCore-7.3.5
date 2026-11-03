#include "Common.h"
#include "DB2Structure.h"
#include "ObjectMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "QuestDef.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "QuestTextSelect.inc"

namespace {
void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
struct Texts{std::string LogTitle,LogDescription,QuestDescription,AreaDescription,QuestCompletionLog,PortraitGiverText,PortraitGiverName,PortraitTurnInText,PortraitTurnInName;};
struct Packet{Texts Info;};
struct Session{LocaleConstant locale;LocaleConstant GetSessionDbLocaleIndex(){return locale;}};
struct Store{QuestTemplateLocale const* locale;QuestTemplateLocale const* GetQuestLocale(uint32){return locale;}};
#undef sQuestDataStore
#define sQuestDataStore (&store)
Packet Select(Quest const* quest,QuestTemplateLocale const* localized,LocaleConstant locale){
 Session session{locale};Session* _session=&session;Store store{localized};
 #include "QuestTextPacket.inc"
 return packet;
}
#undef sQuestDataStore
}

int main(int,char**){
 try{
  auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private native config missing");std::string error;
  Require(sConfigMgr->LoadInitial(config,{},error),"Private native config load failed");
  MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
  Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements(),"World native connection failed");
  WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
  auto readOnly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readOnly&&readOnly->Fetch()[0].GetBool(),"Native read-only connection required");
  auto index=[](std::string const& name){for(uint32 i=0;i<QuestTextColumns.size();++i)if(QuestTextColumns[i]==name)return i;throw std::runtime_error("Native column missing");};
  bool counterfactual=std::getenv("LEGION_QUEST_TEXT_COUNTERFACTUAL")&&std::string(std::getenv("LEGION_QUEST_TEXT_COUNTERFACTUAL"))=="1";
  uint32 rows=0,cases=0,mismatches=0;
  for(uint32 mode=0;mode<4;++mode){
  auto query=std::string(QuestTextSelect);
  auto expression=[&](char const* column,char const* sql){std::string token=std::string(", ")+column+",";auto at=query.find(token);Require(at!=std::string::npos&&query.find(token,at+token.size())==std::string::npos,"Native text projection ambiguous");query.replace(at,token.size(),std::string(", ")+sql+" AS "+column+",");};
  // Read-only SELECT expressions exercise distinct, long UTF-8 and NULL
  // fields through real MySQL Field storage and the complete constructor.
  if(mode){expression("LogDescription",mode==2?"NULL":"CONVERT(0x53686F72742D73686F756C642D737461792D73686F72742DC4AB USING utf8mb4)");expression("QuestDescription",mode==3?"NULL":"REPEAT('Long description $n$b ',150)");}
  query+=" WHERE ID IN(12103,40518,40385,40419,40729,42125,43153) ORDER BY ID";
  auto result=WorldDatabase.Query(query.c_str());Require(bool(result),"Native quest sample missing");
  do{
   auto fields=result->Fetch();Quest quest(fields);++rows;
   if(rows==1){auto offset=[&](std::string const& field){return reinterpret_cast<uintptr_t>(&field)-reinterpret_cast<uintptr_t>(&quest);};std::cout<<"LAYOUT\t"<<sizeof(Field)<<'\t'<<index("LogTitle")<<'\t'<<offset(quest.LogTitle)<<'\t'<<offset(quest.LogDescription)<<'\t'<<offset(quest.QuestDescription)<<'\t'<<offset(quest.AreaDescription)<<'\t'<<offset(quest.QuestCompletionLog)<<'\t'<<offset(quest.PortraitGiverText)<<'\t'<<offset(quest.PortraitGiverName)<<'\t'<<offset(quest.PortraitTurnInText)<<'\t'<<offset(quest.PortraitTurnInName)<<'\n';}
   Require(quest.GetQuestId()==fields[index("ID")].GetUInt32(),"Quest ID shifted");
   Require(quest.AllowableRaces==fields[index("AllowableRaces")].GetUInt64(),"64-bit race mask shifted");
   Require(quest.MaxScalingLevel==fields[index("QuestMaxScalingLevel")].GetInt32(),"Trailing scaling field shifted");
   Require(quest.StartScript==fields[index("StartScript")].GetUInt32()&&quest.CompleteScript==fields[index("CompleteScript")].GetUInt32(),"Trailing scripts shifted");
   Texts expected;
   #include "QuestTextExpected.inc"
   auto check=[&](Texts const& actual,Texts const& wanted,Texts const& fallback){
    #include "QuestTextCheck.inc"
    ++cases;
   };
   Texts loaded;
   #include "QuestTextLoaded.inc"
   check(loaded,expected,expected);
   auto plain=Select(&quest,nullptr,LOCALE_enUS);check(plain.Info,expected,expected);
   QuestTemplateLocale translated;
   #include "QuestTextTranslation.inc"
   auto overlay=Select(&quest,&translated,LOCALE_enUS);Texts overlaid;
   #include "QuestTextOverlay.inc"
   check(overlay.Info,overlaid,expected);
   auto absent=Select(&quest,&translated,LOCALE_ruRU);check(absent.Info,expected,expected);
   // Empty localized fields retain each corresponding base field; they do
   // not exchange the short summary and the full description.
   translated.LogDescription[LOCALE_enUS].clear();translated.QuestDescription[LOCALE_enUS].clear();
   overlaid.LogDescription=expected.LogDescription;overlaid.QuestDescription=expected.QuestDescription;
   auto partial=Select(&quest,&translated,LOCALE_enUS);check(partial.Info,overlaid,expected);
  }while(result->NextRow());
  }
  Require(rows==28,"Expected seven native quests in four field-value contexts");Require(counterfactual?mismatches>0:mismatches==0,"Native text load did not match expected repaired/counterfactual behavior");
  std::cout<<(counterfactual?"EXPECTED COUNTERFACTUAL":"PASS")<<" "<<rows<<" full native Quest constructors / "<<cases<<" text-load and actual query-packet selection contexts / "<<mismatches<<" reversed fallback fields; original SQL projection, real Field accessors, all nine text roles, locale overlays/empty/absent fallback, 64-bit race mask and trailing fields. No database/character writes or client traversal.\n";
  WorldDatabase.Close();MySQL::Library_End();return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}

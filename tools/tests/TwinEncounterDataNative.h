// Raw protocol guards operate only on verified connection-local TEMP tables.
// Expected SQL refusal remains visible without passing it to a world log sink.
#include <mysql.h>
static char const* const TwinEncounterSql[]={
#include "TwinEncounterSql.inc"
};
static void VerifyTwinEncounterSql(){
 MySQLConnectionInfo info(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""));
 for(uint32 mode=0;mode<7;++mode){
  std::unique_ptr<MYSQL,decltype(&mysql_close)> raw(mysql_init(nullptr),&mysql_close);Require(bool(raw),"CAPI allocation");
  Require(mysql_real_connect(raw.get(),info.host.c_str(),info.user.c_str(),info.password.c_str(),info.database.c_str(),std::strtoul(info.port_or_socket.c_str(),nullptr,10),nullptr,0),"Private world CAPI connection");
  auto Execute=[&](char const* sql){Require(!mysql_query(raw.get(),sql),"TEMP SQL fixture statement failed");};
  auto Query=[&](char const* sql){Execute(sql);std::unique_ptr<MYSQL_RES,decltype(&mysql_free_result)> result(mysql_store_result(raw.get()),&mysql_free_result);Require(bool(result),"TEMP query result");std::vector<std::vector<std::string>> rows;while(auto row=mysql_fetch_row(result.get())){std::vector<std::string> fields;for(uint32 i=0;i<mysql_num_fields(result.get());++i)fields.emplace_back(row[i]?row[i]:"NULL");rows.push_back(std::move(fields));}return rows;};
  Execute("CREATE TEMPORARY TABLE twin_encounter_seed LIKE instance_encounters");
  Execute("INSERT INTO twin_encounter_seed SELECT * FROM instance_encounters WHERE entry IN(709,715,716)");
  Execute("CREATE TEMPORARY TABLE instance_encounters LIKE twin_encounter_seed");
  auto ddl=Query("SHOW CREATE TABLE instance_encounters");Require(ddl.size()==1&&ddl[0][1].find("CREATE TEMPORARY TABLE")!=std::string::npos,"SQL repair target must be TEMP before DML");
  Execute("INSERT INTO instance_encounters SELECT * FROM twin_encounter_seed");
  Execute("DELETE FROM instance_encounters WHERE entry=715");
  Execute("INSERT INTO instance_encounters(entry,difficulty,creditType,creditEntry,lastEncounterDungeon,comment) VALUES(715,3,0,15276,0,'Preserve custom comment')");
  if(mode==1)Execute("DELETE FROM instance_encounters WHERE entry=715");
  if(mode==2)Execute("UPDATE instance_encounters SET difficulty=-1 WHERE entry=715");
  if(mode==3)Execute("UPDATE instance_encounters SET difficulty=14 WHERE entry=715");
  if(mode==4)Execute("UPDATE instance_encounters SET creditEntry=15275 WHERE entry=715");
  if(mode==5)Execute("UPDATE instance_encounters SET creditType=1 WHERE entry=715");
  if(mode==6)Execute("INSERT INTO instance_encounters SELECT entry,-1,creditType,creditEntry,lastEncounterDungeon,comment FROM instance_encounters WHERE entry=715");
  auto before=Query("SELECT * FROM instance_encounters ORDER BY entry,difficulty,creditEntry");
  auto unrelated=Query("SELECT * FROM instance_encounters WHERE entry<>715 ORDER BY entry,difficulty,creditEntry");
  uint32 error=0;std::string message;
  for(auto sql:TwinEncounterSql)if(mysql_query(raw.get(),sql)){error=mysql_errno(raw.get());message=mysql_error(raw.get());break;}
  if(mode>=3){
   Require(error==1644&&message.find("unexpected encounter data")!=std::string::npos,"Unknown/duplicate row must be refused by actual SQL guard");
   Require(Query("SELECT * FROM instance_encounters ORDER BY entry,difficulty,creditEntry")==before,"Refused SQL changed TEMP state");
   std::cout<<"CONTROL expected SQL refusal mode="<<mode<<" errno="<<error<<" message="<<message<<'\n';
  }else{
   Require(!error,"Allowed/missing native715 row refused");
   auto target=Query("SELECT difficulty,creditType,creditEntry,lastEncounterDungeon FROM instance_encounters WHERE entry=715");Require(target==std::vector<std::vector<std::string>>{{"-1","0","15276","0"}},"Wrong native715 result");
   if(mode!=1)Require(Query("SELECT comment FROM instance_encounters WHERE entry=715")[0][0]=="Preserve custom comment","Migration overwrote custom comment");
   auto first=Query("SELECT * FROM instance_encounters ORDER BY entry,difficulty,creditEntry");for(auto sql:TwinEncounterSql)Execute(sql);
   Require(Query("SELECT * FROM instance_encounters ORDER BY entry,difficulty,creditEntry")==first,"Second migration is not idempotent");
   Require(Query("SELECT * FROM instance_encounters WHERE entry<>715 ORDER BY entry,difficulty,creditEntry")==unrelated,"Migration changed unrelated encounter rows");
  }
 }
 std::cout<<"PASS native SQL715:actual legacy/missing/already-correct TEMP states,repeat/comment/unrelated preservation,four explicit1644refusals before mutation;no persistent world/character writes\n";
}

// Expected refusals stay visible through the real CAPI response. Every DML
// target is a verified connection-private TEMP table; credentials never print.
#include <mysql.h>
static void VerifyMeljarakSql(){
 MySQLConnectionInfo info(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""));
 for(uint32 mode=0;mode<5;++mode){
  std::unique_ptr<MYSQL,decltype(&mysql_close)> raw(mysql_init(nullptr),&mysql_close);Require(bool(raw),"Raw SQL allocation");Require(mysql_real_connect(raw.get(),info.host.c_str(),info.user.c_str(),info.password.c_str(),info.database.c_str(),std::strtoul(info.port_or_socket.c_str(),nullptr,10),nullptr,0),"Private raw SQL connection");
  auto Execute=[&](char const* sql){Require(!mysql_query(raw.get(),sql),"SQL fixture statement failed");};
  auto Query=[&](char const* sql){Execute(sql);std::unique_ptr<MYSQL_RES,decltype(&mysql_free_result)> result(mysql_store_result(raw.get()),&mysql_free_result);Require(bool(result),"Raw SQL result");std::vector<std::vector<std::string>> rows;while(auto row=mysql_fetch_row(result.get())){std::vector<std::string> fields;for(uint32 i=0;i<mysql_num_fields(result.get());++i)fields.emplace_back(row[i]?row[i]:"NULL");rows.push_back(std::move(fields));}return rows;};
  Execute("CREATE TEMPORARY TABLE meljarak_condition_seed LIKE conditions");Execute("INSERT INTO meljarak_condition_seed SELECT * FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(122354,125873,209457)");Execute("CREATE TEMPORARY TABLE conditions LIKE meljarak_condition_seed");
  auto ddl=Query("SHOW CREATE TABLE conditions");Require(ddl.size()==1&&ddl[0][1].find("CREATE TEMPORARY TABLE")!=std::string::npos,"SQL target must be TEMP before DML");Execute("INSERT INTO conditions SELECT * FROM meljarak_condition_seed");Execute("DELETE FROM conditions WHERE SourceEntry IN(122354,125873)");
  if(mode){for(auto sql:MeljarakSql)Execute(sql);Execute("UPDATE conditions SET Comment='Keep reviewed custom comment' WHERE SourceEntry=122354");}
  if(mode==2)Execute("DELETE FROM conditions WHERE SourceEntry=125873");
  if(mode==3)Execute("UPDATE conditions SET ConditionValue2=299 WHERE SourceEntry=125873");
  if(mode==4)Execute("INSERT INTO conditions SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,1,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment FROM conditions WHERE SourceEntry=125873");
  auto before=Query("SELECT * FROM conditions ORDER BY SourceEntry,ElseGroup");auto unrelated=Query("SELECT * FROM conditions WHERE SourceEntry NOT IN(122354,125873) ORDER BY SourceEntry");uint32 error=0;std::string message;
  for(auto sql:MeljarakSql)if(mysql_query(raw.get(),sql)){error=mysql_errno(raw.get());message=mysql_error(raw.get());break;}
  if(mode>=3){Require(error==1644&&message.find("unexpected target conditions")!=std::string::npos,"Unknown/duplicate condition must be explicitly refused");Require(Query("SELECT * FROM conditions ORDER BY SourceEntry,ElseGroup")==before,"Refused SQL mutated state");std::cout<<"CONTROL expected SQL refusal mode="<<mode<<" errno="<<error<<" message="<<message<<std::endl;}
  else{Require(!error,"Allowed condition state refused");Require(Query("SELECT SourceEntry,SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceEntry IN(122354,125873) ORDER BY SourceEntry")==std::vector<std::vector<std::string>>{{"122354","3","31","0","3","62397"},{"125873","7","31","0","3","62397"}},"Wrong real effect-mask/entry binding");auto first=Query("SELECT * FROM conditions ORDER BY SourceEntry,ElseGroup");for(auto sql:MeljarakSql)Execute(sql);Require(Query("SELECT * FROM conditions ORDER BY SourceEntry,ElseGroup")==first,"Migration repeat overwrote comment/duplicated conditions");Require(Query("SELECT * FROM conditions WHERE SourceEntry NOT IN(122354,125873) ORDER BY SourceEntry")==unrelated,"Unrelated conditions changed");}
 }
 std::cout<<"PASS real SQL five TEMP states:missing/existing/partial,repeat/comment/unrelated preserved,two explicit1644wrong/duplicate refusals,no persistent writes\n";
}

#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <mysql.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
static void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
int main(int,char**){try{
 std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Private SQL config missing");
 MySQL::Library_Init();auto connection=sConfigMgr->GetStringDefault("WorldDatabaseInfo","");WorldDatabase.SetConnectionInfo(connection,1,1);Require(!WorldDatabase.Open(),"Native SQL pool open failed");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 uint32 count=0;
 for(auto text:{"0","1","2147483647","2147483648","4294967295","4294967296","4294967297","5000000000","9999999999","10000000000","1099511627793","9223372036854775807","9223372036854775808","18446744073709551615"}){
  uint64 expected=std::strtoull(text,nullptr,10);auto sql=std::string("SELECT CAST(")+text+" AS UNSIGNED),"+text+",CAST("+text+" AS UNSIGNED)="+text;
  auto row=WorldDatabase.Query(sql.c_str());Require(row&&row->GetFieldCount()==3,"Native result missing");auto fields=row->Fetch();
#ifdef CODEX_SQL_CAST_BASELINE
  if(expected==4294967296ull){Require(fields[0].GetUInt64()==0&&fields[1].GetUInt64()==expected&&fields[2].GetBool(),"Pinned native CAST counterfactual changed");++count;}
#else
  Require(fields[0].GetUInt64()==expected&&fields[1].GetUInt64()==expected&&fields[2].GetBool(),"Native Field64 result truncated");++count;
#endif
 }
 MySQLConnectionInfo info(connection);MYSQL* raw=mysql_init(nullptr);Require(raw,"Native CAPI allocation");Require(mysql_real_connect(raw,info.host.c_str(),info.user.c_str(),info.password.c_str(),info.database.c_str(),std::strtoul(info.port_or_socket.c_str(),nullptr,10),nullptr,0),"Native CAPI connect");
 Require(!mysql_query(raw,"SET SESSION TRANSACTION READ ONLY"),"Native CAPI readonly session");
 MYSQL_STMT* statement=mysql_stmt_init(raw);Require(statement,"Native prepared allocation");
 char const sql[]="SELECT CAST(4294967296 AS UNSIGNED)";Require(!mysql_stmt_prepare(statement,sql,sizeof(sql)-1),"Native prepared parse");
 MYSQL_RES* metadata=mysql_stmt_result_metadata(statement);Require(metadata,"Native prepared metadata");auto field=mysql_fetch_fields(metadata);
#ifdef CODEX_SQL_CAST_BASELINE
 Require(field[0].type==MYSQL_TYPE_LONG,"Original unsigned32 metadata missing");
#else
 Require(field[0].type==MYSQL_TYPE_LONGLONG,"Repaired unsigned64 metadata missing");
#endif
 uint64 result=0;MYSQL_BIND binding{};binding.buffer_type=MYSQL_TYPE_LONGLONG;binding.buffer=&result;binding.buffer_length=sizeof(result);binding.is_unsigned=true;
 Require(!mysql_stmt_bind_result(statement,&binding)&&!mysql_stmt_execute(statement)&&!mysql_stmt_store_result(statement),"Native binary result setup");Require(mysql_stmt_fetch(statement)==0,"Native binary result fetch");
#ifdef CODEX_SQL_CAST_BASELINE
 Require(result==0&&count==1,"Original binary protocol narrowing missing");
 std::cout<<"PASS expected-broken native Field64/text and binary prepared unsigned32 metadata truncate4294967296 while numeric comparison remains true\n";
#else
 Require(result==4294967296ull&&count==14,"Repaired binary protocol narrowed");
 std::cout<<"PASS actual core Field64 across14 integer boundaries and native binary prepared unsigned64 metadata/result4294967296;session readonly,no character writes\n";
#endif
 mysql_free_result(metadata);mysql_stmt_close(statement);mysql_close(raw);WorldDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

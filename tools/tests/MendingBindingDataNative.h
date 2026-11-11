// Real CAPI refusal/readback on verified private TEMP copies only.
static void VerifyMendingSql(){
 MySQLConnectionInfo info(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""));
 for(uint32 mode=0;mode<4;++mode){
  std::unique_ptr<MYSQL,decltype(&mysql_close)> raw(mysql_init(nullptr),&mysql_close);Require(bool(raw),"Mending raw SQL allocation");Require(mysql_real_connect(raw.get(),info.host.c_str(),info.user.c_str(),info.password.c_str(),info.database.c_str(),std::strtoul(info.port_or_socket.c_str(),nullptr,10),nullptr,0),"Mending private SQL connection");
  auto Execute=[&](char const*sql){Require(!mysql_query(raw.get(),sql),"Mending SQL fixture statement failed");};
  auto Query=[&](char const*sql){Execute(sql);std::unique_ptr<MYSQL_RES,decltype(&mysql_free_result)> result(mysql_store_result(raw.get()),&mysql_free_result);Require(bool(result),"Mending SQL result");std::vector<std::vector<std::string>> rows;while(auto row=mysql_fetch_row(result.get())){std::vector<std::string> fields;for(uint32 i=0;i<mysql_num_fields(result.get());++i)fields.emplace_back(row[i]?row[i]:"NULL");rows.push_back(std::move(fields));}return rows;};
  Execute("CREATE TEMPORARY TABLE mending_binding_seed LIKE spell_script_names");Execute("INSERT INTO mending_binding_seed SELECT * FROM spell_script_names WHERE spell_id IN(122147,105984)");Execute("CREATE TEMPORARY TABLE spell_script_names LIKE mending_binding_seed");auto ddl=Query("SHOW CREATE TABLE spell_script_names");Require(ddl.size()==1&&ddl[0][1].find("CREATE TEMPORARY TABLE")!=std::string::npos,"Mending DML table must be TEMP");Execute("INSERT INTO spell_script_names SELECT * FROM mending_binding_seed");Execute("DELETE FROM spell_script_names WHERE spell_id=122147");
  if(mode==1||mode==3)for(auto sql:MendingSql)Execute(sql);
  if(mode>=2)Execute("INSERT INTO spell_script_names VALUES(122147,'keep_custom_mending')");
  auto before=Query("SELECT * FROM spell_script_names ORDER BY spell_id,ScriptName");auto unrelated=Query("SELECT * FROM spell_script_names WHERE spell_id<>122147 ORDER BY spell_id,ScriptName");uint32 error=0;std::string message;
  for(auto sql:MendingSql)if(mysql_query(raw.get(),sql)){error=mysql_errno(raw.get());message=mysql_error(raw.get());break;}
  if(mode>=2){Require(error==1644&&message.find("custom spell bindings")!=std::string::npos,"Custom Mending binding was not refused");Require(Query("SELECT * FROM spell_script_names ORDER BY spell_id,ScriptName")==before,"Refused Mending SQL changed data");std::cout<<"CONTROL expected Mending SQL refusal mode="<<mode<<" errno="<<error<<" message="<<message<<'\n';}
  else{Require(!error,"Allowed Mending SQL refused");Require(Query("SELECT * FROM spell_script_names WHERE spell_id=122147")==std::vector<std::vector<std::string>>{{"122147","spell_meljarak_mending_target"}},"Wrong Mending binding");auto first=Query("SELECT * FROM spell_script_names ORDER BY spell_id,ScriptName");for(auto sql:MendingSql)Execute(sql);Require(Query("SELECT * FROM spell_script_names ORDER BY spell_id,ScriptName")==first&&Query("SELECT * FROM spell_script_names WHERE spell_id<>122147 ORDER BY spell_id,ScriptName")==unrelated,"Mending repeat or unrelated binding changed");}
 }
 std::cout<<"PASS Mending SQL missing/existing/repeat/unrelated and two explicit custom-binding1644refusals;TEMP only\n";
}

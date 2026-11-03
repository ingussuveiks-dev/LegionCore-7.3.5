#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include <filesystem>
#include <cstdlib>
#include <set>
#include <sstream>
int main(int argc,char**argv)
{
 try
 {
  Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
  auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();
  std::string runtime=directory.parent_path().parent_path().string()+'/';char* arguments[]={argv[0],config,runtime.data()};InitializeNative(3,arguments);
  auto rows=WorldDatabase.Query("SELECT guid,id,map,PhaseId FROM conversation ORDER BY guid");Require(bool(rows),"Missing conversation inventory");
  std::set<uint32> ids,spells,covered;uint32 spawns=0,effects=0,actions=0;
  do{auto f=rows->Fetch();ids.insert(f[1].GetUInt32());++spawns;std::cout<<"SPAWN\t"<<f[0].GetUInt64()<<'\t'<<f[1].GetUInt32()<<'\t'<<f[2].GetUInt16()<<'\t'<<f[3].GetString()<<'\n';}while(rows->NextRow());
  for(uint32 id=0;id<sSpellMgr->GetSpellInfoStoreSize();++id)if(auto spell=sSpellMgr->GetSpellInfo(id))
   for(uint32 i=0;i<MAX_SPELL_EFFECTS;++i)if(auto effect=spell->Effects[i])
    if(effect->Effect==SPELL_EFFECT_SUMMON_CONVERSATION&&ids.count(effect->MiscValue))
    {covered.insert(effect->MiscValue);spells.insert(id);++effects;std::cout<<"EFFECT\t"<<effect->MiscValue<<'\t'<<id<<'\t'<<i<<'\t'<<effect->TargetA.GetTarget()<<'\t'<<effect->TargetB.GetTarget()<<'\n';}
  auto list=[](std::set<uint32> const& values){std::ostringstream out;bool first=true;for(auto id:values){if(!first)out<<',';first=false;out<<id;}return out.str();};
  std::string query="SELECT entryorguid,source_type,id,event_type,action_type,action_param1,target_type FROM smart_scripts WHERE (action_type=206 AND action_param1 IN("+list(ids)+"))";
  if(!spells.empty())query+=" OR (action_type=11 AND action_param1 IN("+list(spells)+"))";
  query+=" ORDER BY entryorguid,source_type,id";
  if(auto scripts=WorldDatabase.Query(query.c_str()))do{auto f=scripts->Fetch();std::cout<<"SMART";for(uint32 i=0;i<7;++i)std::cout<<'\t'<<f[i].GetInt64();std::cout<<'\n';++actions;}while(scripts->NextRow());
  std::cout<<"SUMMARY\t"<<spawns<<'\t'<<ids.size()<<'\t'<<covered.size()<<'\t'<<effects<<'\t'<<spells.size()<<'\t'<<actions<<'\n';
  std::cout<<"PASS complete native SpellInfo219/World dynamic-source candidate inventory; existence is not executable admission, duplicate playback or client proof\n";
  WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
 }
 catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}

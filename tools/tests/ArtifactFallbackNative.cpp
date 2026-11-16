// Actual Legion loaders, text builder, menu builder and packet serializers.
// No client, socket, AI, quest credit, character/account connection or save.
#include <Windows.h>
#include <unordered_set>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "CreatureTextMgr.h"
#include "GossipDef.h"
#include "GossipData.h"
#include "QuestData.h"
#include "ChatPackets.h"
#include "NPCPackets.h"
#include "Log.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <map>
#include <tuple>
#include <algorithm>
#include "ArtifactFallbackRows.inc"
#include "NativeCreatureTextBuilder.inc"
static void Require(bool ok,char const* message){if(!ok)throw std::runtime_error(message);}
static std::string Unhex(char const* t){std::string s;auto v=[](char c){return c<='9'?c-'0':c-'A'+10;};for(size_t i=0;t[i];i+=2){Require(t[i+1]!=0,"Odd hex");s+=char((v(t[i])<<4)|v(t[i+1]));}return s;}
static std::vector<uint8> Bytes(WorldPacket const& p){return {p.contents(),p.contents()+p.size()};}
static void Body(WorldPacket const& p,std::string const& text){if(text.empty())return;std::vector<uint8> body(text.begin(),text.end());Require(std::search(p.contents(),p.contents()+p.size(),body.begin(),body.end())!=p.contents()+p.size(),"Serialized text missing");}
static WorldPacket ChatWire(CreatureTextEntry const& t,uint8 gender,LocaleConstant locale,bool fallback,bool omit){
 CreatureTextBuilder builder(nullptr,gender,t.type,t.text,fallback?0:t.BroadcastTextID,t.lang,nullptr);std::unique_ptr<WorldPackets::Chat::Chat> chat(builder(locale));
 auto expected=fallback?t.text:DB2Manager::GetBroadcastTextValue(sBroadcastTextStore.LookupEntry(t.BroadcastTextID),locale,gender);Require(chat->ChatText==expected,"Native chat text selection differs");if(omit)chat->ChatText.clear();WorldPacket p(*chat->Write());if(!omit)Body(p,expected);return p;
}
static WorldPacket MenuWire(MenuRow const& row,GossipMenuItems const& stored,LocaleConstant locale,bool omit){
 GossipMenu menu;menu.SetLocale(locale);menu.SetMenuId(row.menu);menu.AddMenuItem(row.menu,row.id,0x1234,0x5678);Require(menu.GetMenuItems().size()==1,"Actual menu builder did not select one item");auto const& m=menu.GetMenuItems().begin()->second;
 auto expected=stored.OptionText,box=stored.BoxText;if(locale!=LOCALE_none){expected=DB2Manager::GetBroadcastTextValue(sBroadcastTextStore.LookupEntry(row.broadcast),locale);if(auto b=sBroadcastTextStore.LookupEntry(row.boxBroadcast))box=DB2Manager::GetBroadcastTextValue(b,locale);}
 Require(m.Message==expected&&m.BoxMessage==box&&uint32(m.OptionNpc)==row.npc&&m.BoxMoney==row.money&&m.IsCoded==row.coded&&m.Sender==0x1234&&m.Action==0x5678,"Native menu text/action/box selection differs");
 WorldPackets::NPC::GossipMessage packet;packet.GossipID=row.menu;WorldPackets::NPC::ClientGossipOptions option;option.ClientOption=menu.GetMenuItems().begin()->first;option.OptionCost=m.BoxMoney;option.Text=omit?"":m.Message;option.Confirm=m.BoxMessage;option.OptionNPC=m.OptionNpc;option.OptionFlags=m.IsCoded;packet.GossipOptions.push_back(option);WorldPacket p(*packet.Write());if(!omit)Body(p,expected);return p;
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{
 Require(argc==3,"Expected private config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native DB pools failed");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sQuestDataStore->LoadPointsOfInterest();
#ifndef ARTIFACT_FALLBACK_LIVE
 for(auto const& s:ShadowSetup)WorldDatabase.DirectExecute(s.c_str());
 for(auto table:{"creature_text","gossip_menu_option"}){auto r=WorldDatabase.Query((std::string("SHOW CREATE TABLE ")+table).c_str());Require(r&&r->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Shadow table not temporary");}
 for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());
 for(auto const& r:TextRows)WorldDatabase.DirectExecute((std::string("UPDATE creature_text SET Text=CONVERT(0x")+r.before+" USING utf8mb4) WHERE CreatureID="+std::to_string(r.entry)+" AND GroupID="+std::to_string(r.group)+" AND ID="+std::to_string(r.id)+" AND HEX(Text)='"+r.after+"'").c_str());
 for(auto const& r:MenuRows)WorldDatabase.DirectExecute((std::string("UPDATE gossip_menu_option SET OptionText=CONVERT(0x")+r.before+" USING utf8mb4) WHERE MenuID="+std::to_string(r.menu)+" AND OptionID="+std::to_string(r.id)+" AND HEX(OptionText)='"+r.after+"'").c_str());
#endif
 std::map<std::tuple<uint32,uint8,uint8,uint8,bool>,std::vector<uint8>> chatMeta,broadcasts;std::map<std::tuple<uint32,uint32,uint8>,std::vector<uint8>> menuMeta,menuBroadcast;
 for(bool repaired:{false,true}){
#ifndef ARTIFACT_FALLBACK_LIVE
  if(repaired)for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());
#endif
  sCreatureTextMgr->LoadCreatureTexts();sGossipDataStore->LoadGossipMenuItems();
  for(auto const& r:TextRows){auto const& map=sCreatureTextMgr->GetTextMap();auto a=map.find(r.entry);Require(a!=map.end(),"Missing actor");auto g=a->second.find(uint8(r.group));Require(g!=a->second.end()&&g->second.size()==1,"Native group shape changed");auto const& t=g->second.front();
#ifdef ARTIFACT_FALLBACK_LIVE
   auto expected=Unhex(r.after);
#else
   auto expected=Unhex(repaired?r.after:r.before);
#endif
   Require(t.text==expected&&t.creatureId==r.entry&&t.group==r.group&&t.id==r.id&&uint32(t.type)==r.type&&uint32(t.lang)==r.lang&&uint32(t.emote)==r.emote&&t.duration==r.duration&&t.sound==r.sound&&t.BroadcastTextID==r.broadcast&&t.MinTimer==r.minTimer&&t.MaxTimer==r.maxTimer&&t.SpellID==r.spell&&t.probability==r.probability,"Text identity/voice/timing metadata differs");
   auto bc=sBroadcastTextStore.LookupEntry(r.broadcast);Require(bc,"Native Broadcast missing");for(uint8 gender:{uint8(0),uint8(1)})Require(DB2Manager::GetBroadcastTextValue(bc,LOCALE_enUS,gender)==Unhex(r.after),"Default native Broadcast target differs");
   for(uint8 locale=0;locale<TOTAL_LOCALES;++locale)for(uint8 gender:{uint8(0),uint8(1)})for(bool fallback:{false,true}){auto k=std::make_tuple(r.entry,uint8(r.group),locale,gender,fallback);auto wire=Bytes(ChatWire(t,gender,LocaleConstant(locale),fallback,false)),meta=Bytes(ChatWire(t,gender,LocaleConstant(locale),fallback,true));if(!repaired){chatMeta[k]=meta;if(!fallback)broadcasts[k]=wire;}else{Require(chatMeta.at(k)==meta,"Chat packet metadata changed");if(!fallback)Require(broadcasts.at(k)==wire,"Normal Broadcast chat packet changed");}}
   if(repaired)std::cout<<"TEXT "<<r.entry<<':'<<r.group<<" BC"<<r.broadcast<<" PASS\n";
  }
  for(auto const& r:MenuRows){auto bounds=sGossipDataStore->GetGossipMenuItemsMapBounds(r.menu);GossipMenuItems const* t=nullptr;for(auto i=bounds.first;i!=bounds.second;++i)if(i->second.OptionIndex==r.id){Require(!t,"Duplicate native menu item");t=&i->second;}Require(t,"Native menu missing");
#ifdef ARTIFACT_FALLBACK_LIVE
   auto expected=Unhex(r.after);
#else
   auto expected=Unhex(repaired?r.after:r.before);
#endif
   Require(t->OptionText==expected&&t->MenuID==r.menu&&t->OptionIndex==r.id&&uint32(t->OptionNpc)==r.npc&&t->OptionBroadcastTextID==r.broadcast&&t->BoxBroadcastTextID==r.boxBroadcast&&t->ActionMenuID==r.actionMenu&&t->ActionPoiID==r.poi&&t->BoxMoney==r.money&&t->BoxCoded==r.coded&&t->BoxText==Unhex(r.box),"Stored native menu metadata differs");
   auto bc=sBroadcastTextStore.LookupEntry(r.broadcast);Require(bc&&!bc->ConditionID,"Missing or conditional Broadcast needs separate review");for(uint8 gender:{uint8(0),uint8(1)})Require(DB2Manager::GetBroadcastTextValue(bc,LOCALE_enUS,gender)==Unhex(r.after),"Native menu Broadcast target differs");
   for(uint8 locale=0;locale<=TOTAL_LOCALES;++locale){auto loc=locale==TOTAL_LOCALES?LOCALE_none:LocaleConstant(locale);auto k=std::make_tuple(r.menu,r.id,locale);auto wire=Bytes(MenuWire(r,*t,loc,false)),meta=Bytes(MenuWire(r,*t,loc,true));if(!repaired){menuMeta[k]=meta;if(loc!=LOCALE_none)menuBroadcast[k]=wire;}else{Require(menuMeta.at(k)==meta,"Menu packet metadata changed");if(loc!=LOCALE_none)Require(menuBroadcast.at(k)==wire,"Normal Broadcast menu packet changed");}}
   if(repaired)std::cout<<"MENU "<<r.menu<<':'<<r.id<<" BC"<<r.broadcast<<" PASS\n";
  }
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"World session not read-only");HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS100 native artifact fallbacks:80 speech/20 menus,actual loaders/builders/serializers,all locales and speech genders;normal Broadcast packets preserved;no client or whole-quest claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

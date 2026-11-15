// Matching native loaders and serializers. No session, socket or character save.
#define main RetainedNameMain
#include "GameObjectNameNative.cpp"
#undef main
#include "CreatureTextMgr.h"
#include "ChatPackets.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "NPCPackets.h"
#include "QuestDef.h"
#include "QuestPackets.h"
#include <memory>
#include <algorithm>
#include <sstream>
#include "NativeCreatureTextBuilder.inc"
static std::vector<uint8> Bytes(WorldPacket const* p){return {p->contents(),p->contents()+p->size()};}
static std::string Normalize(std::string s){for(auto const& r:RestoreRows)if(s==Unhex(r.after))return Unhex(r.before);return s;}
static std::unique_ptr<Quest> QuestRow(uint32 id){auto r=WorldDatabase.Query((std::string(QuestQuery)+std::to_string(id)).c_str());Require(bool(r),"Quest missing");return std::make_unique<Quest>(r->Fetch());}
static std::vector<uint8> GossipWire(GossipMenuItem const& item,bool mask){WorldPackets::NPC::GossipMessage p;WorldPackets::NPC::ClientGossipOptions o;o.ClientOption=0;o.OptionNPC=item.OptionNpc;o.OptionFlags=item.IsCoded;o.OptionCost=item.BoxMoney;o.Text=mask?"":item.Message;o.Confirm=item.BoxMessage;p.GossipOptions.emplace_back(o);return Bytes(p.Write());}
static std::vector<uint8> RequestWire(Quest const& q,bool mask){WorldPackets::Quest::QuestGiverRequestItems p;p.QuestID=q.GetQuestId();p.QuestTitle=q.LogTitle;p.CompEmoteType=q.EmoteOnIncomplete;p.CompEmoteDelay=q.EmoteOnIncompleteDelay;p.QuestFlags[0]=q.GetFlags();p.QuestFlags[1]=q.FlagsEx;p.CompletionText=mask?"":q.RequestItemsText;return Bytes(p.Write());}
static void Contains(std::vector<uint8> const& p,std::string const& s){std::vector<uint8> body(s.begin(),s.end());Require(!s.empty()&&std::search(p.begin(),p.end(),body.begin(),body.end())!=p.end(),"Serialized UTF8 absent");}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{
 Require(argc==3,"Expected config/runtime");std::string e;Require(sConfigMgr->LoadInitial(argv[1],{},e),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"DB failed");
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();
 // TEMP shadowing narrows loader inputs without altering persistent data.
 for(auto const& s:SetupSql)WorldDatabase.DirectExecute(s.c_str());
#ifndef NEXT_TEXT_LIVE
 for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());for(auto const& s:ResetSql)WorldDatabase.DirectExecute(s.c_str());
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sGossipDataStore->LoadGossipMenuItemsLocales();
 std::map<std::string,std::vector<uint8>> packets;std::map<uint32,std::string> groups;
 auto compare=[&](std::string const& k,std::vector<uint8> const& v,bool fixed){if(!fixed)packets[k]=v;else Require(packets.at(k)==v,"Unrelated native packet changed");};
 for(bool fixed:{false,true}){
#ifndef NEXT_TEXT_LIVE
  if(fixed){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sCreatureTextMgr->LoadCreatureTexts();sGossipDataStore->LoadGossipMenuItems();Require(sObjectMgr->LoadTrinityStrings(),"String loader failed");uint32 found=0;
  for(auto const& gr:GroupRows){auto const& map=sCreatureTextMgr->GetTextMap();auto actor=map.find(gr.entry);Require(actor!=map.end(),"Actor text missing");auto gi=actor->second.find(gr.group);Require(gi!=actor->second.end(),"Group missing");std::ostringstream signature;uint32 index=0;
   for(auto const& t:gi->second){signature<<index<<':'<<t.creatureId<<':'<<uint32(t.group)<<':'<<uint32(t.id)<<':'<<t.duration<<':'<<t.sound<<':'<<t.BroadcastTextID<<':'<<t.MinTimer<<':'<<t.MaxTimer<<':'<<t.SpellID<<':'<<t.probability<<':'<<uint32(t.type)<<':'<<uint32(t.lang)<<':'<<uint32(t.emote)<<':'<<Normalize(t.text)<<'\n';
    for(auto const& r:RestoreRows)if(std::string(r.table)=="creature_text"&&r.a==gr.entry&&r.b==gr.group&&Normalize(t.text)==Unhex(r.before)){
#ifdef NEXT_TEXT_LIVE
     auto expected=Unhex(r.after);
#else
     auto expected=Unhex(fixed?r.after:r.before);
#endif
     Require(t.text==expected,"Native NPC text differs");++found;
    }
    for(uint8 locale=0;locale<TOTAL_LOCALES;++locale)for(uint8 gender:{uint8(0),uint8(1)})for(bool fallback:{false,true}){CreatureTextBuilder builder(nullptr,gender,t.type,t.text,fallback?0:t.BroadcastTextID,t.lang,nullptr);std::unique_ptr<WorldPackets::Chat::Chat> p(builder(LocaleConstant(locale)));auto expected=t.text;if(!fallback)if(auto b=sBroadcastTextStore.LookupEntry(t.BroadcastTextID))expected=DB2Manager::GetBroadcastTextValue(b,LocaleConstant(locale),gender);Require(p->ChatText==expected,"Native chat fallback changed");auto k="ct:"+std::to_string(gr.entry)+":"+std::to_string(gr.group)+":"+std::to_string(index)+":"+std::to_string(locale)+":"+std::to_string(gender)+":"+std::to_string(fallback);auto whole=Bytes(p->Write());if(!fallback&&t.BroadcastTextID)compare(k+"bc",whole,fixed);CreatureTextBuilder empty(nullptr,gender,t.type,"",0,t.lang,nullptr);std::unique_ptr<WorldPackets::Chat::Chat> metadata(empty(LocaleConstant(locale)));compare(k,Bytes(metadata->Write()),fixed);}
    ++index;
   }
   uint32 k=gr.entry*256+gr.group;if(!fixed)groups[k]=signature.str();else Require(groups.at(k)==signature.str(),"Native dialogue order, implicit ID, timing or metadata changed");
  }
  Require(found==49,"Not all49 text rows traversed");
  for(auto const& r:RestoreRows){std::string table=r.table;if(table=="creature_text")continue;
#ifdef NEXT_TEXT_LIVE
   auto expected=Unhex(r.after);
#else
   auto expected=Unhex(fixed?r.after:r.before);
#endif
   auto key=table+":"+std::to_string(r.a)+":"+std::to_string(r.b);
   if(table=="gossip_menu_option"){
    auto bounds=sGossipDataStore->GetGossipMenuItemsMapBounds(r.a);GossipMenuItems const* source=nullptr;for(auto i=bounds.first;i!=bounds.second;++i)if(i->second.OptionIndex==r.b)source=&i->second;Require(source&&source->OptionText==expected,"Native gossip source differs");
    for(int locale=-1;locale<TOTAL_LOCALES;++locale){GossipMenu menu;menu.SetLocale(locale==-1?LOCALE_none:LocaleConstant(locale));menu.AddMenuItem(r.a,r.b,77,88);auto item=menu.GetItem(0);Require(item&&item->Sender==77&&item->Action==88,"Native gossip action lost");auto chosen=expected;if(locale!=-1&&locale!=LOCALE_none){if(auto b=sBroadcastTextStore.LookupEntry(source->OptionBroadcastTextID))chosen=DB2Manager::GetBroadcastTextValue(b,LocaleConstant(locale));else if(auto l=sGossipDataStore->GetGossipMenuItemsLocale(MAKE_PAIR32(r.a,r.b)))ObjectMgr::GetLocaleString(l->OptionText,LocaleConstant(locale),chosen);}Require(item->Message==chosen,"Native gossip locale/BC selection differs");Contains(GossipWire(*item,false),chosen);compare(key+std::to_string(locale),GossipWire(*item,true),fixed);}
   }else if(table=="quest_request_items"){
    auto q=QuestRow(r.a);auto data=WorldDatabase.PQuery("SELECT ID,EmoteOnComplete,EmoteOnIncomplete,EmoteOnCompleteDelay,EmoteOnIncompleteDelay,CompletionText FROM quest_request_items WHERE ID=%u",r.a);Require(bool(data),"Request source missing");q->LoadQuestRequestItems(data->Fetch());Require(q->RequestItemsText==expected,"Native request text differs");Contains(RequestWire(*q,false),expected);compare(key,RequestWire(*q,true),fixed);
   }else if(table=="quest_objectives"){
    auto q=QuestRow(45807);auto data=WorldDatabase.PQuery("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE ID=%u",r.a);Require(bool(data),"Objective source missing");q->LoadQuestObjective(data->Fetch());Require(q->Objectives.size()==1&&q->Objectives[0].Description==expected,"Native objective text differs");WorldPackets::Quest::QueryQuestInfoResponse p;p.Allow=true;p.QuestID=q->GetQuestId();p.Info.Objectives=q->Objectives;Contains(Bytes(p.Write()),expected);WorldPackets::Quest::QueryQuestInfoResponse m;m.Allow=true;m.QuestID=q->GetQuestId();m.Info.Objectives=q->Objectives;m.Info.Objectives[0].Description.clear();compare(key,Bytes(m.Write()),fixed);
   }else{Require(std::string(sObjectMgr->GetTrinityString(r.a,LOCALE_ruRU))==expected,"Native system locale differs");for(uint8 l=0;l<TOTAL_LOCALES;++l)if(l!=LOCALE_ruRU){auto s=std::string(sObjectMgr->GetTrinityString(r.a,LocaleConstant(l)));compare(key+std::to_string(l),{s.begin(),s.end()},fixed);}}
  }
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"World not read-only");HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS87 native dialogue order/implicit IDs, voice/timing, Broadcast/locale/gender, gossip actions and request/objective serializers; no socket/client/save claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

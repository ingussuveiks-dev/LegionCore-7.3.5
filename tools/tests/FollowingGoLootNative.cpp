// Actual Player::SendLoot, native template/loot loaders. No inventory receipt,
// client opening-lock validation, real spawn traversal or collision claim.
#include "NativeFollowingGoLoot.inc"
#include <memory>
#include <sstream>
#include "AchievementMgr.h"
template<class Tag,typename Tag::Type member>struct Accessor{friend typename Tag::Type Access(Tag){return member;}};
struct LoadTag{using Type=uint32(LootStore::*)();friend Type Access(LoadTag);};template struct Accessor<LoadTag,&LootStore::LoadLootTable>;
struct Chest:GameObject{
 Chest(Map& map,uint32 entry,uint32 guid,Position const& pos){Require(Create(guid,entry,&map,1,pos,G3D::Quat(0,0,0,1),255,GO_STATE_READY),"Native chest creation failed");Require(map.AddToMap(static_cast<GameObject*>(this)),"Native chest admission failed");Update(5000);Require(getLootState()==GO_READY,"Native chest initialization differs");}
 ~Chest(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
static void Init(int argc,char** argv){
 Require(argc==3,"Expected config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements()&&!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Pools failed");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 for(auto const& s:SetupSql)WorldDatabase.DirectExecute(s.c_str());
#ifndef FOLLOWING_GO_LIVE
 for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());for(auto const& s:ResetSql)WorldDatabase.DirectExecute(s.c_str());
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sDB2Manager.LoadStores(argv[2],LOCALE_enUS);LoadGameTables(argv[2]);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadPageTexts();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadPersonalLootTemplate();sAchievementMgr->LoadCriteriaList();
 auto count=(LootTemplates_Gameobject.*Access(LoadTag{}))();Require(count==LootRowCount,"Native authored chest loot rows failed validation");for(auto e:Entries)Require(LootTemplates_Gameobject.HaveLootFor(e)&&!sObjectMgr->GetPersonalLootData(e),"Native source/alternate personal source differs");opcodeTable.Initialize();AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();HashMapHolder<GameObject>::SetSize(20000);HashMapHolder<Player>::SetSize(1000);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}}});
}
static std::string Metadata(GameObjectTemplate const& info){std::ostringstream s;s<<info.entry<<':'<<info.type<<':'<<info.displayId<<':'<<info.name<<':'<<info.IconName<<':'<<info.castBarCaption<<':'<<info.unk1<<':'<<info.size<<':'<<info.RequiredLevel<<':'<<info.flags<<':'<<info.faction<<':'<<info.MinGold<<':'<<info.MaxGold<<':'<<info.ScriptId<<':'<<info.AIName;for(auto item:info.QuestItems)s<<':'<<item;for(uint32 i=0;i<MAX_GAMEOBJECT_DATA;++i)if(i!=1)s<<':'<<info.raw.data[i];return s.str();}
static Loot* Earned(Player& player,GameObject& go){return go.IsPersonal()?&player.personalLoot.at(go.GetGUID()):&go.loot;}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{SetUnhandledExceptionFilter(CrashTrace);Init(argc,argv);std::map<uint32,std::string> before;uint32 guid=1000;
 for(bool fixed:{false,true}){
#ifndef FOLLOWING_GO_LIVE
  if(fixed){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sObjectMgr->LoadGameObjectTemplate();auto session=new WorldSession(0,"Following chest readonly native probe",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
  // Socketless session destructor updates account.online; reclaim at process
  // exit, as in the retained LostArmy/LootLink native tests. Login stays readonly.
  {Map map(1220,300,9999967,DIFFICULTY_NONE);NativePlayer owner(session,map,101,{5478.f,470.f,150.f,0});map.GetZoneAndAreaId(owner.m_zoneId,owner.m_areaId,owner.GetPositionX(),owner.GetPositionY(),owner.GetPositionZ());owner.m_mailsLoaded=true; // Declared empty synthetic mailbox; never query a real character.
   auto pos=owner.GetPosition();auto money=owner.GetMoney();
   for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}){owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);
    for(auto entry:Entries){auto info=sObjectMgr->GetGameObjectTemplate(entry);Require(info&&info->type==GAMEOBJECT_TYPE_CHEST,"Native template missing");
#ifdef FOLLOWING_GO_LIVE
     bool repaired=true;
#else
     bool repaired=fixed;
#endif
     Require(info->GetLootId()==(repaired?entry:0),"Native selector did not use repaired Data1");if(!fixed)before[entry]=Metadata(*info);else Require(before.at(entry)==Metadata(*info),"Unrelated native template/lock/quest/personal fields changed");
     {Chest distant(map,entry,guid++,pos);owner.Relocate(pos.GetPositionX()+20.f,pos.GetPositionY(),pos.GetPositionZ(),0);owner.SendLoot(distant.GetGUID(),LOOT_CORPSE);Require(!distant.loot.GetLootOwner()&&!owner.personalLoot.count(distant.GetGUID()),"Native distance denial bypassed");owner.Relocate(pos);owner.SetLootGUID(ObjectGuid::Empty);}
     {Chest source(map,entry,guid++,pos);owner.SendLoot(source.GetGUID(),LOOT_CORPSE);Loot* earned=Earned(owner,source);Require((earned->GetLootOwner()==&owner)==repaired,"Actual Player::SendLoot did not change from empty to authored source");Require(source.getLootState()==GO_ACTIVATED,"Native open state changed");if(fixed&&race==RACE_HUMAN)std::cout<<"GO "<<entry<<" personal="<<source.IsPersonal()<<" PASS\n";owner.SetLootGUID(ObjectGuid::Empty);owner.personalLoot.erase(source.GetGUID());owner.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_LOOTING);Require(owner.GetMoney()==money,"Generation granted money");for(uint8 slot=EQUIPMENT_SLOT_START;slot<owner.GetInventoryEndSlot();++slot)Require(!owner.GetItemByPos(INVENTORY_SLOT_BAG_0,slot),"Generation stored an inventory item");}
    }
   }
  }
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"World not readonly");LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS100 native actual Player::SendLoot old/fixed/live selector and authored loot owner,2 factions/distance denials and nonlink raw/query/flag/script metadata; no client/lock/placement/drop/inventory claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}

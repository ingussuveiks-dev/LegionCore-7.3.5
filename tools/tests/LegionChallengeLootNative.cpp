// Native reward endpoint -> placed chest -> SendLoot -> StoreLootItem/reopen.
// Controlled completion; no earned boss/forces or client traversal claim.
#define CHALLENGE_COMPLETION_SUPPORT_ONLY
#include "LegionChallengeCompletionNative.cpp"
#include "DisableMgr.h"
#include "LootMgr.h"
#include <fstream>

static std::string Numbers(std::set<uint32> const& values){std::string result;for(auto id:values){if(!result.empty())result+=',';result+=std::to_string(id);}return result;}
static void RepairChestShadow(){
 auto file=std::getenv("LEGION_CHEST_MIGRATION");Require(file,"Exact guarded chest migration unavailable");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE completion_go_seed AS SELECT * FROM gameobject_template");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template LIKE completion_go_seed");WorldDatabase.DirectExecute("INSERT INTO gameobject_template SELECT * FROM completion_go_seed");
 auto proof=WorldDatabase.Query("SHOW CREATE TABLE gameobject_template");Require(proof&&proof->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Chest repair must target a TEMP shadow");
 for(unsigned replay=0;replay<2;++replay){std::ifstream stream(file);Require(bool(stream),"Chest migration file missing");std::string line;while(std::getline(stream,line))if(!line.empty()&&line.rfind("--",0)!=0)WorldDatabase.DirectExecute(line.c_str());}
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sObjectMgr->LoadGameObjectTemplate();
}
static void LoadChestLoot(){
 std::set<uint32> ids;for(uint32 entry:{252674u,252677u,252686u,252668u,252665u,252056u,252680u,252671u,252683u,269852u,269871u,269843u,272689u}){auto info=sObjectMgr->GetGameObjectTemplate(entry);std::cout<<"CATALOG_CHEST entry="<<entry<<" exists="<<bool(info)<<" loot="<<(info?info->GetLootId():0)<<" encounter="<<(info?info->chest.DungeonEncounter:0)<<std::endl;Require(info&&info->GetLootId(),"Native chest loot link missing");ids.insert(info->GetLootId());}
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE completion_loot_seed AS SELECT * FROM gameobject_loot_template WHERE Entry IN("+Numbers(ids)+")").c_str());
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_loot_template LIKE completion_loot_seed");WorldDatabase.DirectExecute("INSERT INTO gameobject_loot_template SELECT * FROM completion_loot_seed");
 std::set<uint32> references;auto rows=WorldDatabase.Query("SELECT DISTINCT Reference FROM gameobject_loot_template WHERE Reference>0");if(rows)do{references.insert(rows->Fetch()[0].GetUInt32());}while(rows->NextRow());
 bool changed=true;while(changed&&!references.empty()){auto before=references.size();rows=WorldDatabase.Query(("SELECT DISTINCT Reference FROM reference_loot_template WHERE Entry IN("+Numbers(references)+") AND Reference>0").c_str());if(rows)do{references.insert(rows->Fetch()[0].GetUInt32());}while(rows->NextRow());changed=before!=references.size();}
 WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE completion_reference_seed AS SELECT * FROM reference_loot_template WHERE Entry IN("+(references.empty()?"0":Numbers(references))+")").c_str());
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE reference_loot_template LIKE completion_reference_seed");WorldDatabase.DirectExecute("INSERT INTO reference_loot_template SELECT * FROM completion_reference_seed");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 auto conditions=WorldDatabase.Query(("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup IN("+Numbers(ids)+")").c_str());Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"Load changed native chest loot conditions before testing");
 LootIdSet loaded;if(!references.empty())LootTemplates_Reference.LoadAndCollectLootIds(loaded);loaded.clear();Require(LootTemplates_Gameobject.LoadAndCollectLootIds(loaded)>0,"Actual chest loot loader empty");Check(loaded==ids,"Native loaded chest loot scope differs");
}
static std::string Receipt(Loot const& loot){std::ostringstream out;out<<loot.gold<<':'<<loot.unlootedCount;for(auto const& item:loot.items){out<<':'<<item.item.ItemID<<','<<item.count<<','<<item.is_looted<<','<<item.item.RandomPropertiesSeed<<','<<unsigned(item.item.ItemBonus.Context);for(auto bonus:item.item.ItemBonus.BonusListIDs)out<<','<<bonus;}return out.str();}
static void OpenStore(LifecyclePlayer& owner,CompletionChest& chest,Challenge const& challenge){
 owner.SetPrimarySpecialization(254);owner.SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID,254);owner.SetLootSpecID(254);
 for(uint32 skill:{43u,44u,45u,46u,54u,136u,162u,172u,173u,226u,229u,413u})owner.SetSkill(skill,0,800,800);
 owner.UpdatePosition(chest.GetPositionX(),chest.GetPositionY(),chest.GetPositionZ(),chest.GetOrientation(),true);
 std::cout<<"OPEN_INPUT map="<<owner.GetMapId()<<" entry="<<chest.GetEntry()<<" distance="<<owner.GetExactDist(&chest)<<" scope="<<chest.CanTargetQuestObject(&owner)<<" spawned="<<chest.isSpawnedByDefault()<<" respawn="<<chest.GetRespawnTime()<<" personal="<<chest.IsPersonal()<<" found="<<bool(owner.GetMap()->GetGameObject(chest.GetGUID()))<<std::endl;
 unsigned expectedKeys=!owner.m_challengeKeyInfo.IsActive()&&!sChallengeMgr->HasOploteLoot(owner.GetGUID())?1:0;
 owner.SendLoot(chest.GetGUID(),LOOT_CORPSE);auto found=owner.personalLoot.find(chest.GetGUID());Require(found!=owner.personalLoot.end(),"Native SendLoot did not publish personal container");auto& loot=found->second;
 Require(loot.GetLootOwner()==&owner&&!loot.items.empty(),"Native completed chest generated no usable personal loot");auto first=Receipt(loot);
 unsigned keys=0;for(auto const& item:loot.items)if(item.type==LOOT_ITEM_TYPE_ITEM&&item.item.ItemID==138019)keys+=item.count;Check(keys==expectedKeys,"Template key row replaced gear or duplicated the native eligible keystone grant");
 unsigned gear=0;for(auto const& item:loot.items)if(item.type==LOOT_ITEM_TYPE_ITEM){auto info=sObjectMgr->GetItemTemplate(item.item.ItemID);if(item.item.ItemID!=138019&&info&&info->GetQuality()!=ITEM_QUALITY_LEGENDARY&&!info->IsOtherDrops()&&!info->IsRecipe())gear+=item.count;}if(gear!=challenge.GetItemCount(owner.GetGUID())){std::cout<<"GEAR_MISMATCH actual="<<gear<<" expected="<<unsigned(challenge.GetItemCount(owner.GetGUID()))<<std::endl;for(auto const& item:loot.items){auto info=sObjectMgr->GetItemTemplate(item.item.ItemID);std::cout<<"LOOT_ITEM id="<<item.item.ItemID<<" count="<<item.count<<" quality="<<(info?unsigned(info->GetQuality()):99)<<" class="<<(info?unsigned(info->GetClass()):99)<<" inv="<<(info?unsigned(info->GetInventoryType()):99)<<std::endl;}}Check(gear==challenge.GetItemCount(owner.GetGUID()),"Actual chest gear count differs from the native completion entitlement");
 std::cout<<"CHEST entry="<<chest.GetEntry()<<" encounter="<<chest.GetGOInfo()->chest.DungeonEncounter<<" items="<<loot.items.size()<<" gold="<<loot.gold<<'\n';
 owner.SetLootGUID(ObjectGuid::Empty);owner.SendLoot(chest.GetGUID(),LOOT_CORPSE);Check(Receipt(loot)==first,"Reopening an earned personal chest rerolled its contents");
 uint8 slot=0;while(slot<loot.items.size()&&(loot.items[slot].type!=LOOT_ITEM_TYPE_ITEM||loot.items[slot].is_looted))++slot;Require(slot<loot.items.size(),"Native chest has no item slot to store");
 auto entry=loot.items[slot].item.ItemID;auto count=loot.items[slot].count;auto before=owner.GetItemCount(entry,true);
 std::vector<uint16> fillers;for(unsigned n=0;n<200;++n){ItemPosCountVec where;auto result=owner.CanStoreNewItem(NULL_BAG,NULL_SLOT,where,25,1);if(result!=EQUIP_ERR_OK){Check(result==EQUIP_ERR_INV_FULL,"Native bag filler failed for a reason other than full inventory");break;}Require(!where.empty(),"Native bag filler has no position");auto item=owner.StoreNewItem(where,25,true);Require(item,"Actual bag filler storage failed");fillers.push_back(where.front().pos);}
 Require(!fillers.empty()&&fillers.size()<200,"Native full-bag fixture did not terminate");auto full=Receipt(loot);owner.StoreLootItem(slot,&loot);Check(Receipt(loot)==full&&owner.GetItemCount(entry,true)==before,"Full bags consumed or awarded an unreceived chest item");owner.SetLootGUID(ObjectGuid::Empty);owner.SendLoot(chest.GetGUID(),LOOT_CORPSE);Check(Receipt(loot)==full,"Full-bag reopen rerolled pending loot");
 auto freed=fillers.back();fillers.pop_back();owner.DestroyItem(uint8(freed>>8),uint8(freed),true);owner.StoreLootItem(slot,&loot);
 Check(loot.items[slot].is_looted&&owner.GetItemCount(entry,true)==before+count,"Actual native chest storage did not receive and consume item exactly once");auto stored=Receipt(loot);
 owner.SetLootGUID(ObjectGuid::Empty);owner.SendLoot(chest.GetGUID(),LOOT_CORPSE);Check(Receipt(loot)==stored,"Reopening chest regenerated an already received item");
 owner.StoreLootItem(slot,&loot);Check(owner.GetItemCount(entry,true)==before+count,"Repeated native storage duplicated an already looted item");
 for(auto pos:fillers)owner.DestroyItem(uint8(pos>>8),uint8(pos),true);
 for(uint8 index=0;index<loot.items.size();++index)if(loot.items[index].type==LOOT_ITEM_TYPE_ITEM&&loot.LootItemInSlot(index,&owner)){auto id=loot.items[index].item.ItemID;auto amount=loot.items[index].count;auto prior=owner.GetItemCount(id,true);owner.StoreLootItem(index,&loot);Check(!loot.LootItemInSlot(index,&owner)&&owner.GetItemCount(id,true)==prior+amount,"Remaining actual chest item failed native storage");}
 auto allItems=Receipt(loot);owner.SetLootGUID(ObjectGuid::Empty);owner.SendLoot(chest.GetGUID(),LOOT_CORPSE);Check(Receipt(loot)==allItems,"Reopening after all item pickups regenerated loot");
 owner.SetLootGUID(ObjectGuid::Empty);
}
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);DisableMgr::LoadDisables();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadPersonalLootTemplate();sObjectMgr->LoadScenarioData();(sQuestDataStore->*Access(QuestVectorTag{})).resize(1,nullptr);(sQuestDataStore->*Access(QuestMaxTag{}))=0;ShadowChallengeWrites();RepairChestShadow();LoadChestLoot();unsigned contexts=0;
 for(auto route:sMapChallengeModeStore)if(route->ID==197||route->ID==198||route->ID==199||route->ID==200||route->ID==206||route->ID==207||route->ID==208||route->ID==209||route->ID==210||route->ID==227||route->ID==233||route->ID==234||route->ID==239)for(bool grouped:{false,true})for(bool timed:{false,true}){
  MapInstanced parent(route->MapID,300);InstanceMap map(route->MapID,300,9966000+contexts,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);Position pose{2934.49f,711.757f,554.286f,1.55255f};LifecyclePlayer owner(map,601+contexts*2,pose);auto key=owner.Key(*route);auto member=grouped?std::make_unique<LifecyclePlayer>(map,602+contexts*2,pose):nullptr;LifecycleGroup group(owner,member.get(),*route,*key);RegisteredCompletionGroup registered(group.group,grouped,9966000+contexts);
  auto registeredScenario=sScenarioMgr->AddScenario(&map,1175);Require(registeredScenario,"Native registered loot scenario missing");auto& scenario=*registeredScenario;scenario.CreateChallenge(&owner);auto challenge=scenario.GetChallenge();Require(challenge&&challenge->_canRun,"Native loot challenge unavailable");CompletionChest chest(map,route->ID);challenge->Update(100);challenge->Update(10000);challenge->_challengeTimer=(timed?route->CriteriaCount[0]-1:route->CriteriaCount[0]+1)*1000;scenario.Reward(false,0);Require(challenge->_complete,"Actual reward endpoint did not complete run");Saved();
  OpenStore(owner,chest,*challenge);if(member)OpenStore(*member,chest,*challenge);std::cout<<"LOOT route="<<route->ID<<" party="<<grouped<<" timed="<<timed<<'\n';map.m_scenarios.erase(registeredScenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());++contexts;
 }
 Check(contexts==52,"Native loot context scope changed");std::cout<<"PASS "<<checked<<" native challenge loot checks across52 controlled completion contexts/13 routes,solo+party,timed+late;actual SendLoot/StoreLootItem/reopen. No earned dungeon/client claim\n";
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

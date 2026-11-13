// Whole production pickup and actual native180850/199827 inventory/criteria.
// Earlier victory/stage/poses and the inventory race are controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Item.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "AshbringerStorageActor.inc"
struct StorageBindingsTag {using Type=SpellScriptsContainer ScriptDataStoreMgr::*;friend Type Access(StorageBindingsTag);};
template struct FixtureAccess<StorageBindingsTag,&ScriptDataStoreMgr::_spellScriptsStore>;
struct StorageGoDbTag {using Type=uint64 GameObject::*;friend Type Access(StorageGoDbTag);};
template struct FixtureAccess<StorageGoDbTag,&GameObject::m_DBTableGuid>;
namespace StorageFault {
 bool fill=false;uint32 refuse=0;std::map<uint32,ObjectGuid> source;std::map<uint32,ObjectGuid> caster;
 void Fill(Player* player){ItemPosCountVec destination;Require(player->CanStoreNewItem(NULL_BAG,NULL_SLOT,destination,25,16)==EQUIP_ERR_OK,"Native ordinary bag seed does not fit");Require(player->StoreNewItem(destination,25,true),"Native ordinary bag seed failed");Require(player->GetItemCount(25)==16,"Native bag seed incomplete");}
 class zz_probe_ashbringer_storage : public SpellScript {
  PrepareSpellScript(zz_probe_ashbringer_storage);
  void Before(){if(GetId()==180850&&fill){Fill(GetCaster()->ToPlayer());fill=false;}}
  SpellCastResult Check(){source[GetId()]=GetSpell()->m_castGuid[1];caster[GetId()]=GetSpell()->GetOriginalCaster()?GetSpell()->GetOriginalCaster()->GetGUID():ObjectGuid::Empty;if(GetId()==refuse){refuse=0;return SPELL_FAILED_NOT_READY;}return SPELL_CAST_OK;}
  void Register() override {BeforeCast+=SpellCastFn(zz_probe_ashbringer_storage::Before);OnCheckCast+=SpellCheckCastFn(zz_probe_ashbringer_storage::Check);}
 };
 void Register(){RegisterSpellScript(zz_probe_ashbringer_storage);}
}
class StorageInstance : public InstanceMapScript {
public:StorageInstance():InstanceMapScript("instance_broken_shore",1500){}
 InstanceScript* GetInstanceScript(InstanceMap* map)const override{return new InstanceScript(map);}
};
struct StoragePlayer : NativePlayer {
 using NativePlayer::NativePlayer;
 void Prepare(uint32 race){
  SetByteValue(UNIT_FIELD_BYTES_0,0,race);SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PALADIN);setFactionForRace(race);
  auto model=sChrRacesStore.LookupEntry(race);Require(model,"Native race missing");SetNativeDisplayId(model->MaleDisplayID);SetDisplayId(model->MaleDisplayID);
  GetMap()->GetZoneAndAreaId(m_zoneId,m_areaId,GetPositionX(),GetPositionY(),GetPositionZ());
  for(uint16 field:{UNIT_FIELD_MOD_CASTING_SPEED,UNIT_FIELD_MOD_SPELL_HASTE,UNIT_FIELD_MOD_HASTE,UNIT_FIELD_MOD_HASTE_REGEN})SetFloatValue(field,1.0f);
  SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID,SPEC_PALADIN_RETRIBUTION);SetLootSpecID(SPEC_PALADIN_RETRIBUTION);
  SetSkill(SKILL_2H_SWORDS,1,1,1);
  SetInventorySlotCount(16);SetQuestStatus(38376,QUEST_STATUS_INCOMPLETE);SetQuestSlot(0,38376);
 }
 ~StoragePlayer(){for(uint8 slot=PLAYER_SLOT_START;slot<PLAYER_SLOT_END;++slot)if(auto item=GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->IsInWorld())item->RemoveFromWorld();}
};
struct StorageWeapon : GameObject {
 StorageWeapon(Map& map,Position const& pose){Require(Create(8201,247318,&map,1,pose,G3D::Quat(0,0,0,1),0,GO_STATE_READY),"Real weapon GO creation failed");this->*Access(StorageGoDbTag{})=128117;SetPhaseId({5942},false);Require(map.AddToMap(static_cast<GameObject*>(this)),"Real GO map admission failed");}
 ~StorageWeapon(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
static uint64 StorageCredit(Scenario* scenario,uint32 asset){uint64 count=0;for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap()){auto tree=sCriteriaTreeStore.LookupEntry(row.first);auto criterion=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;if(criterion&&criterion->Type==92&&criterion->Asset==asset)count+=row.second.Counter;}return count;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("conditions","SourceTypeOrReferenceId=12 AND SourceGroup=199827");Shadow("spell_proc_event","1");
#ifdef ASHBRINGER_STORAGE_BASELINE
 Shadow("spell_script_names","spell_id<>180850 OR ScriptName<>'spell_bs_ashbringer_storage'");sScriptDataStore->LoadSpellScriptNames();
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 auto& lootName=LootTemplates_Spell.*Access(LootNameTag{});lootName="spell_loot_template";Require((LootTemplates_Spell.*Access(LootLoadTag{}))()>0,"Native spell loot absent");
 sConditionMgr->LoadConditions();
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
 for(auto table:{"item_instance_artifact","item_instance","character_inventory","gameobject_respawn","creature_respawn"}){
  auto seed=std::string("ashbringer_storage_seed_")+table;
  for(auto sql:{std::string("CREATE TEMPORARY TABLE ")+seed+" LIKE "+table,std::string("CREATE TEMPORARY TABLE ")+table+" LIKE "+seed}){CharacterDatabase.DirectExecute(sql.c_str());CharacterDatabase.Execute(sql.c_str());}
  auto ddl=CharacterDatabase.Query((std::string("SHOW CREATE TABLE ")+table).c_str());Require(ddl&&ddl->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Character TEMP absent");
 }
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
#ifndef ASHBRINGER_STORAGE_BASELINE
 std::string repair="spell_bs_ashbringer_storage";
#ifdef ASHBRINGER_STORAGE_LIVE
 Require(std::binary_search(names.begin(),names.end(),repair),"Actual live storage ScriptName absent");
#else
 if(!std::binary_search(names.begin(),names.end(),repair))names.insert(std::lower_bound(names.begin(),names.end(),repair),repair);
#endif
 sScriptDataStore->LoadSpellScriptNames();
 auto bindings=sScriptDataStore->GetSpellScriptsBounds(180850);uint32 count=0;for(auto it=bindings.first;it!=bindings.second;++it)if(it->second.first==sObjectMgr->GetScriptId(repair.c_str()))++count;
#ifdef ASHBRINGER_STORAGE_LIVE
 Require(count==1,"Actual live storage binding absent/duplicated");
#else
 Require(count<=1,"Storage binding duplicated");if(!count)(sScriptDataStore->*Access(StorageBindingsTag{})).emplace(180850,std::make_pair(sObjectMgr->GetScriptId(repair.c_str()),true));
#endif
#endif
 std::string fault="zz_probe_ashbringer_storage";Require(names.back()<fault,"Fault ID shifts native names");names.push_back(fault);
 for(uint32 id:{180850u,199827u,190777u,192145u})(sScriptDataStore->*Access(StorageBindingsTag{})).emplace(id,std::make_pair(sObjectMgr->GetScriptId(fault.c_str()),true));
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new StorageInstance;new go_bs_the_ashbringer;StorageFault::Register();
#ifndef ASHBRINGER_STORAGE_BASELINE
 RegisterSpellAndAuraScriptPair(spell_bs_ashbringer_storage, aura_bs_ashbringer_storage);
#endif
 sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1500,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1500,{}}});
 auto spawn=WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE guid=128117 AND id=247318 AND map=1500");Require(spawn&&spawn->GetRowCount()==1,"Native weapon spawn absent");auto row=spawn->Fetch();Position pose{row[0].GetFloat(),row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat()};
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_DRAENEI),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<21;++mode){
#ifdef ASHBRINGER_STORAGE_BASELINE
  if(mode>1)continue;
#endif
  StorageFault::fill=false;StorageFault::refuse=0;StorageFault::source.clear();StorageFault::caster.clear();
  MapInstanced parent(1500,300);InstanceMap map(1500,300,9987000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,mode==15?933:775);scenario->SetCurrentStep(3);auto session=Session(),visitorSession=Session();
  {Position playerPose=pose;playerPose.m_positionX+=1;StoragePlayer player(session,map,904,playerPose);player.Prepare(race);player.SetPhaseId({5942},false);StorageWeapon weapon(map,pose);
   StoragePlayer visitor(visitorSession,map,905,playerPose);visitor.Prepare(race);visitor.SetPhaseId({5942},false);visitor.SetGameMaster(mode!=12);
   std::unique_ptr<CreditParty> group;if(mode==12)group=std::make_unique<CreditParty>(&player,&visitor);
   if(mode==14)player.SetPhaseId({5171},false);
   if(mode==16)player.SetQuestStatus(38376,QUEST_STATUS_NONE);
   bool negative=mode==14||mode==15||mode==16;
   Require(AshbringerAcquisition::CanRetrieve(&player,&weapon)!=negative,"Actual pickup admission differs");
   Require(player.CanGetItemForLoot(sObjectMgr->GetItemTemplate(120978),true),"Native Paladin loot eligibility missing");
   if(mode==2)StorageFault::Fill(&player);
   if(mode==3)StorageFault::refuse=199827;
   if(mode==10)StorageFault::refuse=190777;
   if(mode==19)StorageFault::refuse=192145;
   if(mode==20){Require(player.CastSpell(&player,190777,true)==SPELL_CAST_OK&&StorageCredit(scenario,46130)==1,"Native earlier receipt control failed");}
   if(mode==13||mode==18){Require(player.CastSpell(&player,199827,true)==SPELL_CAST_OK&&player.GetItemByEntry(120978),"Native owned retry preparation failed");if(mode==18){auto item=player.GetItemByEntry(120978);player.SwapItem((item->GetBagSlot()<<8)|item->GetSlot(),(INVENTORY_SLOT_BAG_0<<8)|BANK_SLOT_ITEM_START);Require(!player.GetItemByEntry(120978)&&player.GetItemCount(120978,true)==1,"Native bank preparation failed");}}
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;
   if(mode==17){pet=std::make_unique<CreditPet>(player,map);Require(pet->CastSpell(pet.get(),180850,true)!=SPELL_CAST_OK,"Pet obtained player retrieval");}
   else if(mode==11){guardian=std::make_unique<CreditGuardian>(player,map);Require(guardian->CastSpell(guardian.get(),180850,true)!=SPELL_CAST_OK,"Guardian obtained player retrieval");}
   else {
   StorageFault::fill=mode==1;weapon.SetLootState(GO_JUST_DEACTIVATED,&player);
   }
   std::cout<<"IMMEDIATE mode="<<mode<<" owned="<<(player.GetItemByEntry(120978)!=nullptr)<<" credit="<<StorageCredit(scenario,46130)<<std::endl;
   auto oldAttempt=player.getQuestStatus(38376)->ScriptLifetime;
   if(mode==4)weapon.SetLootState(GO_JUST_DEACTIVATED,&player);
   if(mode==5)player.SetQuestStatus(38376,QUEST_STATUS_NONE);
   if(mode==6)player.getQuestStatus(38376)->ScriptLifetime=std::make_shared<uint8>();
   if(mode==7)map.PlayerRelocation(&player,pose.GetPositionX()+100,pose.GetPositionY(),pose.GetPositionZ(),0);
   if(mode==8){auto item=player.GetItemByEntry(120978);Require(item,"Pending storage control missing");player.SwapItem((item->GetBagSlot()<<8)|item->GetSlot(),(INVENTORY_SLOT_BAG_0<<8)|BANK_SLOT_ITEM_START);Require(!player.GetItemByEntry(120978)&&player.GetItemCount(120978,true)==1,"Native pending bank move failed");}
   if(mode==9)player.DestroyItemCount(120978,1,true);
   player.Unit::Update(20);
   auto owned=player.GetItemByEntry(120978);auto credit=StorageCredit(scenario,46130);
   std::cout<<"OBSERVE race="<<race<<" mode="<<mode<<" owned="<<(owned!=nullptr)<<" credit46130="<<credit<<" aura180850="<<player.HasAura(180850)<<" aura192145="<<player.HasAura(192145)<<" stage="<<scenario->GetCurrentStep()<<" GOstate="<<uint32(weapon.getLootState())<<std::endl;
#ifdef ASHBRINGER_STORAGE_BASELINE
   if(mode==1)Require(!owned&&credit,"Pinned cast-OK/storage-failed premature credit not reproduced");else Require(owned&&credit,"Native successful storage control failed");
#else
   if(mode==1||mode==2||mode==3||mode==18){Require(!owned&&!credit&&!player.HasAura(180850)&&!player.HasAura(192145)&&weapon.getLootState()==GO_READY,"Storage failure progressed or stranded pickup");if(mode!=18){player.DestroyItemCount(25,16,true);weapon.SetLootState(GO_JUST_DEACTIVATED,&player);player.Unit::Update(20);Require(player.GetItemByEntry(120978)&&StorageCredit(scenario,46130)==1,"Native pickup retry failed");}}
   else if(mode==5||mode==6||mode==8||mode==9){Require(!credit&&!player.HasAura(180850)&&!player.HasAura(192145),"Ended or changed original stored attempt leaked progress/presentation");}
   else if(mode==10||mode==19){Require(owned&&(mode==10?!credit:credit==1),"Refused follow-up control differs");player.Unit::Update(1000);player.Unit::Update(20);Require(StorageCredit(scenario,46130)==1,"Native follow-up retry lost/duplicated receipt");}
   else if(negative||mode==11||mode==17){Require(!owned&&!credit&&!player.HasAura(180850),"Foreign source advanced retrieval");}
   else {Require(owned&&credit==1&&player.GetItemCount(120978)==1,"Native successful acquisition failed/duplicated");}
   Require(!visitor.GetItemByEntry(120978),"Native group source granted another player's weapon");
   if(mode==0){for(uint32 id:{199827u,190777u,192145u})Require(StorageFault::source[id].GetEntry()==180850&&StorageFault::caster[id]==player.GetGUID(),"Native trigger source cast/original owner provenance changed");}
   if(mode==0){auto aura=player.GetAura(180850);auto tick=aura?aura->GetEffect(EFFECT_6):nullptr;Require(tick&&tick->GetPeriod()>0,"Native retrieval periodic effect missing");auto guid=owned->GetGUID();player.Unit::Update(uint32(tick->GetPeriod()));player.Unit::Update(20);auto equipped=player.GetItemByEntry(120978);Require(equipped&&equipped->GetGUID()==guid&&equipped->IsEquipped(),"Native181257 did not equip the same stored weapon");std::cout<<"PASS native periodic181257 equipped same actual120978 without new grant race="<<race<<std::endl;}
#endif
   player.RemoveAllAuras();map.RemoveAllObjectsInRemoveList();CleanStories(map);
  }session->SetPlayer(nullptr);visitorSession->SetPlayer(nullptr);delete session;delete visitorSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef ASHBRINGER_STORAGE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native success/inventory-race contexts;expected-broken storage-before-credit baseline,not repaired success\n";
#else
 std::cout<<"PASS "<<cases<<" native actual inventory/storage-before-credit pickup contexts\n";
#ifdef ASHBRINGER_STORAGE_LIVE
 std::cout<<"PASS actual live180850 paired SpellScript/AuraScript binding without registry substitution\n";
#endif
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

// Native921 -> real105580 loot -> actual packet storage -> native item aura
// and54824 -> actual211689/211687 ->50536. Earlier stage1/quest/poses seeded.
#define DEVOURER_ESCAPE_SUPPORT_ONLY
#include "DevourerEscapeNative.cpp"
#include "LootPackets.h"
#include "PickPocket.inc"
struct WardstoneBags
{
 Player& player;std::vector<std::unique_ptr<Item>> items;
 explicit WardstoneBags(Player& p):player(p){p.SetInventorySlotCount(16);}
 void Fill(uint32 id){for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<player.GetInventoryEndSlot();++slot){auto item=std::make_unique<Item>();Require(item->Create(13000+id*20+slot,2589,nullptr),"Native backpack placeholder create failed");item->SetOwnerGUID(player.GetGUID());item->SetSlot(slot);(player.*Access(DoorItemsTag{}))[slot]=item.get();items.push_back(std::move(item));}}
 void FreeOne(){Require(!items.empty(),"No controlled backpack slot to release");auto& item=items.back();(player.*Access(DoorItemsTag{}))[item->GetSlot()]=nullptr;items.pop_back();}
 ~WardstoneBags(){for(auto& item:items)(player.*Access(DoorItemsTag{}))[item->GetSlot()]=nullptr;}
};
static uint8 WardstoneSlot(Loot& loot){for(uint8 i=0;i<loot.items.size();++i)if(loot.items[i].item.ItemID==136802)return i;throw std::runtime_error("Actual100% Wardstone loot absent");}
static void WardstoneStore(Player& player,Loot& loot,uint8 slot)
{
 Require(loot.GetGUID().IsLoot()&&sLootMgr->GetLoot(loot.GetGUID())==&loot,"Actual native LootResponse container not registered");
 WorldPacket wire(CMSG_LOOT_ITEM);wire<<uint32(1)<<loot.GetGUID()<<uint8(slot+1);WorldPackets::Loot::AutoStoreLootItem request(std::move(wire));request.Read();
 Require(request.Loot.size()==1&&request.Loot[0].Object==loot.GetGUID()&&request.Loot[0].LootListID==slot+1,"Actual native autostore packet decoding changed");
 player.GetSession()->HandleAutostoreLootItemOpcode(request);
}
int main(int argc,char**argv){try{
 InitializeEscapeNative(argc,argv);sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellLinked();
 auto conditions=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=8 AND SourceGroup=105580) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(921,1784,210097,211684,211686,211687,211689,63268,121308))");Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"New pocket/spell conditions require native loading before this test");
 auto areas=WorldDatabase.Query("SELECT COUNT(*) FROM spell_area WHERE spell IN(921,1784,210097,211684,211686,211687,211689,63268,121308)");Require(areas&&!areas->Fetch()[0].GetUInt32(),"New spell-area restrictions require native loading before this test");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new spell_rog_pick_pocket_probe;sScriptMgr->SwapScriptContext(true);
 Require(sScriptMgr->GetSpellScriptLoader(sObjectMgr->GetScriptId("spell_rog_pick_pocket")),"Actual native pickpocket script unregistered");
 auto& scripts=sScriptDataStore->*Access(EscapeBindingsTag{});for(uint32 id:{921u,1784u,210097u,211684u,211686u,63268u,121308u}){auto range=scripts.equal_range(id);Require(std::distance(range.first,range.second)==(id==921?1:0),"Unreviewed shared spell binding requires registration before this test");if(id==921)Require(range.first->second.first==sObjectMgr->GetScriptId("spell_rog_pick_pocket"),"Native pocket binding differs");}
 auto& name=LootTemplates_Pickpocketing.*Access(LootNameTag{});auto original=name;name+=" WHERE Entry=105580 ORDER BY Entry,Item";auto loaded=(LootTemplates_Pickpocketing.*Access(LootLoadTag{}))();name=original;Require(loaded&&LootTemplates_Pickpocketing.HaveLootFor(105580),"Actual pickpocket loot105580 not loaded");
 auto item=sObjectMgr->GetItemTemplate(136802);Require(item,"Native Wardstone template absent");bool keyAura=false;for(auto effect:item->Effects)if(effect->TriggerType==ITEM_SPELLTRIGGER_ON_NO_DELAY_USE&&effect->SpellID==210097)keyAura=true;Require(keyAura,"Actual on-obtain Cell Door Key aura missing");
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<6;++mode){
 MapInstanced parent(1607,300);InstanceMap map(1607,300,9902000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(1);auto session=Session(),otherSession=Session();
 {Position cagePose{1847.93f,1259.83f,57.0953f,0},cachePose{1870.09f,1272.14f,57.0953f,0},doorPose{1854.51f,1296.27f,62.1306f,0};NativePlayer player(session,map,101,Position{1847.77f,1259.92f,57.09f,0}),member(otherSession,map,102,cagePose);
 for(auto p:{&player,&member}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);}WardstoneBags bags(player);if(mode==1)bags.Fill(cases);std::unique_ptr<CreditParty> party;if(mode==3)party=std::make_unique<CreditParty>(&player,&member);
 Actor source(&map,105580,18100+cases,Position{1849.74f,1255.91f,57.6275f,0});Require(map.AddToMap(static_cast<Creature*>(&source))&&source.GetCreatureType()==CREATURE_TYPE_HUMANOID&&source.GetCreatureTemplate()->pickpocketLootId==105580,"Actual Soulkeeper source changed");
 player.AddAura(211684,&player);player.AddAura(211686,&player);Require(player.HasUnitState(UNIT_STATE_ROOT)&&!player.HasUnitState(UNIT_STATE_STUNNED),"Actual211684 is root26, not stun12");
 if(mode==5){player.AddAura(209238,&player);EscapeTick(player,10000);Require(player.HasUnitState(UNIT_STATE_STUNNED)&&player.CastSpell(&player,1784,false)==SPELL_FAILED_STUNNED&&!source.lootForPickPocketed,"Remaining actual Cheap Shot must prevent Stealth without loot");EscapeTick(player,11000);Require(!player.HasAura(209238)&&!player.HasUnitState(UNIT_STATE_STUNNED)&&player.HasUnitState(UNIT_STATE_ROOT),"Native Cheap Shot expiry must retain the separate cell root");}
 if(mode==2){Require(player.CastSpell(&source,921,false)==SPELL_FAILED_ONLY_SHAPESHIFT&&!source.lootForPickPocketed,"Ordinary non-Stealth refusal lost");}
 if(mode==4){CreditPet pet(player,map);CreditGuardian guardian(player,map);for(Unit* actor:{static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)})actor->CastSpell(&source,921,true);Require(!source.lootForPickPocketed&&!player.HasItemCount(136802,1),"Pet/guardian generated Player pocket loot");player.AddAura(63268,&player);}
 Require(player.CastSpell(&player,1784,false)==SPELL_CAST_OK&&player.GetShapeshiftForm()==FORM_STEALTH,"Actual native Stealth failed while rooted");
 Require(player.CastSpell(&source,921,false)==SPELL_CAST_OK&&source.lootForPickPocketed,"Actual native Pick Pocket failed");auto& loot=source.loot;uint8 slot=WardstoneSlot(loot);auto guid=loot.GetGUID();auto count=loot.unlootedCount;
 if(mode==1){WardstoneStore(player,loot,slot);Require(!player.HasItemCount(136802,1)&&!player.HasAura(210097)&&!loot.items[slot].is_looted&&loot.unlootedCount==count&&scenario->GetCurrentStep()==1,"Full bag consumed key or progressed quest");player.GetSession()->DoLootRelease(source.GetGUID());Require(player.CastSpell(&source,921,false)==SPELL_CAST_OK&&loot.GetGUID()==guid&&loot.unlootedCount==count&&!loot.items[slot].is_looted,"Reopen regenerated/consumed source loot");bags.FreeOne();}
 WardstoneStore(player,loot,slot);EscapeTick(player,100);Require(player.GetItemCount(136802)==1&&player.HasAura(210097)&&scenario->GetCurrentStep()==2&&EscapeLeaf(scenario,54243)==1,"Native storage/item aura/54824 did not advance1->2");Require(!EscapeLeaf(scenario,48002)&&!EscapeLeaf(scenario,48005)&&!DoorCredit(scenario),"Key pickup bypassed separate escape objectives");
 WardstoneStore(player,loot,slot);Require(player.GetItemCount(136802)==1&&EscapeLeaf(scenario,54243)==1,"Replay duplicated key/progress");
 DoorObject cage,cache,door,barrier;uint32 goGuid=8000+cases*4;auto create=[&](DoorObject& go,uint32 entry,Position const& pose){Require(go.Create(goGuid++,entry,&map,1,pose,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&go)),"Actual escape actor absent");go.SetLootState(GO_READY);};create(cage,249809,cagePose);create(cache,248992,cachePose);create(door,249390,doorPose);create(barrier,249391,Position{1855.38f,1292.18f,62.5271f,0});
 cage.Use(&player);EscapeTick(player,100);Require(!player.HasAura(211684)&&player.HasAura(211686)&&EscapeLeaf(scenario,48002)==1,"Actual cage release failed");player.Relocate(cachePose);cache.Use(&player);EscapeTick(player,2500);Require(!player.HasAura(211686)&&EscapeLeaf(scenario,48005)==1,"Actual two-second reclaim failed");player.Relocate(doorPose);door.Use(&player);Require(!player.HasItemCount(136802,1)&&!player.HasAura(210097)&&DoorCredit(scenario)==1&&scenario->GetCurrentStep()==3&&door.GetPhaseMask()==4&&barrier.GetPhaseMask()==4,"Actual final key consumption/aura removal/door sequence failed");
 player.CastStop();player.RemoveAllAuras();player.m_Events.KillAllEvents(true);std::cout<<"CASE race="<<race<<" mode="<<mode<<" native921/storage210097/54824/211689/211687/50536 once; mainLootLeaf="<<EscapeLeaf(scenario,47377)<<std::endl;
 }session->SetPlayer(nullptr);otherSession->SetPlayer(nullptr);delete session;delete otherSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" actual Wardstone acquisition/packet/full-bag/reopen/no-duplicate/Stealth/real group/glyph/pet/guardian contexts and whole native stage1->3 escape sequence. Earlier quest/stage1/poses/backpack and root/disarm are seeded; full capture/client traversal separate\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using SpellEffIndex=uint8;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_CANT_UPGRADE_BATTLE_PET,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_WRONG_BATTLE_PET_TYPE};
constexpr uint32 BATTLEPET_QUALITY_UNCOMMON=2,BATTLEPET_QUALITY_RARE=3,BATTLEPET_QUALITY_EPIC=4,SPELL_EFFECT_HANDLE_LAUNCH_TARGET=1,PLAYER_FIELD_SUMMONED_BATTLE_PET_GUID=1,UNIT_FIELD_BATTLE_PET_COMPANION_GUID=2,PLAYER_FIELD_CURRENT_BATTLE_PET_BREED_QUALITY=3,UNIT_FIELD_WILD_BATTLE_PET_LEVEL=4,ITEM_CHANGED=1,CHAR_DEL_CHAR_INVENTORY_BY_ITEM=1;
constexpr uint32 BATTLEPET_STATE_Stat_Power=0,BATTLEPET_STATE_Stat_Stamina=1,BATTLEPET_STATE_Stat_Speed=2,BATTLEPET_STATE_Stat_Gender=3;
uint32 errors=0;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
struct ObjectGuid{uint64 n=0;void Clear(){n=0;}bool operator==(ObjectGuid o)const{return n==o.n;}bool operator!=(ObjectGuid o)const{return !(*this==o);}};
int32 RoundingFloatValue(float f){return int32(std::round(f));}
struct BreedState{uint32 BreedID,BattlePetStateID,Value;};struct SpeciesState{uint32 SpeciesID,BattlePetStateID,Value;};
std::vector<BreedState*>sBattlePetBreedStateStore;std::vector<SpeciesState*>sBattlePetSpeciesStateStore;
struct Quality{float StateMultiplier=1.5f;};struct Qualities{Quality q;Quality*LookupEntry(uint32 id){assert(id==9||id==10);return &q;}}sBattlePetBreedQualityStore;
struct Species{uint32 PetTypeEnum=0;};struct SpeciesStore{Species s;bool missing=false;Species*LookupEntry(uint32){return missing?nullptr:&s;}Species*AssertEntry(uint32 id){return LookupEntry(id);}}sBattlePetSpeciesStore;
struct BattlePet{ObjectGuid JournalID{7};uint32 Species=1155,Breed=7,Quality=1,Level=1,XP=13,DisplayModelID=0;int32 Health=100,InfoPower=10,InfoMaxHealth=100,InfoSpeed=10,InfoGender=0;bool needSave=false;void UpdateStats();};
#include "Stats.inc"
struct Player;
struct Item{uint32 count=5;ObjectGuid owner{9};bool world=true,inUse=true;uint32 GetCount(){return count;}void SetCount(uint32 n){count=n;}bool IsInWorld(){return world;}ObjectGuid GetOwnerGUID(){return owner;}uint8 GetBagSlot(){return 0;}uint8 GetSlot(){return 1;}uint64 GetGUIDLow(){return 1;}uint32 GetEntry(){return 98715;}void SetInUse(bool b){inUse=b;}void SendUpdateToPlayer(Player*){}void SetState(uint32,Player*){}};
struct Creature{ObjectGuid owner{9},journal{7};bool world=true,alive=true;uint32 display=123;bool IsInWorld(){return world;}bool IsAlive(){return alive;}ObjectGuid GetOwnerGUID(){return owner;}ObjectGuid GetGuidValue(uint32 f){assert(f==2);return journal;}uint32 GetDisplayId(){return display;}void SetUInt32Value(uint32,uint32){assert(false);}};
struct Session{uint32 updates=0;BattlePet snapshot;void SendBattlePetUpdates(BattlePet*p=nullptr,bool added=false){assert(!added&&p);snapshot=*p;++updates;}};
struct CharacterDatabasePreparedStatement{void setUInt64(uint32,uint64){}};struct Database{CharacterDatabasePreparedStatement stmt;auto*GetPreparedStatement(uint32){assert(false);return &stmt;}void Execute(CharacterDatabasePreparedStatement*){assert(false);}}CharacterDatabase;
struct Player{
 std::shared_ptr<BattlePet>pet=std::make_shared<BattlePet>();Creature summon;Creature*shown=&summon;Session session;Item*item=nullptr;bool m_itemUpdateQueueBlocked=false;uint32 _petBattleId=0,wholeDeletes=0,removed=0;ObjectGuid selected{7};bool asPlayer=true;
 Player*ToPlayer(){return asPlayer?this:nullptr;}ObjectGuid GetGUID(){return {9};}ObjectGuid GetGuidValue(uint32 f){assert(f==1);return selected;}Creature*GetSummonedBattlePet(){return shown;}std::shared_ptr<BattlePet>GetBattlePet(ObjectGuid id){return pet&&pet->JournalID==id?pet:nullptr;}bool IsInWorld(){return true;}Session*GetSession(){return &session;}void SetUInt32Value(uint32 f,uint32 v){assert(f==3&&v==pet->Quality);}void ItemRemovedQuestCheck(uint32,uint32 n){removed+=n;}void DestroyItem(uint8,uint8,bool){++wholeDeletes;item->count=0;}void DestroyItemCount(Item*,uint32&,bool);
};
#include "Consume.inc"
struct SpellEffectInfo{float BasePoints=85;uint32 MiscValue=1023;};struct SpellInfo{SpellEffectInfo effect;SpellEffectInfo*GetEffect(uint8){return &effect;}};
struct Targets{Creature*target=nullptr;Item*item=nullptr;Creature*GetUnitTarget(){return target;}Item*GetItemTarget(){return item;}void SetItemTarget(Item*p){item=p;}};
struct Spell{Player*m_caster=nullptr;Item*m_CastItem=nullptr;SpellInfo*m_spellInfo=nullptr;Targets m_targets;uint32 effectHandleMode=1,m_castItemEntry=98715;ObjectGuid m_castItemGUID{1};uint32 failures=0;void SendCastResult(SpellCastResult){++failures;}SpellCastResult CheckBattlePetUpgrade(SpellEffIndex);void EffectUpgradeBattlePet(SpellEffIndex);void BaselineUpgrade(SpellEffIndex);void NativeTakeCastItem();};
#include "Quality.inc"
#include "Check.inc"
#include "Effect.inc"
#define EffectUpgradeBattlePet BaselineUpgrade
#include "Baseline.inc"
#undef EffectUpgradeBattlePet
void Spell::NativeTakeCastItem(){if(!m_CastItem)return;bool expendable=true,withoutCharges=true;
#include "TakeCastItem.inc"
}
namespace WorldPackets{namespace BattlePet{struct NullCmsg{uint32 size=0;uint32 GetSize(){return size;}};}}
struct WorldSession{Player*_player;void SendBattlePetUpdates(BattlePet*p){_player->session.SendBattlePetUpdates(p);}void HandleBattlePetUpdateDisplayNotify(WorldPackets::BattlePet::NullCmsg&);};
#include "Notify.inc"
struct Test{Player player;Item item;SpellInfo info;Spell spell;Test(){player.item=&item;spell.m_caster=&player;spell.m_CastItem=&item;spell.m_spellInfo=&info;spell.m_targets={&player.summon,&item};sBattlePetSpeciesStore.missing=false;sBattlePetSpeciesStore.s.PetTypeEnum=0;}void go(){assert(spell.CheckBattlePetUpgrade(0)==SPELL_CAST_OK);spell.EffectUpgradeBattlePet(0);}void success(uint32 count,uint32 level){assert(player.pet->Quality==3&&player.pet->Level==level&&player.pet->XP==13&&player.pet->needSave&&item.count==count-1);assert(player.session.updates==1&&player.session.snapshot.InfoPower==int32(15*level));assert(!spell.m_CastItem&&!spell.m_targets.item&&!item.inUse);}};
int main(){
 {Test t;t.player.pet->Quality=3;t.item.count=1;t.spell.BaselineUpgrade(0);assert(t.spell.failures==1&&t.item.count==1);t.spell.NativeTakeCastItem();assert(t.item.count==0);std::cout<<"COUNTERFACTUAL actual old effect rejects alreadyRare, then native TakeCastItem final branch consumes its one stone. Repaired prelaunch check returns failure without consumption.\n";}
 for(uint32 family=0;family<10;++family)for(uint32 level:{1u,14u,15u,25u})for(uint32 stack:{1u,5u,20u}){Test t;sBattlePetSpeciesStore.s.PetTypeEnum=family;t.player.pet->Level=level;t.item.count=stack;t.go();t.success(stack,level);t.spell.EffectUpgradeBattlePet(0);assert(t.player.session.updates==1&&t.item.count==stack-1);}
 for(int bad=0;bad<16;++bad){Test t;if(bad==0)t.player.pet.reset();if(bad==1)t.player.shown=nullptr;if(bad==2)t.player.summon.alive=false;if(bad==3)t.player.summon.world=false;if(bad==4)t.player.summon.owner={2};if(bad==5)t.player.summon.journal={8};if(bad==6)t.spell.m_targets.target=nullptr;if(bad==7)t.player._petBattleId=1;if(bad==8)t.item.owner={2};if(bad==9)t.item.world=false;if(bad==10)sBattlePetSpeciesStore.missing=true;if(bad==11)sBattlePetSpeciesStore.s.PetTypeEnum=32;if(bad==12)t.info.effect.MiscValue=0;if(bad==13)t.player.pet->Quality=3;if(bad==14)t.info.effect.BasePoints=99;if(bad==15)t.info.effect.BasePoints=1;assert(t.spell.CheckBattlePetUpgrade(0)!=SPELL_CAST_OK);assert(t.item.count==5);t.spell.EffectUpgradeBattlePet(0);assert(t.item.count==5&&!t.spell.m_CastItem&&!t.item.inUse&&!t.player.session.updates);}
 {Test t;t.info.effect.BasePoints=75;assert(t.spell.CheckBattlePetUpgrade(0)==SPELL_CAST_OK);t.spell.EffectUpgradeBattlePet(0);assert(t.player.pet->Quality==2&&t.item.count==4);}
 {Test t;WorldSession s{&t.player};WorldPackets::BattlePet::NullCmsg p;s.HandleBattlePetUpdateDisplayNotify(p);assert(t.player.pet->DisplayModelID==123&&t.player.pet->needSave&&t.player.session.updates==1);for(int i=0;i<100;++i)s.HandleBattlePetUpdateDisplayNotify(p);assert(t.player.session.updates==1);p.size=1;auto before=errors;s.HandleBattlePetUpdateDisplayNotify(p);assert(errors==before+1&&t.player.session.updates==1);t.player.summon.owner={4};p.size=0;s.HandleBattlePetUpdateDisplayNotify(p);assert(t.player.session.updates==1);t.player.shown=nullptr;s.HandleBattlePetUpdateDisplayNotify(p);assert(t.player.session.updates==1);}
 std::cout<<"PASS120 upgrades: all10 families/1,14,15,25 levels/1,5,20 stacks; native stat recomputation/one-unit decrement,16 prelaunch and late refusals,unchanged XP,exactly-once and owned display sync/malformed notification. Client traversal/persistence separate.\n";
}

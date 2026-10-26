#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
using uint32=uint32_t;
using uint8=uint8_t;
using SpellEffIndex=uint32;
using ItemPosCountVec=std::vector<uint32>;
using GuidSet=std::set<uint32>;
#include "EffectConstants.inc"
enum {ITEM_FIELD_CONTEXT=1,ITEM_QUALITY_LEGENDARY=5,EXPANSION_LEGION=6};
struct LootStore {} LootTemplates_Spell;
struct SpellEffectData {uint32 ItemType=0,MiscValueB=0,Effect=157;};
struct SpellInfo
{
    uint32 Id=199827;
    SpellEffectData effect;
    SpellEffectData* Effects[1]={&effect};
    SpellEffectData const* GetEffect(uint32,uint32)const{return &effect;}
    bool IsArchaeologyCraftingSpell()const{return false;}
    bool IsLootCrafting()const;
};
#include "LootCrafting.inc"
struct Item {uint32 GetUInt32Value(uint32){assert(false);return 0;}};
struct ItemTemplate {uint32 GetQuality()const{return 0;}uint32 GetExpansion()const{return 0;}};
struct ObjectManager {ItemTemplate const* GetItemTemplate(uint32){assert(false);return nullptr;}} objectManager;
auto* sObjectMgr=&objectManager;
struct DataManager {std::vector<uint32> GetItemBonusTree(uint32,uint32,uint32&){assert(false);return {};}} sDB2Manager;
struct Player;
struct Unit
{
    virtual ~Unit()=default;
    virtual bool IsPlayer()const{return false;}
    virtual Player* ToPlayer(){return nullptr;}
};
struct LootItem
{
    struct {uint32 ItemID=120978,RandomPropertiesID=0;struct {std::vector<uint32> BonusListIDs;uint32 Context=0;}ItemBonus;}item;
    uint32 count=1;
};
struct Player:Unit
{
    bool allowed=true,room=true,storeWorks=true,owned=false,sourcePresent=true;
    bool lastAutoResult=false;
    uint32 attempts=0,errors=0,notifications=0,skill=0;
    Item artifact;
    bool IsPlayer()const override{return true;}
    Player* ToPlayer()override{return this;}
    bool SolveResearchProject(uint32,void*){assert(false);return false;}
    bool HasItemCount(uint32){assert(false);return false;}
    void DestroyItemCount(uint32,uint32,bool){assert(false);}
    InventoryResult CanStoreNewItem(uint32 bag,uint32 slot,ItemPosCountVec& destination,uint32 item,uint32 count)
    {
        assert(bag==NULL_BAG&&slot==NULL_SLOT&&item==120978&&count==1);++attempts;
        if(!room)return EQUIP_ERR_INV_FULL;
        destination.push_back(1);return EQUIP_ERR_OK;
    }
    Item* StoreNewItem(ItemPosCountVec const&,uint32 item,bool,uint32,GuidSet const&,std::vector<uint32> const&,uint32)
    {assert(item==120978);if(!storeWorks)return nullptr;owned=true;return &artifact;}
    void SendEquipError(InventoryResult result,void*,void*,uint32 item){assert(result==EQUIP_ERR_INV_FULL&&item==120978);++errors;}
    void SendNewItem(Item* item,uint32 count,bool,bool,bool broadcast){assert(count==1&&broadcast);if(item)++notifications;}
    void UpdateCraftSkill(uint32 spell){assert(spell==199827);++skill;}
    bool AutoStoreLoot(uint32 spell,LootStore const& store,bool broadcast,uint32)
    {
        assert(spell==199827 && &store==&LootTemplates_Spell);
        uint32 max_slot=sourcePresent&&allowed?1:0;
        for(uint32 index=0;index<max_slot;++index)
        {
            LootItem item;LootItem* lootItem=&item;uint32 bag=NULL_BAG,slot=NULL_SLOT;
#include "StoreItem.inc"
        }
        lastAutoResult=[&]() -> bool {
#include "StoreResult.inc"
        }();
        return lastAutoResult;
    }
};
struct Spell
{
    Unit* m_caster=nullptr;
    Unit* unitTarget=nullptr;
    SpellInfo const* m_spellInfo=nullptr;
    uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;
    void* m_targets=nullptr;
    Item* m_CastItem=nullptr;
    void DoCreateItem(uint32,uint32,std::vector<uint32> const&){assert(false);}
    void ExecuteLogEffectCreateItem(uint32,uint32 item){assert(!item);}
    void EffectCreateItem2(SpellEffIndex);
};
#include "CreateItem.inc"
int main()
{
    SpellInfo info;assert(info.IsLootCrafting());
    for(bool source:{false,true})for(bool allowed:{false,true})for(bool room:{false,true})for(bool store:{false,true})
    {
        Player player;player.sourcePresent=source;player.allowed=allowed;player.room=room;player.storeWorks=store;
        Spell spell{&player,&player,&info};spell.EffectCreateItem2(0);
        assert(player.owned==(source&&allowed&&room&&store));
        assert(player.attempts==uint32(source&&allowed));
        assert(player.errors==uint32(source&&allowed&&!room));
        assert(player.notifications==uint32(player.owned));
        assert(player.lastAutoResult==(source&&allowed));
        assert(player.skill==1);
    }
    Player player;player.sourcePresent=false;Spell spell{&player,&player,&info};spell.EffectCreateItem2(0);assert(!player.owned);
    player.sourcePresent=true;spell.EffectCreateItem2(0);assert(player.owned&&player.notifications==1);
    spell.effectHandleMode=0;player.owned=false;spell.EffectCreateItem2(0);assert(!player.owned);
    Unit pet;spell.effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;spell.unitTarget=&pet;spell.EffectCreateItem2(0);assert(!player.owned);
    spell.unitTarget=nullptr;spell.EffectCreateItem2(0);assert(!player.owned);
    std::cout<<"PASS16 native effect157/IsLootCrafting plus actual inventory fragment source/conditions/full-bag/null-store cases; absent source cannot create120978, restored source stores through native effect. Full bags retain error/no item; actual enclosing retrieval progress/retry remains OPEN, not fixed by this data unit.\n";
}

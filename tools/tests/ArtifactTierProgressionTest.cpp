#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using int16=int16_t;using uint32=uint32_t;using uint64=uint64_t;
constexpr uint32 ARTIFACT_POWER_FLAG_NO_LINK_REQUIRED=2,ARTIFACT_POWER_FLAG_SCALES_WITH_NUM_POWERS=8,ARTIFACT_POWER_FLAG_HAS_RANK=32,ARTIFACT_POWER_FLAG_RELIC_TALENT=64;
constexpr int ITEM_MODIFIER_ARTIFACT_TIER=1,ITEM_FIELD_ARTIFACT_XP=2,ITEM_DYNAMIC_FIELD_ARTIFACT_POWERS=3,ITEM_CHANGED=4,GAMEOBJECT_TYPE_ARTIFACT_FORGE=47,CRITERIA_TYPE_OPEN_ARTIFACT_POWERS=5;
enum ArtifactCategory{ARTIFACT_CATEGORY_CLASS=1,ARTIFACT_CATEGORY_FISH=2};
#define TC_LOG_INFO(...) ((void)0)
#include "PowerStruct.inc"
struct ArtifactEntry{uint32 ID,ArtifactCategoryID,Spec;};
struct ArtifactPowerEntry{uint32 ID,ArtifactID,Flags,MaxPurchasableRank,Tier;};
struct ArtifactPowerRankEntry{uint32 SpellID;};
template<class T>struct Store{std::map<uint32,T> rows;T const* LookupEntry(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}uint32 GetNumRows(){return rows.empty()?0:rows.rbegin()->first+1;}};
Store<ArtifactPowerEntry>sArtifactPowerStore;Store<ArtifactEntry>sArtifactStore;
struct Manager{
 Manager* operator->(){return this;}
 std::map<uint32,std::vector<ArtifactPowerEntry const*>> byArtifact;
 std::map<uint32,std::unordered_set<uint32>> links;
 std::map<std::pair<uint32,uint32>,ArtifactPowerRankEntry> ranks;
 auto GetArtifactPowers(uint32 id){return byArtifact[id];}
 std::unordered_set<uint32> const* GetArtifactPowerLinks(uint32 id){auto i=links.find(id);return i==links.end()?nullptr:&i->second;}
 ArtifactPowerRankEntry const* GetArtifactPowerRank(uint32 id,uint32 rank){auto i=ranks.find({id,rank});return i==ranks.end()?nullptr:&i->second;}
}manager;auto& sDB2Manager=manager;
struct GtArtifactLevelXPEntry{uint64 XP=17,XP2=123456789012ULL;};
struct CostTable{GtArtifactLevelXPEntry cost;GtArtifactLevelXPEntry const* GetRow(uint32){return &cost;}}sArtifactLevelXPGameTable;
struct ItemTemplate{uint32 artifactId;uint32 GetArtifactID(){return artifactId;}};
struct Player;
struct Item{
 ItemTemplate proto;uint32 tier=0;uint64 xp=99999999999999ULL;bool equipped=true;int saves=0;
 std::vector<int16>m_artifactPowerIdToIndex;uint16 m_artifactPowerCount=0;
 std::vector<ItemDynamicFieldArtifactPowers> values;
 explicit Item(uint32 id):proto{id}{}
 ItemTemplate* GetTemplate(){return &proto;}uint32 GetEntry(){return proto.artifactId;}
 uint32 GetModifier(int)const{return tier;}uint64 GetUInt64Value(int)const{return xp;}void SetUInt64Value(int,uint64 n){xp=n;}
 void SetState(int,Player*){++saves;}bool IsEquipped(){return equipped;}void ActivateFishArtifact(uint32){}
 auto const& GetArtifactPowers()const{return values;}
 template<class T>T const* GetDynamicStructuredValue(int,int16 index)const{return index<int(values.size())?reinterpret_cast<T const*>(&values[index]):nullptr;}
 void SetDynamicStructuredValue(int,int16 index,ItemDynamicFieldArtifactPowers const* p){if(index>=int(values.size()))values.resize(index+1);values[index]=*p;}
 void InitArtifactPowers(uint8);void InitArtifactsTier(uint8);
 ItemDynamicFieldArtifactPowers const* GetArtifactPower(uint32)const;
 void SetArtifactPower(ItemDynamicFieldArtifactPowers const*,bool createIfMissing=false);
 uint32 GetTotalPurchasedArtifactPowers()const;
};
#include "Init.inc"
#include "Tier.inc"
#include "Lookup.inc"
#include "Total.inc"
struct Player{Item* item;bool forge=true;int applies=0,credits=0;
 bool GetGameObjectIfCanInteractWith(int,int){return forge;}Item* GetItemByGuid(int id){return id==1?item:nullptr;}
 void ApplyArtifactPowerRank(Item*,ArtifactPowerRankEntry const*,bool){++applies;}void UpdateAchievementCriteria(int,int){++credits;}};
namespace WorldPackets{namespace Artifact{struct AddPower{struct Power{uint32 PowerID;uint8 Rank;};int GameObjectGUID=7,ArtifactGUID=1;std::vector<Power>Powers;};}}
struct WorldSession{Player* _player;void HandleArtifactAddPower(WorldPackets::Artifact::AddPower&);};
#include "Purchase.inc"
void Load(char const* path){
 std::ifstream f(path);std::string line,kind;while(std::getline(f,line)){std::istringstream s(line);s>>kind;
 if(kind=="artifact"){ArtifactEntry a{};s>>a.ID>>a.ArtifactCategoryID>>a.Spec;sArtifactStore.rows[a.ID]=a;}
 if(kind=="power"){ArtifactPowerEntry p{};s>>p.ID>>p.ArtifactID>>p.Flags>>p.MaxPurchasableRank>>p.Tier;sArtifactPowerStore.rows[p.ID]=p;}
 if(kind=="rank"){uint32 id,rank,spell;s>>id>>rank>>spell;manager.ranks[{id,rank}]={spell};}
 if(kind=="link"){uint32 a,b;s>>a>>b;manager.links[a].insert(b);manager.links[b].insert(a);}}
 for(auto const& p:sArtifactPowerStore.rows)manager.byArtifact[p.second.ArtifactID].push_back(&p.second);
}
int main(int argc,char**argv){
 assert(argc==2);Load(argv[1]);uint32 classes=0,purchases=0;
 for(auto const& pair:sArtifactStore.rows){auto const& a=pair.second;if(a.ArtifactCategoryID!=ARTIFACT_CATEGORY_CLASS||!a.Spec)continue;
 auto nativePowers=manager.GetArtifactPowers(a.ID);
 if(std::none_of(nativePowers.begin(),nativePowers.end(),[](ArtifactPowerEntry const* p){return !p->Tier&&(p->Flags&ARTIFACT_POWER_FLAG_HAS_RANK);}))continue; // Exclude native display-only joke artifacts such as Oakbringer.
 Item item(a.ID);item.InitArtifactPowers(uint8(a.ID));std::map<uint32,ItemDynamicFieldArtifactPowers> old;
 for(auto power:item.GetArtifactPowers()){auto entry=sArtifactPowerStore.LookupEntry(power.ArtifactPowerId);
 if(entry->ArtifactID==a.ID&&entry->MaxPurchasableRank){power.PurchasedRank=uint8(entry->MaxPurchasableRank);power.CurrentRankWithBonus=power.PurchasedRank;}
 if(entry->Flags&ARTIFACT_POWER_FLAG_RELIC_TALENT)power.CurrentRankWithBonus=2;
 item.SetArtifactPower(&power);old[power.ArtifactPowerId]=power;}
 item.tier=1;item.InitArtifactsTier(uint8(a.ID));
 for(auto const& p:old){auto actual=item.GetArtifactPower(p.first);if(!actual){std::cerr<<"FAIL: base/relic lookup lost after upgrade, artifact "<<a.ID<<" power "<<p.first<<'\n';return 1;}assert(actual->PurchasedRank==p.second.PurchasedRank&&actual->CurrentRankWithBonus==p.second.CurrentRankWithBonus);}
 for(auto power:item.GetArtifactPowers()){auto entry=sArtifactPowerStore.LookupEntry(power.ArtifactPowerId);if(entry->Tier&&entry->MaxPurchasableRank){power.PurchasedRank=1;power.CurrentRankWithBonus=1;item.SetArtifactPower(&power);break;}}
 auto learned=item.values;
 auto count=item.values.size();auto total=item.GetTotalPurchasedArtifactPowers();for(int n=0;n<10;++n)item.InitArtifactsTier(uint8(a.ID));assert(item.values.size()==count&&item.GetTotalPurchasedArtifactPowers()==total);
 for(auto const& power:learned){auto actual=item.GetArtifactPower(power.ArtifactPowerId);assert(actual&&actual->PurchasedRank==power.PurchasedRank&&actual->CurrentRankWithBonus==power.CurrentRankWithBonus);}
 Player player{&item};WorldSession session{&player};
 for(auto const& p:old){auto entry=sArtifactPowerStore.LookupEntry(p.first);if(entry->ArtifactID!=a.ID||entry->Tier||!(entry->Flags&ARTIFACT_POWER_FLAG_HAS_RANK))continue;
 auto before=item.GetArtifactPower(p.first)->PurchasedRank;auto xp=item.xp;WorldPackets::Artifact::AddPower request;request.Powers.push_back({p.first,uint8(before+1)});
 player.forge=false;session.HandleArtifactAddPower(request);assert(item.xp==xp&&item.GetArtifactPower(p.first)->PurchasedRank==before);player.forge=true;
 item.xp=sArtifactLevelXPGameTable.cost.XP2-1;session.HandleArtifactAddPower(request);assert(item.GetArtifactPower(p.first)->PurchasedRank==before&&item.xp==sArtifactLevelXPGameTable.cost.XP2-1);item.xp=xp;
 session.HandleArtifactAddPower(request);assert(item.GetArtifactPower(p.first)->PurchasedRank==before+1&&item.xp==xp-sArtifactLevelXPGameTable.cost.XP2);
 auto paid=item.xp;session.HandleArtifactAddPower(request);assert(item.xp==paid);++purchases;}
 if(!player.credits){std::cerr<<"FAIL: no extra-rank candidates for native artifact "<<a.ID<<'\n';return 2;}assert(item.GetTotalPurchasedArtifactPowers()>total);++classes;
 if(a.ID==38){assert(item.GetArtifactPower(804)->PurchasedRank==4);Item loaded(38);loaded.tier=1;loaded.InitArtifactPowers(38);for(auto const& p:item.values)loaded.SetArtifactPower(&p);assert(loaded.GetArtifactPower(804)->PurchasedRank==4);}
 }
 assert(classes==36);std::cout<<"PASS: "<<classes<<" native class artifacts preserve old/relic indexes and tier idempotence; "<<purchases<<" sequential real base-rank purchases, costs and replay/forge rejection; Scepter reload\n";
}

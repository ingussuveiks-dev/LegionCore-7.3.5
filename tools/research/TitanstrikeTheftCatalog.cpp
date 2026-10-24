#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <cstring>
#include <vector>
// These client-only tables omit the relationship column from the core's SQL
// projection. Supply it to the native file decoder; never guess packed offsets.
struct RelationshipInfo {
 std::vector<DB2FieldMeta> fields;DB2LoadInfo info;
 explicit RelationshipInfo(DB2LoadInfo const* source):fields(source->Fields,source->Fields+source->FieldCount),info(){
  fields.push_back({true,FT_INT,"SpellVisualKitID"});
  info=DB2LoadInfo(fields.data(),fields.size(),source->Meta,source->Statement);
 }
};
#pragma pack(push,1)
struct VisualName {uint32 ID;float radius,speed,scale,minScale,maxScale,alpha;uint32 flags,field7,texture;uint8 type;uint32 model,field11,field12;};
#pragma pack(pop)
struct VisualNameInfo {
 std::vector<DB2FieldMeta> fields;DB2LoadInfo info;
 VisualNameInfo(){
  auto source=SpellVisualEffectNameLoadInfo::Instance();
  fields.push_back({false,FT_INT,"ID"});
  // Production metadata is26972; the unused server SQL projection instead
  // places the model first and declares a byte as int. Decode native order.
  for(uint32 i=0;i<source->Meta->FieldCount;++i)fields.push_back({false,DBCFormer(source->Meta->Types[i]),"NativeField"});
  info=DB2LoadInfo(fields.data(),fields.size(),source->Meta,source->Statement);
 }
};
template<class T> uint32 Parent(DB2Storage<T> const& table,T const* row){
 auto offset=table.GetLoadInfo()->Meta->GetParentIndexFieldOffset();
 if(offset<0)return 0;uint32 id=0;std::memcpy(&id,reinterpret_cast<char const*>(row)+offset,sizeof(id));return id;
}
int main(int argc,char** argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<ItemEntry> items("Item.db2",ItemLoadInfo::Instance());
 RelationshipInfo effectInfo(SpellVisualKitEffectLoadInfo::Instance()),attachInfo(SpellVisualKitModelAttachLoadInfo::Instance());
 DB2Storage<SpellVisualKitEffectEntry> kitEffects("SpellVisualKitEffect.db2",&effectInfo.info);
 DB2Storage<SpellVisualKitModelAttachEntry> attaches("SpellVisualKitModelAttach.db2",&attachInfo.info);
 VisualNameInfo nameInfo;
 DB2Storage<VisualName> names("SpellVisualEffectName.db2",&nameInfo.info);
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!visuals.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!items.Load(argv[1],0))return 2;
 if(!kitEffects.Load(argv[1],0)||!attaches.Load(argv[1],0)||!names.Load(argv[1],0))return 7;
 std::set<uint32> ids={196978,210993,211046,211083,211034,211970,211967,211934,212010,212011,212012,212050,211008,211739,211740,211753,211848};
 for(auto id:ids){auto s=spells.LookupEntry(id);if(!s)return 3;std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';}
 for(auto e:effects)if(ids.count(e->SpellID)&&!e->DifficultyID)std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
 for(auto v:visuals)if(ids.count(v->SpellID))std::cout<<"VISUAL\t"<<v->SpellID<<'\t'<<v->SpellVisualID<<'\n';
 for(auto m:misc)if(ids.count(m->SpellID)&&!m->DifficultyID){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"DURATION\t"<<m->SpellID<<'\t'<<(d?d->Duration:0)<<'\n';}
 for(uint32 id:{42428,69591}){auto d=displays.LookupEntry(id);if(!d)return 4;auto m=models.LookupEntry(d->ModelID);if(!m)return 5;std::cout<<"MODEL\t"<<id<<'\t'<<d->ModelID<<'\t'<<m->FileDataID<<'\t'<<d->StateSpellVisualKitID<<'\t'<<d->ExtendedDisplayInfoID<<'\n';}
 auto item=items.LookupEntry(128861);if(!item)return 6;std::cout<<"ITEM\t128861\t"<<unsigned(item->InventoryType)<<'\n';
 for(auto e:kitEffects)if((Parent(kitEffects,e)==67322||Parent(kitEffects,e)==58494))std::cout<<"KIT_EFFECT\t"<<Parent(kitEffects,e)<<'\t'<<e->ID<<'\t'<<e->EffectType<<'\t'<<e->Effect<<'\n';
 for(auto a:attaches)if((Parent(attaches,a)==67322||Parent(attaches,a)==58494)){
  auto n=names.LookupEntry(a->SpellVisualEffectNameID);if(!n)return 8;
  std::cout<<"EFFECT_NAME\t"<<n->ID<<'\t'<<unsigned(n->type)<<'\t'<<n->model<<'\t'<<n->texture<<'\t'<<n->scale<<'\t'<<n->flags<<'\n';
  std::cout<<"ATTACH\t"<<Parent(attaches,a)<<'\t'<<a->ID<<'\t'<<a->SpellVisualEffectNameID<<'\t'<<n->model<<'\t'<<a->Offset[0]<<'\t'<<a->Offset[1]<<'\t'<<a->Offset[2]<<'\t'<<a->Scale<<'\t'<<unsigned(a->AttachmentID)<<'\t'<<a->AnimKitID<<'\n';
  if(Parent(attaches,a)==58494&&n->model==1133115)
   std::cout<<"WIND_FLOOR\t"<<a->ID<<'\t'<<a->Offset[0]<<'\t'<<a->Offset[1]<<'\t'<<a->Offset[2]<<'\t'<<a->Scale<<'\t'<<a->Yaw<<'\t'<<a->Pitch<<'\t'<<a->Roll<<'\n';
 }
}

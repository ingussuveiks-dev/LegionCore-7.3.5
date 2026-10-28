#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <cctype>
#pragma pack(push,1)
struct EventRow { SpellVisualEventEntry value; int32 visual; };
struct EffectRow { SpellVisualKitEffectEntry value; int32 kit; };
struct AttachmentRow { SpellVisualKitModelAttachEntry value; int32 kit; };
struct NameRow { int32 id;float radius,speed,scale,minScale,maxScale,alpha;uint32 flags;int32 generic,texture;uint8 type;int32 model;uint32 ribbon;int32 dissolve; };
#pragma pack(pop)
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualAnimEntry> animations("SpellVisualAnim.db2",SpellVisualAnimLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!visuals.Load(argv[1],0)||!animations.Load(argv[1],0)||!objects.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();visuals.LoadFromDB();animations.LoadFromDB();objects.LoadFromDB();
    std::set<uint32> selected{201031,201032,34429,200984,204394,204356,202925,202981,202014,206895};
    for(auto spell:spells){std::string name=spell->Name->Str[0];std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});if(name.find("tentacl")!=std::string::npos||name.find("tendril")!=std::string::npos||name.find("blocker")!=std::string::npos)selected.insert(spell->ID);}
    for(uint32 id:selected){auto row=spells.LookupEntry(id);if(!row)return 5;std::cout<<"SPELL\t"<<id<<'\t'<<row->Name->Str[0]<<'\n';}
    for(auto e:effects)if(!e->DifficultyID&&selected.count(e->SpellID))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
    for(auto m:misc)if(!m->DifficultyID&&selected.count(m->SpellID)){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<m->SpellID<<'\t'<<(d?d->Duration:0);for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';}
    std::set<uint32> visualIds,kits;
    for(auto v:visuals)if(selected.count(v->SpellID)){visualIds.insert(v->SpellVisualID);std::cout<<"VISUAL\t"<<v->SpellID<<'\t'<<v->SpellVisualID<<'\t'<<v->CasterPlayerConditionID<<'\t'<<v->ViewerPlayerConditionID<<'\n';}
    auto eventInfo=SpellVisualEventLoadInfo::Instance();std::vector<DB2FieldMeta> eventFields(eventInfo->Fields,eventInfo->Fields+eventInfo->FieldCount);eventFields.push_back({true,FT_INT,"SpellVisualID"});
    DB2LoadInfo fullEvents(eventFields.data(),eventFields.size(),SpellVisualEventMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_EVENT);
    DB2Storage<EventRow> events("SpellVisualEvent.db2",&fullEvents);
    auto effectInfo=SpellVisualKitEffectLoadInfo::Instance();std::vector<DB2FieldMeta> effectFields(effectInfo->Fields,effectInfo->Fields+effectInfo->FieldCount);effectFields.push_back({true,FT_INT,"SpellVisualKitID"});
    DB2LoadInfo fullEffects(effectFields.data(),effectFields.size(),SpellVisualKitEffectMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    DB2Storage<EffectRow> kitEffects("SpellVisualKitEffect.db2",&fullEffects);
    auto attachInfo=SpellVisualKitModelAttachLoadInfo::Instance();std::vector<DB2FieldMeta> attachFields(attachInfo->Fields,attachInfo->Fields+attachInfo->FieldCount);attachFields.push_back({true,FT_INT,"SpellVisualKitID"});
    DB2LoadInfo fullAttachments(attachFields.data(),attachFields.size(),SpellVisualKitModelAttachMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    DB2Storage<AttachmentRow> attachments("SpellVisualKitModelAttach.db2",&fullAttachments);
    if(sizeof(EventRow)!=fullEvents.Meta->GetRecordSize()||sizeof(EffectRow)!=fullEffects.Meta->GetRecordSize()||sizeof(AttachmentRow)!=fullAttachments.Meta->GetRecordSize()||!events.Load(argv[1],0)||!kitEffects.Load(argv[1],0)||!attachments.Load(argv[1],0))return 6;
    for(auto e:events)if(visualIds.count(e->visual)){kits.insert(e->value.SpellVisualKitID);std::cout<<"EVENT\t"<<e->visual<<'\t'<<e->value.StartEvent<<'\t'<<e->value.EndEvent<<'\t'<<e->value.SpellVisualKitID<<'\n';}
    for(auto e:kitEffects)if(kits.count(e->kit)){std::cout<<"KIT\t"<<e->kit<<'\t'<<e->value.EffectType<<'\t'<<e->value.Effect<<'\n';if(e->value.EffectType==6){auto a=animations.LookupEntry(e->value.Effect);if(a)std::cout<<"ANIMATION\t"<<a->ID<<'\t'<<a->InitialAnimID<<'\t'<<a->LoopAnimID<<'\t'<<a->AnimKitID<<'\n';}}
    DB2FieldMeta nameFields[]={{true,FT_INT,"ID"},{false,FT_FLOAT,"EffectRadius"},{false,FT_FLOAT,"BaseMissileSpeed"},{false,FT_FLOAT,"Scale"},{false,FT_FLOAT,"MinAllowedScale"},{false,FT_FLOAT,"MaxAllowedScale"},{false,FT_FLOAT,"Alpha"},{false,FT_INT,"Flags"},{true,FT_INT,"GenericID"},{true,FT_INT,"TextureFileDataID"},{false,FT_BYTE,"Type"},{true,FT_INT,"ModelFileDataID"},{false,FT_INT,"RibbonQualityID"},{true,FT_INT,"DissolveEffectID"}};
    DB2LoadInfo nameInfo(nameFields,std::size(nameFields),SpellVisualEffectNameMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    DB2Storage<NameRow> names("SpellVisualEffectName.db2",&nameInfo);
    if(sizeof(NameRow)!=nameInfo.Meta->GetRecordSize()||!names.Load(argv[1],0))return 8;
    for(auto a:attachments)if(kits.count(a->kit)){auto name=names.LookupEntry(a->value.SpellVisualEffectNameID);std::cout<<"ATTACHMENT\t"<<a->kit<<'\t'<<a->value.ID<<'\t'<<a->value.SpellVisualEffectNameID<<'\t'<<a->value.AnimID<<'\t'<<a->value.AnimKitID<<'\t'<<(name?name->model:0)<<'\t'<<(name?uint32(name->type):0)<<'\n';}
    for(auto a:attachments){auto n=names.LookupEntry(a->value.SpellVisualEffectNameID);if(!n||n->model!=1063325)continue;for(auto k:kitEffects)if(k->kit==a->kit && k->value.EffectType==2 && k->value.Effect==a->value.ID)for(auto e:events)if(e->value.SpellVisualKitID==a->kit)for(auto v:visuals)if(v->SpellVisualID==e->visual){auto spell=spells.LookupEntry(v->SpellID);std::cout<<"DOOR_REVERSE\t"<<n->id<<'\t'<<a->kit<<'\t'<<e->visual<<'\t'<<v->SpellID<<'\t'<<(spell?spell->Name->Str[0]:"")<<'\n';}}
    for(uint32 id:{6391u,24101u}){auto o=objects.LookupEntry(id);if(!o)return 7;std::cout<<"BLOCKER_MODEL\t"<<o->ID<<'\t'<<o->FileDataID<<'\n';}
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 9;displays.LoadFromDB();models.LoadFromDB();
    for(uint32 id:{11686u,65939u,68651u}){auto d=displays.LookupEntry(id);auto m=d?models.LookupEntry(d->ModelID):nullptr;if(!m)return 10;std::cout<<"CREATURE_MODEL\t"<<id<<'\t'<<d->ModelID<<'\t'<<m->FileDataID<<'\t'<<d->StateSpellVisualKitID<<'\n';}
    HotfixDatabase.Close();MySQL::Library_End();return 0;
}

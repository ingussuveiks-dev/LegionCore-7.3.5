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
struct MissileRow { SpellVisualMissileEntry value; uint16 set; };
struct NativeModelFileRow { uint8 flags; int32 id,resource; };
struct NativeBeamRow { int32 id,beam;float sourceMinDistance,fixedLength;int32 flags;int16 sourceOffset,destOffset,sourceAttach,destAttach,sourcePositioner,destPositioner; };
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
    std::set<uint32> selected{201031,201032,34429,200984,204394,204356,202925,202981,202014,206895,204641,204771,206357,206360,206361,206390,203037,207244,204595,204743,215690,63048,201904,207454,202015,206429,206455,215997,230829,192085};
    for(auto spell:spells){std::string name=spell->Name->Str[0];std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});if(name.find("tentacl")!=std::string::npos||name.find("tendril")!=std::string::npos||name.find("blocker")!=std::string::npos||name.find("shadow channel")!=std::string::npos||(spell->ID>=201000&&spell->ID<=201050))selected.insert(spell->ID);}
    for(uint32 id:selected){auto row=spells.LookupEntry(id);if(!row)return 5;std::cout<<"SPELL\t"<<id<<'\t'<<row->Name->Str[0]<<'\n';}
    for(auto e:effects)if(!e->DifficultyID&&selected.count(e->SpellID))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
    for(auto m:misc)if(!m->DifficultyID&&selected.count(m->SpellID)){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<m->SpellID<<'\t'<<(d?d->Duration:0);for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';}
    std::set<uint32> visualIds,kits;
    for(auto v:visuals)if(selected.count(v->SpellID)){visualIds.insert(v->SpellVisualID);std::cout<<"VISUAL\t"<<v->SpellID<<'\t'<<v->SpellVisualID<<'\t'<<v->CasterPlayerConditionID<<'\t'<<v->ViewerPlayerConditionID<<'\n';}
    DB2Storage<SpellVisualEntry> visualData("SpellVisual.db2",SpellVisualLoadInfo::Instance());
    if(!visualData.Load(argv[1],0))return 11;visualData.LoadFromDB();
    // A visual without direct events may delegate to a caster/hostile or
    // low-violence visual. Follow the actual native references, with cycle
    // protection, rather than treating an empty event set as no appearance.
    std::set<uint32> pending=visualIds,visited;
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!id||!visited.insert(id).second)continue;
        auto v=visualData.LookupEntry(id);if(!v){std::cout<<"MISSING_VISUAL\t"<<id<<'\n';continue;}
        std::cout<<"VISUAL_DELEGATE\t"<<id<<'\t'<<v->Flags<<'\t'<<v->CasterSpellVisualID<<'\t'<<v->HostileSpellVisualID<<'\t'<<v->LowViolenceSpellVisualID<<'\t'<<v->SpellVisualMissileSetID<<'\n';
        for(int32 next:{v->CasterSpellVisualID,v->HostileSpellVisualID,v->LowViolenceSpellVisualID})if(next>0){visualIds.insert(next);if(!visited.count(next))pending.insert(next);}
    }
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
    // The disabled production store has newer int32 members where26972 uses
    // six shorts. Read matching packed data rather than changing runtime data.
    DB2FieldMeta beamFields[]={{true,FT_INT,"ID"},{true,FT_INT,"BeamID"},{false,FT_FLOAT,"SourceMinDistance"},{false,FT_FLOAT,"FixedLength"},{true,FT_INT,"Flags"},{true,FT_SHORT,"SourceOffset"},{true,FT_SHORT,"DestOffset"},{true,FT_SHORT,"SourceAttachID"},{true,FT_SHORT,"DestAttachID"},{true,FT_SHORT,"SourcePositionerID"},{true,FT_SHORT,"DestPositionerID"}};
    DB2LoadInfo beamInfo(beamFields,std::size(beamFields),BeamEffectMeta::Instance(),HOTFIX_SEL_BEAM_EFFECT);
    DB2Storage<NativeBeamRow> beams("BeamEffect.db2",&beamInfo);
    if(sizeof(NativeBeamRow)!=beamInfo.Meta->GetRecordSize()||!beams.Load(argv[1],0))return 16;beams.LoadFromDB();
    for(auto k:kitEffects)if(kits.count(k->kit)&&k->value.EffectType==13)if(auto b=beams.LookupEntry(k->value.Effect))
        std::cout<<"BEAM_REFERENCE\t"<<k->kit<<'\t'<<k->value.EffectType<<'\t'<<b->id<<'\t'<<b->beam<<'\t'<<b->flags<<'\t'<<b->sourceAttach<<'\t'<<b->destAttach<<'\t'<<b->sourceMinDistance<<'\t'<<b->fixedLength<<'\n';
    DB2Storage<DissolveEffectEntry> dissolves("DissolveEffect.db2",DissolveEffectLoadInfo::Instance());
    if(!dissolves.Load(argv[1],0))return 17;dissolves.LoadFromDB();
    for(auto k:kitEffects)if(kits.count(k->kit)&&k->value.EffectType==11)if(auto d=dissolves.LookupEntry(k->value.Effect))
        std::cout<<"DISSOLVE\t"<<k->kit<<'\t'<<d->ID<<'\t'<<d->Ramp<<'\t'<<d->StartValue<<'\t'<<d->EndValue<<'\t'<<d->FadeInTime<<'\t'<<d->FadeOutTime<<'\t'<<d->Duration<<'\t'<<d->Scale<<'\t'<<d->CurveID<<'\n';
    DB2FieldMeta nameFields[]={{true,FT_INT,"ID"},{false,FT_FLOAT,"EffectRadius"},{false,FT_FLOAT,"BaseMissileSpeed"},{false,FT_FLOAT,"Scale"},{false,FT_FLOAT,"MinAllowedScale"},{false,FT_FLOAT,"MaxAllowedScale"},{false,FT_FLOAT,"Alpha"},{false,FT_INT,"Flags"},{true,FT_INT,"GenericID"},{true,FT_INT,"TextureFileDataID"},{false,FT_BYTE,"Type"},{true,FT_INT,"ModelFileDataID"},{false,FT_INT,"RibbonQualityID"},{true,FT_INT,"DissolveEffectID"}};
    DB2LoadInfo nameInfo(nameFields,std::size(nameFields),SpellVisualEffectNameMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    DB2Storage<NameRow> names("SpellVisualEffectName.db2",&nameInfo);
    if(sizeof(NameRow)!=nameInfo.Meta->GetRecordSize()||!names.Load(argv[1],0))return 8;
    auto missileInfo=SpellVisualMissileLoadInfo::Instance();std::vector<DB2FieldMeta> missileFields(missileInfo->Fields,missileInfo->Fields+missileInfo->FieldCount);missileFields.push_back({false,FT_SHORT,"SpellVisualMissileSetID"});
    DB2LoadInfo fullMissiles(missileFields.data(),missileFields.size(),SpellVisualMissileMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_MISSILE);
    DB2Storage<MissileRow> missiles("SpellVisualMissile.db2",&fullMissiles);
    if(sizeof(MissileRow)!=fullMissiles.Meta->GetRecordSize()||!missiles.Load(argv[1],0))return 12;
    for(auto v:visualData)if(visualIds.count(v->ID))for(auto m:missiles)if(v->SpellVisualMissileSetID&&m->set==v->SpellVisualMissileSetID){auto n=names.LookupEntry(m->value.SpellVisualEffectNameID);std::cout<<"MISSILE_MODEL\t"<<v->ID<<'\t'<<m->set<<'\t'<<m->value.ID<<'\t'<<m->value.Flags<<'\t'<<m->value.SpellVisualEffectNameID<<'\t'<<(n?n->model:0)<<'\t'<<uint32(m->value.Attachment)<<'\t'<<uint32(m->value.DestinationAttachment)<<'\n';}
    for(auto e:effects)if(!e->DifficultyID&&(e->SpellID==201032||e->SpellID==201031))std::cout<<"RITUAL_PERIOD\t"<<e->SpellID<<'\t'<<e->EffectAuraPeriod<<'\n';
    for(auto a:attachments)if(kits.count(a->kit)){auto name=names.LookupEntry(a->value.SpellVisualEffectNameID);std::cout<<"ATTACHMENT\t"<<a->kit<<'\t'<<a->value.ID<<'\t'<<a->value.SpellVisualEffectNameID<<'\t'<<a->value.AnimID<<'\t'<<a->value.AnimKitID<<'\t'<<(name?name->model:0)<<'\t'<<(name?uint32(name->type):0)<<'\n';}
    for(auto a:attachments)if(a->kit==64833||a->kit==64756){auto n=names.LookupEntry(a->value.SpellVisualEffectNameID);std::cout<<"ATTACH_POSE\t"<<a->kit<<'\t'<<a->value.ID<<'\t'<<a->value.Scale<<'\t'<<unsigned(a->value.AttachmentID)<<'\t'<<unsigned(a->value.Flags)<<'\t'<<a->value.Offset[0]<<'\t'<<a->value.Offset[1]<<'\t'<<a->value.Offset[2]<<'\t'<<(n?n->scale:0)<<'\t'<<(n?n->flags:0)<<'\t'<<(n?n->generic:0)<<'\t'<<a->value.Yaw<<'\t'<<a->value.Pitch<<'\t'<<a->value.Roll<<'\n';}
    for(auto n:names)if(n->type==1&&n->generic==128910)for(auto a:attachments)if(a->value.SpellVisualEffectNameID==n->id)std::cout<<"BLADE_ATTACHMENT_ALTERNATIVE\t"<<n->id<<'\t'<<a->kit<<'\t'<<a->value.ID<<'\t'<<a->value.Scale<<'\t'<<a->value.Offset[0]<<'\t'<<a->value.Offset[1]<<'\t'<<a->value.Offset[2]<<'\n';
    for(auto a:attachments)if(a->kit==65550||a->kit==65551||a->kit==65562){auto n=names.LookupEntry(a->value.SpellVisualEffectNameID);if(n)std::cout<<"BLADE_ALTERNATIVE_POSE\t"<<a->kit<<'\t'<<n->id<<'\t'<<n->scale<<'\t'<<n->flags<<'\t'<<unsigned(a->value.AttachmentID)<<'\t'<<unsigned(a->value.Flags)<<'\t'<<a->value.Yaw<<'\t'<<a->value.Pitch<<'\t'<<a->value.Roll<<'\n';for(auto e:events)if(e->value.SpellVisualKitID==a->kit)for(auto v:visuals)if(v->SpellVisualID==e->visual){auto s=spells.LookupEntry(v->SpellID);std::cout<<"BLADE_ALTERNATIVE_SOURCE\t"<<a->kit<<'\t'<<e->visual<<'\t'<<v->SpellID<<'\t'<<(s?s->Name->Str[0]:"")<<'\n';}}
    std::set<int32> candidateFiles{196078,201538,201539,399046,405891,405892,406125,406354,407609,593963,593965,593967,635337,635342,635344,902209,902211,1119495,1063325};
    for(auto o:objects)if(candidateFiles.count(o->FileDataID))std::cout<<"CANDIDATE_GO_MODEL\t"<<o->ID<<'\t'<<o->FileDataID<<'\n';
    for(auto a:attachments){auto n=names.LookupEntry(a->value.SpellVisualEffectNameID);if(!n||!candidateFiles.count(n->model))continue;for(auto k:kitEffects)if(k->kit==a->kit && k->value.EffectType==2 && k->value.Effect==a->value.ID)for(auto e:events)if(e->value.SpellVisualKitID==a->kit)for(auto v:visuals)if(v->SpellVisualID==e->visual){auto spell=spells.LookupEntry(v->SpellID);std::cout<<"DOOR_REVERSE\t"<<n->model<<'\t'<<n->id<<'\t'<<a->kit<<'\t'<<e->visual<<'\t'<<v->SpellID<<'\t'<<(spell?spell->Name->Str[0]:"")<<'\n';}}
    for(uint32 id:{6391u,24101u,9806u}){auto o=objects.LookupEntry(id);if(!o)return 7;std::cout<<"BLOCKER_MODEL\t"<<o->ID<<'\t'<<o->FileDataID<<'\n';}
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 9;displays.LoadFromDB();models.LoadFromDB();
    for(uint32 id:{11686u,65939u,68651u,68556u,67454u,61698u}){auto d=displays.LookupEntry(id);auto m=d?models.LookupEntry(d->ModelID):nullptr;if(!m)return 10;std::cout<<"CREATURE_MODEL\t"<<id<<'\t'<<d->ModelID<<'\t'<<m->FileDataID<<'\t'<<d->StateSpellVisualKitID<<'\t'<<m->ModelScale<<'\t'<<m->AttachedEffectScale<<'\t'<<d->CreatureModelScale<<'\n';}
    DB2Storage<ItemModifiedAppearanceEntry> modified("ItemModifiedAppearance.db2",ItemModifiedAppearanceLoadInfo::Instance());
    DB2Storage<ItemAppearanceEntry> appearances("ItemAppearance.db2",ItemAppearanceLoadInfo::Instance());
    DB2Storage<ItemDisplayInfoEntry> itemDisplays("ItemDisplayInfo.db2",ItemDisplayInfoLoadInfo::Instance());
    // Matching26972 metadata is bii/9bytes. The disabled production store's
    // newer LodCount member/projection is incompatible; do not interpret its
    // records through that10byte structure or claim absent assets from it.
    DB2FieldMeta modelFields[]={{false,FT_BYTE,"Flags"},{true,FT_INT,"ID"},{true,FT_INT,"ModelResourcesID"}};
    DB2LoadInfo modelInfo(modelFields,std::size(modelFields),ModelFileDataMeta::Instance(),HOTFIX_SEL_MODEL_FILE_DATA);
    DB2Storage<NativeModelFileRow> files("ModelFileData.db2",&modelInfo);
    if(sizeof(NativeModelFileRow)!=modelInfo.Meta->GetRecordSize())return 15;
    if(!modified.Load(argv[1],0)||!appearances.Load(argv[1],0)||!itemDisplays.Load(argv[1],0)||!files.Load(argv[1],0))return 13;
    modified.LoadFromDB();appearances.LoadFromDB();itemDisplays.LoadFromDB();
    for(auto a:modified)if(a->ItemID==128910){auto b=appearances.LookupEntry(a->ItemAppearanceID);auto d=b?itemDisplays.LookupEntry(b->ItemDisplayInfoID):nullptr;if(!d)return 14;std::cout<<"BLADE_APPEARANCE\t"<<unsigned(a->ItemAppearanceModifierID)<<'\t'<<a->ID<<'\t'<<b->ID<<'\t'<<d->ID<<'\t'<<d->ModelResourcesID[0]<<'\t'<<d->ModelResourcesID[1]<<'\n';for(auto f:files)for(int32 resource:d->ModelResourcesID)if(resource&&f->resource==resource){std::cout<<"BASE_BLADE_MODEL\t"<<a->ID<<'\t'<<b->ID<<'\t'<<d->ID<<'\t'<<resource<<'\t'<<f->id<<'\n';if(a->ItemAppearanceModifierID==0){for(auto o:objects)if(o->FileDataID==f->id)std::cout<<"BLADE_GO_DISPLAY\t"<<o->ID<<'\t'<<f->id<<'\n';for(auto c:models)if(c->FileDataID==f->id)for(auto display:displays)if(display->ModelID==c->ID)std::cout<<"BLADE_CREATURE_DISPLAY\t"<<display->ID<<'\t'<<f->id<<'\n';}}}
    HotfixDatabase.Close();MySQL::Library_End();return 0;
}

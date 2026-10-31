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
#include <string>
#include <vector>

#pragma pack(push,1)
struct PhysicalAttachment { SpellVisualKitModelAttachEntry value; int32 kit; };
struct PhysicalEffectName
{
    int32 ID;float radius,speed,scale,minimumScale,maximumScale,alpha;
    uint32 flags;int32 generic,texture;uint8 type;int32 model;uint32 ribbon;int32 dissolve;
};
#pragma pack(pop)

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<WorldMapAreaEntry> areas("WorldMapArea.db2",WorldMapAreaLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    // These client-only records include the parent key and a byte-sized type.
    // Match the established typed visual catalog, not the obsolete core struct.
    auto attachmentInfo=SpellVisualKitModelAttachLoadInfo::Instance();
    std::vector<DB2FieldMeta> attachmentFields(attachmentInfo->Fields,attachmentInfo->Fields+attachmentInfo->FieldCount);
    attachmentFields.push_back({true,FT_INT,"SpellVisualKitID"});
    DB2LoadInfo fullAttachmentInfo(attachmentFields.data(),attachmentFields.size(),SpellVisualKitModelAttachMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    DB2FieldMeta nameFields[]={{true,FT_INT,"ID"},{false,FT_FLOAT,"EffectRadius"},{false,FT_FLOAT,"BaseMissileSpeed"},{false,FT_FLOAT,"Scale"},{false,FT_FLOAT,"MinAllowedScale"},{false,FT_FLOAT,"MaxAllowedScale"},{false,FT_FLOAT,"Alpha"},{false,FT_INT,"Flags"},{true,FT_INT,"GenericID"},{true,FT_INT,"TextureFileDataID"},{false,FT_BYTE,"Type"},{true,FT_INT,"ModelFileDataID"},{false,FT_INT,"RibbonQualityID"},{true,FT_INT,"DissolveEffectID"}};
    DB2LoadInfo nameInfo(nameFields,std::size(nameFields),SpellVisualEffectNameMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    if(sizeof(PhysicalAttachment)!=fullAttachmentInfo.Meta->GetRecordSize()||sizeof(PhysicalEffectName)!=nameInfo.Meta->GetRecordSize())return 12;
    DB2Storage<PhysicalEffectName> effectNames("SpellVisualEffectName.db2",&nameInfo);
    DB2Storage<PhysicalAttachment> attachments("SpellVisualKitModelAttach.db2",&fullAttachmentInfo);
    if(!areas.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!objects.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0)||!effectNames.Load(argv[1],0)||!attachments.Load(argv[1],0))return 5;
    areas.LoadFromDB();displays.LoadFromDB();models.LoadFromDB();objects.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();
    std::cout<<"SOURCE\troot-tables-base-plus-hotfix\tvisual-effect-name-and-parent-attachment-base-only\n";
    for(auto area:areas)if(area->MapID==1541||area->AreaID==8005)
        std::cout<<"MAP_AREA\t"<<area->ID<<'\t'<<area->MapID<<'\t'<<area->AreaID<<'\t'<<area->DisplayMapID<<'\t'<<area->LocLeft<<'\t'<<area->LocRight<<'\t'<<area->LocTop<<'\t'<<area->LocBottom<<'\t'<<area->AreaName->Str[0]<<'\n';
    for(uint32 id:{67670u,25204u})
    {
        auto d=displays.LookupEntry(id);if(!d)return 6;
        auto m=models.LookupEntry(d->ModelID);if(!m)return 7;
        std::cout<<"CREATURE_DISPLAY\t"<<id<<'\t'<<unsigned(d->Flags)<<'\t'<<unsigned(d->CreatureModelAlpha)<<'\t'<<d->StateSpellVisualKitID<<'\t'<<d->ObjectEffectPackageID<<'\t'<<d->ExtendedDisplayInfoID<<'\t'<<d->DissolveEffectID;
        for(auto v:d->TextureVariationFileDataID)std::cout<<'\t'<<v;
        std::cout<<'\n';
        std::cout<<"CREATURE_MODEL\t"<<id<<'\t'<<d->ModelID<<'\t'<<m->FileDataID<<'\t'<<d->CreatureModelScale<<'\t'<<m->ModelScale<<'\t'<<m->CollisionWidth<<'\t'<<m->CollisionHeight<<'\t'<<m->HoverHeight;
        for(auto v:m->GeoBox)std::cout<<'\t'<<v;
        std::cout<<'\n';
    }
    auto object=objects.LookupEntry(9806);if(!object)return 8;
    std::cout<<"GO_MODEL\t9806\t"<<object->FileDataID<<'\t'<<object->GeoBoxMin.X<<'\t'<<object->GeoBoxMin.Y<<'\t'<<object->GeoBoxMin.Z<<'\t'<<object->GeoBoxMax.X<<'\t'<<object->GeoBoxMax.Y<<'\t'<<object->GeoBoxMax.Z<<'\n';
    for(uint32 id:{4578u,4581u})
    {
        auto vehicle=vehicles.LookupEntry(id);if(!vehicle)return 9;
        std::cout<<"VEHICLE\t"<<id<<'\t'<<vehicle->Flags<<'\t'<<unsigned(vehicle->FlagsB)<<'\n';
        for(uint32 index=0;index<8;++index)if(auto seat=seats.LookupEntry(vehicle->SeatID[index]))
            std::cout<<"SEAT\t"<<id<<'\t'<<index<<'\t'<<seat->ID<<'\t'<<seat->Flags<<'\t'<<seat->FlagsB<<'\t'<<seat->FlagsC<<'\t'<<seat->AttachmentOffset.X<<'\t'<<seat->AttachmentOffset.Y<<'\t'<<seat->AttachmentOffset.Z<<'\t'<<seat->EnterMinDuration<<'\t'<<seat->EnterMaxDuration<<'\t'<<seat->ExitMinDuration<<'\t'<<seat->ExitMaxDuration<<'\n';
    }
    for(uint32 id:{26949u,10790u})
    {
        auto effect=effectNames.LookupEntry(id);if(!effect)return 10;
        std::cout<<"VISUAL_EFFECT\t"<<id<<'\t'<<effect->model<<'\t'<<unsigned(effect->type)<<'\t'<<effect->generic<<'\t'<<effect->scale<<'\t'<<effect->alpha<<'\n';
    }
    for(uint32 id:{317084u,317095u})
    {
        auto row=attachments.LookupEntry(id);if(!row)return 11;
        auto attachment=&row->value;
        std::cout<<"VISUAL_ATTACH\t"<<id<<'\t'<<attachment->SpellVisualEffectNameID<<'\t'<<unsigned(attachment->AttachmentID)<<'\t'<<attachment->Scale<<'\t'<<attachment->Yaw<<'\t'<<attachment->Pitch<<'\t'<<attachment->Roll;
        for(auto v:attachment->Offset)std::cout<<'\t'<<v;
        std::cout<<'\n';
    }
    std::cout<<"EOF\tsheilun-physical\n";HotfixDatabase.Close();MySQL::Library_End();
}

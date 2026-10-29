#include "StromkarBladeVisual.h"
#include <unordered_set>
#include "ObjectGuid.h"
#include "Optional.h"
#include "DB2Stores.h"
#include "ByteBuffer.h"
#include "HotfixPackets.h"
#include "WorldPacket.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#pragma pack(push,1)
struct AttachmentEffect { SpellVisualKitEffectEntry value; int32 kit; };
struct AttachmentEvent { SpellVisualEventEntry value; int32 visual; };
#pragma pack(pop)

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    using Row = StromkarBladeVisual::Attachment;
    auto info = StromkarBladeVisual::LoadInfo();
    assert(sizeof(Row) == info->Meta->GetRecordSize());
    assert(info->FieldCount == info->Meta->GetDbFieldCount());
    DB2Storage<Row> store("SpellVisualKitModelAttach.db2", info);
    if (!store.Load(argv[1], 0)) return 2;
    auto before = store.LookupEntry(318611);
    if(before && !StromkarBladeVisual::Supported(before))
        std::cerr<<"NATIVE attachment "<<before->value.ID<<" kit "<<before->kit<<" name "<<before->value.SpellVisualEffectNameID
                 <<" attach "<<unsigned(before->value.AttachmentID)<<" flags "<<unsigned(before->value.Flags)<<" positioner "<<before->value.PositionerID
                 <<" offsets "<<before->value.Offset[0]<<','<<before->value.Offset[1]<<','<<before->value.Offset[2]
                 <<" rotations "<<before->value.Yaw<<','<<before->value.Pitch<<','<<before->value.Roll<<" scale "<<before->value.Scale<<'\n';
    assert(StromkarBladeVisual::Supported(before) && before->value.Scale == 0.025f);
    auto full = store.LookupEntry(319823);
    assert(full && full->kit == 65550 && full->value.SpellVisualEffectNameID == 27013 && full->value.Scale == 1.0f);
    DB2Storage<PositionerEntry> positioners("Positioner.db2",PositionerLoadInfo::Instance());
    DB2Storage<PositionerStateEntry> states("PositionerState.db2",PositionerStateLoadInfo::Instance());
    if(!positioners.Load(argv[1],0)||!states.Load(argv[1],0))return 4;
    auto positioner=positioners.LookupEntry(before->value.PositionerID);assert(positioner);
    std::set<uint32> visited;
    for(uint32 id=positioner->FirstStateID;id&&visited.insert(id).second;)
    {
        auto state=states.LookupEntry(id);assert(state);
        std::cout<<"POSITIONER_STATE\t"<<before->value.PositionerID<<'\t'<<id<<'\t'<<state->PosEntryID<<'\t'<<state->RotEntryID<<'\t'<<state->ScaleEntryID<<'\n';
        id=state->NextStateID;
    }
    std::cout<<"AUTHORED_FULL_SCALE_POSITIONER\t"<<full->value.PositionerID<<'\n';
    auto effectBase=SpellVisualKitEffectLoadInfo::Instance();
    std::vector<DB2FieldMeta> effectFields(effectBase->Fields,effectBase->Fields+effectBase->FieldCount);
    effectFields.push_back({true,FT_INT,"SpellVisualKitID"});
    DB2LoadInfo effectInfo(effectFields.data(),effectFields.size(),SpellVisualKitEffectMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    auto eventBase=SpellVisualEventLoadInfo::Instance();
    std::vector<DB2FieldMeta> eventFields(eventBase->Fields,eventBase->Fields+eventBase->FieldCount);
    eventFields.push_back({true,FT_INT,"SpellVisualID"});
    DB2LoadInfo eventInfo(eventFields.data(),eventFields.size(),SpellVisualEventMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_EVENT);
    DB2Storage<AttachmentEffect> effects("SpellVisualKitEffect.db2",&effectInfo);
    DB2Storage<AttachmentEvent> events("SpellVisualEvent.db2",&eventInfo);
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    assert(sizeof(AttachmentEffect)==effectInfo.Meta->GetRecordSize() && sizeof(AttachmentEvent)==eventInfo.Meta->GetRecordSize());
    if(!effects.Load(argv[1],0)||!events.Load(argv[1],0)||!visuals.Load(argv[1],0))return 3;
    unsigned effectCount=0,eventCount=0;std::set<uint32> visualIDs,spellIDs;
    for(auto e:effects)if(e->value.EffectType==2&&e->value.Effect==318611){assert(e->kit==64756);++effectCount;}
    for(auto e:events)if(e->value.SpellVisualKitID==64756){visualIDs.insert(e->visual);++eventCount;}
    for(auto v:visuals)if(visualIDs.count(v->SpellVisualID))spellIDs.insert(v->SpellID);
    assert(effectCount==1&&eventCount>0&&visualIDs==std::set<uint32>{53568}&&spellIDs==std::set<uint32>{204641});
    auto bytes = [&](uint32 id)
    {
        ByteBuffer buffer;
        store.WriteRecord(id, 0, buffer);
        return std::vector<uint8>(buffer.contents(), buffer.contents()+buffer.size());
    };
    std::map<uint32, std::vector<uint8>> original;
    for (auto row : store) original.emplace(row->value.ID, bytes(row->value.ID));
    assert(original.size() > 10000);
    assert(original.at(318611).size() == sizeof(Row));
    Row source = *before;
    assert(!StromkarBladeVisual::Supported(nullptr));
    for (unsigned variant=0; variant<11; ++variant)
    {
        Row custom=source;
        if(variant==0)++custom.kit;
        if(variant==1)++custom.value.ID;
        if(variant==2)++custom.value.SpellVisualEffectNameID;
        if(variant==3)custom.value.AttachmentID=0;
        if(variant==4)custom.value.Flags=1;
        if(variant==5)custom.value.PositionerID=1;
        if(variant==6)custom.value.Offset[0]=1;
        if(variant==7)custom.value.Yaw=0;
        if(variant==8)custom.value.Pitch=0;
        if(variant==9)custom.value.Roll=1;
        if(variant==10)custom.value.Scale=0.5f;
        assert(!StromkarBladeVisual::Supported(&custom));
    }
    auto count = sDB2Manager.GetHotfixData().size();
    assert(StromkarBladeVisual::Apply(store));
    auto row = store.LookupEntry(318611);
    assert(row->value.Scale == full->value.Scale && StromkarBladeVisual::Supported(row));
    auto payload = bytes(318611);
    constexpr size_t scale = offsetof(Row,value) + offsetof(SpellVisualKitModelAttachEntry,Scale);
    std::cout<<"ROW_LAYOUT\t"<<sizeof(Row)<<'\t'<<scale<<'\t'<<store.GetTableHash()<<'\t'<<info->FieldCount<<'\n';
    for (auto const& pair : original)
    {
        auto current = bytes(pair.first);
        assert(current.size() == pair.second.size());
        for(size_t i=0;i<current.size();++i)
            if(pair.first!=318611 || i<scale || i>=scale+sizeof(float))assert(current[i]==pair.second[i]);
    }
    Row fixed=*row;StromkarBladeVisual::Fix(&fixed);assert(!std::memcmp(&fixed,row,sizeof(Row)));
    assert(sDB2Manager.GetHotfixData().size() == count+1);
    bool notified=false;
    for(auto const& hotfix:sDB2Manager.GetHotfixData())
    {
        if(hotfix.second != 318611 || PAIR64_HIPART(hotfix.first) != store.GetTableHash())continue;
        notified=true;
        WorldPackets::Hotfix::HotfixResponse response;
        WorldPackets::Hotfix::HotfixData record;
        record.ID=hotfix.first;record.RecordID=318611;record.Data.emplace();
        record.Data->append(payload.data(),payload.size());
        response.Hotfixes.emplace_back(std::move(record));
        auto packet=response.Write();
        assert(std::search(packet->contents(),packet->contents()+packet->size(),payload.begin(),payload.end()) != packet->contents()+packet->size());
    }
    assert(notified);
    std::cout<<"PASS native26972 full parent record, "<<original.size()<<" serialized rows preserved except318611 scale0.025->authored1;11 custom refusals, idempotent value fix, actual client hotfix notice/table hash and HotfixResponse payload. Client rendering remains separate.\n";
}

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;
#define ASSERT(condition, ...) assert(condition)
#define UI64LIT(value) UINT64_C(value)
template<typename Value> void EndianConvert(Value&) { }
#include "DB2ColumnCompression.inc"
#include "DB2ColumnMeta.inc"
#include "DB2PalletValue.inc"
struct Meta
{
    bool internal = false;
    bool HasIndexFieldInData() const { return internal; }
    uint32 GetIndexField() const { return 0; }
};
struct LoadInfo { Meta* Meta; };
struct Header { uint32 FieldCount = 2, RecordSize = 16, PackedDataOffset = 0; };
struct CheckedIds
{
    std::vector<uint32> values{10, 42, 77, 9000};
    mutable uint32 last = 0;
    uint32 operator[](uint32 row) const { last = row; return values.at(row); }
};
struct DB2FileLoaderRegularImpl
{
    Meta meta;
    LoadInfo info{&meta};
    LoadInfo* _loadInfo = &info;
    Header header;
    Header* _header = &header;
    char const* _fileName = "controlled-common-data";
    std::unique_ptr<uint8[]> _data;
    CheckedIds _idTable;
    DB2ColumnMeta _columnMeta[2]{};
    std::unordered_map<uint32, uint32> _commonValues[2];
    std::vector<DB2PalletValue> _palletValues[2], _palletArrayValues[2];
    DB2FileLoaderRegularImpl(uint32 stride, bool internal)
    {
        header.RecordSize = stride; meta.internal = internal;
        _data = std::make_unique<uint8[]>(stride * 4 + 8);
        _columnMeta[0].CompressionType = DB2ColumnCompression::None;
        _columnMeta[1].CompressionType = DB2ColumnCompression::CommonData;
        for (uint32 row = 0; row < 4; ++row)
            std::memcpy(_data.get() + row * stride, &_idTable.values[row], sizeof(uint32));
    }
    uint32 RecordGetId(uint8 const*, uint32) const;
    template<typename Value> Value RecordGetVarInt(uint8 const*, uint32, uint32) const;
    uint16 GetFieldOffset(uint32) const;
    uint64 RecordGetPackedValue(uint8 const*, uint32, uint32) const;
    template<typename Value> Value Read(uint32 row) const { return RecordGetVarInt<Value>(_data.get() + row * header.RecordSize, 1, 0); }
};
#include "RecordId.inc"
#include "Packed.inc"
#include "Offset.inc"
#include "VarInt.inc"

template<typename Value> void CheckType(DB2FileLoaderRegularImpl& reader, uint32 bits)
{
    reader._columnMeta[1].CompressionData.commonData.Value = bits;
    reader._commonValues[1] = {{42, bits ^ 0x100}, {9000, bits ^ 0x200}};
    for (uint32 row = 0; row < 4; ++row)
    {
        uint32 expectedBits = bits ^ (row == 1 ? 0x100 : row == 3 ? 0x200 : 0);
        Value expected;
        std::memcpy(&expected, &expectedBits, sizeof(Value));
        Value actual = reader.Read<Value>(row);
        assert(std::memcmp(&actual, &expected, sizeof(Value)) == 0);
        if (!reader.meta.internal) assert(reader._idTable.last == row);
    }
}

int main()
{
#ifdef BASELINE
    DB2FileLoaderRegularImpl reader(16, false);
    assert(reader.Read<uint32>(0) == 0);
    bool rejected = false;
    try { reader.Read<uint32>(1); } catch (std::out_of_range const&) { rejected = true; }
    assert(rejected && reader._idTable.last == UINT32_MAX);
    reader.meta.internal = true; CheckType<uint32>(reader, 100);
    std::cout << "BASELINE: pinned38bad8a2 actual common-data decoder/ID resolver computes UINT32_MAX for external-ID row1; checked table rejects it. Actual native SpellVisual load separately raised ACCESS_VIOLATION. Internal-ID path still resolves correctly.\n";
#else
    for (uint32 stride : {16u, 24u, 40u}) for (bool internal : {false, true})
    {
        DB2FileLoaderRegularImpl reader(stride, internal);
        CheckType<uint32>(reader, 0xF1234567); CheckType<int32>(reader, 0xFFFFAABB);
        CheckType<uint16>(reader, 0xDCBA); CheckType<uint8>(reader, 0xAB);
        CheckType<float>(reader, 0x40500000);
        reader._columnMeta[1].CompressionType = DB2ColumnCompression::None;
        reader._columnMeta[1].BitOffset = 32;
        uint32 plain = 123;
        std::memcpy(reader._data.get() + stride + 4, &plain, sizeof(plain));
        assert(reader.Read<uint32>(1) == plain);
    }
    std::cout << "PASS: actual regular common-data decoder/ID resolver, packed/offset helpers cover120 typed external/internal-ID/default/override row cases across three strides plus ordinary uncompressed controls. Controlled little-endian metadata/arrays, not a full file or rendered client test.\n";
#endif
}
